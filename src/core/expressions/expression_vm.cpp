#include "tachyon/core/expressions/expression_vm.h"
#include "tachyon/core/expressions/expression_engine.h"
#include "tachyon/core/animation/animation_primitives.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace tachyon::expressions {

double ExpressionVM::execute(const Bytecode& code, const ExpressionContext& context) {
    std::vector<double> stack;
    stack.reserve(64);

    const std::size_t n_instr = code.instructions.size();
    for (std::size_t ip = 0; ip < n_instr; ++ip) {
        const auto& instr = code.instructions[ip];
        switch (instr.op) {
            case OpCode::PushConst:
                stack.push_back(code.constants[instr.data]);
                break;
            case OpCode::PushName:
                stack.push_back(static_cast<double>(instr.data));
                break;
            case OpCode::PushVar: {
                const std::string& name = code.names[instr.data];
                auto it = context.variables.find(name);
                stack.push_back(it != context.variables.end() ? it->second : 0.0);
                break;
            }
            case OpCode::Add: {
                double b = stack.back(); stack.pop_back();
                double a = stack.back(); stack.pop_back();
                stack.push_back(a + b);
                break;
            }
            case OpCode::Sub: {
                double b = stack.back(); stack.pop_back();
                double a = stack.back(); stack.pop_back();
                stack.push_back(a - b);
                break;
            }
            case OpCode::Mul: {
                double b = stack.back(); stack.pop_back();
                double a = stack.back(); stack.pop_back();
                stack.push_back(a * b);
                break;
            }
            case OpCode::Div: {
                double b = stack.back(); stack.pop_back();
                double a = stack.back(); stack.pop_back();
                stack.push_back(b != 0.0 ? a / b : 0.0);
                break;
            }
            case OpCode::Pow: {
                double b = stack.back(); stack.pop_back();
                double a = stack.back(); stack.pop_back();
                stack.push_back(std::pow(a, b));
                break;
            }
            case OpCode::Neg: {
                stack.back() = -stack.back();
                break;
            }
            case OpCode::Mod: {
                double b = stack.back(); stack.pop_back();
                double a = stack.back(); stack.pop_back();
                stack.push_back(b != 0.0 ? std::fmod(a, b) : 0.0);
                break;
            }
            case OpCode::Lt: {
                double b = stack.back(); stack.pop_back();
                double a = stack.back(); stack.pop_back();
                stack.push_back(a < b ? 1.0 : 0.0);
                break;
            }
            case OpCode::Gt: {
                double b = stack.back(); stack.pop_back();
                double a = stack.back(); stack.pop_back();
                stack.push_back(a > b ? 1.0 : 0.0);
                break;
            }
            case OpCode::Le: {
                double b = stack.back(); stack.pop_back();
                double a = stack.back(); stack.pop_back();
                stack.push_back(a <= b ? 1.0 : 0.0);
                break;
            }
            case OpCode::Ge: {
                double b = stack.back(); stack.pop_back();
                double a = stack.back(); stack.pop_back();
                stack.push_back(a >= b ? 1.0 : 0.0);
                break;
            }
            case OpCode::Eq: {
                double b = stack.back(); stack.pop_back();
                double a = stack.back(); stack.pop_back();
                stack.push_back(std::abs(a - b) < 1e-12 ? 1.0 : 0.0);
                break;
            }
            case OpCode::Ne: {
                double b = stack.back(); stack.pop_back();
                double a = stack.back(); stack.pop_back();
                stack.push_back(std::abs(a - b) >= 1e-12 ? 1.0 : 0.0);
                break;
            }
            case OpCode::JumpIfFalse: {
                double cond = stack.back(); stack.pop_back();
                if (cond == 0.0) {
                    // data = absolute instruction index to jump to
                    ip = static_cast<std::size_t>(instr.data) - 1; // -1 because loop does ++ip
                }
                break;
            }
            case OpCode::Jump: {
                // Unconditional jump — used for ternary else branch
                ip = static_cast<std::size_t>(instr.data) - 1;
                break;
            }
            case OpCode::Call: {
                std::uint32_t call_name_idx = instr.data & 0xFFFF;
                std::uint32_t arg_count = instr.data >> 16;
                const std::string& name = code.names[call_name_idx];
                
                std::vector<double> args(arg_count);
                for (int i = static_cast<int>(arg_count) - 1; i >= 0; --i) {
                    args[i] = stack.back();
                    stack.pop_back();
                }

                if (name == "sin") stack.push_back(std::sin(args[0]));
                else if (name == "cos") stack.push_back(std::cos(args[0]));
                else if (name == "abs") stack.push_back(std::abs(args[0]));
                else if (name == "min") stack.push_back(std::min(args[0], args[1]));
                else if (name == "max") stack.push_back(std::max(args[0], args[1]));
                else if (name == "clamp") stack.push_back(std::clamp(args[0], args[1], args[2]));
                else if (name == "wiggle") {
                    // Wiggle: wiggle(t, frequency, amplitude, seed)
                    // Uses noise2d for deterministic organic motion
                    if (args.size() >= 4) {
                        double t = args[0];
                        double freq = args[1];
                        double amplitude = args[2];
                        double seed = args[3];
                        stack.push_back(amplitude * tachyon::noise2d(static_cast<float>(t * freq), static_cast<float>(seed)));
                    } else {
                        stack.push_back(0.0);
                    }
                }
                else if (name == "pingpong") {
                    // Ping-pong: bounce value t between 0 and length
                    double t = args[0];
                    double length = args[1];
                    double normalized = std::fmod(t, 2.0 * length);
                    if (normalized > length) normalized = 2.0 * length - normalized;
                    stack.push_back(normalized);
                }
                else if (name == "timeStretch") {
                    // Time stretch: scale time t by factor
                    double t = args[0];
                    double factor = args[1];
                    stack.push_back(t * factor);
                }
                else if (name == "stagger") {
                    // Stagger: delay based on layer index
                    // stagger(base_time) = base_time * layer_index
                    double base_time = args[0];
                    // Get layer_index from context variables
                    auto it = context.variables.find("index");
                    double index = (it != context.variables.end()) ? it->second : 0.0;
                    stack.push_back(base_time * index);
                }
                else if (name == "lerp" || name == "interpolate") {
                    // Support both lerp(a, b, t) and AE-style interpolate(frame, f0, f1, v0, v1).
                    if (args.size() == 3) {
                        double a = args[0];
                        double b = args[1];
                        double t = std::clamp(args[2], 0.0, 1.0);
                        stack.push_back(a + t * (b - a));
                    } else if (args.size() >= 5) {
                        double frame = args[0];
                        double f0 = args[1];
                        double f1 = args[2];
                        double v0 = args[3];
                        double v1 = args[4];
                        if (f1 <= f0) {
                            stack.push_back(v0);
                        } else {
                            double t = std::clamp((frame - f0) / (f1 - f0), 0.0, 1.0);
                            stack.push_back(v0 + t * (v1 - v0));
                        }
                    } else {
                        stack.push_back(0.0);
                    }
                }
                else if (name == "spring") {
                    // Use the actual spring function from animation_primitives.h
                    // spring(t, from, to, freq, damping)
                    if (args.size() >= 5) {
                        stack.push_back(tachyon::spring(args[0], args[1], args[2], args[3], args[4]));
                    } else {
                        stack.push_back(0.0);
                    }
                }
                else if (name == "random") {
                    // Deterministic random: random(seed)
                    if (args.size() >= 1) {
                        // Convert arg to string seed
                        std::string seed_str = std::to_string(static_cast<std::int64_t>(args[0]));
                        double r = tachyon::random(seed_str);
                        if (args.size() >= 3) {
                            // random(seed, min, max)
                            stack.push_back(args[1] + r * (args[2] - args[1]));
                        } else {
                            stack.push_back(r);
                        }
                    } else {
                        stack.push_back(0.0);
                    }
                }
                else if (name == "valueAtTime") {
                    if (args.size() >= 1) {
                        if (context.value_at_time) {
                            stack.push_back(context.value_at_time(args[0]));
                        } else if (context.property_sampler) {
                            stack.push_back(context.property_sampler(static_cast<int>(context.property_index), args[0]));
                        } else {
                            stack.push_back(context.value);
                        }
                    } else {
                        stack.push_back(context.value);
                    }
                }
                else if (name == "noise2d") {
                    // Simplex noise 2D
                    if (args.size() >= 2) {
                        stack.push_back(tachyon::noise2d(static_cast<float>(args[0]), static_cast<float>(args[1])));
                    } else {
                        stack.push_back(0.0);
                    }
                }
                else if (name == "noise3d") {
                    // Simplex noise 3D
                    if (args.size() >= 3) {
                        stack.push_back(tachyon::noise3d(static_cast<float>(args[0]), static_cast<float>(args[1]), static_cast<float>(args[2])));
                    } else {
                        stack.push_back(0.0);
                    }
                }
                else if (name == "noise4d") {
                    // Simplex noise 4D
                    if (args.size() >= 4) {
                        stack.push_back(tachyon::noise4d(static_cast<float>(args[0]), static_cast<float>(args[1]), static_cast<float>(args[2]), static_cast<float>(args[3])));
                    } else {
                        stack.push_back(0.0);
                    }
                }
                else if (name == "prop") {
                    // Cross-Layer Linking: prop("layer.property.path")
                    // Arguments: property_path as name index (pushed by PushName)
                    if (args.size() >= 1 && context.layer_property_resolver) {
                        // The argument is a name index (from PushName)
                        // Convert double to int to get the string from code.names
                        std::uint32_t prop_path_idx = static_cast<std::uint32_t>(static_cast<int>(args[0]));
                        if (prop_path_idx < code.names.size()) {
                            const std::string& full_path = code.names[prop_path_idx];
                            // Parse "layer.property.path" format
                            // Find the first dot to separate layer identifier from property path
                            size_t dot_pos = full_path.find('.');
                            if (dot_pos != std::string::npos) {
                                std::string layer_id = full_path.substr(0, dot_pos);
                                std::string prop_path = full_path.substr(dot_pos + 1);
                                double result = context.layer_property_resolver(layer_id, prop_path);
                                stack.push_back(result);
                            } else {
                                // No dot - treat whole string as property path (use current layer)
                                double result = context.layer_property_resolver("", full_path);
                                stack.push_back(result);
                            }
                        } else {
                            stack.push_back(0.0);
                        }
                    } else {
                        stack.push_back(0.0);
                    }
                }
                else if (name == "prop2") {
                    // prop2(layer_id, property_path) - two string arguments
                    if (args.size() >= 2 && context.layer_property_resolver) {
                        std::uint32_t layer_idx = static_cast<std::uint32_t>(static_cast<int>(args[1]));
                        std::uint32_t path_idx = static_cast<std::uint32_t>(static_cast<int>(args[0]));
                        if (layer_idx < code.names.size() && path_idx < code.names.size()) {
                            const std::string& layer_id = code.names[layer_idx];
                            const std::string& prop_path = code.names[path_idx];
                            double result = context.layer_property_resolver(layer_id, prop_path);
                            stack.push_back(result);
                        } else {
                            stack.push_back(0.0);
                        }
                    } else {
                        stack.push_back(0.0);
                    }
                }
                // --- Math built-ins ---
                else if (name == "floor") stack.push_back(std::floor(args[0]));
                else if (name == "ceil")  stack.push_back(std::ceil(args[0]));
                else if (name == "round") stack.push_back(std::round(args[0]));
                else if (name == "sqrt")  stack.push_back(args[0] >= 0.0 ? std::sqrt(args[0]) : 0.0);
                else if (name == "log")   stack.push_back(args[0] > 0.0 ? std::log(args[0]) : 0.0);
                else if (name == "exp")   stack.push_back(std::exp(args[0]));
                else if (name == "tan")   stack.push_back(std::tan(args[0]));
                else if (name == "atan")  stack.push_back(std::atan(args[0]));
                else if (name == "atan2" && args.size() >= 2) stack.push_back(std::atan2(args[0], args[1]));
                else if (name == "degreesToRadians") stack.push_back(args[0] * (3.14159265358979323846 / 180.0));
                else if (name == "radiansToDegrees") stack.push_back(args[0] * (180.0 / 3.14159265358979323846));
                else if (name == "length" && args.size() >= 2) stack.push_back(std::sqrt(args[0]*args[0] + args[1]*args[1]));
                // --- ease / easeIn / easeOut (cubic Hermite, AE-style) ---
                // ease(t, t1, t2, v1, v2) — smooth step
                else if ((name == "ease" || name == "easeIn" || name == "easeOut") && args.size() >= 5) {
                    double t  = args[0], t1 = args[1], t2 = args[2];
                    double v1 = args[3], v2 = args[4];
                    if (t2 <= t1) { stack.push_back(v1); break; }
                    double u = std::clamp((t - t1) / (t2 - t1), 0.0, 1.0);
                    double s = (name == "easeIn")  ? u * u * (3.0 - 2.0 * u) * u  // quadratic ease-in only
                             : (name == "easeOut") ? u * (2.0 - u)                 // ease-out only
                             : u * u * (3.0 - 2.0 * u);                            // smooth step (ease)
                    stack.push_back(v1 + s * (v2 - v1));
                }
                // --- loopOut / loopIn (cycle and pingpong) ---
                else if (name == "loopOut" || name == "loopIn") {
                    // loopOut(type_ignored, duration) — duration in seconds
                    // Uses value_at_time to sample start/end of the loop range
                    double duration = (args.size() >= 2) ? args[1] : 1.0;
                    if (duration <= 0.0) { stack.push_back(context.value); break; }
                    double t = context.time;
                    double looped_t;
                    if (name == "loopOut") {
                        // cycle forward from t=0
                        looped_t = std::fmod(t, duration);
                        if (looped_t < 0.0) looped_t += duration;
                    } else {
                        // cycle backward (loopIn)
                        double neg_t = -t;
                        looped_t = duration - std::fmod(neg_t, duration);
                        if (looped_t >= duration) looped_t -= duration;
                    }
                    if (context.value_at_time) {
                        stack.push_back(context.value_at_time(looped_t));
                    } else {
                        stack.push_back(context.value);
                    }
                }
                // --- noise / random ---
                else if (name == "noise") {
                    auto fade = [](double t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); };
                    auto lerp = [](double a, double b, double t) { return a + t * (b - a); };
                    auto hash = [](int x, int y = 0) {
                        int n = x + y * 57;
                        n = (n << 13) ^ n;
                        return (1.0 - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
                    };
                    if (args.size() >= 2) { // 2D Noise
                        double x = args[0], y = args[1];
                        int X = (int)std::floor(x), Y = (int)std::floor(y);
                        double fx = x - X, fy = y - Y;
                        double u = fade(fx), v = fade(fy);
                        stack.push_back(lerp(lerp(hash(X, Y), hash(X+1, Y), u), lerp(hash(X, Y+1), hash(X+1, Y+1), u), v));
                    } else if (args.size() >= 1) { // 1D Noise
                        double x = args[0];
                        int X = (int)std::floor(x);
                        double fx = x - X;
                        stack.push_back(lerp(hash(X), hash(X + 1), fade(fx)));
                    } else stack.push_back(0.0);
                }
                else if (name == "random") {
                    std::uint64_t s = (args.size() >= 1) ? (std::uint64_t)(args[0] * 1000.0) : context.seed;
                    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
                    stack.push_back((double)(s >> 32) / 4294967295.0);
                }
                else {
                    stack.push_back(0.0);
                }
                break;
            }
            case OpCode::Ret:
                return stack.empty() ? 0.0 : stack.back();
        }
    }
    return stack.empty() ? 0.0 : stack.back();
}

} // namespace tachyon::expressions
