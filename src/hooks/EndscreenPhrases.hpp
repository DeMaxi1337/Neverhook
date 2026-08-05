#pragma once
#include <string>
#include <vector>

namespace nh::phrases {

const std::vector<std::string>& pool();

std::string random();

bool isReservedMessage(const std::string& msg);

}
