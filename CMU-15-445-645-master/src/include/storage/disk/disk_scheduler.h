//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// disk_scheduler.h
//
// Identification: src/include/storage/disk/disk_scheduler.h
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
/*
 * 这个组件负责在DiskManager上调度读和写操作。你将在src/include/storage/disk/disk_scheduler.h中实现一个名为DiskScheduler的新类，
 * 以及其对应的实现文件在src/storage/disk/disk_scheduler.cpp中。
磁盘调度器可以被其他组件（在这个案例中，是你的Task #3中的BufferPoolManager）用来排队磁盘请求，
 这些请求由DiskRequest结构体（已在src/include/storage/disk/disk_scheduler.h中定义）表示。
 磁盘调度器将维护一个后台工作线程，负责处理已调度的请求。
磁盘调度器将使用一个共享队列来调度和处理DiskRequests。一个线程将请求添加到队列中，磁盘调度器的后台工作线程将处理排队的请求。
 我们提供了一个Channel类在src/include/common/channel.h中以促进线程之间安全地共享数据，但如果你觉得有必要，也可以自由使用自己的实现。
DiskScheduler的构造函数和析构函数已经实现，负责创建和加入后台工作线程。你只需要实现头文件（src/include/storage/disk/disk_scheduler.h）
 和源文件（src/storage/disk/disk_scheduler.cpp）中定义的以下方法：
Schedule(DiskRequest r)
：调度DiskManager执行的请求。DiskRequest结构体指定了请求是读/写，数据应该被写入/从哪里读取，以及操作的页面ID。
 DiskRequest还包括一个std::promise，其值应在请求处理完成后设置为true。
StartWorkerThread() ：后台工作线程的启动方法，处理已调度的请求。工作线程在DiskScheduler构造函数中创建并调用此方法。
 此方法负责获取排队的请求并将它们分派给DiskManager。记得设置DiskRequest的回调值，以向请求发起者信号请求已完成。
 这不应该在DiskScheduler的析构函数被调用之前返回。
最后，DiskRequest的一个字段是std::promise。如果你不熟悉C++的promise和future，可以查看它们的文档。
 在这个项目中，它们基本上提供了一个回调机制，让一个线程知道它们调度的请求何时完成。要查看它们可能如何使用的示例，
 请查看disk_scheduler_test.cpp。
再次说明，实现细节由你决定，但你必须确保你的实现在线程上是安全的。

磁盘管理器
Disk Manager类（src/include/storage/disk/disk_manager.h）从磁盘读取和写入页面数据。
 当你的磁盘调度器处理读或写请求时，它将使用DiskManager::ReadPage()和DiskManager::WritePage()。

 */
#pragma once

#include <future>  // NOLINT
#include <optional>
#include <thread>  // NOLINT

#include "common/channel.h"
#include "storage/disk/disk_manager.h"

namespace bustub {

/**
 * @brief Represents a Write or Read request for the DiskManager to execute.
 * 表示DiskManager要执行的写或读请求。
 */
struct DiskRequest {
  /** Flag indicating whether the request is a write or a read.
   * 指示请求是写还是读的标志。 */
  bool is_write_;

  /**
   *   Pointer to the start of the memory location where a page is either:
   *   1. being read into from disk (on a read).
   *   2. being written out to disk (on a write).
   *   指向内存位置的指针，页面所在的位置是:
   *   1. 正在从磁盘读入(在读时)。
   *   2. 正在写入磁盘(在写时)。
   */
  char *data_;

  /** ID of the page being read from / written to disk.正在从磁盘读取/写入磁盘的页面ID。 */
  page_id_t page_id_;

  /** Callback used to signal to the request issuer when the request has been completed.
   * 回调，用于在请求完成时向请求发出信号。 */
  std::promise<bool> callback_;
};

/**
 * @brief The DiskScheduler schedules disk read and write operations.
 * DiskScheduler对磁盘的读写操作进行调度。
 *
 * A request is scheduled by calling DiskScheduler::Schedule() with an appropriate DiskRequest object. The scheduler
 * maintains a background worker thread that processes the scheduled requests using the disk manager. The background
 * thread is created in the DiskScheduler constructor and joined in its destructor.
 *
 * 通过调用DiskScheduler::Schedule()和一个适当的DiskRequest对象来调度请求。调度器维护一个后台工作线程，该线程使用磁盘管理器处理已调度的请求。
 * 后台线程是在DiskScheduler构造函数中创建的，并加入到它的析构函数中。
 */
class DiskScheduler {
 public:
  explicit DiskScheduler(DiskManager *disk_manager);
  ~DiskScheduler();

  /**
   * TODO(P1): Add implementation
   *
   * @brief Schedules a request for the DiskManager to execute.
   * 调度DiskManager的执行请求
   *
   * @param r The request to be scheduled.
   */
  void Schedule(DiskRequest r);

  /**
   * TODO(P1): Add implementation
   *
   * @brief Background worker thread function that processes scheduled requests.
   *
   * The background thread needs to process requests while the DiskScheduler exists, i.e., this function should not
   * return until ~DiskScheduler() is called. At that point you need to make sure that the function does return.
   * @brief 后台工作线程函数，用于处理已调度的请求。
   *
   * 后台线程需要在DiskScheduler存在期间处理请求，即，这个函数不应该在~DiskScheduler()被调用之前返回。
   * 在那一刻，你需要确保函数确实返回。

   */
  void StartWorkerThread();

  using DiskSchedulerPromise = std::promise<bool>;

  /**
   * @brief Create a Promise object. If you want to implement your own version of promise, you can change this function
   * so that our test cases can use your promise implementation.
   *
   * @return std::promise<bool>
   * @brief 创建一个Promise对象。如果你想实现自己的promise版本，可以更改这个函数
   * 以便我们的测试用例可以使用你的promise实现。
   *
   * @return std::promise<bool>

   */
  auto CreatePromise() -> DiskSchedulerPromise { return {}; };

 private:
  /** Pointer to the disk manager. */
  DiskManager *disk_manager_ __attribute__((__unused__));

  /** A shared queue to concurrently schedule and process requests. When the DiskScheduler's destructor is called,
   * `std::nullopt` is put into the queue to signal to the background thread to stop execution.
   * 一个用于并发调度和处理请求的共享队列。当DiskScheduler的析构函数被调用时，std::nullopt会被放入队列中，
   * 作为信号通知后台线程停止处理请求并退出。*/
  Channel<std::optional<DiskRequest>> request_queue_;

  /** The background thread responsible for issuing scheduled requests to the disk manager.
   * 后台线程负责将已调度的请求发送给磁盘管理器。*/
  std::optional<std::thread> background_thread_;
};
}  // namespace bustub
