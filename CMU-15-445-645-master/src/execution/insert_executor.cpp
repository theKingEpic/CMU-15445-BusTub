#include <memory>

#include "execution/executors/insert_executor.h"

namespace bustub {
/*
 * InsertExecutor将元组插入到表中，并更新任何受影响的索引。它只有一个子节点，生成要插入到表中的值。
 * 规划器将确保这些值具有与表相同的模式。执行器将生成一个整数类型的元组作为输出，表示有多少行已插入到表中。
 * 如果有与表关联的索引，请记住在插入表时更新索引。

提示:有关系统编目的信息，请参阅下面的系统编目部分。要初始化这个执行器，需要查找要插入的表的信息。

提示:有关更新表索引的详细信息，请参阅下面的索引更新部分。

提示:您需要使用TableHeap类来执行表修改。

提示:您只需要在创建或修改TupleMeta时更改is_delete_字段。对于insertion_txn_和deletion_txn_字段，只需将其设置为INVALID_TXN_ID。
 这些字段打算在未来的学期中使用，我们可能会切换到MVCC存储。*/
InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx) {
  plan_ = plan;
  child_executor_ = std::move(child_executor);
}

void InsertExecutor::Init() {
  this->child_executor_->Init();
  this->has_inserted_ = false;
}

auto InsertExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  //统计插入的元组数
  if (has_inserted_) {
    return false;
  }
  this->has_inserted_ = true;
  // count 表示插入操作期间插入的行数
  int count = 0;
  // 获取待插入的表信息及其索引列表
  auto table_info = this->exec_ctx_->GetCatalog()->GetTable(this->plan_->GetTableOid());
  auto schema = table_info->schema_;
  auto indexes = this->exec_ctx_->GetCatalog()->GetTableIndexes(table_info->name_);
  // 从子执行器 child_executor_ 中逐个获取元组并插入到表中，同时更新所有的索引
  // next函数是虚函数，会自动调用seq_scan中的实现
  while (this->child_executor_->Next(tuple, rid)) {
    count++;
    std::optional<RID> new_rid_optional = table_info->table_->InsertTuple(TupleMeta{0, false}, *tuple);
    // 遍历所有索引，为每个索引更新对应的条目
    RID new_rid = new_rid_optional.value();
    for (auto &index_info : indexes) {
      // 从元组中提取索引键
      auto key = tuple->KeyFromTuple(schema, index_info->key_schema_, index_info->index_->GetKeyAttrs());
      // 向索引中插入键和新元组的RID
      index_info->index_->InsertEntry(key, new_rid, this->exec_ctx_->GetTransaction());
    }
  }
  // 创建了一个 vector对象values，其中包含了一个 Value 对象。这个 Value 对象表示一个整数值，值为 count
  // 这里的 tuple 不再对应实际的数据行，而是用来存储插入操作的影响行数
  std::vector<Value> result = {{TypeId::INTEGER, count}};
  *tuple = Tuple(result, &GetOutputSchema());
  return true;
}

}  // namespace bustub
