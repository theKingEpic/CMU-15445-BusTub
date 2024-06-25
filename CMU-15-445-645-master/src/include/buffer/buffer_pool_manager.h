//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager.h
//
// Identification: src/include/buffer/buffer_pool_manager.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
/*
 * 任务 #3 - 缓冲池管理器 接下来，实现缓冲池管理器（BufferPoolManager）。
BufferPoolManager负责使用DiskScheduler从磁盘获取数据库页面并将它们存储在内存中。
当BufferPoolManager被明确指示这样做或者需要驱逐一个页面以为新页面腾出空间时，它还可以调度将脏页面写入磁盘。
为了确保您的实现能够与系统的其他部分正确工作，我们将提供一些已经填写的函数。您也不需要实现实际读写磁盘数据的代码（在我们的实现中称为DiskManager）。
我们将提供该功能。但是，您需要实现DiskScheduler来处理磁盘请求并将它们分派给DiskManager（这是任务 #2）。
系统中的所有内存页面都由Page对象表示。BufferPoolManager不需要理解这些页面的内容。但是，作为系统开发人员，您需要了解Page对象只是缓冲池中内存的容器，
因此并不特定于唯一页面。也就是说，每个Page对象包含一个内存块，DiskManager将使用这个内存块作为从磁盘读取的物理页面内容的复制位置。
BufferPoolManager将重用同一个Page对象来存储来回移动到磁盘的数据。这意味着同一个Page对象在系统的整个生命周期中可能包含不同的物理页面。
Page对象的标识符（page_id）跟踪它包含的是哪个物理页面；如果一个Page对象不包含物理页面，那么它的page_id必须设置为INVALID_PAGE_ID。
每个Page对象还维护一个计数器，用于记录有多少个线程“固定”了该页面。您的BufferPoolManager不允许释放被固定的页面。
每个Page对象还跟踪它是否为脏页面。您的任务是记录一个页面在取消固定之前是否被修改过。您的BufferPoolManager必须在重用该对象之前将脏页面的内容写回磁盘。
您的BufferPoolManager实现将使用您在此作业前几步创建的LRUKReplacer和DiskScheduler类。
 LRUKReplacer将跟踪Page对象的访问情况，以便在必须释放一个帧以为从磁盘复制新的物理页面腾出空间时，它可以决定驱逐哪一个。
 在BufferPoolManager中将page_id映射到frame_id时，再次提醒您STL容器不是线程安全的。DiskScheduler将在DiskManager上调度写入和读取磁盘的操作。

您需要实现头文件（src/include/buffer/buffer_pool_manager.h）和源文件（src/buffer/buffer_pool_manager.cpp）中定义的以下函数：

FetchPage(page_id_t page_id)
 UnpinPage(page_id_t page_id, bool is_dirty)
 FlushPage(page_id_t page_id)
 NewPage(page_id_t* page_id)
 DeletePage(page_id_t page_id)
 FlushAllPages()
 对于FetchPage，如果自由列表中没有可用页面并且所有其他页面当前都被固定，则应返回nullptr。FlushPage应该无论页面的固定状态如何都要刷新页面。

对于UnpinPage，is_dirty参数跟踪一个页面在固定时是否被修改。

AllocatePage私有方法在您想要在NewPage()中创建新页面时为BufferPoolManager提供一个唯一的新页面id。
 另一方面，DeallocatePage()方法是一个模仿在磁盘上释放页面的无操作方法，您应该在您的DeletePage()实现中调用这个方法。

您不需要使您的缓冲池管理器非常高效——在每个面向公共的缓冲池管理器函数中从开始到结束持有缓冲池管理器锁应该就足够了。
 但是，您确实需要确保您的缓冲池管理器具有合理的性能，否则在未来的项目中会出现问题。
 您可以将您的基准测试结果（QPS.1和QPS.2）与其他学生进行比较，看看您的实现是否太慢。

请参考头文件（lru_k_replacer.h, disk_scheduler.h, buffer_pool_manager.h）以获取更详细规格和文档。
 */
#pragma once

#include <list>
#include <memory>
#include <mutex>  // NOLINT
#include <unordered_map>

