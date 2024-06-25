//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_bucket_page.cpp
//
// Identification: src/storage/page/extendible_htable_bucket_page.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <optional>
#include <utility>

#include "common/exception.h"
#include "storage/page/extendible_htable_bucket_page.h"

namespace bustub {

template <typename K, typename V, typename KC>
void ExtendibleHTableBucketPage<K, V, KC>::Init(uint32_t max_size) {
  this->max_size_ = max_size;
  this->size_ = 0;
}

//查找key,如果找到了把key(array[i],first)对应的值(array[i].second)赋给value，返回true
template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Lookup(const K &key, V &value, const KC &cmp) const -> bool {
  for (uint32_t i = 0; i < size_; ++i) {
    if (cmp(array_[i].first, key) == 0) {
      value = array_[i].second;
      return true;
    }
  }
  return false;
}

// 插入一个键值对
template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Insert(const K &key, const V &value, const KC &cmp) -> bool {
  // 如果桶页已满，则返回 false，表示插入失败
  if (this->IsFull()) {
    return false;
  }
  // 遍历桶页中的所有现有键值对
  for (uint32_t i = 0; i < size_; ++i) {
    // 使用键比较器来检查当前键是否已存在
    if (cmp(this->array_[i].first, key) == 0) {
      // 如果键已存在，则返回 false，表示插入失败
      return false;
    }
  }
  // 如果键不存在，则将新的键值对添加到桶页的数组中
  array_[size_++] = std::make_pair(key, value);
  // 插入成功，返回 true
  return true;
}

//删除一个键值对
template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Remove(const K &key, const KC &cmp) -> bool {
  for (uint32_t i = 0; i < size_; ++i) {
    if (cmp(array_[i].first, key) == 0) {
      // 找到键并删除
      for (uint32_t j = i + 1; j < size_; ++j) {
        array_[j - 1] = array_[j];
      }
      size_--;
      return true;
    }
  }
  return false;
}

template <typename K, typename V, typename KC>
void ExtendibleHTableBucketPage<K, V, KC>::RemoveAt(uint32_t bucket_idx) {
  for (uint32_t i = bucket_idx; i <= size_; ++i) {
    array_[i] = array_[i + 1];
  }
  size_--;
}

template <typename K, typename V, typename KC>
void ExtendibleHTableBucketPage<K, V, KC>::Clear() {
  size_ = 0;
}
template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::KeyAt(uint32_t bucket_idx) const -> K {
  return EntryAt(bucket_idx).first;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::ValueAt(uint32_t bucket_idx) const -> V {
  return EntryAt(bucket_idx).second;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::EntryAt(uint32_t bucket_idx) const -> const std::pair<K, V> & {
  return array_[bucket_idx];
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Size() const -> uint32_t {
  return this->size_;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::IsFull() const -> bool {
  return this->size_ == this->max_size_;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::IsEmpty() const -> bool {
  return this->size_ == 0;
}

template class ExtendibleHTableBucketPage<int, int, IntComparator>;
template class ExtendibleHTableBucketPage<GenericKey<4>, RID, GenericComparator<4>>;
template class ExtendibleHTableBucketPage<GenericKey<8>, RID, GenericComparator<8>>;
template class ExtendibleHTableBucketPage<GenericKey<16>, RID, GenericComparator<16>>;
template class ExtendibleHTableBucketPage<GenericKey<32>, RID, GenericComparator<32>>;
template class ExtendibleHTableBucketPage<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
