#include "primer/trie.h"
#include <memory>
#include <stack>
#include <string_view>
#include <utility>
#include "common/exception.h"

namespace bustub {

template <class T>
auto Trie::Get(std::string_view key) const -> const T * {
  // You should walk through the trie to find the node corresponding to the key. If the node doesn't exist, return
  // nullptr. After you find the node, you should use `dynamic_cast` to cast it to `const TrieNodeWithValue<T> *`. If
  // dynamic_cast returns `nullptr`, it means the type of the value is mismatched, and you should return nullptr.
  // Otherwise, return the value.
  // 你应该遍历字典树以找到与键对应的节点。如果节点不存在，返回
  // nullptr。找到节点后，你应该使用 `dynamic_cast` 将其转换为 `const TrieNodeWithValue<T>*`。如果
  // dynamic_cast 返回 `nullptr`，这意味着值的类型不匹配，你应该返回 nullptr。
  // 否则，返回值。
  std::shared_ptr<const TrieNode> cur = root_;
  if (!cur) {
    return nullptr;
  }
  for (char c : key) {  // 遍历字符串key
    auto it = cur->children_.find(c);
    if (it != cur->children_.end()) {  // 存在
      cur = it->second;
    } else {
      return nullptr;
    }
  }
  const auto *trie_node_with_value = dynamic_cast<const TrieNodeWithValue<T> *>(cur.get());
  return (trie_node_with_value != nullptr) ? trie_node_with_value->value_.get() : nullptr;
}

template <class T>
auto CreateNodeWithValueAndCopyChild(std::shared_ptr<T> &value_ptr, std::shared_ptr<const TrieNode> &old_node_const)
    -> std::shared_ptr<bustub::TrieNodeWithValue<T>> {
  auto node = std::make_shared<TrieNodeWithValue<T>>(value_ptr);
  auto old_node = old_node_const ? old_node_const : nullptr;
  node->children_ = old_node ? old_node->children_ : std::map<char, std::shared_ptr<const TrieNode>>();
  return node;
}

template <class T>
auto Trie::Put(std::string_view key, T value) const -> Trie {
  // Note that `T` might be a non-copyable type. Always use `std::move` when creating `shared_ptr` on that value.
  // You should walk through the trie and create new nodes if necessary. If the node corresponding to the key already
  // exists, you should create a new `TrieNodeWithValue`.
  // 你应该遍历字典树，并在必要时创建新节点。（Clone后可以修改）
  // 如果对应于键的节点已经存在，你应该创建一个新的 `TrieNodeWithValue`。 key可能为空
  // throw NotImplementedException("Trie::Remove is not implemented.");
  std::shared_ptr<const TrieNode> cur_old = root_ ? root_ : nullptr;
  std::shared_ptr<TrieNode> cur_new = root_ ? root_->Clone() : std::make_shared<TrieNode>();
  std::shared_ptr<TrieNode> head_node = std::make_shared<TrieNode>();
  head_node->children_['a'] = cur_new;
  std::shared_ptr<T> value_ptr = std::make_shared<T>(std::move(value));
  std::string_view key_head = key.substr(0, key.size() - 1);
  char key_back = key.empty() ? '\0' : key.back();

  /*遍历字典树 两层循环，外层遍历key :  c为当前层次的键字符 exist为是否找到c
   * exist=false
   * 内层遍历字典树 pair为被遍历键值对
   *   c非尾的情况
   *     1. pair->first!=c :
   *       cur_new的子emplace pair
   *     2. pair->first==c :
   *       exist=true cur_new的子Clone
   *    exist==true 无事发生
   *          ==false cur_new的子Clone
   *    迭代指针
   *    cur_old=exist?cur_old->children_[c]:nullptr
   *    cur_new=cur_new->children_[c]
   *    c为尾的情况:
   *     除了Create调用的不一样，其他照常，指针不必迭代
   * */
  // 实现时可以将key分为两部分，不含尾和含尾
  // 对空串的特殊处理:增加头结点，去根节点特殊化,如果是空串重新赋值
  // 有个更艺术的处理方式 key加个a，a+key，这样空值不用特判了

  for (auto &c : key_head) {
    bool exist = false;
    if (cur_old) {
      for (auto &pair : cur_old->children_) {
        if (pair.first == c) {
          exist = true;
          // cur_new->children_[c] = cur_old->children_[c]->Clone();
          cur_new->children_[c] = cur_old->children_.find(c)->second->Clone();
        } else {
          cur_new->children_.emplace(pair.first, pair.second);
        }
      }
    }
    if (!exist) {
      cur_new->children_[c] = std::make_shared<TrieNode>();
    }
    cur_old = exist ? cur_old->children_.find(c)->second : nullptr;
    cur_new = std::const_pointer_cast<TrieNode>(cur_new->children_[c]);
  }

  if (!key_back) {  // 处理空串
    std::shared_ptr<const TrieNode> const_cur_old = cur_old;
    cur_new = CreateNodeWithValueAndCopyChild<T>(value_ptr, const_cur_old);
    head_node->children_['a'] = cur_new;
  } else {
    if (cur_old) {
      bool exist = false;
      for (auto pair : cur_old->children_) {
        if (pair.first == key_back) {
          exist = true;
          cur_new->children_[key_back] = CreateNodeWithValueAndCopyChild(value_ptr, pair.second);
        } else {
          cur_new->children_.emplace(pair.first, pair.second);
        }
      }
      if (!exist) {
        cur_new->children_[key_back] = std::make_shared<TrieNodeWithValue<T>>(value_ptr);
      }
    } else {
      cur_new->children_[key_back] = std::make_shared<TrieNodeWithValue<T>>(value_ptr);
    }
  }

  Trie new_trie(head_node->children_['a']);
  return new_trie;
}

auto Trie::Remove(std::string_view key) const -> Trie {
  // throw NotImplementedException("Trie::Remove is not implemented.");

  // You should walk through the trie and remove nodes if necessary. If the node doesn't contain a value anymore,
  // you should convert it to `TrieNode`. If a node doesn't have children anymore, you should remove it.
  // 你应该遍历字典树，并在必要时删除节点。如果节点不再包含值，
  // 如果节点不再包含值，你应该将它转换为 `TrieNode`。
  // 如果一个节点不再有子节点，你应该删除它。
  // 如果键不存在，返回原树
  if (!root_) {
    return *this;
  }

  // 给根节点加个（父）头结点，键为'a'，key加个'a'，去根节点特殊化
  std::shared_ptr<TrieNode> cur_new = root_->Clone();
  std::shared_ptr<const TrieNode> cur_old = root_;
  std::shared_ptr<TrieNode> head = std::make_shared<TrieNode>();
  head->children_['a'] = cur_new;
  std::shared_ptr<TrieNode> head_father = std::make_shared<TrieNode>();
  head_father->children_['a'] = head;

  std::stack<std::pair<std::shared_ptr<TrieNode>, char>> father_nodes;
  father_nodes.emplace(head_father, 'a');
  father_nodes.emplace(head, 'a');

  for (auto &c : key) {
    if (cur_old->children_.count(c) == 0U) {
      return *this;
    }
    cur_new->children_[c] = cur_old->children_.find(c)->second->Clone();
    father_nodes.emplace(cur_new, c);
    cur_new = std::const_pointer_cast<TrieNode>(cur_new->children_[c]);
    cur_old = cur_old->children_.find(c)->second;
  }
  if (!cur_new->is_value_node_) {
    return *this;
  }
  // 找到目标，如果cur_new有子节点,创建TrieNode并复制cur_new->children_,赋值给栈顶的children_[second]
  // 如果cur_new没有子节点,移除栈顶的children_[second]，pop,循环，检测栈顶的children_[second]是否非值节点且children_为空，
  //   若是则移除子节点，且pop,继续循环,若否则break
  if (!cur_new->children_.empty()) {
    std::shared_ptr<TrieNode> node = std::make_shared<TrieNode>();
    node->children_ = cur_new->children_;
    father_nodes.top().first->children_[father_nodes.top().second] = node;
  } else {
    father_nodes.top().first->children_.erase(father_nodes.top().second);
    father_nodes.pop();
    while (father_nodes.size() > 1) {
      if (!father_nodes.top().first->children_[father_nodes.top().second]->is_value_node_ &&
          father_nodes.top().first->children_[father_nodes.top().second]->children_.empty()) {
        father_nodes.top().first->children_.erase(father_nodes.top().second);
        father_nodes.pop();
      } else {
        break;
      }
    }
  }
  Trie trie1(head->children_['a']);
  return trie1;
}

// Below are explicit instantiation of template functions.
//
// Generally people would write the implementation of template classes and functions in the header file. However, we
// separate the implementation into a .cpp file to make things clearer. In order to make the compiler know the
// implementation of the template functions, we need to explicitly instantiate them here, so that they can be picked
// up by the linker. 下面是模板函数的具体实例化。
// // 通常情况下，人们会将模板类和函数的实现写在头文件中。
// 然而，我们将实现分离到一个.cpp文件中，以使事情更加清晰。
// 为了让编译器知道模板函数的实现，我们需要在这里显式地实例化它们，
// 这样它们就可以被链接器捕获。

template auto Trie::Put(std::string_view key, uint32_t value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const uint32_t *;

template auto Trie::Put(std::string_view key, uint64_t value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const uint64_t *;

template auto Trie::Put(std::string_view key, std::string value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const std::string *;

// If your solution cannot compile for non-copy tests, you can remove the below lines to get partial score.

using Integer = std::unique_ptr<uint32_t>;

template auto Trie::Put(std::string_view key, Integer value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const Integer *;

template auto Trie::Put(std::string_view key, MoveBlocked value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const MoveBlocked *;

}  // namespace bustub