//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// aggregation_executor.h
//
// Identification: src/include/execution/executors/aggregation_executor.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/util/hash_util.h"
#include "container/hash/hash_function.h"
#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/plans/aggregation_plan.h"
#include "storage/table/tuple.h"
#include "type/value_factory.h"

namespace bustub {

/**
 * A simplified hash table that has all the necessary functionality for aggregations.
 * 这是一个简化的哈希表，具有执行聚合操作所需的所有必要功能
 */
class SimpleAggregationHashTable {
 public:
  /**
   * 构造函数，接受聚合表达式和聚合类型作为参数。
   * Construct a new SimpleAggregationHashTable instance.
   * @param agg_exprs the aggregation expressions
   * @param agg_types the types of aggregations
   */
  SimpleAggregationHashTable(const std::vector<AbstractExpressionRef> &agg_exprs,
                             const std::vector<AggregationType> &agg_types)
      : agg_exprs_{agg_exprs}, agg_types_{agg_types} {}

  /** @return The initial aggregate value for this aggregation executor
   * 生成初始聚合值的函数*/
  auto GenerateInitialAggregateValue() -> AggregateValue {
    std::vector<Value> values{};
    for (const auto &agg_type : agg_types_) {
      switch (agg_type) {
        case AggregationType::CountStarAggregate:
          // Count start starts at zero.
          values.emplace_back(ValueFactory::GetIntegerValue(0));
          break;
        case AggregationType::CountAggregate:
        case AggregationType::SumAggregate:
        case AggregationType::MinAggregate:
        case AggregationType::MaxAggregate:
          // Others starts at null.
          values.emplace_back(ValueFactory::GetNullValueByType(TypeId::INTEGER));
          break;
      }
    }
    return {values};
  }

  /**
   * TODO(Student)
   * 合并聚合值的函数
   * 聚合表达式的具体值aggregates_是指在执行聚合操作时，每个分组中的值，这些值被用来进行聚合计算
   * Combines the input into the aggregation result.
   * @param[out] result The output aggregate value
   * @param input The input value
   */
  void CombineAggregateValues(AggregateValue *result, const AggregateValue &input) {
    // 依照不同的聚合操作类型（agg_types_）进行不同的操作
    for (uint32_t i = 0; i < agg_exprs_.size(); i++) {
      // 获取结果值和输入值
      Value &old_val = result->aggregates_[i];
      const Value &new_val = input.aggregates_[i];

      // 遍历每个聚合表达式
      switch (agg_types_[i]) {
        // 无论Value是否为null，均统计其数目
        case AggregationType::CountStarAggregate:
          // 累加1到结果值中
          // 如果输入值不为null，则累加1到结果值中
          if (old_val.IsNull()) {
            // 如果结果值为null，则初始化为0
            old_val = ValueFactory::GetIntegerValue(0);
          }
          old_val = old_val.Add(Value(TypeId::INTEGER, 1));
          break;

          // 统计非null值
        case AggregationType::CountAggregate:
          // 如果输入值不为null，则累加1到结果值中
          if (!new_val.IsNull()) {
            if (old_val.IsNull()) {
              // 如果结果值为null，则初始化为0
              old_val = ValueFactory::GetIntegerValue(0);
            }
            old_val = old_val.Add(Value(TypeId::INTEGER, 1));
          }
          break;

          // 计算非null值的总和
        case AggregationType::SumAggregate:
          // 如果新值不为null，则累加到结果值中
          if (!new_val.IsNull()) {
            if (old_val.IsNull()) {
              // 如果结果值为null，则设置为新值
              old_val = new_val;
            } else {
              // 否则，将新值加到结果值上
              old_val = old_val.Add(new_val);
            }
          }
          break;

          // 找出非null值中的最小值
        case AggregationType::MinAggregate:
          // 如果新值不为null，则比较并更新结果值
          if (!new_val.IsNull()) {
            if (old_val.IsNull()) {
              // 如果结果值为null，则设置为新值
              old_val = new_val;
            } else {
              // 否则，如果新值小于当前值，则更新结果值为新值
              old_val = new_val.CompareLessThan(old_val) == CmpBool::CmpTrue ? new_val.Copy() : old_val;
            }
          }
          break;

          // 找出非null值中的最大值
        case AggregationType::MaxAggregate:
          // 如果新值不为null，则比较并更新结果值
          if (!new_val.IsNull()) {
            if (old_val.IsNull()) {
              // 如果结果值为null，则设置为新值
              old_val = new_val;
            } else {
              // 否则，如果新值大于当前值，则更新结果值为新值
              old_val = new_val.CompareGreaterThan(old_val) == CmpBool::CmpTrue ? new_val.Copy() : old_val;
            }
          }
          break;
      }
    }
  }

