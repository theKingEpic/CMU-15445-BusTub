#include "execution/executors/aggregation_executor.h"
#include <memory>
#include <vector>
/*我们以分组聚合查询语句 select count(name) from table group by camp; 为例简要说明一下聚合执行器的执行流程：
Init() 函数首先从子执行器中逐行获取数据，并根据每行数据构建聚合键和聚合值。其中聚合键用于标识该行数据属于哪一个聚合组，
这里是按照阵营 camp 分组，因此聚合键会有 Piltover、Ionia 和 Shadow Isles 三种取值，这样所有数据被分成三个聚合组。
而聚合值就是待聚合的列的值，这里的聚合列是 name，因此这五个 Tuple 中生成的聚合值即为对应的 name 属性的值。
对于每个提取的数据行，Init() 函数还会通过 InsertCombine()， 将相应的聚合值聚合到到相应的聚合组中。
在InsertCombine()中调用CombineAggregateValues() 函数来实现具体的聚合规则。
经过 Init() 函数的处理，以上六条数据会被整理为 [{“Piltover”: 3}, {“Ionia”: 2}, {“Shadow Isles”: 1}]
三个聚合组（对应于聚合哈希表中的三个键值对）。 其中groupby的值分别为Piltover、Ionia、Shadow
Isles；aggregate的值分别为3、2、1。 最后，Next() 函数会通过哈希迭代器依次获取每个聚合组的键与值，返回给父执行器。
 如果没group by 子句，那么所有数据都会被分到同一个聚合组中并返回。*/
namespace bustub {

AggregationExecutor::AggregationExecutor(ExecutorContext *exec_ctx, const AggregationPlanNode *plan,
                                         std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      child_executor_(std::move(child_executor)),
      aht_(std::make_unique<SimpleAggregationHashTable>(plan_->GetAggregates(), plan_->GetAggregateTypes())),
      aht_iterator_(std::make_unique<SimpleAggregationHashTable::Iterator>(aht_->Begin())) {}

void AggregationExecutor::Init() {
  child_executor_->Init();
  // 根据聚合表达式以及聚合类型创建哈希表
  // SimpleAggregationHashTable 是一个在聚合查询中专为计算聚合设计的散列表（哈希表）
  // 它用于快速地分组数据，并对每个分组应用聚合函数。

  // NEXT方法的輸出參數，用于存储查询结果
  Tuple child_tuple{};
  RID rid{};
  // 遍历子执行器，将子执行器中的获取的数据插入到聚合哈希表中
  // 不能在聚合执行器中完成，因为聚合执行器需要先从子执行器中获取所有数据，然后对这些数据进行分组和聚合操作，最后才能产生输出结果
  while (child_executor_->Next(&child_tuple, &rid)) {
    // 通过tuple获取聚合键和聚合值
    // 聚合键在聚合操作中用来区分不同的分组
    auto agg_key = MakeAggregateKey(&child_tuple);
    auto agg_val = MakeAggregateValue(&child_tuple);
    // 将聚合键和聚合值插入到聚合哈希表中
    aht_->InsertCombine(agg_key, agg_val);
  }
  // 一个指向哈希表开始的迭代器，后面用于遍历哈希表并生成聚合查询的结果。
  aht_iterator_ = std::make_unique<SimpleAggregationHashTable::Iterator>(aht_->Begin());
  copy_with_empty_ = false;
}

auto AggregationExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if (aht_->Begin() != aht_->End()) {
    if (*aht_iterator_ == aht_->End()) {
      return false;
    }
    // 获取聚合键和聚合值
    auto agg_key = aht_iterator_->Key();
    auto agg_val = aht_iterator_->Val();
    //根据聚合键和聚合值生成查询结果元组
    std::vector<Value> values{};
    // 遍历聚合键和聚合值，生成查询结果元组
    // 根据文件要求，有groupby和aggregate两个部分的情况下，groupby也要算上，都添加到value中
    values.reserve(agg_key.group_bys_.size() + agg_val.aggregates_.size());
    for (auto &group_values : agg_key.group_bys_) {
      values.emplace_back(group_values);
    }
    for (auto &agg_value : agg_val.aggregates_) {
      values.emplace_back(agg_value);
    }
    *tuple = {values, &GetOutputSchema()};
    //迭代到下一个聚合键和聚合值
    ++*aht_iterator_;
    // 表示成功返回了一个聚合结果
    return true;
  }
  if (copy_with_empty_) {
    return false;
  }
  copy_with_empty_ = true;
  // 没有groupby语句则生成一个初始的聚合值元组并返回
  if (plan_->GetGroupBys().empty()) {
    std::vector<Value> values{};
    Tuple tuple_buffer{};
    // 检查当前表是否为空，如果为空生成默认的聚合值，对于 count(*) 来说是 0，对于其他聚合函数来说是 integer_null(
    // 默认聚合值要求由GenerateInitialAggregateValue实现
    for (auto &agg_value : aht_->GenerateInitialAggregateValue().aggregates_) {
      values.emplace_back(agg_value);
    }
    *tuple = {values, &GetOutputSchema()};
    return true;
  }
  return false;
}

auto AggregationExecutor::GetChildExecutor() const -> const AbstractExecutor * { return child_executor_.get(); }

}  // namespace bustub

// create table t1(v1 int);select count(*) from t1;
/* 如果在Init中写下面的代码，会出bug:
 * 地点在execution_engine.h的Execute函数中
  executor->Init();
  PollExecutor(executor.get(), plan, result_set);
  会导致aht_对象的agg_exprs 和agg_types发生变化，原因是：
    const std::vector<AbstractExpressionRef> &agg_exprs_;
    const std::vector<AggregationType> &agg_types_;
    这俩是引用常量，


//  auto agg_exprs = std::make_shared<std::vector<AbstractExpressionRef>>(plan_->GetAggregates());
//  auto agg_types = std::make_shared<std::vector<AggregationType>>(plan_->GetAggregateTypes());
//  //aht_ = std::make_unique<SimpleAggregationHashTable>(agg_exprs, agg_types);
//  aht_ = std::make_unique<SimpleAggregationHashTable>(*agg_exprs, *agg_types);
 */