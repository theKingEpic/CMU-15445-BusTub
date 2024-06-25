//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_directory_page.cpp
//
// Identification: src/storage/page/extendible_htable_directory_page.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/page/extendible_htable_directory_page.h"

#include <algorithm>
#include <unordered_map>

#include "common/config.h"
#include "common/logger.h"

namespace bustub {
void ExtendibleHTableDirectoryPage::Init(uint32_t max_depth) {
  this->max_depth_ = max_depth;
  global_depth_ = 0;
  // 初始化目录页ID数组，将所有值设为无效ID
  //将 local_depths_ 数组或容器从开始位置到 2^max_depth 位置的元素全部初始化为0。
  std::fill(local_depths_, local_depths_ + (1 << max_depth), 0);
  std::fill(bucket_page_ids_, bucket_page_ids_ + (1 << max_depth), INVALID_PAGE_ID);
}

auto ExtendibleHTableDirectoryPage::HashToBucketIndex(uint32_t hash) const -> uint32_t {
  // 返回hash的后global_depth位
  return hash & GetGlobalDepthMask();
}

// 计算掩码，全局深度位的低位是 1，其余位是 0
auto ExtendibleHTableDirectoryPage::GetGlobalDepthMask() const -> uint32_t {
  // 例如，如果全局深度为 2，则掩码是二进制的 11（即十进制的 3）
  auto depth = global_depth_;
  uint32_t result = (1 << depth) - 1;
  return result;
}

auto ExtendibleHTableDirectoryPage::GetLocalDepthMask(uint32_t bucket_idx) const -> uint32_t {
  // 首先检查 bucket_idx 是否有效
  if (bucket_idx >= static_cast<uint32_t>(1 << global_depth_)) {
    throw std::out_of_range("Bucket index out of range");
  }
  // 获取 bucket_idx 处的局部深度
  uint32_t local_depth = local_depths_[bucket_idx];
  // 计算掩码，局部深度位的低位是 1，其余位是 0
  return 1 << (local_depth - 1);
}

auto ExtendibleHTableDirectoryPage::GetBucketPageId(uint32_t bucket_idx) const -> page_id_t {
  //获取在指定索引处（bucket_page_ids_[bucket_idx]）的值
  return this->bucket_page_ids_[bucket_idx];
}

void ExtendibleHTableDirectoryPage::SetBucketPageId(uint32_t bucket_idx, page_id_t bucket_page_id) {
  //设置bucket_page_id在指定索引处的值
  this->bucket_page_ids_[bucket_idx] = bucket_page_id;
}

auto ExtendibleHTableDirectoryPage::GetSplitImageIndex(uint32_t bucket_idx) const -> uint32_t {
  return bucket_idx + (1 << (global_depth_ - 1));
}

auto ExtendibleHTableDirectoryPage::GetGlobalDepth() const -> uint32_t { return global_depth_; }

void ExtendibleHTableDirectoryPage::IncrGlobalDepth() {
  //目录全局深度加一
  if (global_depth_ >= max_depth_) {
    return;
  }
  for (int i = 0; i < 1 << global_depth_; i++) {
    // 将现有目录项复制到新的位置，新位置是旧位置索引的两倍
    bucket_page_ids_[(1 << global_depth_) + i] = bucket_page_ids_[i];
    // 同样复制局部深度信息
    local_depths_[(1 << global_depth_) + i] = local_depths_[i];
  }
  global_depth_++;
}

void ExtendibleHTableDirectoryPage::DecrGlobalDepth() {
  if (global_depth_ <= 0) {
    return;
  }
  global_depth_--;
}

auto ExtendibleHTableDirectoryPage::CanShrink() -> bool {
  if (global_depth_ == 0) {
    return false;
  }
  // 检查所有桶的局部深度是否都小于全局深度
  for (uint32_t i = 0; i < Size(); i++) {
    // 有局部深度等于全局深度的->不能收缩
    if (local_depths_[i] == global_depth_) {
      return false;
    }
  }
  return true;
}

auto ExtendibleHTableDirectoryPage::Size() const -> uint32_t {
  // 目录的当前大小是2的global_depth_次方
  return 1 << global_depth_;
}

auto ExtendibleHTableDirectoryPage::GetLocalDepth(uint32_t bucket_idx) const -> uint32_t {
  return local_depths_[bucket_idx];
}

auto ExtendibleHTableDirectoryPage::GetMaxDepth() const -> uint32_t { return max_depth_; }

void ExtendibleHTableDirectoryPage::SetLocalDepth(uint32_t bucket_idx, uint8_t local_depth) {
  local_depths_[bucket_idx] = local_depth;
}

void ExtendibleHTableDirectoryPage::IncrLocalDepth(uint32_t bucket_idx) {
  if (local_depths_[bucket_idx] < global_depth_) {
    ++local_depths_[bucket_idx];
  }
}

void ExtendibleHTableDirectoryPage::DecrLocalDepth(uint32_t bucket_idx) {
  if (local_depths_[bucket_idx] > 0) {
    --local_depths_[bucket_idx];
  }
}

}  // namespace bustub
