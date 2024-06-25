//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.h
//
// Identification: src/include/buffer/lru_k_replacer.h
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

/*
 * 任务 #1 - LRU-K 替换策略
这个组件负责跟踪缓冲池中的页面使用情况。你将在 src/include/buffer/lru_k_replacer.h 中实现一个名为 LRUKReplacer 的新类，
并在 src/buffer/lru_k_replacer.cpp 中实现相应的实现文件。请注意，LRUKReplacer 是一个独立的类，与任何其他 Replacer 类
都没有关系。你只需要实现 LRU-K 替换策略。即使有对应的文件，你也不需要实现 LRU 或时钟替换策略。
LRU-K
算法淘汰的是在替换器中所有帧中具有最大向后k距离的帧。向后k距离是通过当前时间戳和第k次先前访问的时间戳之间的差异来计算的。
如果一个帧的历史访问次数少于k次，其向后k距离被赋予 +inf。当多个帧具有 +inf
向后k距离时，替换器将淘汰具有最早整体时间戳的帧 （即，其最不近期的记录访问是所有帧中最不近期的访问）。

LRUKReplacer 的最大大小与缓冲池的大小相同，因为它包含了缓冲池管理器中所有帧的占位符。然而，在任何给定的时刻，
 并不是替换器中的所有帧都被认为是可淘汰的。LRUKReplacer 的大小由可淘汰帧的数量表示。LRUKReplacer 初始化时没有任何帧。
 只有在帧被标记为可淘汰时，替换器的大小才会增加。

你需要实现本课程讨论的 LRU-K 策略。你需要在头文件（src/include/buffer/lru_k_replacer.h）和源文件
 （src/buffer/lru_k_replacer.cpp）中实现以下方法：
Evict(frame_id_t* frame_id) ：淘汰与替换器跟踪的所有其他可淘汰帧相比具有最大向后k距离的帧。
 将帧id存储在输出参数中并返回 True。如果没有可淘汰的帧则返回 False。

RecordAccess(frame_id_t frame_id) ：记录给定的帧id在当前时间戳被访问。
 这个方法应该在 BufferPoolManager 中固定页面后被调用。

Remove(frame_id_t frame_id) ：清除与一个帧相关的所有访问历史。
 这个方法只应该在 BufferPoolManager 中删除页面时被调用。

SetEvictable(frame_id_t frame_id, bool set_evictable) ：这个方法控制一个帧是否可被淘汰，
 同时也控制 LRUKReplacer 的大小。当你实现 BufferPoolManager 时，你将知道何时调用这个函数。
 具体来说，当一个页面的固定计数达到0时，其对应的帧被标记为可淘汰，替换器的大小增加。

Size() ：这个方法返回当前 LRUKReplacer 中可淘汰帧的数量。
实现的细节由你决定。你可以使用内置的 STL 容器。你可以假设不会耗尽内存，但必须确保你的实现在线程上是安全的。

 */
#pragma once

#include <algorithm>
#include <limits>
#include <list>
#include <mutex>  // NOLINT
#include <unordered_map>
#include <vector>

#include "common/config.h"
#include "common/macros.h"

namespace bustub {

enum class AccessType { Unknown = 0, Lookup, Scan, Index };
// lookup 查找

class LRUKNode {
 private:
  /** History of last seen K timestamps of this page. Least recent timestamp stored in front.
   * 该页最后出现的K个时间戳的历史记录。最不近期的时间戳存储在前面。*/

  // Remove maybe_unused if you start using them. Feel free to change the member variables as you want.
  //如果您开始使用maybe_unused，请删除它们。您可以随意更改成员变量。

