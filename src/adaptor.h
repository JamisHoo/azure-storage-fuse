#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>
#include <functional>
#include <chrono>

struct FileStatus {
    bool is_directory;
    size_t file_size;
    std::chrono::time_point<std::chrono::system_clock> last_modified_time;
};

struct DirectoryEntry {
    std::string name;
    FileStatus status;
};

class BaseAdaptor {
public:
    virtual int getattr(const std::string&, FileStatus&) = 0;
    virtual int read(const std::string&, char* buff, size_t size, size_t offset) = 0;
    virtual int list(const std::string&, std::vector<DirectoryEntry>&, std::string& continuation_token) = 0;

    virtual ~BaseAdaptor() = default;
};

extern std::unordered_map<std::string, std::shared_ptr<BaseAdaptor>> g_adaptors;
