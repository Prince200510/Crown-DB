#ifndef CROWNDB_EXPRESSION_H
#define CROWNDB_EXPRESSION_H
#include "Types.h"
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

enum class OpType {
    Equal,
    NotEqual,
    LessThan,
    LessEqual,
    GreaterThan,
    GreaterEqual,
    And,
    Or,
    Not,
    Add,
    Subtract,
    Multiply,
    Divide
};

class Expression {
public:
    virtual ~Expression() = default;
    virtual TypedValue evaluate(const std::vector<std::string>& col_names, const std::vector<TypedValue>& row) const = 0;
};

class ConstantExpression : public Expression {
private:
    TypedValue val_;

public:
    explicit ConstantExpression(TypedValue val) : val_(std::move(val)) {}

    TypedValue evaluate(const std::vector<std::string>&, const std::vector<TypedValue>&) const override {
        return val_;
    }
};

class ColumnRefExpression : public Expression {
private:
    std::string col_name_;

public:
    explicit ColumnRefExpression(std::string name) : col_name_(std::move(name)) {}
    const std::string& col_name() const { 
        return col_name_; 
    }

    TypedValue evaluate(const std::vector<std::string>& col_names, const std::vector<TypedValue>& row) const override {
        for (size_t i = 0; i < col_names.size(); ++i) {
            if (col_names[i] == col_name_) {
                if (i < row.size()) return row[i];
                break;
            }
        }
        return TypedValue::make_null();
    }
};

class UnaryExpression : public Expression {
private:
    OpType op_;
    std::shared_ptr<Expression> child_;

public:
    UnaryExpression(OpType op, std::shared_ptr<Expression> child)
        : op_(op), child_(std::move(child)) {}

    TypedValue evaluate(const std::vector<std::string>& col_names, const std::vector<TypedValue>& row) const override {
        TypedValue val = child_->evaluate(col_names, row);
        if (op_ == OpType::Not) {
            if (val.type() == DataType::Boolean) {
                return TypedValue(!val.get_boolean());
            }
            if (val.type() == DataType::Integer) {
                return TypedValue(val.get_integer() == 0);
            }
            return TypedValue(false);
        }
        if (op_ == OpType::Subtract) {
            if (val.type() == DataType::Integer) {
                return TypedValue(-val.get_integer());
            }
            if (val.type() == DataType::Float) {
                return TypedValue(-val.get_float());
            }
        }
        return val;
    }
};

class BinaryExpression : public Expression {
private:
    OpType op_;
    std::shared_ptr<Expression> left_;
    std::shared_ptr<Expression> right_;

    static double to_double(const TypedValue& v) {
        if (v.type() == DataType::Integer) return static_cast<double>(v.get_integer());
        if (v.type() == DataType::Float) return v.get_float();
        return 0.0;
    }

    static bool is_num(const TypedValue& v) {
        return v.type() == DataType::Integer || v.type() == DataType::Float;
    }

public:
    BinaryExpression(OpType op, std::shared_ptr<Expression> left, std::shared_ptr<Expression> right)
        : op_(op), left_(std::move(left)), right_(std::move(right)) {}

    OpType op() const { return op_; }
    const std::shared_ptr<Expression>& left() const { return left_; }
    const std::shared_ptr<Expression>& right() const { return right_; }

    TypedValue evaluate(const std::vector<std::string>& col_names, const std::vector<TypedValue>& row) const override {
        TypedValue l = left_->evaluate(col_names, row);

        if (op_ == OpType::And) {
            bool l_bool = (l.type() == DataType::Boolean) ? l.get_boolean() : (l.type() == DataType::Integer && l.get_integer() != 0);
            if (!l_bool) return TypedValue(false);
            TypedValue r = right_->evaluate(col_names, row);
            bool r_bool = (r.type() == DataType::Boolean) ? r.get_boolean() : (r.type() == DataType::Integer && r.get_integer() != 0);
            return TypedValue(r_bool);
        }

        if (op_ == OpType::Or) {
            bool l_bool = (l.type() == DataType::Boolean) ? l.get_boolean() : (l.type() == DataType::Integer && l.get_integer() != 0);
            if (l_bool) return TypedValue(true);
            TypedValue r = right_->evaluate(col_names, row);
            bool r_bool = (r.type() == DataType::Boolean) ? r.get_boolean() : (r.type() == DataType::Integer && r.get_integer() != 0);
            return TypedValue(r_bool);
        }

        TypedValue r = right_->evaluate(col_names, row);

        if (op_ == OpType::Equal) {
            if (is_num(l) && is_num(r)) {
                return TypedValue(to_double(l) == to_double(r));
            }
            if (l.type() == DataType::Text && r.type() == DataType::Text) {
                return TypedValue(l.get_text() == r.get_text());
            }
            if (l.type() == DataType::Boolean && r.type() == DataType::Boolean) {
                return TypedValue(l.get_boolean() == r.get_boolean());
            }
            if (l.type() == DataType::Date && r.type() == DataType::Date) {
                return TypedValue(l.get_date() == r.get_date());
            }
            return TypedValue(l == r);
        }

        if (op_ == OpType::NotEqual) {
            if (is_num(l) && is_num(r)) {
                return TypedValue(to_double(l) != to_double(r));
            }
            if (l.type() == DataType::Text && r.type() == DataType::Text) {
                return TypedValue(l.get_text() != r.get_text());
            }
            if (l.type() == DataType::Boolean && r.type() == DataType::Boolean) {
                return TypedValue(l.get_boolean() != r.get_boolean());
            }
            if (l.type() == DataType::Date && r.type() == DataType::Date) {
                return TypedValue(l.get_date() != r.get_date());
            }
            return TypedValue(l != r);
        }

        if (is_num(l) && is_num(r)) {
            double dl = to_double(l);
            double dr = to_double(r);
            if (op_ == OpType::LessThan) return TypedValue(dl < dr);
            if (op_ == OpType::LessEqual) return TypedValue(dl <= dr);
            if (op_ == OpType::GreaterThan) return TypedValue(dl > dr);
            if (op_ == OpType::GreaterEqual) return TypedValue(dl >= dr);
            if (op_ == OpType::Add) return TypedValue(dl + dr);
            if (op_ == OpType::Subtract) return TypedValue(dl - dr);
            if (op_ == OpType::Multiply) return TypedValue(dl * dr);
            if (op_ == OpType::Divide) return TypedValue(dr != 0.0 ? dl / dr : 0.0);
        }

        return TypedValue(false);
    }
};

#endif
