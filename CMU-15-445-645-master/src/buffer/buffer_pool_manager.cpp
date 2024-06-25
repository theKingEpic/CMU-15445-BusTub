//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager.h"

#include "common/exception.h"
#include "common/macros.h"
#include "storage/page/page_guard.h"

namespace bustub {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, size_t replacer_k,
                                     LogManager *log_manager)
    : pool_size_(pool_size), disk_scheduler_(std::make_unique<DiskScheduler>(disk_manager)), log_manager_(log_manager) {
  // we allocate a consecutive memory space for the buffer pool
  pages_ = new Page[pool_size_];
  replacer_ = std::make_unique<LRUKReplacer>(pool_size, replacer_k);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManager::~BufferPoolManager() { delete[] pages_; }

auto BufferPoolManager::NewPage(page_id_t *page_id) -> Page * {  // 给定一个页面，在缓冲池中找到替换frame，替换掉
  Page *page;
  frame_id_t frame_id = -1;
  std::scoped_lock lock(latch_);
  // 如果free_list_里有值
  if (!free_list_.empty()) {
    // 获取free_list_容器的最后一个元素并移除，并让page为新内存地址
    frame_id = free_list_.back();
    free_list_.pop_back();
    page = pages_ + frame_id;
  } else {
    // free_list_里没值，看replacer_里有没有能替换的
    if (!replacer_->Evict(&frame_id)) {
      return nullptr;
    }
    page = pages_ + frame_id;
  }
  // 和flushpage方法一样，如果page地址上原frame里存放的页面是从内存中拿出的，其page_id对应的页就是脏的
  // 如果替换帧有一个脏页面，您应该先将其写回磁盘。您还需要为新页面重置内存和元数据
  if (page->IsDirty()) {
    auto promise = disk_scheduler_->CreatePromise();
    auto future = promise.get_future();
    disk_scheduler_->Schedule({true, page->GetData(), page->GetPageId(), std::move(promise)});
    future
        .get();  //同步等待异步写回操作完成，并确保页面的状态被正确更新。这是确保数据一致性和避免磁盘写入操作的中断的关键步骤
    // ! clean
    page->is_dirty_ = false;
  }
  // 获取一个新的页面ID(注释所要求)
  *page_id = AllocatePage();
  // 把旧的映射删掉，因为需要为新页面重置内存和元数据
  page_table_.erase(page->GetPageId());
  // 建立新的映射
  page_table_.emplace(*page_id, frame_id);
  // 把新page的参数更新下
  page->page_id_ = *page_id;
  page->pin_count_ = 1;
  // ResetMemory方法：将页面中的所有数据清零
  page->ResetMemory();
  // 更新replacer_
  replacer_->RecordAccess(frame_id);
  replacer_->SetEvictable(frame_id, false);
  //记得通过调用replacer. SetEvictable(frame_id,false)
  //来“固定”帧，以防止在缓冲池管理器“取消固定”它之前，替换器驱逐该帧。
  //如果不这么做，可能会造成较大的资源浪费，具体解释如下
  /*
   * 当一个新页面被创建并加入到缓冲池时，通常我们期望该页面能够保持在内存中，至少在它被使用完成之前。
   * 如果该页面在被使用之前被替换出去，就会违反这种期望，因为我们预期新页面应该在内存中可用。
     这种情况被称为“错误地替换出去”，是因为缓冲池的主要目标是提供高效的数据访问，而且新页面是一个需要被频繁访问的页面。
     因此，如果新页面在被使用之前被替换出去，这可能会导致性能下降，因为后续需要再次访问该页面时，就需要重新从磁盘读取，增加了访问延迟。
     因此，将新页面所在的frame_id标记为不可驱逐状态，可以避免这种情况的发生，保证了新页面在被使用之前不会被替换出去，从而维护了系统的性能和一致性。
   */
  return page;
}

auto BufferPoolManager::FetchPage(page_id_t page_id, [[maybe_unused]] AccessType access_type) -> Page * {
  // 从缓冲池中获取请求的页面。如果page_id需要从磁盘获取但所有帧当前都在使用且不可驱逐（换句话说，被固定了），则返回nullptr。
  /* 首先在缓冲池中搜索page_id。如果没有找到，从自由列表或替换器中选择一个替换帧（总是先从自由列表中查找），
   * 通过调度一个读取DiskRequest的disk_scheduler_->Schedule() 从磁盘读取页面，并替换帧中的旧页面。与NewPage() 类似，
   * 如果旧页面是脏的， 您需要将其写回磁盘并更新新页面的元数据。 另外，记得像您在NewPage()
   * 中那样禁用驱逐并记录帧的访问历史。*/
  if (page_id == INVALID_PAGE_ID) {
    return nullptr;
  }
  std::scoped_lock lock(latch_);
  if (page_table_.find(page_id) != page_table_.end()) {
    // 找到page
    auto frame_id = page_table_[page_id];
    auto page = pages_ + frame_id;
    // 更新replacer
    replacer_->RecordAccess(frame_id);
    replacer_->SetEvictable(frame_id, false);
    // 更新固定数
    page->pin_count_ += 1;
    return page;
  }
  // Newpage 方法里的
  Page *page;
  frame_id_t frame_id = -1;
  if (!free_list_.empty()) {
    frame_id = free_list_.back();
    free_list_.pop_back();
    page = pages_ + frame_id;
  } else {
    if (!replacer_->Evict(&frame_id)) {
      return nullptr;
    }
    page = pages_ + frame_id;
  }
  if (page->IsDirty()) {
    auto promise = disk_scheduler_->CreatePromise();
    auto future = promise.get_future();
    disk_scheduler_->Schedule({true, page->GetData(), page->GetPageId(), std::move(promise)});
    future.get();
    page->is_dirty_ = false;
  }
  page_table_.erase(page->GetPageId());
  page_table_.emplace(page_id, frame_id);
  page->page_id_ = page_id;
  page->pin_count_ = 1;
  page->ResetMemory();
  replacer_->RecordAccess(frame_id);
  replacer_->SetEvictable(frame_id, false);
  // 从磁盘中读页，读完后写回（之前的是写完后写回）
  auto promise = disk_scheduler_->CreatePromise();
  auto future = promise.get_future();
  disk_scheduler_->Schedule({false, page->GetData(), page->GetPageId(), std::move(promise)});
  future.get();
  return page;
}

auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty, [[maybe_unused]] AccessType access_type) -> bool {
  if (page_id == INVALID_PAGE_ID) {
    return false;
  }
  std::scoped_lock lock(latch_);
  if (page_table_.find(page_id) == page_table_.end()) {
    return false;
  }
  auto frame_id = page_table_[page_id];
  auto page = pages_ + frame_id;
  // 设置脏位,如果原本是脏的或传进的is_dirty是脏的，最终就是脏的
  page->is_dirty_ = is_dirty || page->is_dirty_;
  // if pin count is 0
  if (page->GetPinCount() == 0) {
    return false;
  }
  // pin要-1
  page->pin_count_ -= 1;
  // 如果-1后为0,调用lru-k中的SetEvictable方法，把帧设为可驱逐的
  if (page->GetPinCount() == 0) {
    replacer_->SetEvictable(frame_id, true);
  }
  return true;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  // 使用 DiskManager::WritePage() 方法将页面刷新到磁盘，无论脏标志位如何。刷新后重置页面的脏标志位。
  // 如果page对象不包含物理页面
  if (page_id == INVALID_PAGE_ID) {
    return false;
  }
  std::scoped_lock lock(latch_);
  // 如果页面不在缓冲池中（页id->缓冲池帧id映射中找不到）
  if (page_table_.find(page_id) == page_table_.end()) {
    return false;
  }
  // 获得page_id在缓冲池中的位置 pages 页面数组，存储缓冲池中的所有页面
  auto page = pages_ + page_table_[page_id];
  // 写回，这里creatpromise方法返回了一个std::promise对象   dis_scheduler_磁盘调度器，用于调度磁盘上的读写操作
  auto promise = disk_scheduler_->CreatePromise();
  auto future = promise.get_future();
  /*
   * struct DiskRequest {
       * 指示请求是写还是读的标志。
        bool is_write_;

         *   指向内存位置的指针，页面所在的位置是:
         *   1. 正在从磁盘读入(在读时)。
         *   2. 正在写入磁盘(在写时)。
        char *data_;

        // 正在从磁盘读取/写入磁盘的页面ID。
        page_id_t page_id_;

        //回调，用于在请求完成时向请求发出信号。
        std::promise<bool> callback_;
    };
   */
  disk_scheduler_->Schedule({true, page->GetData(), page->GetPageId(), std::move(promise)});
  future.get();
  // 脏位恢复
  page->is_dirty_ = false;
  return true;
}