  [[maybe_unused]] std::list<size_t> history_;
  [[maybe_unused]] size_t k_;
  [[maybe_unused]] frame_id_t fid_;
  [[maybe_unused]] bool is_evictable_{false};  //记录一个页面是否可以被驱逐
};

/**
 * LRUKReplacer implements the LRU-k replacement policy.
 *
 * The LRU-k algorithm evicts a frame whose backward k-distance is maximum of all frames.
 * Backward k-distance is computed as the difference in time between current timestamp and the timestamp of kth previous
 access.
 *
 * A frame with less than k historical references is given +inf as its backward k-distance.
 * When multiple frames have +inf backward k-distance,classical LRU algorithm is used to choose victim.
    LRUKReplacer实现LRU-k替换策略。

    LRU-k算法将所有帧中向后k距离最大的帧驱逐出去。
    向后k-距离计算为当前时间戳与第k次访问时间戳之间的时间差。

    对于少于k个历史参考的帧，给出其向后k距离+inf。
    当多帧具有+inf - backward k-distance时，采用经典LRU算法选择受害者。
 */
class LRUKReplacer {
 public:
  /**
   *
   * TODO(P1): Add implementation
   *
   * @brief a new LRUKReplacer. 初始化，定义越界范围和k值
   * @param num_frames LRUReplacer需要存储的最大帧数
   */
  explicit LRUKReplacer(size_t num_frames, size_t k);

  DISALLOW_COPY_AND_MOVE(LRUKReplacer);

  /**
   * TODO(P1): Add implementation
   *
   * @brief Destroys the LRUReplacer.
   */
  ~LRUKReplacer() = default;

  /**
   * TODO(P1): Add implementation
   *
   * @brief Find the frame with largest backward k-distance and evict that frame.
   * Only frames that are marked as 'evictable' are candidates for eviction.
   *
   * A frame with less than k historical references is given +inf as its backward k-distance.
   * If multiple frames have inf backward k-distance, then evict frame with earliest timestamp based on LRU.
   *
   * Successful eviction of a frame should decrement the size of replacer and remove the frame's access history.
   *
   * @param[out] frame_id id of frame that is evicted.
   * @return true if a frame is evicted successfully, false if no frames can be evicted.
   * @brief 查找具有最大向后k距离的帧并进行淘汰。
       只有被标记为'可淘汰'的帧才有可能被淘汰。

       如果一个帧的历史引用次数少于k次，那么它的向后k距离被设定为无穷大（+inf）。
       如果有多个帧具有无穷大的向后k距离，则根据LRU（最近最少使用）算法，淘汰时间戳最早的帧。

       成功淘汰一个帧应减少替换器（replacer）的大小，并移除该帧的访问历史。

       @param[out] frame_id 被淘汰帧的id。
       @return 如果成功淘汰一个帧则返回true，如果没有帧可以被淘汰则返回false。
   */
  auto Evict(frame_id_t *frame_id) -> bool;

  /**
   * TODO(P1): Add implementation
   *
   * @brief Record the event that the given frame id is accessed at current timestamp.
   * Create a new entry for access history if frame id has not been seen before.
   *
   * If frame id is invalid (ie. larger than replacer_size_), throw an exception. You can
   * also use BUSTUB_ASSERT to abort the process if frame id is invalid.
   *
   * @param frame_id id of frame that received a new access.
   * @param access_type type of access that was received. This parameter is only needed for
   * leaderboard tests.
   * @brief 记录给定帧id在当前时间戳被访问的事件。
       如果之前没有见过该帧id，则为访问历史创建一个新的条目。

       如果帧id无效（即大于replacer_size_），抛出一个异常。如果帧id无效，也可以使用BUSTUB_ASSERT来终止进程。

    @param frame_id 接收到新访问的帧的id。
    @param access_type 接收到的访问类型。这个参数仅用于排行榜测试。
   */
  void RecordAccess(frame_id_t frame_id, AccessType access_type = AccessType::Unknown);

