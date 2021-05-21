#pragma once

#ifdef _WIN32
#pragma warning(disable : 26812)
#endif

#define FUSE_USE_VERSION 30
#include <fuse3/fuse.h>
#undef FUSE_USE_VERSION

#include "adaptor.h"

#ifndef _WIN32
using fuse_off_t = off_t;
using fuse_stat = struct stat;
#endif

int fs_open(const char* path, fuse_file_info* fi);
int fs_getattr(const char* path, fuse_stat* stbuf, fuse_file_info* fi);
int fs_read(const char* path, char* buff, size_t size, fuse_off_t offset, fuse_file_info* fi);
int fs_release(const char* path, fuse_file_info* fi);

int fs_opendir(const char* path, fuse_file_info* fi);
int fs_readdir(
    const char* path,
    void* buff,
    fuse_fill_dir_t filler,
    fuse_off_t offset,
    fuse_file_info* fi,
    fuse_readdir_flags flags);
int fs_releasedir(const char* path, fuse_file_info* fi);