void BufferPoolManager::FlushAllPages() {
  std::scoped_lock lock(latch_);
  for (size_t current_size = 0; current_size < pool_size_; current_size++) {
    // 获得page_id在缓冲池中的位置
    auto page = pages_ + current_size;
    if (page->GetPageId() == INVALID_PAGE_ID) {
      continue;
    }
    // 和flush方法一样
    auto promise = disk_scheduler_->CreatePromise();
    auto future = promise.get_future();
    disk_scheduler_->Schedule({true, page->GetData(), page->GetPageId(), std::move(promise)});
    future.get();
    page->is_dirty_ = false;
  }
}

auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool {
  // 从缓冲池中删除一个页面。如果 page_id 不在缓冲池中，不做任何操作并返回 true。如果页面被固定且无法删除，立即返回
  // false 从页面表中删除页面后，停止在替换器中跟踪该帧，并将帧添加回空闲列表。同时，重置页面的内存和元数据。
  // 最后，应该调用 DeallocatePage() 来模拟在磁盘上释放页面。
  if (page_id == INVALID_PAGE_ID) {
    return true;
  }
  std::scoped_lock lock(latch_);
  // 如果页面存在
  if (page_table_.find(page_id) != page_table_.end()) {
    auto frame_id = page_table_[page_id];
    auto page = pages_ + frame_id;
    // 如果页面被固定
    if (page->GetPinCount() > 0) {
      return false;
    }
    // 删除页面
    page_table_.erase(page_id);
    free_list_.push_back(frame_id);
    replacer_->Remove(frame_id);
    // 清空内存，更新元数据
    page->ResetMemory();
    page->page_id_ = INVALID_PAGE_ID;
    page->is_dirty_ = false;
    page->pin_count_ = 0;
  }
  // 最后，应该调用 DeallocatePage() 来模拟在磁盘上释放页面。
  DeallocatePage(page_id);
  return true;
}

auto BufferPoolManager::AllocatePage() -> page_id_t { return next_page_id_++; }

auto BufferPoolManager::FetchPageBasic(page_id_t page_id) -> BasicPageGuard {
  auto page = FetchPage(page_id);
  return {this, page};
}

auto BufferPoolManager::FetchPageRead(page_id_t page_id) -> ReadPageGuard {
  auto page = FetchPage(page_id);
  if (page != nullptr) {
    page->RLatch();
  }
  return {this, page};
}

auto BufferPoolManager::FetchPageWrite(page_id_t page_id) -> WritePageGuard {
  auto page = FetchPage(page_id);
  if (page != nullptr) {
    page->WLatch();
  }
  return {this, page};
}

auto BufferPoolManager::NewPageGuarded(page_id_t *page_id) -> BasicPageGuard {
  auto page = NewPage(page_id);
  return {this, page};
}

}  // namespace bustub
