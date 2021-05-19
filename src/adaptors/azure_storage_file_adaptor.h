#pragma once

#include "../adaptor.h"

struct AzureStorageFileAdaptor : public BaseAdaptor {
    int getattr(const std::string&, FileStatus&) override;
    int read(const std::string&, char* buff, size_t size, size_t offset) override;
private:
    // auth, container
};
