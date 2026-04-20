#pragma once

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
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
    Noise,
    Spring,
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
    std::uint64_t seed{0};
    std::span<const double> properties{};
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
            case ExpressionOp::Noise: {
                const std::uint64_t x = static_cast<std::uint64_t>(context.frame_number) ^ context.seed ^ static_cast<std::uint64_t>(instr.operand);
                std::uint64_t z = x + 0x9e3779b97f4a7c15ULL;
                z = (z ^ (z >> 30U)) * 0xbf58476d1ce4e5b9ULL;
                z = (z ^ (z >> 27U)) * 0x94d049bb133111ebULL;
                z ^= (z >> 31U);
                push(static_cast<double>(z & 0xFFFFFFFFULL) / static_cast<double>(0xFFFFFFFFULL));
                break;
            }
            case ExpressionOp::Spring: {
                // Spring(from, to, frequency, damping)
                const double damping = pop();      // Damping ratio (0-1 = underdamped, 1 = critically, >1 = overdamped)
                const double freq = pop();         // Frequency in Hz
                const double to = pop();           // Target value
                const double from = pop();         // Start value
                
                const double t = context.time_seconds;
                if (t <= 0.0) {
                    push(from);
                    break;
                }

                const double omega_n = freq * 2.0 * 3.14159265358979323846;
                const double delta = from - to;
                
                double val = to;
                if (damping < 1.0) {
                    // Underdamped
                    const double omega_d = omega_n * std::sqrt(1.0 - damping * damping);
                    const double decay = std::exp(-damping * omega_n * t);
                    val = to + delta * decay * (std::cos(omega_d * t) + (damping * omega_n / omega_d) * std::sin(omega_d * t));
                } else if (damping > 1.0) {
                    // Overdamped
                    const double decay_a = std::exp((-damping + std::sqrt(damping * damping - 1.0)) * omega_n * t);
                    const double decay_b = std::exp((-damping - std::sqrt(damping * damping - 1.0)) * omega_n * t);
                    // Simplify: just linear combination of two decays
                    val = to + delta * 0.5 * (decay_a + decay_b);
                } else {
                    // Critically damped
                    const double decay = std::exp(-omega_n * t);
                    val = to + delta * (1.0 + omega_n * t) * decay;
                }
                
                push(val);
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
