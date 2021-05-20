#pragma once

#include <map>
#include <cstdlib>
#include <cstring>

#include "../adaptor.h"

class RootDirectoryAdaptor : public BaseAdaptor {
public:
    RootDirectoryAdaptor() {}

    ~RootDirectoryAdaptor() override = default;

    int getattr(const std::string& path, FileStatus& file_status) override {
        if (path == ".") {
            file_status.is_directory = true;
            file_status.file_size = 0;
            return 0;
        }
        std::abort();
        auto ite = g_adaptors.find(path);
        if (ite == g_adaptors.end()) {
            return -ENOENT;
        }
        file_status.is_directory = true;
        file_status.file_size = 0;
        return 0;
    }

    int read(const std::string& /* path */, char* /* buff */, size_t /* size */, size_t /* offset */) override {
        // There shouldn't be any files in root directory.
        std::abort();
    }

    int list(const std::string& path, std::vector<DirectoryEntry>& directory_entries, std::string& continuation_token) override {
        if (path == ".") {
            directory_entries.clear();
            for (auto i : g_adaptors) {
                if (i.first.empty())
                    continue;
                DirectoryEntry e;
                e.name = i.first;
                e.status.is_directory = true;
                e.status.file_size = 0;
                directory_entries.emplace_back(std::move(e));
            }
            continuation_token.clear();
            return 0;
        }
        std::abort();
    }
};
