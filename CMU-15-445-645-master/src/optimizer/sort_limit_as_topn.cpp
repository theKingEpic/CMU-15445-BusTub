#include "execution/plans/limit_plan.h"
#include "execution/plans/sort_plan.h"
#include "execution/plans/topn_plan.h"
#include "optimizer/optimizer.h"

namespace bustub {

auto Optimizer::OptimizeSortLimitAsTopN(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  // TODO(student): implement sort + limit -> top N optimizer rule
  // 对所有子节点递归应用这一优化
  std::vector<bustub::AbstractPlanNodeRef> optimized_children;
  // 用 CloneWithChildren 方法克隆原始计划，并用优化后的子节点替换原始的子节点
  for (const auto &child : plan->GetChildren()) {
    optimized_children.emplace_back(OptimizeSortLimitAsTopN(child));
  }
  auto optimized_plan = plan->CloneWithChildren(std::move(optimized_children));
  // 看优化为Top-N的条件满不满足，即有没有limit+orderby
  if (optimized_plan->GetType() == PlanType::Limit) {
    const auto &limit_plan = dynamic_cast<const LimitPlanNode &>(*optimized_plan);
    auto child = optimized_plan->children_[0];
    if (child->GetType() == PlanType::Sort) {
      const auto &sort_plan = dynamic_cast<const SortPlanNode &>(*child);
      //满足就换
      return std::make_shared<TopNPlanNode>(optimized_plan->output_schema_, optimized_plan->children_[0],
                                            sort_plan.GetOrderBy(), limit_plan.limit_);
    }
  }
  //不满足输出原计划
  return optimized_plan;
}
}  // namespace bustub
