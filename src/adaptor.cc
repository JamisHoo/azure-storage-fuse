#include "adaptor.h"

#include <string>
#include <unordered_map>
#include <memory>

std::unordered_map<std::string, std::shared_ptr<BaseAdaptor>> g_adaptors;
