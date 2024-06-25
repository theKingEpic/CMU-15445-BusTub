//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// index_scan_plan.h
//
// Identification: src/include/execution/plans/index_scan_plan.h
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
/*
 * 这段代码定义了一个名为 `IndexScanPlanNode` 的类，该类位于 `bustub` 命名空间中。
 * `IndexScanPlanNode` 是一个执行计划节点，用于标识应该使用可选谓词进行扫描的表。
`IndexScanPlanNode` 类继承自
`AbstractPlanNode`，并添加了一些特定的属性和方法，用于处理索引扫描的操作。以下是对该类的详细分析：
1. 构造函数 `IndexScanPlanNode` 接受以下参数：
   - `output`：该扫描计划节点的输出格式。
   - `table_oid`：要扫描的表的标识符。
   - `index_oid`：要扫描的索引的标识符。
   - `filter_predicate`：推送到索引扫描的谓词（可选）。
   - `pred_key`：用于点查找的键（可选）。
   构造函数将这些参数存储在成员变量中，并调用基类 `AbstractPlanNode` 的构造函数。
2. `GetType` 方法：重写基类的方法，返回 `PlanType::IndexScan`，表示这是一个索引扫描计划节点。
3. `GetIndexOid` 方法：返回索引扫描节点中使用的索引的标识符。
4. `BUSTUB_PLAN_NODE_CLONE_WITH_CHILDREN` 宏：这是一个克隆宏，用于实现计划节点的克隆功能，包括其子节点。
5. 成员变量：
   - `table_oid_`：索引创建的表的对象标识符。
   - `index_oid_`：应该扫描的索引的对象标识符。
   - `filter_predicate_`：索引扫描中用于过滤的谓词。
   - `pred_key_`：用于查找的常量值键。
6. `PlanNodeToString`
方法：重写基类的方法，返回一个格式化的字符串，用于调试或输出计划节点的信息。如果存在过滤谓词，则将其包含在字符串中。
总的来说，`IndexScanPlanNode` 类定义了一个用于索引扫描的执行计划节点，它包含了扫描的表和索引信息，
 以及可选的过滤谓词和点查找键。这个计划节点可以用于优化器生成的查询执行计划中，以便在执行查询时进行索引扫描操作。
*/
#pragma once

#include <string>
#include <utility>

#include "catalog/catalog.h"
#include "concurrency/transaction.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/expressions/constant_value_expression.h"
#include "execution/plans/abstract_plan.h"

namespace bustub {
/**
 * IndexScanPlanNode identifies a table that should be scanned with an optional predicate.
 */
class IndexScanPlanNode : public AbstractPlanNode {
 public:
  /**
   * Creates a new index scan plan node with filter predicate.
   * @param output The output format of this scan plan node
   * @param table_oid The identifier of table to be scanned
   * @param filter_predicate The predicate pushed down to index scan.
   * @param pred_key The key for point lookup
   */
  IndexScanPlanNode(SchemaRef output, table_oid_t table_oid, index_oid_t index_oid,
                    AbstractExpressionRef filter_predicate = nullptr, ConstantValueExpression *pred_key = nullptr)
      : AbstractPlanNode(std::move(output), {}),
        table_oid_(table_oid),
        index_oid_(index_oid),
        filter_predicate_(std::move(filter_predicate)),
        pred_key_(pred_key) {}

  auto GetType() const -> PlanType override { return PlanType::IndexScan; }

  /** @return the identifier of the table that should be scanned */
  auto GetIndexOid() const -> index_oid_t { return index_oid_; }

  BUSTUB_PLAN_NODE_CLONE_WITH_CHILDREN(IndexScanPlanNode);

  /** The table which the index is created on. */
  table_oid_t table_oid_;

  /** The index whose tuples should be scanned. */
  index_oid_t index_oid_;

  /** The predicate to filter in index scan.
   * For Fall 2023, after you implemented seqscan to indexscan optimizer rule,
   * we can use this predicate to do index point lookup
   */
  AbstractExpressionRef filter_predicate_;

  /**
   * The constant value key to lookup.
   * For example when dealing "WHERE v = 1" we could store the constant value 1 here
   */
  const ConstantValueExpression *pred_key_;

  // Add anything you want here for index lookup

 protected:
  auto PlanNodeToString() const -> std::string override {
    if (filter_predicate_) {
      return fmt::format("IndexScan {{ index_oid={}, filter={} }}", index_oid_, filter_predicate_);
    }
    return fmt::format("IndexScan {{ index_oid={} }}", index_oid_);
  }
};

}  // namespace bustub
