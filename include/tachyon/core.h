#pragma once

#include <ostream>
#include <string>

namespace tachyon {

std::string version_string();
void print_version(std::ostream& out);

} // namespace tachyon
