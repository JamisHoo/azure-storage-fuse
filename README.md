# Azure Storage Fuse

Azure Storage Fuse is an open source project used to mount Azure Storage service as local drive on Azure VMs, on-premises systems or any client desktops. Currently it supports [Azure Data Lake Storage](https://docs.microsoft.com/en-us/azure/storage/blobs/data-lake-storage-introduction), [Azure Blob Storage](https://docs.microsoft.com/en-us/azure/storage/blobs/storage-blobs-introduction) and [Azure Files Storage](https://docs.microsoft.com/en-us/azure/storage/files/storage-files-introduction). But it's created with great extensibility so support for any other cloud storage services can be easily added.

## Features

Azure Storage Fuse can help you:

- List directories and files of cloud storage
- Read directories and files attributes of cloud storage
- Read files of cloud storage
- Mount different types of cloud storage services under different mountpoints at the same time

Features will be added in the future:

- Write files of cloud storage
- Delete files and directories of cloud storage
- More attributes (like owner, group, permission and last access time etc.) and extended attributes

## Getting Started on Linux

1. Install FUSE3

    on CentOS/Fedora/REHL:

    ```bash
    yum install fuse3 fuse3-devel
    ```

    or on Ubuntu/Debian:
    
    ```bash
    apt-get install fuse3 libfuse3-dev
    ```

2. Build

    Navigate into source folder
    
    ```bash
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build .
    ```

3. Run

    For example, we want to mount some cloud storage specified in `config.json` under directory `mnt`.
    `-c` is optional, it will try to load `config.json` in the working directory by default.
    
    ```bash
    mkdir mnt
    ./azure_storage_fuse -c config.json mnt
    ```

## Getting Started on Windows

1. Install [WinFsp](http://www.secfs.net/winfsp/)

    Check "Developer" when installing WinFsp to ensure that all necessary header and library files are included in the installation.

2. Build

    Navigate into source folder

    ```bat
    mkdir build
    cd build
    cmake .. -A x64
    cmake --build . --config Release
    ```

3. Run

    For example, we want to mount some cloud storage specified in `config.json` under an unsed drive `D:`.
    `-c` is optional, it will try to load `config.json` in the working directory by default.
    
    ```bat
    azure_storage_fuse -c config.json D:
    ```

## Getting Started on macOS

I've never tried to run it on macOS, but it should be the same as on Linux because there's this [FUSE for macOS](https://osxfuse.github.io/) project.

## Configuration File

You can define several cloud storage services in configuration file. Below is an example of configuration file and explanation for each field.

```json
[
{
    "type": "azure storage datalake",
    "account_name": "your account name",
    "container_name": "your container name",
    "account_key": "your account shared key"
},
{
    "type": "azure storage blob",
    "account_name": "your account name",
    "container_name": "your container name",
    "account_key": "your account shared key"
},
{
    "type": "azure storage file",
    "account_name": "your account name",
    "container_name": "your share name",
    "account_key": "your account shared key"
}
]
```

| Field           | Description |
|-----------------|-------------|
| type            | Currently we support "azure storage datalake", "azure storage blob" and "azure storage file". DataLake service is recommended over Blob service, since Blob service doesn't support real directory hierarchy, which may lead to some glitches in some edge cases. |
| account\_name   | Your Azure storage account name. |
| account\_key    | Your Azure storage account shared key. |
| container\_name | Filesystem name for DataLake service, container name for Blob service or share name for File service. |
| mount\_at       | Optional. A container is by default mounted on a subdirectory named `[account_name]_[container_name]`. Use this value to override the default value. Note that it's your responsibility to avoid duplication. |
| enabled         | Optional. Application will ignore this setting if the value is `false`. |
