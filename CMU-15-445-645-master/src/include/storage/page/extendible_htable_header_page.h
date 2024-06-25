//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_header_page.h
//
// Identification: src/include/storage/page/extendible_htable_header_page.h
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

/**
 * Header page format:
 *  ---------------------------------------------------
 * | DirectoryPageIds(2048) | MaxDepth (4) | Free(2044)
 *  ---------------------------------------------------
 */

#pragma once

#include <cstdlib>
#include "common/config.h"
#include "common/macros.h"

namespace bustub {

static constexpr uint64_t HTABLE_HEADER_PAGE_METADATA_SIZE = sizeof(uint32_t);
static constexpr uint64_t HTABLE_HEADER_MAX_DEPTH = 9;
static constexpr uint64_t HTABLE_HEADER_ARRAY_SIZE = 1 << HTABLE_HEADER_MAX_DEPTH;

class ExtendibleHTableHeaderPage {
 public:
  // Delete all constructor / destructor to ensure memory safety
  ExtendibleHTableHeaderPage() = delete;
  DISALLOW_COPY_AND_MOVE(ExtendibleHTableHeaderPage);

  /**
   * After creating a new header page from buffer pool, must call initialize
   * method to set default values
   *
   * @param max_depth Max depth in the header page
   * 在从缓冲池创建新的头部页面后，必须调用初始化方法以设置默认值
     @param max_depth 头部页面中的最大深度
   */
  void Init(uint32_t max_depth = HTABLE_HEADER_MAX_DEPTH);  //初始化数组

  /**
   * Get the directory index that the key is hashed to
   *
   * @param hash the hash of the key
   * @return directory index the key is hashed to
   *
   * 获取键哈希到的目录索引
     @param hash 键的哈希值
     @return 键哈希到的目录索引
   */
  auto HashToDirectoryIndex(uint32_t hash) const
      -> uint32_t;  //获取该键哈希对应的目录索引，如位深度为2，32位哈希值为0x5f129982，最高有效位前2位为01，应该返回01

  /**
   * Get the directory page id at an index
   *
   * @param directory_idx index in the directory page id array
   * @return directory page_id at index
   *
   * 获取目录页面ID数组中某个索引处的目录页面ID
     @param directory_idx 目录页面ID数组中的索引
     @return 索引处的目录页面ID
   */
  auto GetDirectoryPageId(uint32_t directory_idx) const
      -> uint32_t;  //获取在指定索引处（directory_page_ids_[directory_idx]）的值

  /**
   * @brief Set the directory page id at an index
   *
   * @param directory_idx index in the directory page id array
   * @param directory_page_id page id of the directory
   */
  void SetDirectoryPageId(uint32_t directory_idx, page_id_t directory_page_id);  //设置directory page_id在指定索引处的值

  /**
   * @brief Get the maximum number of directory page ids the header page could handle获取标题页可以处理的最大目录页 ID
   * 数
   */
  auto MaxSize() const -> uint32_t;  //获取页头可处理的最大目录页ID数（2^max_depth）

  /**
   * Prints the header's occupancy information
   */
  void PrintHeader() const;

 private:
  page_id_t directory_page_ids_[HTABLE_HEADER_ARRAY_SIZE];  //第二层DirectoryPage的PageId数组
  uint32_t max_depth_;                                      //位深度x，PageId数组的大小为 2^x
};

static_assert(sizeof(page_id_t) == 4);

static_assert(sizeof(ExtendibleHTableHeaderPage) ==
              sizeof(page_id_t) * HTABLE_HEADER_ARRAY_SIZE + HTABLE_HEADER_PAGE_METADATA_SIZE);

static_assert(sizeof(ExtendibleHTableHeaderPage) <= BUSTUB_PAGE_SIZE);

}  // namespace bustub