#include "buffer/lru_k_replacer.h"
#include "common/config.h"
#include "recovery/log_manager.h"
#include "storage/disk/disk_scheduler.h"
#include "storage/page/page.h"
#include "storage/page/page_guard.h"

namespace bustub {

/**
 * BufferPoolManager reads disk pages to and from its internal buffer pool.
 * BufferPoolManager从其内部缓冲池中读取磁盘页面。
 */
class BufferPoolManager {
 public:
  /**
   * @brief Creates a new BufferPoolManager. 创建一个新的BufferPoolManager。
   * @param pool_size the size of the buffer pool 缓冲池的大小
   * @param disk_manager the disk manager 磁盘管理器
   * @param replacer_k the LookBack constant k for the LRU-K replacer LRU-K替换器的LookBack常数k
   * @param log_manager the log manager (for testing only: nullptr = disable logging). Please ignore this for P1.
   * 日志管理器(仅用于测试:nullptr =禁用日志记录)。请忽略P1。
   */
  BufferPoolManager(size_t pool_size, DiskManager *disk_manager, size_t replacer_k = LRUK_REPLACER_K,
                    LogManager *log_manager = nullptr);

  /**
   * @brief Destroy an existing BufferPoolManager.
   */
  ~BufferPoolManager();

  /** @brief Return the size (number of frames) of the buffer pool. */
  auto GetPoolSize() -> size_t { return pool_size_; }

  /** @brief Return the pointer to all the pages in the buffer pool. */
  auto GetPages() -> Page * { return pages_; }

  /**
   * TODO(P1): Add implementation
   *
   * @brief Create a new page in the buffer pool. Set page_id to the new page's id, or nullptr if all frames
   * are currently in use and not evictable (in another word, pinned).
   *
   * You should pick the replacement frame from either the free list or the replacer (always find from the free list
   * first), and then call the AllocatePage() method to get a new page id. If the replacement frame has a dirty page,
   * you should write it back to the disk first. You also need to reset the memory and metadata for the new page.
   *
   * Remember to "Pin" the frame by calling replacer.SetEvictable(frame_id, false)
   * so that the replacer wouldn't evict the frame before the buffer pool manager "Unpin"s it.
   * Also, remember to record the access history of the frame in the replacer for the lru-k algorithm to work.
   *
   * @param[out] page_id id of created page
   * @return nullptr if no new pages could be created, otherwise pointer to new page
   *
   * @brief
   在缓冲池中创建一个新页面。将page_id设置为新页面的id，如果所有帧当前都在使用且不可驱逐（换句话说，被固定了），则设置为nullptr。
   *
   * 您应该从自由列表或替换器（总是先从自由列表中查找）中选择替换帧，然后调用AllocatePage()方法以获取一个新的页面id。
   * 如果替换帧有一个脏页面，您应该先将其写回磁盘。您还需要为新页面重置内存和元数据。
   *
   * 记得通过调用replacer.SetEvictable(frame_id,false)来“固定”帧，以防止在缓冲池管理器“取消固定”它之前，替换器驱逐该帧。
   * 还要记得在替换器中记录帧的访问历史，以便lru-k算法可以工作。
   *
   * @param[out] page_id 创建的页面的id
   * @return 如果无法创建新页面，则返回nullptr，否则返回指向新页面的指针。

   */
  auto NewPage(page_id_t *page_id) -> Page *;

  /**
   * TODO(P2): Add implementation
   *
   * @brief PageGuard wrapper for NewPage
   *
   * Functionality should be the same as NewPage, except that
   * instead of returning a pointer to a page, you return a
   * BasicPageGuard structure.
   *
   * @param[out] page_id, the id of the new page
   * @return BasicPageGuard holding a new page
   *
   * @brief NewPage的PageGuard包装器
    *
    * 功能应与NewPage相同，不同之处在于返回一个指向页面的指针，您返回一个BasicPageGuard结构。
    *
    * @param[out] page_id, 新页面的id
    * @return 包含新页面的BasicPageGuard

   */
  auto NewPageGuarded(page_id_t *page_id) -> BasicPageGuard;

