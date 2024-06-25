//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// index.h
//
// Identification: src/include/storage/index/index.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
/*
 * 这段代码定义了两个类：`IndexMetadata` 和 `Index`。这两个类位于 `bustub` 命名空间中。
1. `IndexMetadata`
类用于存储索引对象的元数据。这个类的对象维护了索引的元数据，包括索引的名称、索引所创建的表的名称、索引键的架构、
 以及索引键与基表列之间的映射关系。此外，该类还提供了判断索引是否为主键的方法。
2. `Index`
类是一个基类，用于派生出不同类型的索引。这个类的对象主要维护了底层表的架构信息、索引键与元组键之间的映射关系，
 并提供了一种抽象的方式，让外部世界可以与底层的索引实现进行交互，而不需要暴露实际的实现接口。
 此外，该类还处理了谓词扫描、插入、删除、谓词插入、点查询和全索引扫描等操作。

这两个类的主要方法和属性如下：
- `IndexMetadata` 类：
  - 构造函数：接受索引名称、表名称、元组架构、键属性和是否为主键等参数，创建一个 `IndexMetadata` 对象。
  - `GetName()`：返回索引的名称。
  - `GetTableName()`：返回索引所创建的表的名称。
  - `GetKeySchema()`：返回表示索引键的架构对象指针。
  - `GetIndexColumnCount()`：返回索引键中的列数。
  - `GetKeyAttrs()`：返回索引列与基表列之间的映射关系。
  - `IsPrimaryKey()`：返回索引是否为主键。
  - `ToString()`：返回用于调试的字符串表示。
- `Index` 类：
  - 构造函数：接受一个指向 `IndexMetadata` 对象的 unique_ptr，创建一个 `Index` 对象。
  - `GetMetadata()`：返回与索引关联的元数据对象的非拥有指针。
  - `GetIndexColumnCount()`：返回索引键中的列数。
  - `GetName()`：返回索引的名称。
  - `GetKeySchema()`：返回表示索引键的架构对象指针。
  - `GetKeyAttrs()`：返回索引列与基表列之间的映射关系。
  - `ToString()`：返回用于调试的字符串表示。
  - `InsertEntry()`：插入一个索引项。这是一个纯虚函数，需要在派生类中实现。
  - `DeleteEntry()`：删除一个索引项。这是一个纯虚函数，需要在派生类中实现。
  - `ScanKey()`：根据提供的键搜索索引。这是一个纯虚函数，需要在派生类中实现。
总的来说，这段代码提供了一个用于存储和管理数据库索引的框架，包括索引的元数据和索引的基本操作。具体的索引实现需要在派生类中完成。
*/
#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "catalog/schema.h"
#include "storage/table/tuple.h"
#include "type/value.h"

namespace bustub {

class Transaction;

/**
 * class IndexMetadata - Holds metadata of an index object.
 *
 * The metadata object maintains the tuple schema and key attribute of an
 * index, since the external callers does not know the actual structure of
 * the index key, so it is the index's responsibility to maintain such a
 * mapping relation and does the conversion between tuple key and index key
 * 元数据对象维护索引的元组模式和键属性，因为外部调用者不知道索引键的实际结构，
 * 所以维护这种映射关系并在元组键和索引键之间进行转换是索引的责任
 */
class IndexMetadata {
 public:
  IndexMetadata() = delete;

  /**
   * Construct a new IndexMetadata instance.
   * @param index_name The name of the index
   * @param table_name The name of the table on which the index is created
   * @param tuple_schema The schema of the indexed key
   * @param key_attrs The mapping from indexed columns to base table columns
   */
  IndexMetadata(std::string index_name, std::string table_name, const Schema *tuple_schema,
                std::vector<uint32_t> key_attrs, bool is_primary_key)
      : name_(std::move(index_name)),
        table_name_(std::move(table_name)),
        key_attrs_(std::move(key_attrs)),
        is_primary_key_(is_primary_key) {
    key_schema_ = std::make_shared<Schema>(Schema::CopySchema(tuple_schema, key_attrs_));
  }

  ~IndexMetadata() = default;

  /** @return The name of the index */
  inline auto GetName() const -> const std::string & { return name_; }

  /** @return The name of the table on which the index is created */
  inline auto GetTableName() -> const std::string & { return table_name_; }

