#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct FileStatus
{
  bool is_directory = false;
  size_t file_size = 0;
  std::chrono::time_point<std::chrono::system_clock> last_modified_time;
};

struct DirectoryEntry
{
  std::string name;
  FileStatus status;
};

class BaseAdaptor {
public:
  virtual int getattr(const std::string& path, FileStatus& file_status) = 0;
  virtual int read(const std::string& path, char* buff, size_t size, size_t offset) = 0;
  virtual int list(
      const std::string& path,
      std::vector<DirectoryEntry>& directory_entries,
      std::string& continuation_token)
      = 0;

  virtual ~BaseAdaptor() = default;
};

extern std::unordered_map<std::string, std::shared_ptr<BaseAdaptor>> g_adaptors;
