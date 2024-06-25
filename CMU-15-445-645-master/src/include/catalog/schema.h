//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// schema.h
//
// Identification: src/include/catalog/schema.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
/*  这段代码定义了BusTub数据库系统中用于描述表或索引列的结构的类和结构。具体内容如下：
    Schema 类：表示表或索引的列结构。
    SchemaRef：一个智能指针类型，用于引用 Schema 对象。
    CopySchema 函数：用于创建一个新 Schema 对象，它是通过复制指定的列和属性来实现的。
    GetColumns 方法：返回 Schema 对象中所有列的集合。
    GetColumn 方法：根据列的索引返回指定列的详细信息。
    GetColIdx 方法：根据列的名称返回列的索引，如果列不存在，则抛出异常。
    TryGetColIdx 方法：根据列的名称返回列的索引，如果列不存在，则返回 std::nullopt。
    GetUnlinedColumns 方法：返回未内联的列的索引集合。
    GetColumnCount 方法：返回 Schema 对象中列的总数。
    GetUnlinedColumnCount 方法：返回未内联的列的总数。
    GetLength 方法：返回一个元组所需的总字节数。
    IsInlined 方法：返回一个布尔值，指示所有列是否都是内联的。
    ToString 方法：返回 Schema 对象的字符串表示形式。
    fmt::formatter 特化：为 Schema 类和其指针类型提供了格式化支持，以便于打印和输出。
    这个文件是BusTub数据库系统中元数据管理的一部分，它定义了表和索引列的基本数据结构和操作。
    通过这些抽象类和结构，BusTub系统可以灵活地描述和管理表和索引的列。

    在数据库系统中，Schema 是一个关键概念，它描述了表的结构，包括列的名称、数据类型、大小和其他属性。Schema
   是数据库元数据的一部分，用于定义数据库中的数据如何组织和存储。 在BusTub数据库系统中，Schema
   类用于表示表或索引的列结构。它包含了以下关键信息： 列信息：每个表或索引的列都有一个唯一的名称和数据类型。Schema
   类存储了这些列的详细信息。 列顺序：Schema 类还记录了列的顺序，这对于解析和操作表中的数据非常重要。
    内联状态：某些数据库系统支持列的内联和分离存储。Schema 类可以指示哪些列是内联存储的，哪些列是分离存储的。
    元数据操作：Schema 类提供了操作列的接口，如获取列的名称、数据类型、索引等。
    打印和格式化：Schema 类还提供了将自身格式化为字符串的方法，这有助于调试和文档生成。*/
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "catalog/column.h"
#include "common/exception.h"
#include "type/type.h"

namespace bustub {

class Schema;
using SchemaRef = std::shared_ptr<const Schema>;

class Schema {
 public:
  /**
   * Constructs the schema corresponding to the vector of columns, read left-to-right.
   * @param columns columns that describe the schema's individual columns
   */
  explicit Schema(const std::vector<Column> &columns);  // explicit 防止隐式类型转换

  static auto CopySchema(const Schema *from, const std::vector<uint32_t> &attrs) -> Schema {
    std::vector<Column> cols;
    cols.reserve(attrs.size());
    for (const auto i : attrs) {
      cols.emplace_back(from->columns_[i]);
    }
    return Schema{cols};
  }

  /** @return all the columns in the schema */
  auto GetColumns() const -> const std::vector<Column> & { return columns_; }

  /**
   * Returns a specific column from the schema.
   * @param col_idx index of requested column
   * @return requested column
   */
  auto GetColumn(const uint32_t col_idx) const -> const Column & { return columns_[col_idx]; }

  /**
   * Looks up and returns the index of the first column in the schema with the specified name.
   * If multiple columns have the same name, the first such index is returned.
   * @param col_name name of column to look for
   * @return the index of a column with the given name, throws an exception if it does not exist
   */
  auto GetColIdx(const std::string &col_name) const -> uint32_t {
    if (auto col_idx = TryGetColIdx(col_name)) {
      return *col_idx;
    }
    UNREACHABLE("Column does not exist");
  }

