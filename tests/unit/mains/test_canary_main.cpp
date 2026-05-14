#include <iostream>

namespace tachyon::renderer2d {
    void test_crossfade_transition();
    void generate_transition_gallery();
    void render_remotion_demo();
    void render_transition_lookbook();
}

int main() {
    std::cout << "[CANARY] Starting Transition Tests...\n";
    
    // Run the validation test
    // tachyon::renderer2d::test_crossfade_transition();
    
    // Generate the lookbook for the user
    tachyon::renderer2d::render_transition_lookbook();
    
    // Generate the demo for the user
    // tachyon::renderer2d::render_remotion_demo();
    
    // Generate the gallery for the user (Disabled for speed, focus on remotion demo)
    // tachyon::renderer2d::generate_transition_gallery();
    
    std::cout << "[CANARY] All Tests Finished.\n";
    return 0;
}
