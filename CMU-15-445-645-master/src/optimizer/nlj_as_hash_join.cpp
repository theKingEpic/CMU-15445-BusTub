#include <algorithm>
#include <memory>
#include "execution/expressions/column_value_expression.h"
#include "execution/expressions/comparison_expression.h"
#include "execution/expressions/logic_expression.h"
#include "execution/plans/abstract_plan.h"
#include "execution/plans/hash_join_plan.h"
#include "execution/plans/nested_loop_join_plan.h"
#include "optimizer/optimizer.h"

namespace bustub {
// 解析一个表达式，并提取出左右两侧的关键表达式
void ParseAndExpression(const AbstractExpressionRef &predicate,
                        std::vector<AbstractExpressionRef> *left_key_expressions,
                        std::vector<AbstractExpressionRef> *right_key_expressions) {
  // 尝试将谓词转换为逻辑表达式，与或非  test用不到
  auto *logic_expression_ptr = dynamic_cast<LogicExpression *>(predicate.get());
  // 递归处理逻辑逻辑表达式
  if (logic_expression_ptr != nullptr) {
    // left child
    ParseAndExpression(logic_expression_ptr->GetChildAt(0), left_key_expressions, right_key_expressions);
    // right child
    ParseAndExpression(logic_expression_ptr->GetChildAt(1), left_key_expressions, right_key_expressions);
  }

  // 尝试将谓词转换为比较表达式
  auto *comparison_ptr = dynamic_cast<ComparisonExpression *>(predicate.get());
  // 如果是比较表达式
  if (comparison_ptr != nullptr) {
    auto column_value_1 = dynamic_cast<const ColumnValueExpression &>(*comparison_ptr->GetChildAt(0));
    // auto column_value_2 = dynamic_cast<const ColumnValueExpression &>(*comparison_ptr->GetChildAt(1));
    // 区分每个数据元素是从左侧表还是右侧表提取的，例如 A.id = B.id时，系统需要知道 A.id 和 B.id 分别属于哪个数据源
    /** GetTupleIdx() -> Tuple index 0 = left side of join, tuple index 1 = right side of join */
    if (column_value_1.GetTupleIdx() == 0) {
      left_key_expressions->emplace_back(comparison_ptr->GetChildAt(0));
      right_key_expressions->emplace_back(comparison_ptr->GetChildAt(1));
    } else {
      left_key_expressions->emplace_back(comparison_ptr->GetChildAt(1));
      right_key_expressions->emplace_back(comparison_ptr->GetChildAt(0));
    }
  }
}

auto Optimizer::OptimizeNLJAsHashJoin(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  // TODO(student): implement NestedLoopJoin -> HashJoin optimizer rule
  // Note for 2023 Fall: You should support join keys of any number of conjunction of equity-conditions:
  // E.g. <column expr> = <column expr> AND <column expr> = <column expr> AND ...
  std::vector<AbstractPlanNodeRef> optimized_children;
  for (const auto &child : plan->GetChildren()) {
    // 递归调用,优化子节点
    optimized_children.emplace_back(OptimizeNLJAsHashJoin(child));
  }
  /*通过调用 CloneWithChildren 方法，代码创建了一个新的计划节点，
   * 该节点在结构上与原始节点相同，但是其子节点已经被替换为优化后的版本。这样做的好处是，优化器可以保持原始查询计划的结构，
   * 同时对其子节点应用优化规则，从而生成一个新的、更优的计划。

使用 std::move 是为了高效地转移 optimized_children 向量的所有权。由于 optimized_children 在后续代码中不再使用，
   通过 std::move 可以避免不必要的拷贝，提高效率。

   总的来说，这行代码是实现查询优化器中的一个常见模式，即递归地优化查询计划树中的每个节点，同时保留原始计划的结构。*/
  auto optimized_plan = plan->CloneWithChildren(std::move(optimized_children));

  if (optimized_plan->GetType() == PlanType::NestedLoopJoin) {
    const auto &join_plan = dynamic_cast<const NestedLoopJoinPlanNode &>(*optimized_plan);
    // 获取谓词
    auto predicate = join_plan.Predicate();
    std::vector<AbstractExpressionRef> left_key_expressions;
    std::vector<AbstractExpressionRef> right_key_expressions;
    // 提取左右两侧关键表达式，分别放到left_key_expressions和right_key_expressions里)
    ParseAndExpression(predicate, &left_key_expressions, &right_key_expressions);
    return std::make_shared<HashJoinPlanNode>(join_plan.output_schema_, join_plan.GetLeftPlan(),
                                              join_plan.GetRightPlan(), left_key_expressions, right_key_expressions,
                                              join_plan.GetJoinType());
  }
  return optimized_plan;
}

}  // namespace bustub
