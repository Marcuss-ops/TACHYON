#pragma once

#include "tachyon/runtime/core/math_contract.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace tachyon {

enum class ExpressionOp : std::uint8_t {
    PushConst,
    LoadTime,
    LoadFrame,
    LoadProp,
    Add,
    Sub,
    Mul,
    Div,
    Neg,
    Sin,
    Cos,
    Abs,
    Min,
    Max,
    Clamp,
    Lerp,
    Interpolate,
    MapRange,
    ValueAtTime,
    Noise,
    Spring,
    PingPong,
    TimeStretch,
    Stagger,
    Return
};

struct ExpressionInstr {
    ExpressionOp op{ExpressionOp::Return};
    std::uint32_t operand{0};
};

struct CompiledExpression {
    std::vector<ExpressionInstr> code;
    std::vector<double> constants;
};

struct ExpressionContext {
    double time_seconds{0.0};
    std::int64_t frame_number{0};
    std::uint32_t layer_index{0};
    std::uint64_t seed{0};
    std::span<const double> properties{};
    
    /// Callback to sample a property at a specific time.
    /// Args: (property_idx, time_seconds) -> value
    std::function<double(std::uint32_t, double)> sample_callback;
};

class ExpressionVM {
public:
    [[nodiscard]] static double evaluate(const CompiledExpression& expression, const ExpressionContext& context) {
        std::array<double, 64> stack{};
        std::size_t sp = 0;

        const auto push = [&stack, &sp](double value) {
            if (sp < stack.size()) {
                stack[sp++] = value;
            }
        };

        const auto pop = [&stack, &sp]() -> double {
            if (sp == 0) {
                return 0.0;
            }
            return stack[--sp];
        };

        for (const auto& instr : expression.code) {
            switch (instr.op) {
            case ExpressionOp::PushConst:
                if (instr.operand < expression.constants.size()) {
                    push(expression.constants[instr.operand]);
                } else {
                    push(0.0);
                }
                break;
            case ExpressionOp::LoadTime:
                push(context.time_seconds);
                break;
            case ExpressionOp::LoadFrame:
                push(static_cast<double>(context.frame_number));
                break;
            case ExpressionOp::LoadProp:
                if (instr.operand < context.properties.size()) {
                    push(context.properties[instr.operand]);
                } else {
                    push(0.0);
                }
                break;
            case ExpressionOp::Add: {
                const double b = pop();
                const double a = pop();
                push(a + b);
                break;
            }
            case ExpressionOp::Sub: {
                const double b = pop();
                const double a = pop();
                push(a - b);
                break;
            }
            case ExpressionOp::Mul: {
                const double b = pop();
                const double a = pop();
                push(a * b);
                break;
            }
            case ExpressionOp::Div: {
                const double b = pop();
                const double a = pop();
                push(b == 0.0 ? 0.0 : a / b);
                break;
            }
            case ExpressionOp::Neg:
                push(-pop());
                break;
            case ExpressionOp::Sin:
                push(std::sin(pop()));
                break;
            case ExpressionOp::Cos:
                push(std::cos(pop()));
                break;
            case ExpressionOp::Abs:
                push(std::abs(pop()));
                break;
            case ExpressionOp::Min: {
                const double b = pop();
                const double a = pop();
                push(std::min(a, b));
                break;
            }
            case ExpressionOp::Max: {
                const double b = pop();
                const double a = pop();
                push(std::max(a, b));
                break;
            }
            case ExpressionOp::Clamp: {
                const double hi = pop();
                const double lo = pop();
                const double value = pop();
                push(std::clamp(value, lo, hi));
                break;
            }
            case ExpressionOp::Interpolate:
            case ExpressionOp::Lerp: {
                const double t = pop();
                const double b = pop();
                const double a = pop();
                push(a + (b - a) * t);
                break;
            }
            case ExpressionOp::MapRange: {
                const double outMax = pop();
                const double outMin = pop();
                const double inMax = pop();
                const double inMin = pop();
                const double value = pop();
                if (std::abs(inMax - inMin) < 1e-9) {
                    push(outMin);
                } else {
                    const double t = (value - inMin) / (inMax - inMin);
                    push(math_contract::lerp(outMin, outMax, t));
                }
                break;
            }
            case ExpressionOp::ValueAtTime: {
                const double time_offset = pop();
                const double property_idx = pop();
                if (context.sample_callback) {
                    push(context.sample_callback(static_cast<std::uint32_t>(property_idx), time_offset));
                } else {
                    push(0.0);
                }
                break;
            }
            case ExpressionOp::Noise: {
                const std::uint64_t x = static_cast<std::uint64_t>(context.frame_number) ^ context.seed ^ static_cast<std::uint64_t>(instr.operand);
                push(math_contract::noise(x));
                break;
            }
            case ExpressionOp::Spring: {
                const double damping = pop();      // Damping ratio (zeta)
                const double freq = pop();         // Undamped natural frequency (Hz)
                const double to = pop();           // Target equilibrium position
                const double from = pop();         // Initial displacement at t=0
                
                push(math_contract::spring(context.time_seconds, from, to, freq, damping));
                break;
            }
            case ExpressionOp::PingPong: {
                const double duration = pop();
                const double t = pop();
                if (duration <= 0.0) {
                    push(t);
                } else {
                    const double fold = std::floor(t / duration);
                    const double rem = std::fmod(t, duration);
                    if (static_cast<std::uint64_t>(fold) % 2 == 0) {
                        push(rem);
                    } else {
                        push(duration - rem);
                    }
                }
                break;
            }
            case ExpressionOp::TimeStretch: {
                const double scale = pop();
                const double t = pop();
                push(t * scale);
                break;
            }
            case ExpressionOp::Stagger: {
                const double delay = pop();
                push(static_cast<double>(context.layer_index) * delay);
                break;
            }
            case ExpressionOp::Return:
                if (sp == 0) {
                    return 0.0;
                }
                return stack[sp - 1];
            }
        }

        return sp == 0 ? 0.0 : stack[sp - 1];
    }
};

} // namespace tachyon
