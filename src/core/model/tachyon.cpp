#include "tachyon/core/core.h"
#include "tachyon/version.h"

#include <iostream>

namespace tachyon {
std::string version_string() {
    return TACHYON_VERSION_STR;
}

void print_version(std::ostream& out) {
    out << "TACHYON Version: " << TACHYON_VERSION_STR << std::endl;
}
}
