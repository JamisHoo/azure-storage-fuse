#pragma once

#include <string>
#include <vector>

#include <azure/storage/blobs.hpp>

#include "../adaptor.h"

struct AzureStorageBlobAdaptor : public BaseAdaptor
{
  AzureStorageBlobAdaptor(
      const std::string& account,
      const std::string& blob_container,
      const std::string& account_key);
  ~AzureStorageBlobAdaptor() override = default;

  int getattr(const std::string& path, FileStatus& file_status) override;
  int read(const std::string& path, char* buff, size_t size, size_t offset) override;
  int list(
      const std::string& path,
      std::vector<DirectoryEntry>& directory_entries,
      std::string& continuation_token);

private:
  std::shared_ptr<Azure::Storage::StorageSharedKeyCredential> m_key_credential;
  std::string m_blob_container_url;
};
