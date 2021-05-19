#pragma once

#include <map>
#include <cstdlib>
#include <cstring>

#include "../adaptor.h"

class DebugFileAdaptor : public BaseAdaptor {
public:
    DebugFileAdaptor() {
        FileStatus file_status;
        file_status.is_directory = false;
        file_status.file_size = 10;
        m_files["file1"] = file_status;
        m_file1_content = "1234567890";

        file_status.file_size = 20;
        m_files["file2"] = file_status;
        m_file2_content = m_file1_content + m_file1_content;

        file_status.is_directory = true;
        file_status.file_size = 0;
        m_files["dir1"] = file_status;
        m_files["dir2"] = file_status;

        for (int i = 0; i < 1000; ++i) {
            file_status.is_directory = false;
            file_status.file_size = 0;
            m_files["dir1/file" + std::to_string(i)] = file_status;
        }
    }

    ~DebugFileAdaptor() override = default;

    int getattr(const std::string& path, FileStatus& file_status) override {
        if (path == ".") {
            file_status.is_directory = true;
            file_status.file_size = 0;
            return 0;
        }
        auto ite = m_files.find(path);
        if (ite == m_files.end())
            std::abort();
        file_status = ite->second;
        return 0;
    }

    int read(const std::string& path, char* buff, size_t size, size_t offset) override {
        std::string content;
        if (path == "file1")
            content = m_file1_content;
        else if (path == "file2")
            content = m_file2_content;
        size_t read_size = std::min(content.size() - offset, size);
        if (read_size > 0)
            std::memcpy(buff, &content[offset], read_size);
        return read_size;
    }

    int list(const std::string& path, std::vector<DirectoryEntry>& directory_entries, std::string& continuation_token) override {
        auto ite = m_files.lower_bound(path);
        if (path == ".") {
            ite = m_files.begin();
        }
        if (!continuation_token.empty()) {
            ite = m_files.find(continuation_token);
            if (ite == m_files.end())
                std::abort();
            continuation_token.clear();
        }
        for (; ite != m_files.end(); ++ite) {
            std::string p = ite->first;
            if (p == path)
                continue;
            if (directory_entries.size() == 10) {
                continuation_token = p;
                break;
            }
            bool is_prefix;
            if (path == ".")
                is_prefix = true;
            else
                is_prefix = p.length() > path.length() && p.substr(0, path.length()) == path && p[path.length()] == '/';
            if (!is_prefix)
                break;
            bool is_immediate;
            if (path == ".")
                is_immediate = p.find('/', 0) == std::string::npos;
            else
                is_immediate = p.find('/', path.length() + 1) == std::string::npos;
            if (!is_immediate)
                continue;
            DirectoryEntry e;
            if (path == ".")
                e.name = p;
            else
                e.name = p.substr(path.length() + 1);
            e.is_directory = ite->second.is_directory;
            directory_entries.emplace_back(std::move(e));
        }
        return 0;
    }

private:
    std::map<std::string, FileStatus> m_files;

    std::string m_file1_content;
    std::string m_file2_content;
};
