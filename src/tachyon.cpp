#include "tachyon/version.h"
#include <iostream>

namespace tachyon {
    void print_version() {
        std::cout << "TACHYON Version: " << TACHYON_VERSION_STR << std::endl;
    }
}
