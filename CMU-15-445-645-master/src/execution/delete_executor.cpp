#include <memory>

#include "execution/executors/delete_executor.h"

namespace bustub {

DeleteExecutor::DeleteExecutor(ExecutorContext *exec_ctx, const DeletePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      child_executor_(std::move(child_executor)),
      tbl_info_(exec_ctx->GetCatalog()->GetTable(plan_->table_oid_)),
      tbl_index_(exec_ctx->GetCatalog()->GetTableIndexes(tbl_info_->name_)) {}

void DeleteExecutor::Init() {
  child_executor_->Init();
  done_ = false;
}

auto DeleteExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool {
  // 只要执行一次next函數
  if (done_) {
    return false;
  }
  this->done_ = true;
  int count = 0;
  // 获取table_info indexes相关基本信息，后面需要使用
  auto table_info = this->exec_ctx_->GetCatalog()->GetTable(this->plan_->GetTableOid());
  auto indexes = this->exec_ctx_->GetCatalog()->GetTableIndexes(table_info->name_);
  Tuple child_tuple{};
  RID child_rid{};
  while (this->child_executor_->Next(&child_tuple, &child_rid)) {
    count++;
    // 将每个元组都标记为已删除
    table_info->table_->UpdateTupleMeta(TupleMeta{0, true}, child_rid);
    // 删除直接的索引
    for (auto &index_info : indexes) {
      auto index = index_info->index_.get();
      auto key_attrs = index_info->index_->GetKeyAttrs();
      auto old_key = child_tuple.KeyFromTuple(table_info->schema_, *index->GetKeySchema(), key_attrs);
      // 从索引中删除旧元组的条目
      index->DeleteEntry(old_key, child_rid, this->exec_ctx_->GetTransaction());
    }
  }

  std::vector<Value> result = {{TypeId::INTEGER, count}};
  *tuple = Tuple(result, &GetOutputSchema());

  return true;
}

}  // namespace bustub
