#include "file_ops.h"

#ifdef _WIN32
#include <sys/stat.h>
#endif

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <tuple>

struct file_context {
    std::string container_name;
    std::string object_name;
    std::shared_ptr<BaseAdaptor> adaptor;
};

struct directory_context {
    std::string container_name;
    std::string object_name;
    std::shared_ptr<BaseAdaptor> adaptor;

    std::vector<DirectoryEntry> directory_entries;
    size_t pos = 0;
    std::string continuation_token;
    bool new_listing = true;
};

std::tuple<std::string, std::string> parse_path(const std::string& path) {
    if (path[0] != '/')
        std::abort();
    auto i = path.find("/", 1);

    if (i != std::string::npos) {
        std::string container_name = path.substr(1, i - 1);
        std::string object_name = path.substr(i + 1);
        return std::make_tuple(container_name, object_name);
    } else {
        std::string container_name = path.substr(1);
        std::string object_name = ".";
        return std::make_tuple(container_name, object_name);
    }
}

std::shared_ptr<BaseAdaptor> resolve_path(const std::string& container_name) {
    auto ite = g_adaptors.find(container_name);
    return ite == g_adaptors.end() ? nullptr : ite->second;
}

int fs_open(const char* path, fuse_file_info* fi) {
    auto [container_name, object_name] = parse_path(path);

    auto adaptor = resolve_path(container_name);
    if (!adaptor)
        return -EACCES;

    FileStatus file_status;
    int ret = adaptor->getattr(object_name, file_status);
    if (ret < 0)
        return ret;

    if (file_status.is_directory)
        return -EISDIR;

    file_context* context = new file_context;
    context->container_name = container_name;
    context->object_name = object_name;
    context->adaptor = adaptor;
    fi->fh = reinterpret_cast<uint64_t>(context);

    return 0;
}

int fs_getattr(const char* path, fuse_stat* stbuf, fuse_file_info* fi) {
    (void)path;
    std::shared_ptr<BaseAdaptor> adaptor;
    std::string container_name;
    std::string object_name;
    if (fi) {
        file_context* context = reinterpret_cast<file_context*>(fi->fh);
        adaptor = context->adaptor;
        container_name = context->container_name;
        object_name = context->object_name;
    } else {
        std::tie(container_name, object_name) = parse_path(path);

        adaptor = resolve_path(container_name);
        if (!adaptor)
            return -EACCES;
    }

    FileStatus file_status;
    int ret = adaptor->getattr(object_name, file_status);
    if (ret < 0)
        return ret;

    std::memset(stbuf, 0, sizeof(*stbuf));

    stbuf->st_mode = file_status.is_directory ? (S_IFDIR | 0775) : (S_IFREG | 0442);
    stbuf->st_nlink = 1;
    stbuf->st_size = file_status.file_size;
    stbuf->st_uid = 1000;
    stbuf->st_gid = 1000;

#ifdef _WIN32
    stbuf->st_mtim.tv_sec = 0;
    stbuf->st_mtim.tv_nsec = 0;
#else
    stbuf->st_mtime = 0;
#endif
    return 0;
}

int fs_read(const char* /* path */, char* buff, size_t size, fuse_off_t offset, fuse_file_info* fi) {
    file_context* context = reinterpret_cast<file_context*>(fi->fh);
    int ret = context->adaptor->read(context->object_name, buff, size, offset);
    return ret;
}

int fs_release(const char* /* path */, fuse_file_info* fi) {
    delete reinterpret_cast<file_context*>(fi->fh);
    return 0;
}

int fs_opendir(const char* path, fuse_file_info* fi) {
    (void)path;
    auto [container_name, object_name] = parse_path(path);

    auto adaptor = resolve_path(container_name);
    if (!adaptor)
        return -EACCES;

    FileStatus file_status;
    int ret = adaptor->getattr(object_name, file_status);
    if (ret < 0)
        return ret;

    if (!file_status.is_directory)
        return -ENOTDIR;

    directory_context* context = new directory_context;
    context->container_name = container_name;
    context->object_name = object_name;
    context->adaptor = adaptor;
    
    fi->fh = reinterpret_cast<uint64_t>(context);

    return 0;
}

int fs_readdir(const char* path, void* buff, fuse_fill_dir_t filler, fuse_off_t offset, struct fuse_file_info* fi, fuse_readdir_flags /* flags */) {
    (void)path;
    directory_context* context = reinterpret_cast<directory_context*>(fi->fh);
    auto& adaptor = context->adaptor;

    if (offset == 0)
    {
        int ret = filler(buff, ".", nullptr, offset + 1, fuse_fill_dir_flags(0));
        if (ret != 0)
            return 0;
        offset++;
    }
    if (offset == 1) {
        int ret = filler(buff, "..", nullptr, offset + 1, fuse_fill_dir_flags(0));
        if (ret != 0)
            return 0;
        offset++;
    }

    do {
        for (; context->pos < context->directory_entries.size(); context->pos++) {
            int ret = filler(buff, context->directory_entries[context->pos].name.data(), nullptr, offset + 1, fuse_fill_dir_flags(0));
            if (ret != 0)
                return 0;
            ++offset;
        }
        context->directory_entries.clear();
        context->pos = 0;
        if (!context->continuation_token.empty() || context->new_listing) {
            int ret = adaptor->list(context->object_name, context->directory_entries, context->continuation_token);
            if (ret < 0)
                return ret;
            context->new_listing = false;
        }
    } while (!context->directory_entries.empty() || !context->continuation_token.empty());

    return 0;
}

int fs_releasedir(const char* /* path */, fuse_file_info* fi) {
    delete reinterpret_cast<directory_context*>(fi->fh);
    return 0;
}
