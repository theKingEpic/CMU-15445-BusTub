//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// aggregation_plan.h
//
// Identification: src/include/execution/plans/aggregation_plan.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
/*
 * 这段代码定义了BusTub数据库管理系统中的聚合计划节点，它支持SQL中的聚合函数，如COUNT、SUM、MIN和MAX。具体功能如下：
1. `AggregationType`枚举：列出了所有支持的聚合函数类型。
2. `AggregationPlanNode`类：表示SQL聚合函数的计划节点，可以处理COUNT、SUM、MIN和MAX等聚合操作。
3. 构造函数：接受输出模式、子计划、分组表达式、聚合表达式和聚合类型作为参数，创建聚合计划节点。
4. `GetType()`函数：返回计划节点的类型（聚合）。
5. `GetChildPlan()`函数：返回聚合计划节点的子节点。
6. `GetGroupByAt()`函数：返回指定索引处的分组表达式。
7. `GetGroupBys()`函数：返回所有分组表达式。
8. `GetAggregateAt()`函数：返回指定索引处的聚合表达式。
9. `GetAggregates()`函数：返回所有聚合表达式。
10. `GetAggregateTypes()`函数：返回所有聚合类型。
11. `InferAggSchema()`函数：根据分组表达式、聚合表达式和聚合类型推断出输出模式。
12. `AggregateKey`结构体：表示聚合操作中的键，包含分组值，并重载了`==`运算符以比较两个聚合键是否相等。
13. `AggregateValue`结构体：表示每个运行聚合的值。
14. `std::hash`模板特化：为`AggregateKey`定义哈希函数，以支持对聚合键进行哈希。
15. `fmt::formatter`模板特化：为`AggregationType`定义格式化器，以支持将聚合类型格式化为字符串。
 */
#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "common/util/hash_util.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/plans/abstract_plan.h"
#include "fmt/format.h"
#include "storage/table/tuple.h"

namespace bustub {

/** AggregationType enumerates all the possible aggregation functions in our system */
// CountStarAggregate 是COUNT(*) 函数。计算一个查询结果中的行数，
// CountAggregate 对应于 COUNT 函数，用于计算一个查询结果中特定列或表达式的非空值数量
enum class AggregationType { CountStarAggregate, CountAggregate, SumAggregate, MinAggregate, MaxAggregate };

/**
 * AggregationPlanNode represents the various SQL aggregation functions.
 * For example, COUNT(), SUM(), MIN() and MAX().
 *
 * NOTE: To simplify this project, AggregationPlanNode must always have exactly one child.
 */
class AggregationPlanNode : public AbstractPlanNode {
 public:
  /**
   * Construct a new AggregationPlanNode.
   * @param output_schema The output format of this plan node
   * @param child The child plan to aggregate data over
   * @param group_bys The group by clause of the aggregation
   * @param aggregates The expressions that we are aggregating
   * @param agg_types The types that we are aggregating
   */
  AggregationPlanNode(SchemaRef output_schema, AbstractPlanNodeRef child, std::vector<AbstractExpressionRef> group_bys,
                      std::vector<AbstractExpressionRef> aggregates, std::vector<AggregationType> agg_types)
      : AbstractPlanNode(std::move(output_schema), {std::move(child)}),
        group_bys_(std::move(group_bys)),
        aggregates_(std::move(aggregates)),
        agg_types_(std::move(agg_types)) {}

  /** @return The type of the plan node */
  auto GetType() const -> PlanType override { return PlanType::Aggregation; }

  /** @return the child of this aggregation plan node */
  auto GetChildPlan() const -> AbstractPlanNodeRef {
    BUSTUB_ASSERT(GetChildren().size() == 1, "Aggregation expected to only have one child.");
    return GetChildAt(0);
  }

  /** @return The idx'th group by expression */
  auto GetGroupByAt(uint32_t idx) const -> const AbstractExpressionRef & { return group_bys_[idx]; }

  /** @return The group by expressions */
  auto GetGroupBys() const -> const std::vector<AbstractExpressionRef> & { return group_bys_; }

  /** @return The idx'th aggregate expression */
  auto GetAggregateAt(uint32_t idx) const -> const AbstractExpressionRef & { return aggregates_[idx]; }

  /** @return The aggregate expressions */
  auto GetAggregates() const -> const std::vector<AbstractExpressionRef> & { return aggregates_; }

  /** @return The aggregate types */
  auto GetAggregateTypes() const -> const std::vector<AggregationType> & { return agg_types_; }

  static auto InferAggSchema(const std::vector<AbstractExpressionRef> &group_bys,
                             const std::vector<AbstractExpressionRef> &aggregates,
                             const std::vector<AggregationType> &agg_types) -> Schema;

  BUSTUB_PLAN_NODE_CLONE_WITH_CHILDREN(AggregationPlanNode);

  /** The GROUP BY expressions */
  std::vector<AbstractExpressionRef> group_bys_;
  /** The aggregation expressions */
  std::vector<AbstractExpressionRef> aggregates_;
  /** The aggregation types */
  std::vector<AggregationType> agg_types_;

 protected:
  auto PlanNodeToString() const -> std::string override;
};

/** AggregateKey represents a key in an aggregation operation */
struct AggregateKey {
  /** The group-by values */
  std::vector<Value> group_bys_;

  /**
   * Compares two aggregate keys for equality.
   * @param other the other aggregate key to be compared with
   * @return `true` if both aggregate keys have equivalent group-by expressions, `false` otherwise
   */
  auto operator==(const AggregateKey &other) const -> bool {
    for (uint32_t i = 0; i < other.group_bys_.size(); i++) {
      if (group_bys_[i].CompareEquals(other.group_bys_[i]) != CmpBool::CmpTrue) {
        return false;
      }
    }
    return true;
  }
};

/** AggregateValue represents a value for each of the running aggregates */
struct AggregateValue {
  /** The aggregate values */
  std::vector<Value> aggregates_;
};

}  // namespace bustub

namespace std {

/** Implements std::hash on AggregateKey */
template <>
struct hash<bustub::AggregateKey> {
  auto operator()(const bustub::AggregateKey &agg_key) const -> std::size_t {
    size_t curr_hash = 0;
    for (const auto &key : agg_key.group_bys_) {
      if (!key.IsNull()) {
        curr_hash = bustub::HashUtil::CombineHashes(curr_hash, bustub::HashUtil::HashValue(&key));
      }
    }
    return curr_hash;
  }
};

}  // namespace std

template <>
struct fmt::formatter<bustub::AggregationType> : formatter<std::string> {
  template <typename FormatContext>
  auto format(bustub::AggregationType c, FormatContext &ctx) const {
    using bustub::AggregationType;
    std::string name = "unknown";
    switch (c) {
      case AggregationType::CountStarAggregate:
        name = "count_star";
        break;
      case AggregationType::CountAggregate:
        name = "count";
        break;
      case AggregationType::SumAggregate:
        name = "sum";
        break;
      case AggregationType::MinAggregate:
        name = "min";
        break;
      case AggregationType::MaxAggregate:
        name = "max";
        break;
    }
    return formatter<std::string>::format(name, ctx);
  }
};
