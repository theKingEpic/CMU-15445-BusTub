//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// abstract_expression.h
//
// Identification: src/include/expression/abstract_expression.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
/*
 * 这段代码定义了一个名为 `AbstractExpression` 的类，该类位于 `bustub` 命名空间中。`AbstractExpression`
是系统中所有表达式的基类，
 * 表达式被建模为树，即每个表达式可以有一个可变数量的子表达式。
以下是对该类的详细分析：
1. 构造函数 `AbstractExpression` 接受两个参数：子表达式的列表 `children` 和表达式的返回类型
`ret_type`。它将这些参数存储在成员变量 `children_` 和 `ret_type_` 中。
2. 虚析构函数：确保派生类的析构函数被正确调用。
3. `Evaluate` 方法：一个纯虚方法，用于计算表达式在给定元组和模式下的值。这个方法需要由派生类实现。
4. `EvaluateJoin` 方法：一个纯虚方法，用于计算表达式在左右两个元组连接时的值。这个方法需要由派生类实现。
5. `GetChildAt` 方法：返回指定索引处的子表达式。
6. `GetChildren` 方法：返回所有子表达式的列表。
7. `GetReturnType` 方法：返回表达式的返回类型。
8. `ToString` 方法：返回表达式的字符串表示。默认实现返回 `<unknown>`，派生类可以重写这个方法以提供更具体的字符串表示。
9. `CloneWithChildren` 方法：一个纯虚方法，用于创建具有新子表达式的新表达式。这个方法需要由派生类实现。
10. `BUSTUB_EXPR_CLONE_WITH_CHILDREN` 宏：这是一个克隆宏，用于实现表达式的克隆功能，包括其子表达式。
11. 成员变量 `children_`：存储表达式子节点的列表。
12. 成员变量 `ret_type_`：存储表达式的返回类型。
此外，代码还包含了一些模板特化的 `fmt::formatter` 结构体，用于格式化 `AbstractExpression` 及其派生类的对象。
 这些特化使得可以使用 `fmt` 库来格式化表达式对象，例如在打印调试信息时。
总的来说，`AbstractExpression`
类为表达式树提供了一个基类，其中包含了一些通用的方法和属性，以及一些需要由派生类实现的纯虚方法。
 这个类为表达式求值、表达式树的遍历和表达式树的克隆提供了一个框架。
*/
#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "catalog/schema.h"
#include "fmt/format.h"
#include "storage/table/tuple.h"

#define BUSTUB_EXPR_CLONE_WITH_CHILDREN(cname)                                                                   \
  auto CloneWithChildren(std::vector<AbstractExpressionRef> children) const->std::unique_ptr<AbstractExpression> \
      override {                                                                                                 \
    auto expr = cname(*this);                                                                                    \
    expr.children_ = children;                                                                                   \
    return std::make_unique<cname>(std::move(expr));                                                             \
  }

namespace bustub {

class AbstractExpression;
using AbstractExpressionRef = std::shared_ptr<AbstractExpression>;

/**
 * AbstractExpression is the base class of all the expressions in the system.
 * Expressions are modeled as trees, i.e. every expression may have a variable number of children.
 * AbstractExpression是系统中所有表达式的基类。表达式被建模为树，即每个表达式可以有一个可变数量的子表达式。
 */
class AbstractExpression {
 public:
  /**
   * Create a new AbstractExpression with the given children and return type.
   * @param children the children of this abstract expression
   * @param ret_type the return type of this abstract expression when it is evaluated
   * 此抽象表达式在求值时的返回类型
   */
  AbstractExpression(std::vector<AbstractExpressionRef> children, TypeId ret_type)
      : children_{std::move(children)}, ret_type_{ret_type} {}

  /** Virtual destructor. */
  virtual ~AbstractExpression() = default;

  /** @return The value obtained by evaluating the tuple with the given schema
   * 用给定模式对元组求值获得的值*/
  virtual auto Evaluate(const Tuple *tuple, const Schema &schema) const -> Value = 0;

  /**
   * Returns the value obtained by evaluating a JOIN.
   * @param left_tuple The left tuple
   * @param left_schema The left tuple's schema
   * @param right_tuple The right tuple
   * @param right_schema The right tuple's schema
   * @return The value obtained by evaluating a JOIN on the left and right
   * 通过对左右两个JOIN求值获得的值
   */
  virtual auto EvaluateJoin(const Tuple *left_tuple, const Schema &left_schema, const Tuple *right_tuple,
                            const Schema &right_schema) const -> Value = 0;

  /** @return the child_idx'th child of this expression
   * 该表达式的第child_idx个子节点*/
  auto GetChildAt(uint32_t child_idx) const -> const AbstractExpressionRef & { return children_[child_idx]; }

  /** @return the children of this expression, ordering may matter */
  auto GetChildren() const -> const std::vector<AbstractExpressionRef> & { return children_; }

  /** @return the type of this expression if it were to be evaluated */
  virtual auto GetReturnType() const -> TypeId { return ret_type_; }

  /** @return the string representation of the plan node and its children */
  virtual auto ToString() const -> std::string { return "<unknown>"; }

  /** @return a new expression with new children */
  virtual auto CloneWithChildren(std::vector<AbstractExpressionRef> children) const
      -> std::unique_ptr<AbstractExpression> = 0;

  /** The children of this expression. Note that the order of appearance of children may matter. */
  std::vector<AbstractExpressionRef> children_;

 private:
  /** The return type of this expression. */
  TypeId ret_type_;
};

}  // namespace bustub

template <typename T>
struct fmt::formatter<T, std::enable_if_t<std::is_base_of<bustub::AbstractExpression, T>::value, char>>
    : fmt::formatter<std::string> {
  template <typename FormatCtx>
  auto format(const T &x, FormatCtx &ctx) const {
    return fmt::formatter<std::string>::format(x.ToString(), ctx);
  }
};

template <typename T>
struct fmt::formatter<std::unique_ptr<T>, std::enable_if_t<std::is_base_of<bustub::AbstractExpression, T>::value, char>>
    : fmt::formatter<std::string> {
  template <typename FormatCtx>
  auto format(const std::unique_ptr<T> &x, FormatCtx &ctx) const {
    if (x != nullptr) {
      return fmt::formatter<std::string>::format(x->ToString(), ctx);
    }
    return fmt::formatter<std::string>::format("", ctx);
  }
};

template <typename T>
struct fmt::formatter<std::shared_ptr<T>, std::enable_if_t<std::is_base_of<bustub::AbstractExpression, T>::value, char>>
    : fmt::formatter<std::string> {
  template <typename FormatCtx>
  auto format(const std::shared_ptr<T> &x, FormatCtx &ctx) const {
    if (x != nullptr) {
      return fmt::formatter<std::string>::format(x->ToString(), ctx);
    }
    return fmt::formatter<std::string>::format("", ctx);
  }
};
