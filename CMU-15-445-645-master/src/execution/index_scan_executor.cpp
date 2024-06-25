//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// index_scan_executor.cpp
//
// Identification: src/execution/index_scan_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include "execution/executors/index_scan_executor.h"

namespace bustub {
IndexScanExecutor::IndexScanExecutor(ExecutorContext *exec_ctx, const IndexScanPlanNode *plan)
    : AbstractExecutor(exec_ctx) {
  this->plan_ = plan;
}

void IndexScanExecutor::Init() {
  // 获取与计划节点中指定的表ID对应的表信息
  auto table_info = exec_ctx_->GetCatalog()->GetTable(plan_->table_oid_);
  table_heap_ = table_info->table_.get();
  // 获取与计划节点中指定的索引ID对应的索引信息
  auto index_info = exec_ctx_->GetCatalog()->GetIndex(plan_->index_oid_);
  // 把索引信息转化为哈希索引
  this->htable_ = dynamic_cast<HashTableIndexForTwoIntegerColumn *>(index_info->index_.get());
  // 获取表的schema
  auto table_schema = index_info->key_schema_;
  // 获取键值
  auto key = plan_->pred_key_;
  auto value = key->val_;
  std::vector<Value> values{value};
  // 使用索引的键模式创建键
  Tuple index_key(values, &table_schema);
  // result_rids_清空，后面负责存索引查询到的rid
  result_rids_.clear();
  // 执行索引扫描并填充result_rids_列表
  htable_->ScanKey(index_key, &result_rids_, exec_ctx_->GetTransaction());
  has_inserted_ = false;
}

auto IndexScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if (has_inserted_) {
    return false;
  }
  has_inserted_ = true;
  // 没有找到相关的元组信息
  if (result_rids_.empty()) {
    return false;
  }
  TupleMeta meta{};
  meta = table_heap_->GetTuple(*result_rids_.begin()).first;
  // 确保索引到的元组是没有被删除的
  if (!meta.is_deleted_) {
    *tuple = table_heap_->GetTuple(*result_rids_.begin()).second;
    *rid = *result_rids_.begin();
  }
  return true;
}
}  // namespace bustub