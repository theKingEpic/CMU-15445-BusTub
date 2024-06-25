#include "storage/page/page_guard.h"
#include "buffer/buffer_pool_manager.h"

namespace bustub {

// noexcept表示该函数不会抛出异常
// 新的守卫对象的行为与原来的守卫对象完全相同
BasicPageGuard::BasicPageGuard(BasicPageGuard &&that) noexcept {
  bpm_ = that.bpm_;
  page_ = that.page_;
  //将移动对象的资源保持为中立状态，防止其被析构时双重释放资源
  that.page_ = nullptr;
  that.bpm_ = nullptr;
  this->is_dirty_ = that.is_dirty_;
}

// 释放页面守卫应清除所有内容
void BasicPageGuard::Drop() {
  if (bpm_ != nullptr && page_ != nullptr) {
    bpm_->UnpinPage(page_->GetPageId(), is_dirty_);
  }
  bpm_ = nullptr;
  page_ = nullptr;
}

// 移动赋值函数
auto BasicPageGuard::operator=(BasicPageGuard &&that) noexcept -> BasicPageGuard & {
  // 释放this当前持有的资源
  Drop();
  bpm_ = that.bpm_;
  page_ = that.page_;
  that.page_ = nullptr;
  that.bpm_ = nullptr;
  this->is_dirty_ = that.is_dirty_;
  return *this;
}

BasicPageGuard::~BasicPageGuard() { Drop(); };  // NOLINT

auto BasicPageGuard::UpgradeRead() -> ReadPageGuard {
  if (page_ != nullptr) {
    page_->RLatch();
  }
  auto read_page_guard = ReadPageGuard(bpm_, page_);
  bpm_ = nullptr;
  page_ = nullptr;
  return read_page_guard;
}

auto BasicPageGuard::UpgradeWrite() -> WritePageGuard {
  if (page_ != nullptr) {
    page_->WLatch();
  }
  auto write_page_guard = WritePageGuard(bpm_, page_);
  bpm_ = nullptr;
  page_ = nullptr;
  return write_page_guard;
};  // NOLINT

// ReadPageGuard 的移动构造函数 和BasicPageGuard很像
ReadPageGuard::ReadPageGuard(ReadPageGuard &&that) noexcept { guard_ = std::move(that.guard_); }

auto ReadPageGuard::operator=(ReadPageGuard &&that) noexcept -> ReadPageGuard & {
  // Drop一定要加！要把里面的删掉，新的才能往里放
  Drop();
  guard_ = std::move(that.guard_);
  return *this;
}

void ReadPageGuard::Drop() {
  if (guard_.page_ != nullptr) {
    guard_.page_->RUnlatch();
  }
  guard_.Drop();
}

ReadPageGuard::~ReadPageGuard() { Drop(); }  // NOLINT

WritePageGuard::WritePageGuard(WritePageGuard &&that) noexcept { guard_ = std::move(that.guard_); }

auto WritePageGuard::operator=(WritePageGuard &&that) noexcept -> WritePageGuard & {
  Drop();
  guard_ = std::move(that.guard_);
  return *this;
}

void WritePageGuard::Drop() {
  if (guard_.page_ != nullptr) {
    guard_.page_->WUnlatch();
  }
  guard_.is_dirty_ = true;
  guard_.Drop();
}

WritePageGuard::~WritePageGuard() { Drop(); }  // NOLINT
}  // namespace bustub
