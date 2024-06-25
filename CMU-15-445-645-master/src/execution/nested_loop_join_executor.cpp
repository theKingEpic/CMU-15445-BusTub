//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// nested_loop_join_executor.cpp
//
// Identification: src/execution/nested_loop_join_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/nested_loop_join_executor.h"
#include "binder/table_ref/bound_join_ref.h"
#include "common/exception.h"
#include "type/value_factory.h"

namespace bustub {

NestedLoopJoinExecutor::NestedLoopJoinExecutor(ExecutorContext *exec_ctx, const NestedLoopJoinPlanNode *plan,
                                               std::unique_ptr<AbstractExecutor> &&left_executor,
                                               std::unique_ptr<AbstractExecutor> &&right_executor)
    : AbstractExecutor(exec_ctx) {
  this->plan_ = plan;
  this->left_executor_ = std::move(left_executor);
  this->right_executor_ = std::move(right_executor);
  if (!(plan->GetJoinType() == JoinType::LEFT || plan->GetJoinType() == JoinType::INNER)) {
    // Note for 2023 Fall: You ONLY need to implement left join and inner join.
    throw bustub::NotImplementedException(fmt::format("join type {} not supported", plan->GetJoinType()));
  }
}

void NestedLoopJoinExecutor::Init() {
  left_executor_->Init();
  right_executor_->Init();
  left_bool_ = left_executor_->Next(&left_tuple_, &left_rid_);
  has_done_ = false;
}

auto NestedLoopJoinExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  Tuple right_tuple{};
  RID right_rid{};
  auto predicate = plan_->Predicate();
  if (plan_->GetJoinType() == JoinType::LEFT) {
    // 左连接
    while (left_bool_) {
      while (right_executor_->Next(&right_tuple, &right_rid)) {
        // 判断是否满足连接条件
        auto join_result = predicate->EvaluateJoin(&left_tuple_, left_executor_->GetOutputSchema(), &right_tuple,
                                                   right_executor_->GetOutputSchema());
        // 对每一个左边的元组进行判断右边有没有和它匹配的，若有join_result.GetAs为true
        if (join_result.GetAs<bool>()) {
          std::vector<Value> values;
          // 一个元组对应多个值，要把所有值都放进去
          for (uint32_t i = 0; i < this->left_executor_->GetOutputSchema().GetColumnCount(); i++) {
            values.emplace_back(left_tuple_.GetValue(&this->left_executor_->GetOutputSchema(), i));
          }
          // 连接操作右边元组的值均不为null的情况下
          for (uint32_t i = 0; i < this->right_executor_->GetOutputSchema().GetColumnCount(); i++) {
            values.emplace_back(right_tuple.GetValue(&this->right_executor_->GetOutputSchema(), i));
          }
          *tuple = Tuple{values, &GetOutputSchema()};
          has_done_ = true;
          return true;
        }
      }
      // has done为false证明左表元组的找不到右边可以匹配的
      if (!has_done_) {
        std::vector<Value> values;
        // 文档要求：左连接操作要求左边的元组都存在，如果不存在满足条件的右边元组，则将右边元组的值都设置为null
        for (uint32_t i = 0; i < this->left_executor_->GetOutputSchema().GetColumnCount(); i++) {
          values.emplace_back(left_tuple_.GetValue(&this->left_executor_->GetOutputSchema(), i));
        }
        // 连接操作右边元组的值均为null
        for (uint32_t i = 0; i < this->right_executor_->GetOutputSchema().GetColumnCount(); i++) {
          values.emplace_back(
              ValueFactory::GetNullValueByType(this->right_executor_->GetOutputSchema().GetColumn(i).GetType()));
        }
        *tuple = Tuple{values, &GetOutputSchema()};
        has_done_ = true;
        return true;
      }
      // 接着匹配左表的下个元组
      left_bool_ = left_executor_->Next(&this->left_tuple_, &this->left_rid_);
      right_executor_->Init();
      has_done_ = false;
    }
    return false;
  }
  // 内连接和左外连接差不多

  // 内连接
  while (left_bool_) {
    while (right_executor_->Next(&right_tuple, &right_rid)) {
      auto join_result = predicate->EvaluateJoin(&left_tuple_, left_executor_->GetOutputSchema(), &right_tuple,
                                                 right_executor_->GetOutputSchema());
      if (join_result.GetAs<bool>()) {
        std::vector<Value> values;
        for (uint32_t i = 0; i < left_executor_->GetOutputSchema().GetColumnCount(); i++) {
          values.emplace_back(left_tuple_.GetValue(&left_executor_->GetOutputSchema(), i));
        }
        for (uint32_t i = 0; i < right_executor_->GetOutputSchema().GetColumnCount(); i++) {
          values.emplace_back(right_tuple.GetValue(&right_executor_->GetOutputSchema(), i));
        }
        *tuple = Tuple{values, &GetOutputSchema()};
        has_done_ = true;
        return true;
      }
    }
    left_bool_ = left_executor_->Next(&left_tuple_, &left_rid_);
    right_executor_->Init();
    has_done_ = false;
  }
  return false;
}

}  // namespace bustub