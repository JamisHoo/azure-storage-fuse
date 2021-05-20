#include "azure_storage_blob_adaptor.h"

using namespace Azure::Storage::Blobs;

namespace {
    int translate_exception(const Azure::Storage::StorageException& e) {
        if (e.StatusCode == Azure::Core::Http::HttpStatusCode::Forbidden)
            return -EACCES;
        if (e.StatusCode == Azure::Core::Http::HttpStatusCode::NotFound)
            return -ENOENT;
        if (e.StatusCode == Azure::Core::Http::HttpStatusCode::InternalServerError || e.StatusCode == Azure::Core::Http::HttpStatusCode::ServiceUnavailable)
            return -ETXTBSY;
        return 0;
    }
}

AzureStorageBlobAdaptor::AzureStorageBlobAdaptor(const std::string& account, const std::string& filesystem, const std::string& account_key) :
    m_key_credential(std::make_shared<Azure::Storage::StorageSharedKeyCredential>(account, account_key)),
    m_blob_container_url("https://" + account + ".blob.core.windows.net/" + filesystem) {}

int AzureStorageBlobAdaptor::getattr(const std::string& path, FileStatus& file_status) {
    auto blob_container_client = BlobContainerClient(m_blob_container_url, m_key_credential);
    if (path == ".") {
        try {
            auto properties = blob_container_client.GetProperties().Value;
            file_status.is_directory = true;
            file_status.file_size = 0;
            file_status.last_modified_time = std::chrono::system_clock::time_point(properties.LastModified);
        } catch (Azure::Storage::StorageException& e) {
            int ret = translate_exception(e);
            if (ret != 0)
                return ret;
            throw;
        }
        return 0;
    }

    // Blob service doesn't have full directory support. If there are any blobs prefixing with "path/", we consider path as a directory.
    ListBlobsOptions list_options;
    list_options.Prefix = path + '/';
    list_options.Include = Models::ListBlobsIncludeFlags::Metadata;
    list_options.PageSizeHint = 1;
    try {
        auto blobs_page = blob_container_client.ListBlobsByHierarchy("/", list_options);
        while (blobs_page.HasPage() && blobs_page.Blobs.empty())
            blobs_page.MoveToNextPage();
        if (blobs_page.HasPage()) {
            file_status.is_directory = true;
            file_status.file_size = 0;
            return 0;
        }
    } catch (Azure::Storage::StorageException& e) {
        int ret = translate_exception(e);
        if (ret != 0)
            return ret;
        throw;
    }
    auto blob_client = blob_container_client.GetBlobClient(path);
    try {
        auto properties = blob_client.GetProperties().Value;
        file_status.is_directory = false;
        file_status.file_size = properties.BlobSize;
        file_status.last_modified_time = std::chrono::system_clock::time_point(properties.LastModified);
    } catch (Azure::Storage::StorageException& e) {
        int ret = translate_exception(e);
        if (ret != 0)
            return ret;
        throw;
    }
    return 0;
}

int AzureStorageBlobAdaptor::read(const std::string& path, char* buff, size_t size, size_t offset) {
    auto blob_client = BlobClient(m_blob_container_url + "/" + path, m_key_credential);
    DownloadBlobToOptions download_options;
    download_options.Range = Azure::Core::Http::HttpRange();
    download_options.Range.Value().Offset = offset;
    download_options.Range.Value().Length = size;
    download_options.TransferOptions.InitialChunkSize = 1 * 1024 * 1024;
    try {
        auto downloadResult = blob_client.DownloadTo(reinterpret_cast<uint8_t*>(buff), size, download_options).Value;
        return downloadResult.ContentRange.Length.Value();
    } catch (Azure::Storage::StorageException& e) {
        if (e.StatusCode == Azure::Core::Http::HttpStatusCode::RangeNotSatisfiable) {
            // offset >= file size
            return 0;
        }
        int ret = translate_exception(e);
        if (ret != 0)
            return ret;
        throw;
    }
}

int AzureStorageBlobAdaptor::list(const std::string& path, std::vector<DirectoryEntry>& directory_entries, std::string& continuation_token) {
    auto container_client = BlobContainerClient(m_blob_container_url, m_key_credential);
    ListBlobsOptions list_options;
    if (path != ".")
        list_options.Prefix = path + '/';
    if (!continuation_token.empty())
        list_options.ContinuationToken = continuation_token;
    list_options.Include = Models::ListBlobsIncludeFlags::Metadata;
    try {
        auto paths_page = container_client.ListBlobsByHierarchy("/", list_options);
        for (auto& p : paths_page.Blobs) {
            DirectoryEntry e;
            e.name = std::move(p.Name);
            if (path != ".")
                e.name = e.name.substr(path.length() + 1);
            e.status.is_directory = false;
            e.status.file_size = p.BlobSize;
            e.status.last_modified_time = std::chrono::system_clock::time_point(p.Details.LastModified);
            directory_entries.emplace_back(std::move(e));
        }
        for (auto& p : paths_page.BlobPrefixes) {
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
        // It is possible that both "path" and "path/" exist in blob service. If so, local filesystem will be screwed. There's nothing we can do about it.
        continuation_token = paths_page.NextPageToken.HasValue() ? paths_page.NextPageToken.Value() : std::string();
    } catch (Azure::Storage::StorageException& e) {
        int ret = translate_exception(e);
        if (ret != 0)
            return ret;
        throw;
    }
    return 0;
}
