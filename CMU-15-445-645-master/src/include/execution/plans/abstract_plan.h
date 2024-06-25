//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// abstract_plan.h
//
// Identification: src/include/execution/plans/abstract_plan.h
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "catalog/schema.h"
#include "fmt/format.h"

namespace bustub {
/*
 * 这个宏定义的目的是为 AbstractPlanNode 的子类提供以下功能：

    创建一个子类的克隆对象。
    设置克隆对象的子节点。
    返回克隆对象的智能指针。
 */
#define BUSTUB_PLAN_NODE_CLONE_WITH_CHILDREN(cname)                                                          \
  auto CloneWithChildren(std::vector<AbstractPlanNodeRef> children) const->std::unique_ptr<AbstractPlanNode> \
      override {                                                                                             \
    auto plan_node = cname(*this);                                                                           \
    plan_node.children_ = children;                                                                          \
    return std::make_unique<cname>(std::move(plan_node));                                                    \
  }

/** PlanType represents the types of plans that we have in our system.
 * PlanType 枚举：定义了查询计划中的不同类型，如顺序扫描（SeqScan）、索引扫描（IndexScan）、插入（Insert）、
 * 更新（Update）、删除（Delete）、聚合（Aggregation）、限制（Limit）、嵌套循环连接（NestedLoopJoin）、
 * 嵌套索引连接（NestedIndexJoin）、哈希连接（HashJoin）、过滤（Filter）、值（Values）、投影（Projection）、
 * 排序（Sort）、TOP-N（TopN）、TOP-N按组（TopNPerGroup）、模拟扫描（MockScan）、初始检查（InitCheck）和窗口（Window）。*/
enum class PlanType {
  SeqScan,
  IndexScan,
  Insert,
  Update,
  Delete,
  Aggregation,
  Limit,
  NestedLoopJoin,
  NestedIndexJoin,
  HashJoin,
  Filter,
  Values,
  Projection,
  Sort,
  TopN,
  TopNPerGroup,
  MockScan,
  InitCheck,
  Window
};

class AbstractPlanNode;
using AbstractPlanNodeRef = std::shared_ptr<const AbstractPlanNode>;

/**
 * AbstractPlanNode represents all the possible types of plan nodes in our system.
 * Plan nodes are modeled as trees, so each plan node can have a variable number of children.
 * Per the Volcano model, the plan node receives the tuples of its children.
 * The ordering of the children may matter.
 */
class AbstractPlanNode {
 public:
  /**
   * Create a new AbstractPlanNode with the specified output schema and children.
   * 表示查询计划中的一个节点，可以有多个子节点。每个节点都有一个输出模式（output_schema），
   * 用于描述该节点输出的元组结构。节点还可以有子节点（children），子节点按照某种顺序排列，并且每个子节点都有自己的输出模式。
   * @param output_schema The schema for the output of this plan node
   * @param children The children of this plan node
   */
  AbstractPlanNode(SchemaRef output_schema, std::vector<AbstractPlanNodeRef> children)
      : output_schema_(std::move(output_schema)), children_(std::move(children)) {}

  /** Virtual destructor. */
  virtual ~AbstractPlanNode() = default;

  /** @return the schema for the output of this plan node */
  auto OutputSchema() const -> const Schema & { return *output_schema_; }

  /** @return the child of this plan node at index child_idx */
  auto GetChildAt(uint32_t child_idx) const -> AbstractPlanNodeRef { return children_[child_idx]; }

  /** @return the children of this plan node */
  auto GetChildren() const -> const std::vector<AbstractPlanNodeRef> & { return children_; }

  /** @return the type of this plan node */
  virtual auto GetType() const -> PlanType = 0;

  /** @return the string representation of the plan node and its children */
  auto ToString(bool with_schema = true) const -> std::string {
    if (with_schema) {
      return fmt::format("{} | {}{}", PlanNodeToString(), output_schema_, ChildrenToString(2, with_schema));
    }
    return fmt::format("{}{}", PlanNodeToString(), ChildrenToString(2, with_schema));
  }

  /** @return the cloned plan node with new children */
  virtual auto CloneWithChildren(std::vector<AbstractPlanNodeRef> children) const
      -> std::unique_ptr<AbstractPlanNode> = 0;

  /**
   * The schema for the output of this plan node. In the volcano model, every plan node will spit out tuples,
   * and this tells you what schema this plan node's tuples will have.
   */
  SchemaRef output_schema_;

  /** The children of this plan node. */
  std::vector<AbstractPlanNodeRef> children_;

 protected:
  /** @return the string representation of the plan node itself */
  virtual auto PlanNodeToString() const -> std::string { return "<unknown>"; }

  /** @return the string representation of the plan node's children */
  auto ChildrenToString(int indent, bool with_schema = true) const -> std::string;

 private:
};

}  // namespace bustub

template <typename T>
struct fmt::formatter<T, std::enable_if_t<std::is_base_of<bustub::AbstractPlanNode, T>::value, char>>
    : fmt::formatter<std::string> {
  template <typename FormatCtx>
  auto format(const T &x, FormatCtx &ctx) const {
    return fmt::formatter<std::string>::format(x.ToString(), ctx);
  }
};

template <typename T>
struct fmt::formatter<std::unique_ptr<T>, std::enable_if_t<std::is_base_of<bustub::AbstractPlanNode, T>::value, char>>
    : fmt::formatter<std::string> {
  template <typename FormatCtx>
  auto format(const std::unique_ptr<T> &x, FormatCtx &ctx) const {
    return fmt::formatter<std::string>::format(x->ToString(), ctx);
  }
};
