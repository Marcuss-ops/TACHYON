#pragma once

#include "tachyon/core/math/algebra/vector3.h"
#include <string>
#include <vector>

namespace tachyon::scene {

enum class ConstraintType {
    Point,
    Orient,
    Parent,
    LookAt
};

struct ConstraintTarget {
    std::string layer_id;
    float weight{1.0f};
    math::Vector3 offset{math::Vector3::zero()};
};

struct ConstraintSpec {
    std::string id;
    ConstraintType type;
    std::vector<ConstraintTarget> targets;
    bool maintain_offset{true};
    float weight{1.0f};
};

struct IKSpec {
    std::string id;
    std::string target_id;
    std::string pole_vector_id;
    int chain_length{2};
    int iterations{10};
    float weight{1.0f};
    bool use_pole_vector{false};
};

} // namespace tachyon::scene

