#include "azure_storage_datalake_adaptor.h"

using namespace Azure::Storage::Files::DataLake;

namespace {
int translate_exception(const Azure::Storage::StorageException& e)
{
  if (e.StatusCode == Azure::Core::Http::HttpStatusCode::Forbidden)
    return -EACCES;
  if (e.StatusCode == Azure::Core::Http::HttpStatusCode::NotFound)
    return -ENOENT;
  if (e.StatusCode == Azure::Core::Http::HttpStatusCode::InternalServerError
      || e.StatusCode == Azure::Core::Http::HttpStatusCode::ServiceUnavailable)
    return -ETXTBSY;
  return 0;
}
} // namespace

AzureStorageDataLakeAdaptor::AzureStorageDataLakeAdaptor(
    const std::string& account,
    const std::string& filesystem,
    const std::string& account_key)
    : m_key_credential(
        std::make_shared<Azure::Storage::StorageSharedKeyCredential>(account, account_key)),
      m_blob_container_url("https://" + account + ".blob.core.windows.net/" + filesystem),
      m_filesystem_url("https://" + account + ".dfs.core.windows.net/" + filesystem)
{
}

int AzureStorageDataLakeAdaptor::getattr(const std::string& path, FileStatus& file_status)
{
  if (path == ".")
  {
    auto fs_client = DataLakeFileSystemClient(m_filesystem_url, m_key_credential);
    try
    {
      auto properties = fs_client.GetProperties().Value;
      file_status.is_directory = true;
      file_status.file_size = 0;
      file_status.last_modified_time
          = std::chrono::system_clock::time_point(properties.LastModified);
    }
    catch (Azure::Storage::StorageException& e)
    {
      int ret = translate_exception(e);
      if (ret != 0)
        return ret;
      throw;
    }
    return 0;
  }
  auto path_client = DataLakePathClient(m_filesystem_url + "/" + path, m_key_credential);
  try
  {
    auto properties = path_client.GetProperties().Value;
    file_status.is_directory = properties.IsDirectory;
    file_status.file_size = properties.FileSize;
    file_status.last_modified_time = std::chrono::system_clock::time_point(properties.LastModified);
  }
  catch (Azure::Storage::StorageException& e)
  {
    int ret = translate_exception(e);
    if (ret != 0)
      return ret;
    throw;
  }
  return 0;
}

int AzureStorageDataLakeAdaptor::read(
    const std::string& path,
    char* buff,
    size_t size,
    size_t offset)
{
  auto file_client = DataLakeFileClient(m_filesystem_url + "/" + path, m_key_credential);
  DownloadFileToOptions download_options;
  download_options.Range = Azure::Core::Http::HttpRange();
  download_options.Range.Value().Offset = offset;
  download_options.Range.Value().Length = size;
  download_options.TransferOptions.InitialChunkSize = 1 * 1024 * 1024;
  try
  {
    auto downloadResult
        = file_client.DownloadTo(reinterpret_cast<uint8_t*>(buff), size, download_options).Value;
    int64_t bytes_read = downloadResult.ContentRange.Length.Value();
    if (bytes_read > std::numeric_limits<int>::max())
    {
      std::abort();
    }
    return static_cast<int>(bytes_read);
  }
  catch (Azure::Storage::StorageException& e)
  {
    if (e.StatusCode == Azure::Core::Http::HttpStatusCode::RangeNotSatisfiable)
    {
      // offset >= file size
      return 0;
    }
    int ret = translate_exception(e);
    if (ret != 0)
      return ret;
    throw;
  }
}

int AzureStorageDataLakeAdaptor::list(
    const std::string& path,
    std::vector<DirectoryEntry>& directory_entries,
    std::string& continuation_token)
{
  auto container_client
      = Azure::Storage::Blobs::BlobContainerClient(m_blob_container_url, m_key_credential);
  Azure::Storage::Blobs::ListBlobsOptions list_options;
  if (path != ".")
    list_options.Prefix = path + '/';
  if (!continuation_token.empty())
    list_options.ContinuationToken = continuation_token;
  list_options.Include = Azure::Storage::Blobs::Models::ListBlobsIncludeFlags::Metadata;
  try
  {
    auto paths_page = container_client.ListBlobsByHierarchy("/", list_options);
    for (auto& p : paths_page.Blobs)
    {
      DirectoryEntry e;
      e.name = std::move(p.Name);
      if (path != ".")
        e.name = e.name.substr(path.length() + 1);
      e.status.is_directory = false;
      e.status.file_size = p.BlobSize;
      e.status.last_modified_time = std::chrono::system_clock::time_point(p.Details.LastModified);
      directory_entries.emplace_back(std::move(e));
    }
    for (auto& p : paths_page.BlobPrefixes)
    {
      if (p.back() == '/')
        p.pop_back();
      DirectoryEntry e;
      e.name = std::move(p);
      if (path != ".")
        e.name = e.name.substr(path.length() + 1);
      e.status.is_directory = true;
      e.status.file_size = 0;
      directory_entries.emplace_back(std::move(e));
    }
    continuation_token
        = paths_page.NextPageToken.HasValue() ? paths_page.NextPageToken.Value() : std::string();
  }
  catch (Azure::Storage::StorageException& e)
  {
    int ret = translate_exception(e);
    if (ret != 0)
      return ret;
    throw;
  }
  return 0;
}
