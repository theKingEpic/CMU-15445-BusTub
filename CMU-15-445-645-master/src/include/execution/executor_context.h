//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// executor_context.h
//
// Identification: src/include/execution/executor_context.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <deque>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "catalog/catalog.h"
#include "concurrency/transaction.h"
#include "execution/check_options.h"
#include "execution/executors/abstract_executor.h"
#include "storage/page/tmp_tuple_page.h"

namespace bustub {
class AbstractExecutor;
/**
 * ExecutorContext stores all the context necessary to run an executor.
 */
class ExecutorContext {
 public:
  /**
   * Creates an ExecutorContext for the transaction that is executing the query.
   * @param transaction The transaction executing the query
   * @param catalog The catalog that the executor uses
   * @param bpm The buffer pool manager that the executor uses
   * @param txn_mgr The transaction manager that the executor uses
   * @param lock_mgr The lock manager that the executor uses
   */
  ExecutorContext(Transaction *transaction, Catalog *catalog, BufferPoolManager *bpm, TransactionManager *txn_mgr,
                  LockManager *lock_mgr, bool is_delete)
      : transaction_(transaction),
        catalog_{catalog},
        bpm_{bpm},
        txn_mgr_(txn_mgr),
        lock_mgr_(lock_mgr),
        is_delete_(is_delete) {
    //一个双向队列，用于存储执行器对，可能是用于嵌套循环连接（Nested Loop Join）的检查
    nlj_check_exec_set_ = std::deque<std::pair<AbstractExecutor *, AbstractExecutor *>>(
        std::deque<std::pair<AbstractExecutor *, AbstractExecutor *>>{});
    //一个共享指针，指向CheckOptions类的实例，这个类可能用于配置执行器的某些选项
    check_options_ = std::make_shared<CheckOptions>();
  }
  /*
   * 这里发生了两件事：
  在圆括号内，使用std::deque<std::pair<AbstractExecutor *, AbstractExecutor *>>{}
   创建了一个临时且空的std::deque对象。这个临时对象是一个初始化列表，它用于初始化std::deque容器，使其不包含任何元素。

  然后，这个临时对象被用作构造函数的参数，以初始化nlj_check_exec_set_成员变量。
   这个成员变量也是一个std::deque容器，它将被初始化为与临时对象相同的空状态。

   这种写法实际上是多余的，因为std::deque的默认构造函数就会创建一个空的容器。因此，这段代码可以被简化为：
   nlj_check_exec_set_ = std::deque<std::pair<AbstractExecutor *, AbstractExecutor *>>();
   */
  ~ExecutorContext() = default;

  DISALLOW_COPY_AND_MOVE(ExecutorContext);

  /** @return the running transaction */
  auto GetTransaction() const -> Transaction * { return transaction_; }

  /** @return the catalog */
  auto GetCatalog() -> Catalog * { return catalog_; }

  /** @return the buffer pool manager */
  auto GetBufferPoolManager() -> BufferPoolManager * { return bpm_; }

  /** @return the log manager - don't worry about it for now */
  auto GetLogManager() -> LogManager * { return nullptr; }

  /** @return the lock manager */
  auto GetLockManager() -> LockManager * { return lock_mgr_; }

  /** @return the transaction manager */
  auto GetTransactionManager() -> TransactionManager * { return txn_mgr_; }

  /** @return the set of nlj check executors */
  auto GetNLJCheckExecutorSet() -> std::deque<std::pair<AbstractExecutor *, AbstractExecutor *>> & {
    return nlj_check_exec_set_;
  }

  /** @return the check options */
  auto GetCheckOptions() -> std::shared_ptr<CheckOptions> { return check_options_; }

  void AddCheckExecutor(AbstractExecutor *left_exec, AbstractExecutor *right_exec) {
    nlj_check_exec_set_.emplace_back(left_exec, right_exec);
  }

  void InitCheckOptions(std::shared_ptr<CheckOptions> &&check_options) {
    BUSTUB_ASSERT(check_options, "nullptr");
    check_options_ = std::move(check_options);
  }

  /** As of Fall 2023, this function should not be used. */
  auto IsDelete() const -> bool { return is_delete_; }

 private:
  /** The transaction context associated with this executor context */
  Transaction *transaction_;
  /** The database catalog associated with this executor context */
  Catalog *catalog_;
  /** The buffer pool manager associated with this executor context */
  BufferPoolManager *bpm_;
  /** The transaction manager associated with this executor context */
  TransactionManager *txn_mgr_;
  /** The lock manager associated with this executor context */
  LockManager *lock_mgr_;
  /** The set of NLJ check executors associated with this executor context */
  std::deque<std::pair<AbstractExecutor *, AbstractExecutor *>> nlj_check_exec_set_;
  /** The set of check options associated with this executor context */
  std::shared_ptr<CheckOptions> check_options_;
  bool is_delete_;
};

}  // namespace bustub
