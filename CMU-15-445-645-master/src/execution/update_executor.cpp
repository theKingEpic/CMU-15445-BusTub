#include <memory>

#include "execution/executors/update_executor.h"

namespace bustub {

UpdateExecutor::UpdateExecutor(ExecutorContext *exec_ctx, const UpdatePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      table_info_(exec_ctx->GetCatalog()->GetTable(plan_->table_oid_)),
      child_executor_(std::move(child_executor)),
      tbl_index_(exec_ctx->GetCatalog()->GetTableIndexes(table_info_->name_)) {}
void UpdateExecutor::Init() {
  this->child_executor_->Init();
  this->has_inserted_ = false;
}

auto UpdateExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  // 只要执行一次next函數
  if (has_inserted_) {
    return false;
  }
  this->has_inserted_ = true;
  int count = 0;
  // 获取table_info indexes相关基本信息，后面需要使用
  auto table_info = this->exec_ctx_->GetCatalog()->GetTable(this->plan_->GetTableOid());
  auto indexes = this->exec_ctx_->GetCatalog()->GetTableIndexes(table_info->name_);
  Tuple child_tuple{};
  RID child_rid{};
  while (this->child_executor_->Next(&child_tuple, &child_rid)) {
    count++;
    // 更新逻辑不是直接更新相应的值，而是把原来的元组设置为已删除，然后插入新的元组
    // 将每个元组都标记为已删除
    table_info->table_->UpdateTupleMeta(TupleMeta{0, true}, child_rid);
    std::vector<Value> new_values{};
    // 预分配足够的内存空间，以便 new_values 能够在不重新分配内存的情况下存储至少.size() 个 Value 类型的元素
    new_values.reserve(plan_->target_expressions_.size());
    // 获取要插入的新元组
    for (const auto &expr : plan_->target_expressions_) {
      new_values.push_back(expr->Evaluate(&child_tuple, child_executor_->GetOutputSchema()));
    }
    auto update_tuple = Tuple{new_values, &table_info->schema_};
    // 插入新的元組
    std::optional<RID> new_rid_optional = table_info->table_->InsertTuple(TupleMeta{0, false}, update_tuple);
    // 遍历所有索引，为每个索引更新对应的条目
    RID new_rid = new_rid_optional.value();
    // 更新所有相关索引，需要先删除直接的索引，然后插入新的索引信息
    for (auto &index_info : indexes) {
      auto index = index_info->index_.get();
      auto key_attrs = index_info->index_->GetKeyAttrs();
      auto old_key = child_tuple.KeyFromTuple(table_info->schema_, *index->GetKeySchema(), key_attrs);
      auto new_key = update_tuple.KeyFromTuple(table_info->schema_, *index->GetKeySchema(), key_attrs);
      // 从索引中删除旧元组的条目
      index->DeleteEntry(old_key, child_rid, this->exec_ctx_->GetTransaction());
      // 向索引中插入新元组的条目
      index->InsertEntry(new_key, new_rid, this->exec_ctx_->GetTransaction());
    }
  }

  std::vector<Value> result = {{TypeId::INTEGER, count}};
  *tuple = Tuple(result, &GetOutputSchema());

  return true;

  //  /** proj3; 先删后增 */
  //  if (has_inserted_) {
  //    return false;
  //  }
  //  //初始化一个变量update_nums来记录更新的数量。
  //  int32_t update_nums{0};
  //  //创建一个TupleMeta对象meta，用于存储元数据，初始化时指定无效的事务ID和未删除状态
  //  TupleMeta meta{INVALID_TXN_ID, false};
  //  //定义一个变量once_loc用于存储RID（记录ID）
  //  int64_t once_loc;
  //  //定义一个布尔变量once_update，用于标记是否已经更新过
  //  bool once_update{false};
  //  //循环调用child_executor_->Next方法，获取元组和RID，直到没有更多的元组
  //  while (child_executor_->Next(tuple, rid)) {
  //    //如果已经更新过，并且当前的RID等于once_loc，则退出循环
  //    //底层实现不会填补被删除的tuple的空缺，这使得底层顺序查询可能会循环修改新增的update_tuple，为减小开销采用两个标记位的维护
  //    if (once_update && once_loc == rid->Get()) {  // 基于底层新增tuple都是从当前末端顺序自增的观察，否则不可信
  //      break;  // 因为rid->Get()的实现结合了page_id和slot_num，所以不用担心table不止一页导致类似溢出归0后id冲突的问题
  //    }         // 根据proj3中对delete的描述，被删除的空缺会在commit时执行，确实也比较合理。
  //    // delete
  //    meta.is_deleted_ = true;//设置元数据的is_deleted_属性为true，表示当前的元组被删除.
  //    table_info_->table_->UpdateTupleMeta(meta, *rid);//调用UpdateTupleMeta方法更新元数据
  //    //遍历所有索引
  //    for (auto index : tbl_index_) {
  //      auto key = tuple->KeyFromTuple(table_info_->schema_, index->key_schema_, index->index_->GetKeyAttrs());
  //      index->index_->DeleteEntry(key, *rid, exec_ctx_->GetTransaction());
  //    }
  //    // prepare
  //    std::vector<Value> values{};
  //    values.reserve(child_executor_->GetOutputSchema().GetColumnCount());
  //    for (const auto &expr : plan_->target_expressions_) {  // child_node返回的rows需要处理，例如对指定coloum修改值
  //      values.push_back(expr->Evaluate(tuple, child_executor_->GetOutputSchema()));
  //    }
  //    Tuple update_tuple = Tuple(values, &child_executor_->GetOutputSchema());  // 此处应保留和pushback时的schema一致
  //    // insert
  //    meta.is_deleted_ = false;  // tbl_info_->table_->UpdateTupleMeta(meta, *rid); 不需要，InsertTuple实现了；
  //    auto new_tuple_rid = table_info_->table_->InsertTuple(meta, update_tuple);
  //    if (!new_tuple_rid.has_value()) {
  //      continue;
  //    }
  //    for (auto index : tbl_index_) {
  //      auto key = update_tuple.KeyFromTuple(table_info_->schema_, index->key_schema_, index->index_->GetKeyAttrs());
  //      index->index_->InsertEntry(key, new_tuple_rid.value(), exec_ctx_->GetTransaction());
  //    }
  //    // increment
  //    ++update_nums;
  //    if (!once_update) {
  //      once_loc = new_tuple_rid->Get();
  //      once_update = true;
  //    }
  //  }
  //  has_inserted_ = true;
  //  *tuple = Tuple({Value(INTEGER, update_nums)}, &GetOutputSchema());
  //  return true;
}
}  // namespace bustub
