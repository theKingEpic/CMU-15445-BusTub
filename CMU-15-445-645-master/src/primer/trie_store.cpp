#include "primer/trie_store.h"
#include "common/exception.h"

namespace bustub {
/*
在拥有可用于单线程环境的写时复制trie之后，为多线程环境实现并发键值存储。
对于原始的Trie类，每次修改Trie时，我们都需要获得新的根来访问新的内容。但是对于并发键值存储，put和delete方法没有返回值。
这要求您使用并发原语来同步读和写，以便在整个过程中不会丢失数据。
并发键值存储应该并发地为多个读取器和一个写入器提供服务。也就是说，当有人修改树时，仍然可以在旧根上执行读操作。
当有人正在读时，仍然可以执行写操作而不需要等待读操作。
同样，如果我们从这个trie中得到一个值的引用，那么无论我们如何修改这个trie，我们都应该能够访问它。
来自Trie的Get函数只返回一个指针。如果存储此值的trie节点已被删除，则指针将悬空。
因此，在TrieStore中，我们返回一个ValueGuard，该ValueGuard存储对该值的引用和对应于该trie结构根的TrieNode，
以便在存储ValueGuard时可以访问该值。
为了实现这一点，我们在trie_store.cpp中为您提供了TrieStore::Get的伪代码。
请仔细阅读并思考如何实现TrieStore::Put和TrieStore::Remove。
*/
template <class T>
auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<T>> {
  // Pseudocode:伪代码
  // (1) Take the root lock, get the root, and release the root lock. Don't look up the value in the
  //     trie while holding the root lock.
  // (2) Lookup the value in the trie.
  // (3) If the value is found, return a ValueGuard object that holds a reference to the value and the
  //     root. Otherwise, return std::nullopt.
  // throw NotImplementedException("TrieStore::Get is not implemented.");
  // (1)获取根锁，获取根，然后释放根锁。当持有根锁时，不要在tree中查找值。
  Trie root;
  {
    std::scoped_lock<std::mutex> lock(root_lock_);
    root = root_;
  }  // 在这个块结束时 root_lock_ 会被自动解锁
  // (2)查找树中的值。
  const T *value = root.Get<T>(key);
  // (3)如果找到该值，返回一个ValueGuard对象，该对象包含对该值和根的引用。否则，返回std::nullopt。
  if (value != nullptr) {
    ValueGuard<T> vg(root, *value);
    return std::optional<ValueGuard<T>>(vg);
  }
  return std::nullopt;
}

template <class T>
void TrieStore::Put(std::string_view key, T value) {
  // You will need to ensure there is only one writer at a time. Think of how you can achieve this.
  // The logic should be somehow similar to `TrieStore::Get`.
  // 你需要确保一次只有一个写入器。想想你如何做到这一点。
  // 逻辑应该类似于' TrieStore::Get '。
  // (1)使用unique_lock作为写锁
  // (2)获取根锁，获取根，然后释放根锁。当持有根锁时，不要在tree中查找值。
  // (3)插入。
  // (4)修改根
  // throw NotImplementedException("TrieStore::Put is not implemented.");
  std::unique_lock<std::mutex> write_lock(write_lock_);

  std::unique_ptr<Trie> root_ptr;
  {
    std::scoped_lock<std::mutex> lock(root_lock_);
    root_ptr = std::make_unique<Trie>(root_);
  }

  Trie new_root = root_ptr->Put(key, std::move(value));

  {
    std::scoped_lock<std::mutex> lock(root_lock_);
    root_ = new_root;
  }
}

void TrieStore::Remove(std::string_view key) {
  // You will need to ensure there is only one writer at a time. Think of how you can achieve this.
  // The logic should be somehow similar to `TrieStore::Get`.
  // 你需要确保一次只有一个写入器。想想你如何做到这一点。
  // 逻辑应该类似于' TrieStore::Get '。
  // throw NotImplementedException("TrieStore::Remove is not implemented.");
  std::unique_lock<std::mutex> write_lock(write_lock_);

  std::unique_ptr<Trie> root_ptr;
  {
    std::scoped_lock<std::mutex> lock(root_lock_);
    root_ptr = std::make_unique<Trie>(root_);
  }

  Trie new_root = root_ptr->Remove(key);

  {
    std::scoped_lock<std::mutex> lock(root_lock_);
    root_ = new_root;
  }
}

// Below are explicit instantiation of template functions.
// 下面是模板函数的显式实例化。

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<uint32_t>>;
template void TrieStore::Put(std::string_view key, uint32_t value);

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<std::string>>;
template void TrieStore::Put(std::string_view key, std::string value);

// If your solution cannot compile for non-copy tests,
// you can remove the below lines to get partial score.
//如果您的解决方案不能编译非复制测试，您可以删除以下行以获得部分分数。

using Integer = std::unique_ptr<uint32_t>;

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<Integer>>;
template void TrieStore::Put(std::string_view key, Integer value);

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<MoveBlocked>>;
template void TrieStore::Put(std::string_view key, MoveBlocked value);

}  // namespace bustub