  /** 插入并合并值的函数
   * Inserts a value into the hash table and then combines it with the current aggregation.
   * @param agg_key the key to be inserted
   * @param agg_val the value to be inserted
   */
  void InsertCombine(const AggregateKey &agg_key, const AggregateValue &agg_val) {
    if (ht_.count(agg_key) == 0) {
      ht_.insert({agg_key, GenerateInitialAggregateValue()});
    }
    CombineAggregateValues(&ht_[agg_key], agg_val);
  }

  /** 清空哈希表的函数
   * Clear the hash table
   */
  void Clear() { ht_.clear(); }

  /** An iterator over the aggregation hash table
   * 定义迭代器类，用于遍历聚合哈希表*/
  class Iterator {
   public:
    /** Creates an iterator for the aggregate map. */
    explicit Iterator(std::unordered_map<AggregateKey, AggregateValue>::const_iterator iter) : iter_{iter} {}

    /** @return The key of the iterator */
    auto Key() -> const AggregateKey & { return iter_->first; }

    /** @return The value of the iterator */
    auto Val() -> const AggregateValue & { return iter_->second; }

    /** @return The iterator before it is incremented */
    auto operator++() -> Iterator & {
      ++iter_;
      return *this;
    }

    /** @return `true` if both iterators are identical */
    auto operator==(const Iterator &other) -> bool { return this->iter_ == other.iter_; }

    /** @return `true` if both iterators are different */
    auto operator!=(const Iterator &other) -> bool { return this->iter_ != other.iter_; }

   private:
    /** Aggregates map */
    std::unordered_map<AggregateKey, AggregateValue>::const_iterator iter_;
  };

  /** @return Iterator to the start of the hash table */
  auto Begin() -> Iterator { return Iterator{ht_.cbegin()}; }

  /** @return Iterator to the end of the hash table */
  auto End() -> Iterator { return Iterator{ht_.cend()}; }

 private:
  /** The hash table is just a map from aggregate keys to aggregate values */
  std::unordered_map<AggregateKey, AggregateValue> ht_{};
  /** The aggregate expressions that we have */
  const std::vector<AbstractExpressionRef> &agg_exprs_;
  /** The types of aggregations that we have */
  const std::vector<AggregationType> &agg_types_;
};

/**定义AggregationExecutor类，这是一个执行聚合操作的执行器
 * AggregationExecutor executes an aggregation operation (e.g. COUNT, SUM, MIN, MAX)
 * over the tuples produced by a child executor.
 */
class AggregationExecutor : public AbstractExecutor {
 public:
  /**
   * Construct a new AggregationExecutor instance.
   * @param exec_ctx The executor context
   * @param plan The insert plan to be executed
   * @param child_executor The child executor from which inserted tuples are pulled (may be `nullptr`)
   */
  AggregationExecutor(ExecutorContext *exec_ctx, const AggregationPlanNode *plan,
                      std::unique_ptr<AbstractExecutor> &&child_executor);

  /** Initialize the aggregation */
  void Init() override;

  /**
   * Yield the next tuple from the insert.
   * @param[out] tuple The next tuple produced by the aggregation
   * @param[out] rid The next tuple RID produced by the aggregation
   * @return `true` if a tuple was produced, `false` if there are no more tuples
   */
  auto Next(Tuple *tuple, RID *rid) -> bool override;

  /** @return The output schema for the aggregation */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); };

  /** Do not use or remove this function, otherwise you will get zero points. */
  auto GetChildExecutor() const -> const AbstractExecutor *;

 private:
  /** @return The tuple as an AggregateKey
   * 生成聚合键的函数*/
  auto MakeAggregateKey(const Tuple *tuple) -> AggregateKey {
    std::vector<Value> keys;
    for (const auto &expr : plan_->GetGroupBys()) {
      keys.emplace_back(expr->Evaluate(tuple, child_executor_->GetOutputSchema()));
    }
    return {keys};
  }

  /** @return The tuple as an AggregateValue */
  auto MakeAggregateValue(const Tuple *tuple) -> AggregateValue {
    std::vector<Value> vals;
    for (const auto &expr : plan_->GetAggregates()) {
      vals.emplace_back(expr->Evaluate(tuple, child_executor_->GetOutputSchema()));
    }
    return {vals};
  }

 private:
  /** The aggregation plan node */
  const AggregationPlanNode *plan_;

  /** The child executor that produces tuples over which the aggregation is computed */
  std::unique_ptr<AbstractExecutor> child_executor_;

  /** Simple aggregation hash table */
  // TODO(Student): Uncomment SimpleAggregationHashTable aht_;
  std::unique_ptr<SimpleAggregationHashTable> aht_;
  /** Simple aggregation hash table iterator */
  // TODO(Student): Uncomment SimpleAggregationHashTable::Iterator aht_iterator_;
  std::unique_ptr<SimpleAggregationHashTable::Iterator> aht_iterator_;
  // bool has_inserted_;
  bool copy_with_empty_;
};
}  // namespace bustub
