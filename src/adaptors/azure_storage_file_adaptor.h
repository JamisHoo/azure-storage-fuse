#pragma once

#include <string>
#include <vector>

#include <azure/storage/files/shares.hpp>

#include "../adaptor.h"

struct AzureStorageFileAdaptor: public BaseAdaptor {
    AzureStorageFileAdaptor(const std::string& account, const std::string& filesystem, const std::string& account_key);
    int getattr(const std::string&, FileStatus&) override;
    int read(const std::string&, char* buff, size_t size, size_t offset) override;
    int list(const std::string& path, std::vector<DirectoryEntry>& directory_entries, std::string& continuation_token);
private:
    std::shared_ptr<Azure::Storage::StorageSharedKeyCredential> m_key_credential;
    std::string m_share_url;
};
