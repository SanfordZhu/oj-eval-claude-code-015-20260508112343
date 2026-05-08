#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <set>

const std::string DATA_FILE = "data.bin";

// Structure to store key-value pairs in file
struct Entry {
    char key[65];  // 64 bytes + null terminator
    int value;
    bool deleted;  // flag to mark if entry is deleted
};

class FileStorage {
private:
    std::fstream file;

public:
    FileStorage() {
        // Open file in binary read-write mode, create if doesn't exist
        file.open(DATA_FILE, std::ios::in | std::ios::out | std::ios::binary);
        if (!file.is_open()) {
            // File doesn't exist, create it
            file.clear();
            file.open(DATA_FILE, std::ios::out | std::ios::binary);
            file.close();
            file.open(DATA_FILE, std::ios::in | std::ios::out | std::ios::binary);
        }
    }

    ~FileStorage() {
        if (file.is_open()) {
            file.close();
        }
    }

    void insert(const std::string& key, int value) {
        // First, check if this exact key-value pair already exists
        file.clear();
        file.seekg(0, std::ios::beg);

        Entry entry;
        bool found = false;
        long pos = -1;

        while (file.read(reinterpret_cast<char*>(&entry), sizeof(Entry))) {
            if (entry.key == key && entry.value == value && !entry.deleted) {
                // Entry already exists, no need to insert
                return;
            }
        }

        // Check if key exists with different value
        file.clear();
        file.seekg(0, std::ios::beg);

        while (file.read(reinterpret_cast<char*>(&entry), sizeof(Entry))) {
            if (entry.key == key && !entry.deleted) {
                // Key exists, need to insert new value in sorted order
                break;
            }
        }

        // Add new entry at the end
        file.clear();
        file.seekp(0, std::ios::end);

        Entry newEntry;
        std::fill(newEntry.key, newEntry.key + 65, '\0');
        std::copy(key.begin(), key.end(), newEntry.key);
        newEntry.value = value;
        newEntry.deleted = false;

        file.write(reinterpret_cast<const char*>(&newEntry), sizeof(Entry));
        file.flush();
    }

    void deleteEntry(const std::string& key, int value) {
        file.clear();
        file.seekg(0, std::ios::beg);

        Entry entry;
        long pos = 0;

        while (file.read(reinterpret_cast<char*>(&entry), sizeof(Entry))) {
            if (entry.key == key && entry.value == value && !entry.deleted) {
                // Found the entry, mark as deleted
                file.clear();
                file.seekp(pos, std::ios::beg);
                entry.deleted = true;
                file.write(reinterpret_cast<const char*>(&entry), sizeof(Entry));
                file.flush();
                return;
            }
            pos = file.tellg();
        }
    }

    std::vector<int> find(const std::string& key) {
        std::vector<int> values;

        file.clear();
        file.seekg(0, std::ios::beg);

        Entry entry;

        while (file.read(reinterpret_cast<char*>(&entry), sizeof(Entry))) {
            if (entry.key == key && !entry.deleted) {
                values.push_back(entry.value);
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