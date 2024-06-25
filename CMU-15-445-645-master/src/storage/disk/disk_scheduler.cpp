//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// disk_scheduler.cpp
//
// Identification: src/storage/disk/disk_scheduler.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/disk/disk_scheduler.h"
#include "common/exception.h"
#include "storage/disk/disk_manager.h"

namespace bustub {

DiskScheduler::DiskScheduler(DiskManager *disk_manager) : disk_manager_(disk_manager) {
  // Spawn the background thread生成后台线程
  background_thread_.emplace([&] { StartWorkerThread(); });
  /*.
   * emplace是C++11标准引入的一个构造函数，用于在容器中直接构造对象，而不是先构造一个临时对象然后进行拷贝或移动。
   * ([&]：这是一个lambda表达式的开始，&表示捕获外部作用域中的所有变量，通过引用的方式进行捕获。这意味着在lambda内部可以访问并修改这些变量。
   */
}

DiskScheduler::~DiskScheduler() {
  // Put a `std::nullopt` in the queue to signal to exit the loop
  //在队列中放一个' std::nullopt '来表示退出循环
  request_queue_.Put(std::nullopt);
  if (background_thread_.has_value()) {
    background_thread_->join();
  }
  /*
   * 如果background_thread_有值，即存在一个后台线程，这行代码将调用该线程的join()方法。
   * join()方法会阻塞当前线程，直到background_thread_中的线程执行完毕。这样做是为了确保在DiskScheduler对象被销毁之前，
   * 后台线程已经完成了它的任务，并且可以安全地释放资源。
   * 这个析构函数通过向队列中放入一个特殊的标记来通知工作线程退出，然后等待后台线程完成执行，最后才结束。
   * 这样做可以确保在DiskScheduler对象被销毁时，所有的后台工作都已经完成，避免资源泄漏或者其他潜在的问题。
   */
}

void DiskScheduler::Schedule(DiskRequest r) { request_queue_.Put(std::make_optional<DiskRequest>(std::move(r))); }

void DiskScheduler::StartWorkerThread() {
  std::optional<DiskRequest> request;
  // 是否循环看request.has_value()
  while ((request = request_queue_.Get(), request.has_value())) {
    /* 这里使用了逗号运算符来在一个表达式中同时赋值和检查request是否有值。如果request有值，即不是std::nullopt，循环继续；
     * 如果request是std::nullopt，循环结束。*/
    if (request->is_write_) {
      disk_manager_->WritePage(request->page_id_, request->data_);
    } else {
      disk_manager_->ReadPage(request->page_id_, request->data_);
    }
    // 请求已处理完成，值设为true
    request->callback_.set_value(true);
  }
}
}  // namespace bustub
