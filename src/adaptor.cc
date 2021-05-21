#include "adaptor.h"

#include <memory>
#include <string>
#include <unordered_map>

std::unordered_map<std::string, std::shared_ptr<BaseAdaptor>> g_adaptors;