  /**
   * TODO(P1): Add implementation
   *
   * @brief Fetch the requested page from the buffer pool. Return nullptr if page_id needs to be fetched from the disk
   * but all frames are currently in use and not evictable (in another word, pinned).
   *
   * First search for page_id in the buffer pool. If not found, pick a replacement frame from either the free list or
   * the replacer (always find from the free list first), read the page from disk by scheduling a read DiskRequest with
   * disk_scheduler_->Schedule(), and replace the old page in the frame. Similar to NewPage(), if the old page is dirty,
   * you need to write it back to disk and update the metadata of the new page
   *
   * In addition, remember to disable eviction and record the access history of the frame like you did for NewPage().
   *
   * @param page_id id of page to be fetched
   * @param access_type type of access to the page, only needed for leaderboard tests.
   * @return nullptr if page_id cannot be fetched, otherwise pointer to the requested page
   *
   * @brief
   从缓冲池中获取请求的页面。如果page_id需要从磁盘获取但所有帧当前都在使用且不可驱逐（换句话说，被固定了），则返回nullptr。
    *
    * 首先在缓冲池中搜索page_id。如果没有找到，从自由列表或替换器中选择一个替换帧（总是先从自由列表中查找），
    * 通过调度一个读取DiskRequest的disk_scheduler_->Schedule()从磁盘读取页面，并替换帧中的旧页面。与NewPage()类似，如果旧页面是脏的，
    * 您需要将其写回磁盘并更新新页面的元数据。
    *
    * 另外，记得像您在NewPage()中那样禁用驱逐并记录帧的访问历史。
    *
    * @param page_id 要获取的页面的id
    * @param access_type 访问页面的类型，仅在排行榜测试中需要。
    * @return 如果无法获取page_id，则返回nullptr，否则返回指向请求页面的指针。

   */
  auto FetchPage(page_id_t page_id, AccessType access_type = AccessType::Unknown) -> Page *;

  /**
   * TODO(P2): Add implementation
   *
   * @brief PageGuard wrappers for FetchPage
   *
   * Functionality should be the same as FetchPage, except
   * that, depending on the function called, a guard is returned.
   * If FetchPageRead or FetchPageWrite is called, it is expected that
   * the returned page already has a read or write latch held, respectively.
   *
   * @param page_id, the id of the page to fetch
   * @return PageGuard holding the fetched page
   *
   * @brief 用于 FetchPage 的 PageGuard 包装器
   *
   * 功能应与 FetchPage 相同，不同之处在于，根据调用的函数返回一个保护对象（guard）。
   * 如果调用 FetchPageRead 或 FetchPageWrite，预期返回的页面已经分别持有了读或写锁。
   *
   * @param page_id, 要获取的页面的 id
   * @return 持有获取页面的 PageGuard

   */
  auto FetchPageBasic(page_id_t page_id) -> BasicPageGuard;
  auto FetchPageRead(page_id_t page_id) -> ReadPageGuard;
  auto FetchPageWrite(page_id_t page_id) -> WritePageGuard;

  /**
   * TODO(P1): Add implementation
   *
   * @brief Unpin the target page from the buffer pool. If page_id is not in the buffer pool or its pin count is already
   * 0, return false.
   *
   * Decrement the pin count of a page. If the pin count reaches 0, the frame should be evictable by the replacer.
   * Also, set the dirty flag on the page to indicate if the page was modified.
   *
   * @param page_id id of page to be unpinned
   * @param is_dirty true if the page should be marked as dirty, false otherwise
   * @param access_type type of access to the page, only needed for leaderboard tests.
   * @return false if the page is not in the page table or its pin count is <= 0 before this call, true otherwise
   *
   * @brief 从缓冲池中取消固定目标页面。如果 page_id 不在缓冲池中，或者其固定计数已经为 0，返回 false。
   *
   * 减少一个页面的固定计数。如果固定计数达到 0，那么这个帧应该可以被替换器驱逐。
   * 同时，设置页面的脏标志位，以指示页面是否被修改。
   *
   * @param page_id 要取消固定的页面的 id
   * @param is_dirty 如果页面应该被标记为脏，则为 true，否则为 false
   * @param access_type 访问页面的类型，仅用于排行榜测试。
   * @return 如果页面不在页面表，或者在这个调用之前其固定计数 <= 0，返回 false，否则返回 true

   */
  auto UnpinPage(page_id_t page_id, bool is_dirty, AccessType access_type = AccessType::Unknown) -> bool;

