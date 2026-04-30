#include "tachyon/core/expressions/expression_vm.h"
#include "tachyon/core/expressions/expression_engine.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace tachyon::expressions {

double ExpressionVM::execute(const Bytecode& code, const ExpressionContext& context) {
    std::vector<double> stack;
    stack.reserve(64);

    for (const auto& instr : code.instructions) {
        switch (instr.op) {
            case OpCode::PushConst:
                stack.push_back(code.constants[instr.data]);
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
            case OpCode::Call: {
                std::uint32_t name_idx = instr.data & 0xFFFF;
                std::uint32_t arg_count = instr.data >> 16;
                const std::string& name = code.names[name_idx];
                
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
                    // Lightweight wiggle function for the VM.
                    stack.push_back(std::sin(args[0] * args[4]) * args[1]); 
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
                else if (name == "lerp") {
                    // Linear interpolation
                    double a = args[0];
                    double b = args[1];
                    double t = args[2];
                    stack.push_back(a + (b - a) * t);
                }
                else if (name == "interpolate") {
                    // Same as lerp
                    double a = args[0];
                    double b = args[1];
                    double t = args[2];
                    stack.push_back(a + (b - a) * t);
                }
                else if (name == "spring") {
                    // Spring: simplified spring physics
                    // spring(t, from, to, freq, damping)
                    double t = args[0];
                    double from = args[1];
                    double to = args[2];
                    double freq = args[3];
                    double damping = args[4];
                    // Simplified: just return a value based on time
                    double omega = 2.0 * 3.14159 * freq;
                    double decay = std::exp(-damping * t);
                    stack.push_back(from + (to - from) * (1.0 - decay * std::cos(omega * t)));
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
