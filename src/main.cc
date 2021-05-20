#include <fstream>
#include <cstring>
#include <cctype>
#include <iostream>
#include <vector>
#include <string>

#include <nlohmann/json.hpp>

#include "file_ops.h"
#include "adaptors/root_directory_adaptor.h"
#include "adaptors/azure_storage_datalake_adaptor.h"
#include "adaptors/azure_storage_blob_adaptor.h"
#include "adaptors/azure_storage_file_adaptor.h"

namespace {
    bool file_exists(const std::string& filename) {
        std::ifstream fin(filename);
        return fin.is_open();
    }
}

int main(int argc, char** argv) {
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i)
        args.emplace_back(std::string(argv[i]));

    std::string config_file;
    std::string mount_point;
    int fuse_args_n = -1;
    if (argc >= 4 && args[1] == "-c") {
        config_file = args[2];
        if (file_exists(config_file)) {
            mount_point = args[3];
            fuse_args_n = 4;
        }
    } else if (argc >= 2) {
        config_file = "config.json";
        if (file_exists(config_file)) {
            mount_point = args[1]; 
            fuse_args_n = 2;
        }
    }
    if (fuse_args_n == -1) {
        std::cout << "Usage: " << args[0] << " -c [config file]" << std::endl;
        return 0;
    }

    g_adaptors.emplace("", std::make_shared<RootDirectoryAdaptor>());

    std::ifstream fin(config_file);
    nlohmann::json j;
    fin >> j;
    for (const auto& container : j) {
        std::string mount_at;
        std::shared_ptr<BaseAdaptor> adaptor;

        if (container.contains("enabled") && container["enabled"] == false)
            continue;

        std::string type = container["type"];
        if (type == "azure storage datalake") {
            std::string account_name = container["account_name"];
            std::string container_name = container["container_name"];
            std::string account_key = container["account_key"];
            mount_at = account_name + "_" + container_name;
            if (container.contains("mount_at"))
                mount_at = container["mount_at"];
            adaptor = std::make_shared<AzureStorageDataLakeAdaptor>(account_name, container_name, account_key);
        } else if (type == "azure storage blob") {
            std::string account_name = container["account_name"];
            std::string container_name = container["container_name"];
            std::string account_key = container["account_key"];
            mount_at = account_name + "_" + container_name;
            if (container.contains("mount_at"))
                mount_at = container["mount_at"];
            adaptor = std::make_shared<AzureStorageBlobAdaptor>(account_name, container_name, account_key);
        } else if (type == "azure storage file") {
            std::string account_name = container["account_name"];
            std::string container_name = container["container_name"];
            std::string account_key = container["account_key"];
            mount_at = account_name + "_" + container_name;
            if (container.contains("mount_at"))
                mount_at = container["mount_at"];
            adaptor = std::make_shared<AzureStorageFileAdaptor>(account_name, container_name, account_key);
        }

        auto inserted = g_adaptors.emplace(mount_at, std::move(adaptor)).second;
        if (!inserted) {
            std::cout << "duplicate container name: " << mount_at << std::endl;
            return 1;
        }
    }

    std::vector<char*> fuse_args;
    fuse_args.emplace_back(argv[0]);
    std::string fuse_f = "-f";
    fuse_args.emplace_back(&fuse_f[0]);
    for (int i = fuse_args_n; i < argc; ++i)
        fuse_args.emplace_back(argv[i]);
    fuse_args.emplace_back(&mount_point[0]);

    struct fuse_operations vrfs_operations;
    std::memset(&vrfs_operations, 0, sizeof(vrfs_operations));
    vrfs_operations.open = fs_open;
    vrfs_operations.getattr = fs_getattr;
    vrfs_operations.read = fs_read;
    vrfs_operations.release = fs_release;
    vrfs_operations.opendir = fs_opendir;
    vrfs_operations.readdir = fs_readdir;
    vrfs_operations.releasedir = fs_releasedir;

    return fuse_main(static_cast<int>(fuse_args.size()), fuse_args.data(), &vrfs_operations, nullptr);
}
