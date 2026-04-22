#pragma once

#include <string>
#include <vector>
#include <memory>

namespace tachyon::expressions {

enum class ASTNodeType {
    Number,
    Variable,
    BinaryOp,
    UnaryOp,
    FunctionCall,
    PropertyAccess
};

struct ASTNode {
    virtual ~ASTNode() = default;
    virtual ASTNodeType type() const = 0;
};

struct NumberNode : public ASTNode {
    double value;
    explicit NumberNode(double v) : value(v) {}
    ASTNodeType type() const override { return ASTNodeType::Number; }
};

struct VariableNode : public ASTNode {
    std::string name;
    explicit VariableNode(std::string n) : name(std::move(n)) {}
    ASTNodeType type() const override { return ASTNodeType::Variable; }
};

enum class BinaryOperator {
    Add, Sub, Mul, Div, Pow
};

struct BinaryOpNode : public ASTNode {
    BinaryOperator op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    
    BinaryOpNode(BinaryOperator o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
    ASTNodeType type() const override { return ASTNodeType::BinaryOp; }
};

enum class UnaryOperator {
    Negate
};

struct UnaryOpNode : public ASTNode {
    UnaryOperator op;
    std::unique_ptr<ASTNode> operand;
    
    UnaryOpNode(UnaryOperator o, std::unique_ptr<ASTNode> opnd)
        : op(o), operand(std::move(opnd)) {}
    ASTNodeType type() const override { return ASTNodeType::UnaryOp; }
};

struct FunctionCallNode : public ASTNode {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> arguments;
    
    FunctionCallNode(std::string n, std::vector<std::unique_ptr<ASTNode>> args)
        : name(std::move(n)), arguments(std::move(args)) {}
    ASTNodeType type() const override { return ASTNodeType::FunctionCall; }
};

struct PropertyAccessNode : public ASTNode {
    std::unique_ptr<ASTNode> object;
    std::string property_name;

    PropertyAccessNode(std::unique_ptr<ASTNode> obj, std::string prop)
        : object(std::move(obj)), property_name(std::move(prop)) {}
    ASTNodeType type() const override { return ASTNodeType::PropertyAccess; }
};

} // namespace tachyon::expressions
