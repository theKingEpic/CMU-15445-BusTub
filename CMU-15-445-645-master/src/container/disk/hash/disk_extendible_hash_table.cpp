//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// disk_extendible_hash_table.cpp
//
// Identification: src/container/disk/hash/disk_extendible_hash_table.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "common/config.h"
#include "common/exception.h"
#include "common/logger.h"
#include "common/macros.h"
#include "common/rid.h"
#include "common/util/hash_util.h"
#include "container/disk/hash/disk_extendible_hash_table.h"
#include "storage/index/hash_comparator.h"
#include "storage/page/extendible_htable_bucket_page.h"
#include "storage/page/extendible_htable_directory_page.h"
#include "storage/page/extendible_htable_header_page.h"
#include "storage/page/page_guard.h"

namespace bustub {

template <typename K, typename V, typename KC>
DiskExtendibleHashTable<K, V, KC>::DiskExtendibleHashTable(const std::string &name, BufferPoolManager *bpm,
                                                           const KC &cmp, const HashFunction<K> &hash_fn,
                                                           uint32_t header_max_depth, uint32_t directory_max_depth,
                                                           uint32_t bucket_max_size)
    : bpm_(bpm),
      cmp_(cmp),
      hash_fn_(std::move(hash_fn)),
      header_max_depth_(header_max_depth),
      directory_max_depth_(directory_max_depth),
      bucket_max_size_(bucket_max_size) {
  this->index_name_ = name;
  // 初始化header页
  header_page_id_ = INVALID_PAGE_ID;
  //从缓冲池管理器（BufferPoolManager）请求一个新页面，并将其保护在一个guard对象中，以便在异常情况下可以自动释放。
  //同时，它将新页面的ID存储在header_page_id_变量中。
  auto header_guard = bpm->NewPageGuarded(&header_page_id_);
  //将保护的新页面转换为ExtendibleHTableHeaderPage类型的可变指针，这样就可以对其进行初始化和修改
  auto header_page = header_guard.AsMut<ExtendibleHTableHeaderPage>();
  //调用Init方法来初始化头部页，传入header_max_depth_作为参数，这是头部页可以支持的最大深度
  header_page->Init(header_max_depth_);
}

/*****************************************************************************
 * SEARCH
 *****************************************************************************/
template <typename K, typename V, typename KC>
// 在哈希表中查找与给定键相关联的值
// key:要查找的键;result:与给定键相关联的值; transaction 当前事务
auto DiskExtendibleHashTable<K, V, KC>::GetValue(const K &key, std::vector<V> *result, Transaction *transaction) const
    -> bool {
  // 获取header page
  ReadPageGuard header_guard = bpm_->FetchPageRead(header_page_id_);
  auto header_page = header_guard.As<ExtendibleHTableHeaderPage>();
  // 通过hash值获取dir_page_id。若dir_page_id为非法id则未找到
  auto hash = Hash(key);
  auto dir_index = header_page->HashToDirectoryIndex(hash);
  page_id_t directory_page_id = header_page->GetDirectoryPageId(dir_index);
  if (directory_page_id == INVALID_PAGE_ID) {
    return false;
  }
  // 获取dir_page
  header_guard.Drop();
  ReadPageGuard directory_guard = bpm_->FetchPageRead(directory_page_id);
  auto directory_page = directory_guard.As<ExtendibleHTableDirectoryPage>();
  // 通过hash值获取bucket_page_id。若bucket_page_id为非法id则未找到
  auto bucket_index = directory_page->HashToBucketIndex(hash);
  auto bucket_page_id = directory_page->GetBucketPageId(bucket_index);
  LOG_DEBUG("要获得的bucket_page_id为：%d,哈系值为%d", bucket_page_id, hash);
  if (bucket_page_id == INVALID_PAGE_ID) {
    return false;
  }
  ReadPageGuard bucket_guard = bpm_->FetchPageRead(bucket_page_id);
  // 获取bucket_page
  directory_guard.Drop();
  auto bucket_page = bucket_guard.As<ExtendibleHTableBucketPage<K, V, KC>>();
  // 在bucket_page上查找
  V value;
  if (bucket_page->Lookup(key, value, cmp_)) {
    result->push_back(value);
    return true;
  }
  return false;
}

template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::SplitBucket(ExtendibleHTableDirectoryPage *directory,
                                                    ExtendibleHTableBucketPage<K, V, KC> *bucket, uint32_t bucket_idx)
    -> bool {
  // 创建新bucket_page
  page_id_t split_page_id;
  WritePageGuard split_bucket_guard = bpm_->NewPageGuarded(&split_page_id).UpgradeWrite();
  if (split_page_id == INVALID_PAGE_ID) {
    return false;
  }
  //将split_bucket_guard转换为可变的桶页指针，并初始化这个新桶页
  auto split_bucket = split_bucket_guard.AsMut<ExtendibleHTableBucketPage<K, V, KC>>();
  split_bucket->Init(bucket_max_size_);
  //获取分裂索引和局部深度，并在目录中设置新桶页的ID和局部深度。
  uint32_t split_idx = directory->GetSplitImageIndex(bucket_idx);
  uint32_t local_depth = directory->GetLocalDepth(bucket_idx);
  directory->SetBucketPageId(split_idx, split_page_id);
  directory->SetLocalDepth(split_idx, local_depth);
  //记录日志，显示分裂的桶页ID。
  LOG_DEBUG("Spilt bucket_page_id:%d", split_page_id);
  // 将原来满的bucket_page拆分到两个page页中

  // 获取原始满桶的页ID。
  page_id_t bucket_page_id = directory->GetBucketPageId(bucket_idx);
  if (bucket_page_id == INVALID_PAGE_ID) {
    return false;
  }

  // 先将原来的数据取出，放置在entries容器中
  //将原始桶页中的所有条目取出并放入一个列表中。
  int size = bucket->Size();
  std::list<std::pair<K, V>> entries;
  for (int i = 0; i < size; i++) {
    entries.emplace_back(bucket->EntryAt(i));  //在容器的末尾就地构造并插入元素
  }
  //把满桶里的内容放到容器里以后要把原本的满桶清空
  // 清空bucket:size_ = 0
  bucket->Clear();

  // 分到两个bucket_page中
  //遍历列表中的每个条目，计算哈希值，并根据哈希值确定目标桶的索引和页ID。
  for (const auto &entry : entries) {
    uint32_t target_idx = directory->HashToBucketIndex(Hash(entry.first));
    page_id_t target_page_id = directory->GetBucketPageId(target_idx);
    //根据目标页ID，将条目重新插入到原始桶页或新桶页中。
    if (target_page_id == bucket_page_id) {
      bucket->Insert(entry.first, entry.second, cmp_);
    } else if (target_page_id == split_page_id) {
      split_bucket->Insert(entry.first, entry.second, cmp_);
    }
  }
  //如果一切正常，返回true表示分裂操作成功
  return true;
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/

template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::Insert(const K &key, const V &value, Transaction *transaction) -> bool {
  std::vector<V> values_found;
  bool key_exists = GetValue(key, &values_found, transaction);
  if (key_exists) {
    // 已存在直接返回false表示不插入重复键
    return false;
  }
  auto hash_key = Hash(key);
  // 获取header page
  WritePageGuard header_guard = bpm_->FetchPageWrite(header_page_id_);
  auto header_page = header_guard.AsMut<ExtendibleHTableHeaderPage>();
  // 使用header_page来获取目录索引
  auto directory_index = header_page->HashToDirectoryIndex(hash_key);
  //用目录索引获取目录页，然后找到頁的目录ID
  page_id_t directory_page_id = header_page->GetDirectoryPageId(directory_index);
  // 若dir_page_id为非法id则在新的dir_page添加
  if (directory_page_id == INVALID_PAGE_ID) {
    return InsertToNewDirectory(header_page, directory_index, hash_key, key, value);
  }
  // 对directory加锁
  header_guard.Drop();
  WritePageGuard directory_guard = bpm_->FetchPageWrite(directory_page_id);
  // 获取 dir page
  auto directory_page = directory_guard.AsMut<ExtendibleHTableDirectoryPage>();
  // 通过hash值获取bucket_page_id。若bucket_page_id为非法id则在新的bucket_page添加
  auto bucket_index = directory_page->HashToBucketIndex(hash_key);
  auto bucket_page_id = directory_page->GetBucketPageId(bucket_index);
  if (bucket_page_id == INVALID_PAGE_ID) {
    return InsertToNewBucket(directory_page, bucket_index, key, value);
  }

  // 对bucket加锁
  WritePageGuard bucket_guard = bpm_->FetchPageWrite(bucket_page_id);
  auto bucket_page = bucket_guard.AsMut<ExtendibleHTableBucketPage<K, V, KC>>();
  // LOG_DEBUG("要插入的bucket是：%d，哈希数是%d",bucket_page_id,hash_key);
  // 获取bucket_page插入元素，如果插入失败则代表该bucket_page满了
  if (bucket_page->Insert(key, value, cmp_)) {
    LOG_DEBUG("Insert bucket %d Success!", bucket_page_id);
    return true;
  }

  //旧的全局深度
  auto h = 1U << directory_page->GetGlobalDepth();
  // 判断是否能添加度，不能则返回
  if (directory_page->GetLocalDepth(bucket_index) == directory_page->GetGlobalDepth()) {
    if (directory_page->GetGlobalDepth() >= directory_page->GetMaxDepth()) {
      return false;
    }
    directory_page->IncrGlobalDepth();
    // 需要更新目录页 h是旧的全局深度的幂，directory_page->GetGlobalDepth()是新的全局深度的幂，二者差一倍，如从4->8
    // 举例，h是4,新的是8,i为4～7,i-h为0～3
    for (uint32_t i = h; i < (1U << directory_page->GetGlobalDepth()); ++i) {
      auto new_bucket_page_id = directory_page->GetBucketPageId(i - h);
      auto new_local_depth = directory_page->GetLocalDepth(i - h);
      directory_page->SetBucketPageId(i, new_bucket_page_id);
      directory_page->SetLocalDepth(i, new_local_depth);
    }
  }
  directory_page->IncrLocalDepth(bucket_index);
  directory_page->IncrLocalDepth(bucket_index + h);
  // 拆分bucket
  if (!SplitBucket(directory_page, bucket_page, bucket_index)) {
    return false;
  }
  bucket_guard.Drop();
  directory_guard.Drop();
  //注意拆分桶这里，拆分一次不一定就拆好了，可能一个还是满的，一个空的，所以还要回到insert方法，接着判断满没满
  return Insert(key, value, transaction);
}

template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::InsertToNewDirectory(ExtendibleHTableHeaderPage *header, uint32_t directory_idx,
                                                             uint32_t hash, const K &key, const V &value) -> bool {
  page_id_t directory_page_id = INVALID_PAGE_ID;
  WritePageGuard directory_guard = bpm_->NewPageGuarded(&directory_page_id).UpgradeWrite();
  //将保护对象转换为ExtendibleHTableDirectoryPage类型的可变（可写）指针。
  auto directory_page = directory_guard.AsMut<ExtendibleHTableDirectoryPage>();
  directory_page->Init(directory_max_depth_);                     //初始化目录
  header->SetDirectoryPageId(directory_idx, directory_page_id);   //在头部页面设置目录页的ID
  uint32_t bucket_idx = directory_page->HashToBucketIndex(hash);  //使用哈希值来计算桶索引
  LOG_DEBUG("InsertToNewDirectory directory_page_id:%d", directory_page_id);
  return InsertToNewBucket(directory_page, bucket_idx, key, value);  //将键值对插入新桶，并返回结果
}

template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::InsertToNewBucket(ExtendibleHTableDirectoryPage *directory, uint32_t bucket_idx,
                                                          const K &key, const V &value) -> bool {
  page_id_t bucket_page_id = INVALID_PAGE_ID;
  WritePageGuard bucket_guard = bpm_->NewPageGuarded(&bucket_page_id).UpgradeWrite();
  auto bucket_page = bucket_guard.AsMut<ExtendibleHTableBucketPage<K, V, KC>>();
  bucket_page->Init(bucket_max_size_);
  directory->SetBucketPageId(bucket_idx, bucket_page_id);
  LOG_DEBUG("InsertToNewBucket bucket_page_id:%d", bucket_page_id);
  return bucket_page->Insert(key, value, cmp_);
}

//在哈希表因为桶分裂而需要调整目录条目时被调用，更新目录页中的映射，确保目录正确地指向新旧桶。
//在可扩展哈希表中，桶分裂是通过增加目录的局部深度来实现的，这可能会导致目录条目的重新映射
template <typename K, typename V, typename KC>
void DiskExtendibleHashTable<K, V, KC>::UpdateDirectoryMapping(ExtendibleHTableDirectoryPage *directory,
                                                               uint32_t new_bucket_idx, page_id_t new_bucket_page_id,
                                                               uint32_t new_local_depth, uint32_t local_depth_mask) {
  for (uint32_t i = 0; i < (1U << directory->GetGlobalDepth()); ++i) {
    // 检查当前目录条目是否指向原桶（即与新桶具有相同的页面ID）
    if (directory->GetBucketPageId(i) == directory->GetBucketPageId(new_bucket_idx)) {
      //检查当前目录条目在新局部深度位上的值是否为1
      if ((i & local_depth_mask) != 0U) {
        // 如果这个目录项的在新局部深度位上的值为1，应该指向新桶
        directory->SetBucketPageId(i, new_bucket_page_id);
        directory->SetLocalDepth(i, new_local_depth);
      } else {
        // 否则，它仍然指向原桶，但其局部深度需要更新
        directory->SetLocalDepth(i, new_local_depth);
      }
    }
  }
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::Remove(const K &key, Transaction *transaction) -> bool {
  // 使用哈希函数计算键的哈希值
  uint32_t hash = Hash(key);
  // 获取并锁定头部页面的写权限
  WritePageGuard header_guard = bpm_->FetchPageWrite(header_page_id_);
  // 将头部页面转换为可变类型
  auto header_page = header_guard.AsMut<ExtendibleHTableHeaderPage>();
  // 计算目录索引
  uint32_t directory_index = header_page->HashToDirectoryIndex(hash);
  // 获取目录页面ID
  page_id_t directory_page_id = header_page->GetDirectoryPageId(directory_index);
  // 如果目录页面ID无效，返回false
  if (directory_page_id == INVALID_PAGE_ID) {
    return false;
  }
  // 释放头部页面的锁
  header_guard.Drop();
  // 获取并锁定目录页面的写权限
  WritePageGuard directory_guard = bpm_->FetchPageWrite(directory_page_id);
  // 将目录页面转换为可变类型
  auto directory_page = directory_guard.AsMut<ExtendibleHTableDirectoryPage>();
  // 计算桶索引
  uint32_t bucket_index = directory_page->HashToBucketIndex(hash);
  // 获取桶页面ID
  page_id_t bucket_page_id = directory_page->GetBucketPageId(bucket_index);
  // 如果桶页面ID无效，返回false
  if (bucket_page_id == INVALID_PAGE_ID) {
    return false;
  }
  // 获取并锁定桶页面的写权限
  WritePageGuard bucket_guard = bpm_->FetchPageWrite(bucket_page_id);
  // 将桶页面转换为可变类型
  auto bucket_page = bucket_guard.AsMut<ExtendibleHTableBucketPage<K, V, KC>>();
  // 从桶页面中删除键值对
  bool res = bucket_page->Remove(key, cmp_);
  // 释放桶页面的锁
  bucket_guard.Drop();
  // 如果删除失败，返回false
  if (!res) {
    return false;
  }
  // 检查页面ID，用于后续的合并操作
  auto check_page_id = bucket_page_id;
  // 获取并锁定检查页面的读权限
  ReadPageGuard check_guard = bpm_->FetchPageRead(check_page_id);
  // 将检查页面转换为常量类型
  auto check_page = reinterpret_cast<const ExtendibleHTableBucketPage<K, V, KC> *>(check_guard.GetData());
  // 获取局部深度
  uint32_t local_depth = directory_page->GetLocalDepth(bucket_index);
  // 获取全局深度
  uint32_t global_depth = directory_page->GetGlobalDepth();
  // 如果局部深度大于0，尝试合并桶
  while (local_depth > 0) {
    // 获取要合并的桶的索引
    uint32_t convert_mask = 1 << (local_depth - 1);
    uint32_t merge_bucket_index = bucket_index ^ convert_mask;
    // 获取合并桶的局部深度
    uint32_t merge_local_depth = directory_page->GetLocalDepth(merge_bucket_index);
    // 获取合并桶的页面ID
    page_id_t merge_page_id = directory_page->GetBucketPageId(merge_bucket_index);
    // 获取并锁定合并桶的读权限
    ReadPageGuard merge_guard = bpm_->FetchPageRead(merge_page_id);
    // 将合并桶页面转换为常量类型
    auto merge_page = reinterpret_cast<const ExtendibleHTableBucketPage<K, V, KC> *>(merge_guard.GetData());
    // 如果局部深度不同或者两个桶都不为空，跳出循环
    if (merge_local_depth != local_depth || (!check_page->IsEmpty() && !merge_page->IsEmpty())) {
      break;
    }
    // 如果检查页面为空，删除它并更新检查页面和ID
    if (check_page->IsEmpty()) {
      bpm_->DeletePage(check_page_id);
      check_page = merge_page;
      check_page_id = merge_page_id;
      check_guard = std::move(merge_guard);
    } else {
      // 否则，删除合并桶
      bpm_->DeletePage(merge_page_id);
    }
    // 减少目录页面的局部深度
    directory_page->DecrLocalDepth(bucket_index);
    // 更新局部深度
    local_depth = directory_page->GetLocalDepth(bucket_index);
    // 获取局部深度掩码
    uint32_t local_depth_mask = directory_page->GetLocalDepthMask(bucket_index);
    // 计算掩码索引，用于确定目录条目更新的起始位置
    uint32_t mask_idx = bucket_index & local_depth_mask;
    // 计算需要更新的目录条目数量，这通常是2的幂次，取决于全局深度和局部深度的差值
    uint32_t update_count = 1 << (global_depth - local_depth);
    // 遍历需要更新的每个目录条目
    for (uint32_t i = 0; i < update_count; ++i) {
      // 计算当前需要更新的目录条目的索引
      uint32_t tmp_idx = (i << local_depth) + mask_idx;
      // 更新目录映射，将目录条目指向检查页面（即未被删除的桶）
      UpdateDirectoryMapping(directory_page, tmp_idx, check_page_id, local_depth, 0);
    }
  }
  // 如果目录页面可以收缩（全局深度大于局部深度），减少全局深度
  while (directory_page->CanShrink()) {
    directory_page->DecrGlobalDepth();
  }
  return true;
}

template class DiskExtendibleHashTable<int, int, IntComparator>;
template class DiskExtendibleHashTable<GenericKey<4>, RID, GenericComparator<4>>;
template class DiskExtendibleHashTable<GenericKey<8>, RID, GenericComparator<8>>;
template class DiskExtendibleHashTable<GenericKey<16>, RID, GenericComparator<16>>;
template class DiskExtendibleHashTable<GenericKey<32>, RID, GenericComparator<32>>;
template class DiskExtendibleHashTable<GenericKey<64>, RID, GenericComparator<64>>;
}  // namespace bustub
