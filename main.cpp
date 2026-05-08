#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <set>
#include <cstring>

const std::string DATA_FILE = "data.bin";
const std::string INDEX_FILE = "index.bin";

// Structure to store key-value pairs in file
struct Entry {
    char key[65];  // 64 bytes + null terminator
    int value;
    bool deleted;  // flag to mark if entry is deleted
};

// Index entry to store key and file position
struct IndexEntry {
    char key[65];
    long data_pos;  // position in data file
};

class FileStorage {
private:
    std::fstream data_file;
    std::fstream index_file;
    std::unordered_map<std::string, std::vector<long>> memory_index;
    bool use_memory_index;

    void buildMemoryIndex() {
        memory_index.clear();

        index_file.clear();
        index_file.seekg(0, std::ios::beg);

        IndexEntry idx_entry;
        while (index_file.read(reinterpret_cast<char*>(&idx_entry), sizeof(IndexEntry))) {
            memory_index[idx_entry.key].push_back(idx_entry.data_pos);
        }
        use_memory_index = true;
    }

public:
    FileStorage() : use_memory_index(false) {
        // Open data file
        data_file.open(DATA_FILE, std::ios::in | std::ios::out | std::ios::binary);
        if (!data_file.is_open()) {
            data_file.clear();
            data_file.open(DATA_FILE, std::ios::out | std::ios::binary);
            data_file.close();
            data_file.open(DATA_FILE, std::ios::in | std::ios::out | std::ios::binary);
        }

        // Open index file
        index_file.open(INDEX_FILE, std::ios::in | std::ios::out | std::ios::binary);
        if (!index_file.is_open()) {
            index_file.clear();
            index_file.open(INDEX_FILE, std::ios::out | std::ios::binary);
            index_file.close();
            index_file.open(INDEX_FILE, std::ios::in | std::ios::out | std::ios::binary);
        } else {
            // If index file exists, build memory index
            buildMemoryIndex();
        }
    }

    ~FileStorage() {
        if (data_file.is_open()) {
            data_file.close();
        }
        if (index_file.is_open()) {
            index_file.close();
        }
    }

    void insert(const std::string& key, int value) {
        // Check if already exists
        if (use_memory_index) {
            auto it = memory_index.find(key);
            if (it != memory_index.end()) {
                for (long pos : it->second) {
                    data_file.clear();
                    data_file.seekg(pos, std::ios::beg);
                    Entry entry;
                    if (data_file.read(reinterpret_cast<char*>(&entry), sizeof(Entry))) {
                        if (!entry.deleted && entry.value == value) {
                            return;  // Already exists
                        }
                    }
                }
            }
        } else {
            // Scan data file
            data_file.clear();
            data_file.seekg(0, std::ios::beg);
            Entry entry;
            while (data_file.read(reinterpret_cast<char*>(&entry), sizeof(Entry))) {
                if (!entry.deleted && entry.key == key && entry.value == value) {
                    return;  // Already exists
                }
            }
        }

        // Add new entry
        Entry newEntry;
        std::fill(newEntry.key, newEntry.key + 65, '\0');
        std::copy(key.begin(), key.end(), newEntry.key);
        newEntry.value = value;
        newEntry.deleted = false;

        data_file.clear();
        data_file.seekp(0, std::ios::end);
        long data_pos = data_file.tellp();
        data_file.write(reinterpret_cast<const char*>(&newEntry), sizeof(Entry));
        data_file.flush();

        // Add to index
        IndexEntry idxEntry;
        std::fill(idxEntry.key, idxEntry.key + 65, '\0');
        std::copy(key.begin(), key.end(), idxEntry.key);
        idxEntry.data_pos = data_pos;

        index_file.clear();
        index_file.seekp(0, std::ios::end);
        index_file.write(reinterpret_cast<const char*>(&idxEntry), sizeof(IndexEntry));
        index_file.flush();

        // Update memory index
        if (use_memory_index) {
            memory_index[key].push_back(data_pos);
        }
    }

    void deleteEntry(const std::string& key, int value) {
        if (use_memory_index) {
            auto it = memory_index.find(key);
            if (it != memory_index.end()) {
                for (long pos : it->second) {
                    data_file.clear();
                    data_file.seekg(pos, std::ios::beg);
                    Entry entry;
                    if (data_file.read(reinterpret_cast<char*>(&entry), sizeof(Entry))) {
                        if (!entry.deleted && entry.value == value) {
                            entry.deleted = true;
                            data_file.clear();
                            data_file.seekp(pos, std::ios::beg);
                            data_file.write(reinterpret_cast<const char*>(&entry), sizeof(Entry));
                            data_file.flush();
                            return;
                        }
                    }
                }
            }
        } else {
            // Scan data file
            data_file.clear();
            data_file.seekg(0, std::ios::beg);

            Entry entry;
            long pos = 0;

            while (data_file.read(reinterpret_cast<char*>(&entry), sizeof(Entry))) {
                if (!entry.deleted && entry.key == key && entry.value == value) {
                    entry.deleted = true;
                    data_file.clear();
                    data_file.seekp(pos, std::ios::beg);
                    data_file.write(reinterpret_cast<const char*>(&entry), sizeof(Entry));
                    data_file.flush();
                    return;
                }
                pos = data_file.tellg();
            }
        }
    }

    std::vector<int> find(const std::string& key) {
        std::vector<int> values;

        if (use_memory_index) {
            auto it = memory_index.find(key);
            if (it != memory_index.end()) {
                for (long pos : it->second) {
                    data_file.clear();
                    data_file.seekg(pos, std::ios::beg);
                    Entry entry;
                    if (data_file.read(reinterpret_cast<char*>(&entry), sizeof(Entry))) {
                        if (!entry.deleted) {
                            values.push_back(entry.value);
                        }
                    }
                }
            }
        } else {
            // Scan data file
            data_file.clear();
            data_file.seekg(0, std::ios::beg);

            Entry entry;

            while (data_file.read(reinterpret_cast<char*>(&entry), sizeof(Entry))) {
                if (!entry.deleted && entry.key == key) {
                    values.push_back(entry.value);
                }
            }
        }

        // Sort values in ascending order
        std::sort(values.begin(), values.end());

        return values;
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    FileStorage storage;

    int n;
    std::cin >> n;
    std::cin.ignore();  // consume newline

    for (int i = 0; i < n; i++) {
        std::string line;
        std::getline(std::cin, line);

        std::string cmd;
        size_t pos = line.find(' ');
        cmd = line.substr(0, pos);

        if (cmd == "insert") {
            size_t key_pos = pos + 1;
            size_t value_pos = line.find(' ', key_pos);
            std::string key = line.substr(key_pos, value_pos - key_pos);
            int value = std::stoi(line.substr(value_pos + 1));

            storage.insert(key, value);
        } else if (cmd == "delete") {
            size_t key_pos = pos + 1;
            size_t value_pos = line.find(' ', key_pos);
            std::string key = line.substr(key_pos, value_pos - key_pos);
            int value = std::stoi(line.substr(value_pos + 1));

            storage.deleteEntry(key, value);
        } else if (cmd == "find") {
            std::string key = line.substr(pos + 1);

            std::vector<int> values = storage.find(key);

            if (values.empty()) {
                std::cout << "null\n";
            } else {
                for (size_t j = 0; j < values.size(); j++) {
                    if (j > 0) std::cout << " ";
                    std::cout << values[j];
                }
                std::cout << "\n";
            }
        }
    }

    return 0;
}