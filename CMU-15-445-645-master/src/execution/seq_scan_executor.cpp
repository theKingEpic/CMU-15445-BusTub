#include "execution/executors/seq_scan_executor.h"
#include "concurrency/transaction_manager.h"
/*在这个项目中，您将向BusTub添加新的操作符执行器和查询优化。BusTub使用迭代器(即Volcano)查询处理模型，
 * 其中每个执行器实现一个Next函数来获取下一个元组结果。当DBMS调用执行器的Next函数时，执行器返回(1)单个元组或(2)没有元组的指示符。
 * 使用这种方法，每个执行器实现一个循环，该循环继续在其子上调用Next以检索元组并逐个处理它们。

在BusTub的迭代器模型实现中，每个执行器的Next函数除了返回一个元组之外还返回一个记录标识符(RID)。记录标识符作为元组的唯一标识符。

执行器是从executor_factory.cpp中的执行计划创建的。
*/
namespace bustub {

SeqScanExecutor::SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan)
    : AbstractExecutor(exec_ctx), plan_(plan), iter_(nullptr) {}

void SeqScanExecutor::Init() {
  table_heap_ = GetExecutorContext()->GetCatalog()->GetTable(plan_->GetTableOid())->table_.get();
  auto iter = table_heap_->MakeIterator();
  rids_.clear();
  while (!iter.IsEnd()) {
    rids_.push_back(iter.GetRID());
    ++iter;
  }
  rid_iter_ = rids_.begin();
}
/*SeqScanExecutor迭代表并每次返回一个元组。

提示:在使用TableIterator对象时，确保您理解了自增前操作符和自增后操作符之间的区别。
 如果将++iter与iter++混淆，可能会得到奇怪的输出。(点击这里快速复习一下。)

提示:不要产生在TableHeap中被删除的元组。检查每个元组对应的TupleMeta的is_deleted_字段。

提示:顺序扫描的输出是每个匹配元组及其原始记录标识符(RID)的副本。

注意:BusTub不支持DROP TABLE和DROP INDEX。您可以通过重新启动shell来重置数据库。*/
auto SeqScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  TupleMeta meta{};
  do {
    if (rid_iter_ == rids_.end()) {
      return false;
    }
    meta = table_heap_->GetTuple(*rid_iter_).first;
    if (!meta.is_deleted_) {
      *tuple = table_heap_->GetTuple(*rid_iter_).second;
      *rid = *rid_iter_;
    }
    ++rid_iter_;
  } while (meta.is_deleted_ ||
           (plan_->filter_predicate_ != nullptr &&
            !plan_->filter_predicate_
                 ->Evaluate(tuple, GetExecutorContext()->GetCatalog()->GetTable(plan_->GetTableOid())->schema_)
                 .GetAs<bool>()));
  return true;
}
}  // namespace bustub