  /** @return A schema object pointer that represents the indexed key */
  inline auto GetKeySchema() const -> Schema * { return key_schema_.get(); }

  /**
   * @return The number of columns inside index key (not in tuple key)
   *
   * NOTE: this must be defined inside the cpp source file because it
   * uses the member of catalog::Schema which is not known here.
   */
  auto GetIndexColumnCount() const -> std::uint32_t { return static_cast<uint32_t>(key_attrs_.size()); }

  /** @return The mapping relation between indexed columns and base table columns */
  inline auto GetKeyAttrs() const -> const std::vector<uint32_t> & { return key_attrs_; }

  /** @return is primary key */
  inline auto IsPrimaryKey() const -> bool { return is_primary_key_; }

  /** @return A string representation for debugging */
  auto ToString() const -> std::string {
    std::stringstream os;

    os << "IndexMetadata["
       << "Name = " << name_ << ", "
       << "Type = B+Tree, "
       << "Table name = " << table_name_ << "] :: ";
    os << key_schema_->ToString();

    return os.str();
  }

 private:
  /** The name of the index */
  std::string name_;
  /** The name of the table on which the index is created */
  std::string table_name_;
  /** The mapping relation between key schema and tuple schema */
  const std::vector<uint32_t> key_attrs_;
  /** The schema of the indexed key */
  std::shared_ptr<Schema> key_schema_;
  /** Is primary key? */
  bool is_primary_key_;
};

/////////////////////////////////////////////////////////////////////
// Index class definition
/////////////////////////////////////////////////////////////////////

/**
 * class Index - Base class for derived indices of different types
 *
 * The index structure majorly maintains information on the schema of the
 * underlying table and the mapping relation between index key
 * and tuple key, and provides an abstracted way for the external world to
 * interact with the underlying index implementation without exposing
 * the actual implementation's interface.
 *
 * Index object also handles predicate scan, in addition to simple insert,
 * delete, predicate insert, point query, and full index scan. Predicate scan
 * only supports conjunction, and may or may not be optimized depending on
 * the type of expressions inside the predicate.
 */
class Index {
 public:
  /**
   * Construct a new Index instance.
   * @param metadata An owning pointer to the index metadata
   */
  explicit Index(std::unique_ptr<IndexMetadata> &&metadata) : metadata_{std::move(metadata)} {}

  virtual ~Index() = default;

  /** @return A non-owning pointer to the metadata object associated with the index */
  auto GetMetadata() const -> IndexMetadata * { return metadata_.get(); }

  /** @return The number of indexed columns */
  auto GetIndexColumnCount() const -> std::uint32_t { return metadata_->GetIndexColumnCount(); }

  /** @return The index name */
  auto GetName() const -> const std::string & { return metadata_->GetName(); }

  /** @return The index key schema */
  auto GetKeySchema() const -> Schema * { return metadata_->GetKeySchema(); }

  /** @return The index key attributes */
  auto GetKeyAttrs() const -> const std::vector<uint32_t> & { return metadata_->GetKeyAttrs(); }

  /** @return A string representation for debugging */
  auto ToString() const -> std::string {
    std::stringstream os;
    os << "INDEX: (" << GetName() << ")";
    os << metadata_->ToString();
    return os.str();
  }

  ///////////////////////////////////////////////////////////////////
  // Point Modification
  ///////////////////////////////////////////////////////////////////

  /**
   * Insert an entry into the index.
   * @param key The index key
   * @param rid The RID associated with the key
   * @param transaction The transaction context
   * @returns whether insertion is successful
   */
  virtual auto InsertEntry(const Tuple &key, RID rid, Transaction *transaction) -> bool = 0;

  /**
   * Delete an index entry by key.
   * @param key The index key
   * @param rid The RID associated with the key (unused)
   * @param transaction The transaction context
   */
  virtual void DeleteEntry(const Tuple &key, RID rid, Transaction *transaction) = 0;

  /**
   * Search the index for the provided key.
   * @param key The index key
   * @param result The collection of RIDs that is populated with results of the search
   * @param transaction The transaction context
   */
  virtual void ScanKey(const Tuple &key, std::vector<RID> *result, Transaction *transaction) = 0;

 private:
  /** The Index structure owns its metadata */
  std::unique_ptr<IndexMetadata> metadata_;
};

}  // namespace bustub
