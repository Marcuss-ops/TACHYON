#include <iostream>
#include <vector>
#include <memory>
#include <tachyon/runtime/execution/frame_executor.h>
#include <tachyon/runtime/core/data_snapshot.h>
#include <tachyon/core/scene/evaluated_state.h>

int main() {
    std::cout << "Sizeof ExecutedFrame: " << sizeof(tachyon::ExecutedFrame) << std::endl;
    std::cout << "Alignof ExecutedFrame: " << alignof(tachyon::ExecutedFrame) << std::endl;
    std::cout << "Sizeof DataSnapshot: " << sizeof(tachyon::DataSnapshot) << std::endl;
    std::cout << "Alignof DataSnapshot: " << alignof(tachyon::DataSnapshot) << std::endl;
    std::cout << "Sizeof EvaluatedLayerState: " << sizeof(tachyon::scene::EvaluatedLayerState) << std::endl;
    std::cout << "Alignof EvaluatedLayerState: " << alignof(tachyon::scene::EvaluatedLayerState) << std::endl;
    return 0;
}
