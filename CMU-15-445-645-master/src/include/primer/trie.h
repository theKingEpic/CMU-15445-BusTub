#pragma once

#include <algorithm>
#include <cstddef>
#include <future>  // NOLINT
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bustub {

/// A special type that will block the move constructor and move assignment operator. Used in TrieStore tests.
/// 一个特殊的类型，用于阻止移动构造函数和移动赋值运算符。在 TrieStore 的测试中使用。

class MoveBlocked {
 public:
  explicit MoveBlocked(std::future<int> wait) : wait_(std::move(wait)) {}

  MoveBlocked(const MoveBlocked &) = delete;
  MoveBlocked(MoveBlocked &&that) noexcept {
    if (!that.waited_) {
      that.wait_.get();
    }
    that.waited_ = waited_ = true;
  }

  auto operator=(const MoveBlocked &) -> MoveBlocked & = delete;
  auto operator=(MoveBlocked &&that) noexcept -> MoveBlocked & {
    if (!that.waited_) {
      that.wait_.get();
    }
    that.waited_ = waited_ = true;
    return *this;
  }

  bool waited_{false};
  std::future<int> wait_;
};

// A TrieNode is a node in a Trie.
class TrieNode {
 public:
  // Create a TrieNode with no children.
  TrieNode() = default;

  // Create a TrieNode with some children.
  explicit TrieNode(std::map<char, std::shared_ptr<const TrieNode>> children) : children_(std::move(children)) {}

  virtual ~TrieNode() = default;

  // Clone returns a copy of this TrieNode. If the TrieNode has a value, the value is copied. The return
  // type of this function is a unique_ptr to a TrieNode.
  //
  // You cannot use the copy constructor to clone the node because it doesn't know whether a `TrieNode`
  // contains a value or not.
  //
  // Note: if you want to convert `unique_ptr` into `shared_ptr`, you can use `std::shared_ptr<T>(std::move(ptr))`.
  [[nodiscard]] virtual auto Clone() const -> std::unique_ptr<TrieNode> {
    return std::make_unique<TrieNode>(children_);
  }

  // A map of children, where the key is the next character in the key, and the value is the next TrieNode.
  // You MUST store the children information in this structure. You are NOT allowed to remove the `const` from
  // the structure.
  // 一个子节点映射，其中键是关键字中的下一个字符，值是下一个 TrieNode。
  // 你必须在这个结构中存储子节点的信息。不允许从结构中移除 `const`。
  // TrieNode本身不能被修改，但键值对可以被修改
  std::map<char, std::shared_ptr<const TrieNode>> children_;

  // Indicates if the node is the terminal node.
  bool is_value_node_{false};

  // You can add additional fields and methods here except storing children. But in general, you don't need to add extra
  // fields to complete this project.
};

// A TrieNodeWithValue is a TrieNode that also has a value of type T associated with it.
// TrieNodeWithValue 是一个 TrieNode，它还与类型 T 相关联的值。
template <class T>
class TrieNodeWithValue : public TrieNode {
 public:
  // Create a trie node with no children and a value.
  // 创建一个没有子节点但带有值的 Trie 节点。
  explicit TrieNodeWithValue(std::shared_ptr<T> value) : value_(std::move(value)) { this->is_value_node_ = true; }

  // Create a trie node with children and a value.
  // 创建一个带有子节点和值的 Trie 节点。
  TrieNodeWithValue(std::map<char, std::shared_ptr<const TrieNode>> children, std::shared_ptr<T> value)
      : TrieNode(std::move(children)), value_(std::move(value)) {
    this->is_value_node_ = true;
  }

  // Override the Clone method to also clone the value.
  //
  // Note: if you want to convert `unique_ptr` into `shared_ptr`, you can use `std::shared_ptr<T>(std::move(ptr))`.
  [[nodiscard]] auto Clone() const -> std::unique_ptr<TrieNode> override {
    return std::make_unique<TrieNodeWithValue<T>>(children_, value_);
  }

  // The value associated with this trie node.
  std::shared_ptr<T> value_;
};

// A Trie is a data structure that maps strings to values of type T. All operations on a Trie should not
// modify the trie itself. It should reuse the existing nodes as much as possible, and create new nodes to
// represent the new trie.
//
// You are NOT allowed to remove any `const` in this project, or use `mutable` to bypass the const checks.
// Trie 是一种数据结构，将字符串映射到类型为 T 的值。对 Trie 的所有操作都不应修改 Trie 本身。
// 应尽可能重用现有节点，并创建新节点来表示新的 Trie。
//
// 在这个项目中，不允许移除任何 `const`，或使用 `mutable` 来绕过 const 检查。
class Trie {
 private:
  // The root of the trie.
  std::shared_ptr<const TrieNode> root_{nullptr};

  // Create a new trie with the given root.
  explicit Trie(std::shared_ptr<const TrieNode> root) : root_(std::move(root)) {}

 public:
  // Create an empty trie.
  Trie() = default;
  // Get the value associated with the given key.
  // 1. If the key is not in the trie, return nullptr.
  // 2. If the key is in the trie but the type is mismatched, return nullptr.
  // 3. Otherwise, return the value.
  // 获取与给定键关联的值。
  // 1. 如果键不在 Trie 中，返回 nullptr。
  // 2. 如果键在 Trie 中但类型不匹配，返回 nullptr。
  // 3. 否则，返回值。
  template <class T>
  auto Get(std::string_view key) const -> const T *;

  // Put a new key-value pair into the trie. If the key already exists, overwrite the value.
  // Returns the new trie.
  // 将新的键值对放入 Trie 中。如果键已经存在，覆盖值。
  // 返回新的 Trie。
  template <class T>
  auto Put(std::string_view key, T value) const -> Trie;

  // Remove the key from the trie. If the key does not exist, return the original trie.
  // Otherwise, returns the new trie.
  // 从字典树中删除键。如果键不存在，则返回原始字典树。
  // 否则，返回新的字典树。
  [[nodiscard]] auto Remove(std::string_view key) const -> Trie;

  // Get the root of the trie, should only be used in test cases.
  // 获取 Trie 的根，仅应在测试用例中使用。
  [[nodiscard]] auto GetRoot() const -> std::shared_ptr<const TrieNode> { return root_; }
};

}  // namespace bustub