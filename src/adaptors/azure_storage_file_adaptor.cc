#include "azure_storage_file_adaptor.h"

using namespace Azure::Storage::Files::Shares;

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

AzureStorageFileAdaptor::AzureStorageFileAdaptor(const std::string& account, const std::string& filesystem, const std::string& account_key) :
    m_key_credential(std::make_shared<Azure::Storage::StorageSharedKeyCredential>(account, account_key)),
    m_share_url("https://" + account + ".file.core.windows.net/" + filesystem) {}

int AzureStorageFileAdaptor::getattr(const std::string& path, FileStatus& file_status) {
    if (path == ".") {
        auto fs_client = ShareClient(m_share_url, m_key_credential);
        try {
            auto properties = fs_client.GetProperties().Value;
            file_status.is_directory = true;
            file_status.file_size = 0;
        } catch (Azure::Storage::StorageException& e) {
            int ret = translate_exception(e);
            if (ret != 0)
                return ret;
            throw;
        }
        return 0;
    }
    try {
        auto file_client = ShareFileClient(m_share_url + "/" + path, m_key_credential);
        auto properties = file_client.GetProperties().Value;
        file_status.is_directory = false;
        file_status.file_size = properties.FileSize;
        return 0;
    } catch (Azure::Storage::StorageException& e) {
        if (e.StatusCode != Azure::Core::Http::HttpStatusCode::NotFound)
        {
            int ret = translate_exception(e);
            if (ret != 0)
                return ret;
            throw;
        }
    }
    try {
        auto directory_client = ShareDirectoryClient(m_share_url + "/" + path, m_key_credential);
        auto properties = directory_client.GetProperties().Value;
        file_status.is_directory = true;
        file_status.file_size = 0;
        return 0;
    } catch (Azure::Storage::StorageException& e) {
        int ret = translate_exception(e);
        if (ret != 0)
            return ret;
        throw;
    }
}

int AzureStorageFileAdaptor::read(const std::string& path, char* buff, size_t size, size_t offset) {
    auto file_client = ShareFileClient(m_share_url + "/" + path, m_key_credential);
    DownloadFileToOptions download_options;
    download_options.Range = Azure::Core::Http::HttpRange();
    download_options.Range.Value().Offset = offset;
    download_options.Range.Value().Length = size;
    download_options.TransferOptions.InitialChunkSize = 1 * 1024 * 1024;
    try {
        auto downloadResult = file_client.DownloadTo(reinterpret_cast<uint8_t*>(buff), size, download_options).Value;
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

int AzureStorageFileAdaptor::list(const std::string& path, std::vector<DirectoryEntry>& directory_entries, std::string& continuation_token) {
    std::string url = m_share_url;
    if (path != ".")
        url += "/" + path;
    auto directory_client = ShareDirectoryClient(url, m_key_credential);

    ListFilesAndDirectoriesOptions list_options;
    if (!continuation_token.empty())
        list_options.ContinuationToken = continuation_token;
    try {
        auto paths_page = directory_client.ListFilesAndDirectories(list_options);
        for (auto& p : paths_page.Files) {
            DirectoryEntry e;
            e.name = std::move(p.Name);
            e.status.is_directory = false;
            e.status.file_size = p.Details.ContentLength;
            directory_entries.emplace_back(std::move(e));
        }
        for (auto& p : paths_page.Directories) {
            DirectoryEntry e;
            e.name = std::move(p.Name);
            e.status.is_directory = true;
            e.status.file_size = 0;
            directory_entries.emplace_back(std::move(e));
        }
        continuation_token = paths_page.NextPageToken.HasValue() ? paths_page.NextPageToken.Value() : std::string();
        return 0;
    } catch (Azure::Storage::StorageException& e) {
        int ret = translate_exception(e);
        if (ret != 0)
            return ret;
        throw;
    }
}
