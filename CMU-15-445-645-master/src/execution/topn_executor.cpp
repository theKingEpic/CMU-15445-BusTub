#include "execution/executors/topn_executor.h"
#include <iterator>
#include "common/rid.h"
#include "storage/table/tuple.h"

namespace bustub {

TopNExecutor::TopNExecutor(ExecutorContext *exec_ctx, const TopNPlanNode *plan,
                           std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void TopNExecutor::Init() {
  child_executor_->Init();
  tuples_.clear();
  //捕获了外部作用域中 this 指针的 lambda 表达式 比较函数
  auto compare = [this](const Tuple tpl1, const Tuple tpl2) -> bool {
    // 遍历所有的键值对 范围基础的循环 for( : )
    for (auto &[type, expr] : this->plan_->GetOrderBy()) {
      // 计算 tpl1 和 tpl2 中对应排序键的表达式的值
      auto left_value = expr->Evaluate(&tpl1, this->child_executor_->GetOutputSchema());
      auto right_value = expr->Evaluate(&tpl2, this->child_executor_->GetOutputSchema());
      // 比较两个表达式的值
      if (left_value.CompareLessThan(right_value) == CmpBool::CmpTrue) {
        // 如果左边的值小于右边的值，并且排序类型是升序（ASC），则返回 false，意味着左边的元组应该排在右边元组前面
        return type != OrderByType::DESC;
      }
      if (left_value.CompareGreaterThan(right_value) == CmpBool::CmpTrue) {
        // 如果左边的值大于右边的值，并且排序类型是降序（DESC），则返回 true，意味着左边的元组应该排在右边元组前面
        return type == OrderByType::DESC;
      }
    }
    return true;
  };
  // decltype 类型推断关键字，用于推断一个表达式的类型
  std::priority_queue<Tuple, std::vector<Tuple>, decltype(compare)> pq(compare);
  Tuple tuple{};
  RID rid{};
  while (child_executor_->Next(&tuple, &rid)) {
    pq.emplace(tuple);
    if (pq.size() > plan_->GetN()) {
      pq.pop();
    }
  }
  while (!pq.empty()) {
    tuples_.emplace_back(pq.top());
    pq.pop();
  }
  // 因为 tuples_ 中的元组是从最大堆中弹出并插入的，这意味着它们在向量中的顺序是从最大到最小的。
  // 因此，使用逆向迭代器可以按正确的顺序（从最小到最大）遍历元组
  // 向右为正方向的话，这里最小是最左边，最大是最右边
  it_ = tuples_.rbegin();
}

auto TopNExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  while (it_ != tuples_.rend()) {
    *tuple = *it_;
    ++it_;
    return true;
  }
  return false;
}

// std::distance 函数，该函数计算两个迭代器之间的距离
auto TopNExecutor::GetNumInHeap() -> size_t { return std::distance(it_, tuples_.rend()); }

}  // namespace bustub