  /**
   * TODO(P1): Add implementation
   *
   * @brief Toggle whether a frame is evictable or non-evictable. This function also
   * controls replacer's size. Note that size is equal to number of evictable entries.
   *
   * If a frame was previously evictable and is to be set to non-evictable, then size should
   * decrement. If a frame was previously non-evictable and is to be set to evictable,
   * then size should increment.
   *
   * If frame id is invalid, throw an exception or abort the process.
   *
   * For other scenarios, this function should terminate without modifying anything.
   *
   * @param frame_id id of frame whose 'evictable' status will be modified
   * @param set_evictable whether the given frame is evictable or not
   * @brief 切换一个帧是可淘汰的还是不可淘汰的。这个函数也控制着替换器的大小。注意，大小等于可淘汰条目的数量。
       如果一个帧之前是可淘汰的，现在要设置为不可淘汰，那么大小应该减少。如果一个帧之前是不可淘汰的，现在要设置为可淘汰，那么大小应该增加。
       如果帧id无效，抛出一个异常或终止进程。
       对于其他情况，这个函数应该在不修改任何内容的情况下终止。

     @param frame_id 将修改其'可淘汰'状态的帧的id
     @param set_evictable 给定的帧是否可淘汰

   */
  void SetEvictable(frame_id_t frame_id, bool set_evictable);

  /**
   * TODO(P1): Add implementation
   *
   * @brief Remove an evictable frame from replacer, along with its access history.
   * This function should also decrement replacer's size if removal is successful.
   *
   * Note that this is different from evicting a frame, which always remove the frame
   * with largest backward k-distance. This function removes specified frame id,
   * no matter what its backward k-distance is.
   *
   * If Remove is called on a non-evictable frame, throw an exception or abort the
   * process.
   *
   * If specified frame is not found, directly return from this function.
   *
   * @param frame_id id of frame to be removed
   *

    @brief 从替换器中移除一个可淘汰的帧，以及其访问历史。
           如果移除成功，这个函数还应该减少替换器的大小。
           注意，这与淘汰一个帧不同，后者总是移除具有最大向后k距离的帧。
           这个函数移除指定的帧id，无论其向后k距离是多少。
           如果在不可淘汰的帧上调用Remove，抛出一个异常或终止进程。
           如果没有找到指定的帧，直接从函数返回。

    @param frame_id 要移除的帧的id
   */
  void Remove(frame_id_t frame_id);

  /**
   * TODO(P1): Add implementation
   *
   * @brief Return replacer's size, which tracks the number of evictable frames.
   * 返回替换器的大小，该大小跟踪可淘汰帧的数量。
   *
   * @return size_t
   */
  auto Size() -> size_t;

 private:
  // TODO(student): implement me! You can replace these member variables as you like.
  // Remove maybe_unused if you start using them.
  [[maybe_unused]] std::unordered_map<frame_id_t, LRUKNode> node_store_;
  [[maybe_unused]] size_t current_timestamp_{0};  //当前的时间戳,每进行一次record操作加一
  [[maybe_unused]] size_t curr_size_{0};          //当前存放的可驱逐页面数量
  [[maybe_unused]] size_t max_size_{0};           //最多可驱逐页面数量
  [[maybe_unused]] size_t replacer_size_;         //整个主存大小（用于判断页是否非法越界）
  [[maybe_unused]] size_t k_;                     // lru-k的k
  [[maybe_unused]] std::mutex latch_;
  [[maybe_unused]] std::mutex lock_guard_;  //加锁标志（std::mutex）

  using timestamp = std::list<size_t>;           //记录单个页时间戳的列表
  using k_time = std::pair<frame_id_t, size_t>;  //页号对应的第k次被访问的时间戳

  std::unordered_map<frame_id_t, timestamp> time_frame_;  //页号与记录的访问时间的映射,用于记录所有页的时间戳
  std::unordered_map<frame_id_t, size_t> recorded_cnt_;  //记录访问次数
  std::unordered_map<frame_id_t, bool> evictable_;       //用于记录是否可以被驱逐

  std::list<frame_id_t> new_frame_;  //记录不满足k次访问页的页号(历史访问队列)
  std::unordered_map<frame_id_t, std::list<frame_id_t>::iterator>
      new_locate_;  //页号到历史访问队列的哈希表(unordered_map基于哈希表实现)

  std::list<k_time> cache_frame_;  //到达k次页的链表的页号（缓存队列）,存储页号和第k次被访问的时间戳
  std::unordered_map<frame_id_t, std::list<k_time>::iterator> cache_locate_;  //页号到缓存队列的哈希表
  static auto CmpTimestamp(const LRUKReplacer::k_time &f1, const LRUKReplacer::k_time &f2) -> bool;
};

}  // namespace bustub
