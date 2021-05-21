cmake_minimum_required(VERSION 2.8)

project(azure-storage-fuse)
set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Only support 64bit platforms")
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
find_package(nlohmann_json)

if(WIN32)
    find_path(FUSE3_INCLUDE_DIR fuse3/fuse.h REQUIRED PATHS "C:/Program Files (x86)/WinFsp/inc")
    find_file(FUSE3_LIBRARY winfsp-x64.dll REQUIRED PATHS "C:/Program Files (x86)/WinFsp/bin")
    find_library(FUSE3_IMPORT_LIBRARY winfsp-x64 REQUIRED PATHS "C:/Program Files (x86)/WinFsp/lib")
else()
    find_path(FUSE3_INCLUDE_DIR fuse3/fuse.h REQUIRED)
    find_library(FUSE3_LIBRARY fuse3 REQUIRED)
endif()

add_library(FUSE3 SHARED IMPORTED)
set_target_properties(FUSE3 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${FUSE3_INCLUDE_DIR})
set_target_properties(FUSE3 PROPERTIES IMPORTED_LOCATION ${FUSE3_LIBRARY})
if(WIN32)
    set_target_properties(FUSE3 PROPERTIES IMPORTED_IMPLIB ${FUSE3_IMPORT_LIBRARY})
endif()

add_subdirectory(azure-sdk-for-cpp EXCLUDE_FROM_ALL)

set(SOURCE
    src/adaptor.h
    src/adaptor.cc
    src/adaptors/azure_storage_blob_adaptor.h
    src/adaptors/azure_storage_blob_adaptor.cc
    src/adaptors/azure_storage_datalake_adaptor.h
    src/adaptors/azure_storage_datalake_adaptor.cc
    src/adaptors/azure_storage_file_adaptor.h
    src/adaptors/azure_storage_file_adaptor.cc
    src/adaptors/root_directory_adaptor.h
    src/file_ops.h
    src/file_ops.cc
    src/main.cc
)

add_executable(azure_storage_fuse ${SOURCE})
target_link_libraries(azure_storage_fuse Threads::Threads FUSE3 nlohmann_json::nlohmann_json)
target_link_libraries(azure_storage_fuse azure-storage-blobs azure-storage-files-datalake azure-storage-files-shares)

if(WIN32)
    target_compile_definitions(azure_storage_fuse PUBLIC NOMINMAX)
    add_custom_command(TARGET azure_storage_fuse POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FUSE3_LIBRARY}" $<TARGET_FILE_DIR:azure_storage_fuse>)
endif()

if(MSVC)
    target_compile_options(azure_storage_fuse PUBLIC /W4 /WX /MP)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(azure_storage_fuse PUBLIC -Wall -Wextra -Werror -pedantic)
    target_compile_options(azure_storage_fuse PRIVATE -O2)
endif()

configure_file(src/config.json config.json COPYONLY)