  /**
   * Looks up and returns the index of the first column in the schema with the specified name.
   * If multiple columns have the same name, the first such index is returned.
   * @param col_name name of column to look for
   * @return the index of a column with the given name, `std::nullopt` if it does not exist
   */
  auto TryGetColIdx(const std::string &col_name) const -> std::optional<uint32_t> {
    for (uint32_t i = 0; i < columns_.size(); ++i) {
      if (columns_[i].GetName() == col_name) {
        return std::optional{i};
      }
    }
    return std::nullopt;
  }

  /** @return the indices of non-inlined columns */
  auto GetUnlinedColumns() const -> const std::vector<uint32_t> & { return uninlined_columns_; }

  /** @return the number of columns in the schema for the tuple */
  auto GetColumnCount() const -> uint32_t { return static_cast<uint32_t>(columns_.size()); }

  /** @return the number of non-inlined columns */
  auto GetUnlinedColumnCount() const -> uint32_t { return static_cast<uint32_t>(uninlined_columns_.size()); }

  /** @return the number of bytes used by one tuple */
  inline auto GetLength() const -> uint32_t { return length_; }

  /** @return true if all columns are inlined, false otherwise */
  inline auto IsInlined() const -> bool { return tuple_is_inlined_; }

  /** @return string representation of this schema */
  auto ToString(bool simplified = true) const -> std::string;

 private:
  /** Fixed-length column size, i.e. the number of bytes used by one tuple. */
  uint32_t length_;

  /** All the columns in the schema, inlined and uninlined. */
  std::vector<Column> columns_;

  /** True if all the columns are inlined, false otherwise. */
  /*在数据库管理系统中，元组（tuple）通常指的是表中的一行数据。在 Bustub 中，tuple_is_inlined_
是一个布尔成员变量，它用来表示一个元组是否是内联的。 当 tuple_is_inlined_ 为 true
时，这意味着元组的所有列都存储在同一个内存位置，没有被分散存储。在这种情况下，元组的数据是连续的，可以直接访问，不需要额外的指针或间接访问。
相反，当 tuple_is_inlined_ 为 false 时，元组的数据可能存储在不同的位置，可能需要通过指针或其他形式的间接访问来获取。
   这通常发生在元组包含大型数据类型（如文本或二进制数据）时，这些数据可能不适合或不可能内联存储在元组中。
简而言之，tuple_is_inlined_ 是一个标志，用于指示元组的存储方式是内联还是非内联，这影响到元组数据的访问效率和存储布局。*/
  bool tuple_is_inlined_{true};

  /** Indices of all uninlined columns. */
  std::vector<uint32_t> uninlined_columns_;
};

}  // namespace bustub

template <typename T>
struct fmt::formatter<T, std::enable_if_t<std::is_base_of<bustub::Schema, T>::value, char>>
    : fmt::formatter<std::string> {
  template <typename FormatCtx>
  auto format(const bustub::Schema &x, FormatCtx &ctx) const {
    return fmt::formatter<std::string>::format(x.ToString(), ctx);
  }
};

template <typename T>
struct fmt::formatter<std::shared_ptr<T>, std::enable_if_t<std::is_base_of<bustub::Schema, T>::value, char>>
    : fmt::formatter<std::string> {
  template <typename FormatCtx>
  auto format(const std::shared_ptr<T> &x, FormatCtx &ctx) const {
    if (x != nullptr) {
      return fmt::formatter<std::string>::format(x->ToString(), ctx);
    }
    return fmt::formatter<std::string>::format("", ctx);
  }
};

template <typename T>
struct fmt::formatter<std::unique_ptr<T>, std::enable_if_t<std::is_base_of<bustub::Schema, T>::value, char>>
    : fmt::formatter<std::string> {
  template <typename FormatCtx>
  auto format(const std::unique_ptr<T> &x, FormatCtx &ctx) const {
    if (x != nullptr) {
      return fmt::formatter<std::string>::format(x->ToString(), ctx);
    }
    return fmt::formatter<std::string>::format("", ctx);
  }
};
