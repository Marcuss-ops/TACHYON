#include <iostream>

namespace tachyon::renderer2d {
    void test_crossfade_transition();
    void generate_transition_gallery();
}

int main() {
    std::cout << "[CANARY] Starting Transition Tests...\n";
    
    // Run the validation test
    tachyon::renderer2d::test_crossfade_transition();
    
    // Generate the gallery for the user
    tachyon::renderer2d::generate_transition_gallery();
    
    std::cout << "[CANARY] All Tests Finished.\n";
    return 0;
}