  /**
   * TODO(P1): Add implementation
   *
   * @brief Flush the target page to disk.
   *
   * Use the DiskManager::WritePage() method to flush a page to disk, REGARDLESS of the dirty flag.
   * Unset the dirty flag of the page after flushing.
   *
   * @param page_id id of page to be flushed, cannot be INVALID_PAGE_ID
   * @return false if the page could not be found in the page table, true otherwise
   *
   * @brief 将目标页面刷新到磁盘。
   *
   * 使用 DiskManager::WritePage() 方法将页面刷新到磁盘，无论脏标志位如何。刷新后重置页面的脏标志位。
   *
   * @param page_id 要刷新的页面的 id，不能是 INVALID_PAGE_ID
   * @return 如果在页面表中找不到该页面，返回 false，否则返回 true

   */
  auto FlushPage(page_id_t page_id) -> bool;

  /**
   * TODO(P1): Add implementation
   *
   * @brief Flush all the pages in the buffer pool to disk.
   */
  void FlushAllPages();

  /**
   * TODO(P1): Add implementation
   *
   * @brief Delete a page from the buffer pool. If page_id is not in the buffer pool, do nothing and return true. If the
   * page is pinned and cannot be deleted, return false immediately.
   *
   * After deleting the page from the page table, stop tracking the frame in the replacer and add the frame
   * back to the free list. Also, reset the page's memory and metadata. Finally, you should call DeallocatePage() to
   * imitate freeing the page on the disk.
   *
   * @param page_id id of page to be deleted
   * @return false if the page exists but could not be deleted, true if the page didn't exist or deletion succeeded
   *
   * @brief 从缓冲池中删除一个页面。如果 page_id 不在缓冲池中，不做任何操作并返回
   true。如果页面被固定且无法删除，立即返回 false。
   *
   * 从页面表中删除页面后，停止在替换器中跟踪该帧，并将帧添加回空闲列表。同时，重置页面的内存和元数据。
   * 最后，应该调用 DeallocatePage() 来模拟在磁盘上释放页面。
   *
   * @param page_id 要删除的页面的 id
   * @return 如果页面存在但无法删除，返回 false；如果页面不存在或删除成功，返回 true

   */
  auto DeletePage(page_id_t page_id) -> bool;

 private:
  /** Number of pages in the buffer pool. 缓冲池大小，即缓冲池中能够容纳的页面数量*/
  const size_t pool_size_;
  /** The next page id to be allocated 下一个页面ID，用于分配新的页面ID */
  std::atomic<page_id_t> next_page_id_ = 0;

  /** Array of buffer pool pages.页面数组，存储缓冲池中的所有页面 */
  Page *pages_;
  /** Pointer to the disk sheduler. 磁盘调度器，用于调度磁盘上的读写操作*/
  std::unique_ptr<DiskScheduler> disk_scheduler_ __attribute__((__unused__));
  /** Pointer to the log manager. Please ignore this for P1.日志管理器，用于管理日志 */
  LogManager *log_manager_ __attribute__((__unused__));
  /** Page table for keeping track of buffer pool pages. 将页面ID映射到帧ID，用于跟踪缓冲池中的页面位置*/
  std::unordered_map<page_id_t, frame_id_t> page_table_;
  /** Replacer to find unpinned pages for replacement.替换器，用于把pin=0页面驱逐，以释放空间给新的页面 */
  std::unique_ptr<LRUKReplacer> replacer_;
  /** List of free frames that don't have any pages on them. 空闲帧列表，存储当前没有页面驻留的帧ID*/
  std::list<frame_id_t> free_list_;
  /** This latch protects shared data structures. We recommend updating this comment to describe what it protects. 锁*/
  std::mutex latch_;

  /**
   * @brief Allocate a page on disk. Caller should acquire the latch before calling this function.
   * @return the id of the allocated page
   */
  auto AllocatePage() -> page_id_t;

  /**
   * @brief Deallocate a page on disk. Caller should acquire the latch before calling this function.
   * @param page_id id of the page to deallocate
   */
  void DeallocatePage(__attribute__((unused)) page_id_t page_id) {
    // This is a no-nop right now without a more complex data structure to track deallocated pages
  }

  // TODO(student): You may add additional private members and helper functions
};
}  // namespace bustub
