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
