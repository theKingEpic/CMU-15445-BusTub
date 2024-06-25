#pragma once

#include <optional>
#include <shared_mutex>
#include <utility>

#include "primer/trie.h"

namespace bustub {

// This class is used to guard the value returned by the trie. It holds a reference to the root so
// that the reference to the value will not be invalidated.
// 这个类用于保护由trie返回的值。它保存对根的引用，因此
// 对值的引用不会无效。
template <class T>
class ValueGuard {
 public:
  ValueGuard(Trie root, const T &value) : root_(std::move(root)), value_(value) {}
  auto operator*() const -> const T & { return value_; }

 private:
  Trie root_;
  const T &value_;
};

// This class is a thread-safe wrapper around the Trie class. It provides a simple interface for
// accessing the trie. It should allow concurrent reads and a single write operation at the same
// time.
// 这个类是Trie类的一个线程安全包装。它为访问trie提供了一个简单的接口。它应该允许在同一时间并发读和一个写操作。
class TrieStore {
 public:
  // This function returns a ValueGuard object that holds a reference to the value in the trie. If
  // the key does not exist in the trie, it will return std::nullopt.
  // 该函数返回一个ValueGuard对象，该对象保存着对trie中值的引用。如果键不存在，它将返回std::nullopt。
  template <class T>
  auto Get(std::string_view key) -> std::optional<ValueGuard<T>>;

  // This function will insert the key-value pair into the trie. If the key already exists in the
  // trie, it will overwrite the value.
  // 这个函数将键值对插入到树中。如果键已经存在于// tree中，它将覆盖该值。
  template <class T>
  void Put(std::string_view key, T value);

  // This function will remove the key-value pair from the trie.
  // 该函数将从树中删除键值对
  void Remove(std::string_view key);

 private:
  // This mutex protects the root. Every time you want to access the trie root or modify it, you
  // will need to take this lock.
  // 这个互斥锁保护根节点。每次您想要访问或修改树的根目录时，您都需要使用这个锁。
  std::mutex root_lock_;

  // This mutex sequences all writes operations and allows only one write operation at a time.
  // 该互斥锁对所有写操作进行排序，每次只允许一个写操作。
  std::mutex write_lock_;

  // Stores the current root for the trie.
  // 存储当前根目录。
  Trie root_;
};

}  // namespace bustub
