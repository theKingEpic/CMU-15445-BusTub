#include "execution/expressions/column_value_expression.h"
#include "execution/expressions/comparison_expression.h"
#include "execution/expressions/logic_expression.h"
#include "execution/plans/index_scan_plan.h"
#include "execution/plans/seq_scan_plan.h"
#include "optimizer/optimizer.h"
#include "storage/index/generic_key.h"
/*
 *
 * 这段代码是 `bustub`
数据库系统中的优化器的一部分，具体是优化器中的一个规则，用于将顺序扫描（SeqScan）转换为索引扫描（IndexScan）。
 * 这个规则的目的是当顺序扫描计划带有谓词时，如果该谓词可以使用索引来更高效地执行，那么就将顺序扫描转换为索引扫描。
以下是代码的详细分析：
1. `OptimizeSeqScanAsIndexScan` 方法：这个方法是优化器的一个成员方法，它接受一个计划节点 `plan`
作为参数，并返回一个优化后的计划节点。
2. 优化器的第一步是对所有子节点递归应用这一优化规则。这是因为优化器是以计划树的形式工作的，需要对整个树进行遍历和优化。
3. `optimized_plan` 是通过调用 `plan->CloneWithChildren`
方法创建的，这个方法会创建一个与原始计划相同类型的新计划，但是使用优化后的子节点。
4. 接下来，代码检查 `optimized_plan` 是否为顺序扫描计划。如果是，那么尝试将其转换为索引扫描。
5. 从顺序扫描计划中获取谓词 `predicate`。如果谓词为空，那么顺序扫描是合适的，不需要转换。
6.
如果谓词不为空，那么检查表是否有索引。如果没有索引，或者谓词是一个逻辑表达式（包含多个条件），那么顺序扫描是必要的，不需要转换。
7. 如果谓词是一个比较表达式，并且是比较类型为“等于”（`ComparisonType::Equal`），那么这个谓词可能适用于索引扫描。
8. 代码尝试获取比较表达式的左子节点，这应该是一个列值表达式（`ColumnValueExpression`），表示正在比较的列。
9. 然后，代码检查是否存在与谓词中列对应的索引。如果存在，那么获取索引的 OID 和列的名称。
10. 最后，创建一个新的索引扫描计划节点 `IndexScanPlanNode`，使用顺序扫描计划的输出模式、表 OID、索引
OID、谓词和谓词的常量值键。
11. 如果谓词不能使用索引扫描，那么返回原始的顺序扫描计划。
总的来说，这段代码实现了将顺序扫描转换为索引扫描的优化规则。它检查了计划节点是否为顺序扫描，谓词是否为单一的比较表达式，并且比较的列是否有对应的索引。
 如果条件满足，它就创建一个新的索引扫描计划来替代顺序扫描计划。这样做可以提高查询的执行效率，因为索引扫描通常比顺序扫描更快。
*/
namespace bustub {

auto Optimizer::OptimizeSeqScanAsIndexScan(const bustub::AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  // TODO(student): implement seq scan with predicate -> index scan optimizer rule
  // The Filter Predicate Pushdown has been enabled for you in optimizer.cpp when forcing starter rule
  // 当强制启动规则时，在optimizer.cpp中已经为您启用了过滤器谓词下推
  // 对所有子节点递归应用这一优化
  std::vector<bustub::AbstractPlanNodeRef> optimized_children;
  for (const auto &child : plan->GetChildren()) {
    optimized_children.emplace_back(OptimizeSeqScanAsIndexScan(child));
  }
  // 递归转化其所有的子节点
  auto optimized_plan = plan->CloneWithChildren(std::move(optimized_children));
  // 如果plan计划为顺序扫描，则转化为索引扫描
  if (optimized_plan->GetType() == PlanType::SeqScan) {
    const auto &seq_plan = dynamic_cast<const bustub::SeqScanPlanNode &>(*optimized_plan);
    // 获取计划的谓词（where语句之后的内容）
    auto predicate = seq_plan.filter_predicate_;
    // 如果谓词为空仍然执行顺序扫描
    if (predicate != nullptr) {
      auto table_name = seq_plan.table_name_;
      // 获取表的索引，看该表是否支持索引扫描
      auto table_idx = catalog_.GetTableIndexes(table_name);
      // 将predicate转化为LogicExpression，查看是否为逻辑谓词
      auto logic_expr = std::dynamic_pointer_cast<LogicExpression>(predicate);
      /*
       * 在数据库查询优化中，多个条件通常指的是查询谓词中包含多个独立的条件表达式，这些条件表达式之间通过逻辑运算符（如
`AND` 或
`OR`）连接。为了判断一个谓词是否包含多个条件，我们可以检查谓词是否可以被转换为一个逻辑表达式（`LogicExpression`），因为逻辑表达式通常用于表示多个条件的组合。
在代码中，`predicate` 是一个 `AbstractExpression` 类型的指针，它可能是 `ComparisonExpression` 或者
`LogicExpression`。`LogicExpression` 是一个抽象基类，它包含逻辑运算符，如 `AND` 或 `OR`，并且可以有多个子表达式。
通过使用 `std::dynamic_pointer_cast`，代码尝试将 `predicate` 转换为 `LogicExpression` 类型的智能指针。如果转换成功，说明
`predicate` 是一个逻辑表达式，它包含多个条件。如果转换失败，说明 `predicate`
不是逻辑表达式，或者它可能是一个比较表达式，其中包含单个条件。 以下是如何判断 `predicate` 是否包含多个条件的步骤：
1. 尝试将 `predicate` 转换为 `LogicExpression` 类型的智能指针。
2. 如果转换成功，检查逻辑表达式的子表达式数量。如果子表达式数量大于 1，那么 `predicate` 包含多个条件。
3. 如果转换失败，检查 `predicate` 是否为 `ComparisonExpression`。如果是，它可能包含单个条件。
4. 如果 `predicate` 不是逻辑表达式或比较表达式，那么它可能是一个常量表达式或其他类型的表达式，它不包含多个条件。
在代码中，如果 `logic_expr` 为 `nullptr`，则表示 `predicate` 不是逻辑表达式，或者它可能是一个比较表达式。如果
`logic_expr` 不为 `nullptr`，那么 `predicate` 包含多个条件。在这种情况下，逻辑表达式 `logic_expr` 可以通过其
`GetChildren` 方法获取子表达式，然后通过检查子表达式的数量来确定是否包含多个条件。
*/
      // 沒有索引或者有多个谓词条件,返回顺序扫描
      if (!table_idx.empty() && !logic_expr) {
        auto equal_expr = std::dynamic_pointer_cast<ComparisonExpression>(predicate);
        // 需要判断是否为条件谓词
        if (equal_expr) {
          auto com_type = equal_expr->comp_type_;
          // 只能是等值判断才能转化为索引扫描
          if (com_type == ComparisonType::Equal) {
            // 获取表的id
            auto table_oid = seq_plan.table_oid_;
            // 返回索引扫描节点
            auto column_expr = dynamic_cast<const ColumnValueExpression &>(*equal_expr->GetChildAt(0));
            // 根据谓词的列，获取表的索引信息
            auto column_index = column_expr.GetColIdx();
            auto col_name = this->catalog_.GetTable(table_oid)->schema_.GetColumn(column_index).GetName();
            // 如果存在相关索引，获取表索引info
            for (auto *index : table_idx) {
              const auto &columns = index->index_->GetKeyAttrs();
              std::vector<uint32_t> column_ids;
              column_ids.push_back(column_index);
              if (columns == column_ids) {
                // 获取pred-key
                auto pred_key = std::dynamic_pointer_cast<ConstantValueExpression>(equal_expr->GetChildAt(1));
                // 从智能指针中获取裸指针
                ConstantValueExpression *raw_pred_key = pred_key ? pred_key.get() : nullptr;
                return std::make_shared<IndexScanPlanNode>(seq_plan.output_schema_, table_oid, index->index_oid_,
                                                           predicate, raw_pred_key);
              }
            }
          }
        }
      }
    }
  }
  return optimized_plan;
}

}  // namespace bustub
