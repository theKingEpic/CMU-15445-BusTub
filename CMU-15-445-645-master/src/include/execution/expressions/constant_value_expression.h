//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// constant_value_expression.h
//
// Identification: src/include/expression/constant_value_expression.h
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
/*
 * 这段代码定义了一个名为 `ConstantValueExpression` 的类，该类位于 `bustub` 命名空间中。`ConstantValueExpression`
是一个表达式类，用于表示常量值。 `ConstantValueExpression` 类继承自
`AbstractExpression`，并添加了一些特定的属性和方法，用于处理常量值表达式。以下是对该类的详细分析：
1. 构造函数 `ConstantValueExpression` 接受一个 `Value` 类型的参数 `val`，并将其存储在成员变量 `val_` 中。
 同时，它调用基类 `AbstractExpression` 的构造函数，将表达式的类型设置为常量值的类型。
2. `Evaluate` 方法：重写基类的方法，返回存储在 `val_` 中的常量值。由于常量值不依赖于任何元组或模式，所以该方法不需要使用
`tuple` 或 `schema` 参数。
3. `EvaluateJoin` 方法：重写基类的方法，返回存储在 `val_` 中的常量值。同样，由于常量值不依赖于任何元组或模式，
 所以该方法不需要使用 `left_tuple`、`left_schema`、`right_tuple` 或 `right_schema` 参数。
4. `ToString` 方法：重写基类的方法，返回存储在 `val_` 中的常量值的字符串表示。
5. `BUSTUB_EXPR_CLONE_WITH_CHILDREN` 宏：这是一个克隆宏，用于实现表达式的克隆功能。
6. 成员变量 `val_`：存储常量值表达式中的常量值。
总的来说，`ConstantValueExpression` 类定义了一个用于表示常量值的表达式。这个类可以用于存储和评估查询中的常量值，
 例如在过滤条件或投影列表中使用。由于常量值不依赖于元组或模式，其评估结果始终是相同的常量值。
*/
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "execution/expressions/abstract_expression.h"

namespace bustub {
/**
 * ConstantValueExpression represents constants.
 */
class ConstantValueExpression : public AbstractExpression {
 public:
  /** Creates a new constant value expression wrapping the given value. */
  explicit ConstantValueExpression(const Value &val) : AbstractExpression({}, val.GetTypeId()), val_(val) {}

  auto Evaluate(const Tuple *tuple, const Schema &schema) const -> Value override { return val_; }

  auto EvaluateJoin(const Tuple *left_tuple, const Schema &left_schema, const Tuple *right_tuple,
                    const Schema &right_schema) const -> Value override {
    return val_;
  }

  /** @return the string representation of the plan node and its children */
  auto ToString() const -> std::string override { return val_.ToString(); }

  BUSTUB_EXPR_CLONE_WITH_CHILDREN(ConstantValueExpression);

  Value val_;
};
}  // namespace bustub
