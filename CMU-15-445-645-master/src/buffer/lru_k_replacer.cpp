//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"
#include "common/exception.h"

namespace bustub {

//初始化，定义越界范围和k值
LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) { max_size_ = num_frames; }

//驱逐一个页面，并保存到frame_id中
auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  std::lock_guard<std::mutex> lock(latch_);
  /**
   * 如果没有可以驱逐元素
   */
  if (Size() == 0) {
    return false;
  }
  /**
   * 首先尝试删除距离为无限大的缓存
   */
  for (auto it = new_frame_.rbegin(); it != new_frame_.rend(); it++) {
    auto frame = *it;
    if (evictable_[frame])  //如果可以被删除
    {
      recorded_cnt_[frame] = 0;
      new_locate_.erase(frame);
      new_frame_.remove(frame);
      *frame_id = frame;
      curr_size_--;
      time_frame_[frame].clear();
      return true;
    }
  }
  /**
   * 再尝试删除已经访问过K次的缓存
   */
  for (auto it = cache_frame_.begin(); it != cache_frame_.end(); it++) {
    auto frame = (*it).first;
    if (evictable_[frame]) {
      recorded_cnt_[frame] = 0;
      cache_frame_.erase(it);
      cache_locate_.erase(frame);
      *frame_id = frame;
      curr_size_--;
      time_frame_[frame].clear();
      return true;
    }
  }
  return false;
}

//增加一个页面的访问记录
void LRUKReplacer::RecordAccess(frame_id_t frame_id, [[maybe_unused]] AccessType access_type) {
  std::lock_guard<std::mutex> lock(latch_);

  //越界处理
  if (frame_id > static_cast<frame_id_t>(replacer_size_)) {
    throw std::exception();
  }

  current_timestamp_++;                //时间戳增加
  recorded_cnt_[frame_id]++;           //该页访问次数增加
  auto cnt = recorded_cnt_[frame_id];  //当前frame的访问次数
  time_frame_[frame_id].push_back(current_timestamp_);
  /**
   * 如果是新加入的记录
   */
  if (cnt == 1) {
    if (curr_size_ == max_size_)  //当前可存放的可驱逐的页面数量等于最多可驱逐的页面数量
    {
      frame_id_t frame;
      Evict(&frame);  //驱逐一个页面，并保存到frame_id中
    }
    evictable_[frame_id] = true;
    curr_size_++;
    new_frame_.push_front(frame_id);
    new_locate_[frame_id] = new_frame_.begin();
  }
  /**
   * 如果记录达到k次，则需要从新队列(历史访问队列)中加入到老队列(缓存队列)中
   */
  if (cnt == k_) {
    new_frame_.erase(new_locate_[frame_id]);  //从新队列中删除
    new_locate_.erase(frame_id);

    auto kth_time = time_frame_[frame_id].front();  //获取当前页面的倒数第k次出现的时间,
    // front返回第一个元素，而此时cnt正好是k
    k_time new_cache(frame_id, kth_time);
    auto it = std::upper_bound(cache_frame_.begin(), cache_frame_.end(), new_cache, CmpTimestamp);  //二分查找插入点
    //从数组的begin位置到end-1位置二分查找第一个大于num的数字，找到返回该数字的地址，不存在则返回end。通过返回的地址减去起始地址begin,得到找到数字在数组中的下标。
    it = cache_frame_.insert(it, new_cache);
    cache_locate_[frame_id] = it;
    return;
  }
  /**
   * 如果记录在k次以上，需要将该frame放到指定的位置。由于我们需要维护的是第k次访问的时间戳，因此我们应该在k+1时更新记录，使得仍然维护k
   */
  if (cnt > k_) {
    //这行代码会删除 time_frame_ 映射中与键 frame_id 相关联的 std::list<size_t> 中的第一个元素。
    //由于 std::list 中的元素是按照插入顺序排列的，这实际上就是删除了最早插入的元素。
    //因此，删除的是最早插入的元素，而不是新插入的最后一个元素。
    time_frame_[frame_id].erase(time_frame_[frame_id].begin());

    cache_frame_.erase(cache_locate_[frame_id]);    //去除原来的位置
    auto kth_time = time_frame_[frame_id].front();  //获取当前页面的倒数第k次出现的时间
    k_time new_cache(frame_id, kth_time);

    auto it = std::upper_bound(cache_frame_.begin(), cache_frame_.end(), new_cache, CmpTimestamp);  //二分查找插入点
    it = cache_frame_.insert(it, new_cache);
    cache_locate_[frame_id] = it;
    return;
  }
  /**
   * 如果cnt<k_，是不需要做更新动作的
   */
}

//设置一个页面是否可以被驱逐
void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  std::lock_guard<std::mutex> lock(latch_);
  // 如果该页没有被记录（可能是新页面或者已经被驱逐），则直接返回
  if (recorded_cnt_[frame_id] == 0) {
    return;
  }
  // 获取该页当前的可驱逐状态
  auto status = evictable_[frame_id];
  // 更新该页的可驱逐状态为传入的新状态
  evictable_[frame_id] = set_evictable;
  // 如果页面之前是可驱逐的，现在被设置为不可驱逐
  if (status && !set_evictable) {
    // 减少最多可驱逐的页面数量和当前存放的可驱逐页面数量
    --max_size_;
    --curr_size_;
  }
  // 如果页面之前是不可驱逐的，现在被设置为可驱逐
  if (!status && set_evictable) {
    // 增加最大容量和当前容量
    ++max_size_;
    ++curr_size_;
  }
}

//移除指定页面（仅在BufferPoolManager中删除页面时调用。）
void LRUKReplacer::Remove(frame_id_t frame_id) {
  std::lock_guard<std::mutex> lock(latch_);
  if (frame_id > static_cast<frame_id_t>(replacer_size_)) {
    throw std::exception();
  }
  auto cnt = recorded_cnt_[frame_id];
  if (cnt == 0) {
    return;
  }
  if (!evictable_[frame_id]) {
    throw std::exception();
  }
  if (cnt < k_) {
    new_frame_.erase(new_locate_[frame_id]);
    new_locate_.erase(frame_id);
    recorded_cnt_[frame_id] = 0;
    time_frame_[frame_id].clear();
    curr_size_--;
  } else {
    cache_frame_.erase(cache_locate_[frame_id]);
    cache_locate_.erase(frame_id);
    recorded_cnt_[frame_id] = 0;
    time_frame_[frame_id].clear();
    curr_size_--;
  }
}

//返回可驱逐页面的大小
auto LRUKReplacer::Size() -> size_t { return curr_size_; }

auto LRUKReplacer::CmpTimestamp(const LRUKReplacer::k_time &f1, const LRUKReplacer::k_time &f2) -> bool {
  return f1.second < f2.second;
}

}  // namespace bustub
