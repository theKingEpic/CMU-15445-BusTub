#include "execution/executors/projection_executor.h"
#include "storage/table/tuple.h"

namespace bustub {
/*
 * ColumnValueExpression:直接将子执行器的一列放置到输出中。语法#0.0表示第一个子节点中的第一列。您将在连接计划中看到类似#0.0 = #1.0的内容。
ConstantExpression:表示一个常量值(例如1)。
算术表达式:表示算术计算的树。例如，1 +
2将由一个带有两个ConstantExpression(1和2)作为子表达式的ArithmeticExpression来表示。*/
ProjectionExecutor::ProjectionExecutor(ExecutorContext *exec_ctx, const ProjectionPlanNode *plan,
                                       std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void ProjectionExecutor::Init() {
  // Initialize the child executor
  child_executor_->Init();
}

auto ProjectionExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  Tuple child_tuple{};

  // Get the next tuple
  const auto status = child_executor_->Next(&child_tuple, rid);

  if (!status) {
    return false;
  }

  // Compute expressions
  std::vector<Value> values{};
  values.reserve(GetOutputSchema().GetColumnCount());
  for (const auto &expr : plan_->GetExpressions()) {
    values.push_back(expr->Evaluate(&child_tuple, child_executor_->GetOutputSchema()));
  }

  *tuple = Tuple{values, &GetOutputSchema()};

  return true;
}
}  // namespace bustub
