// Copyright 2022 RisingLight Project Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/*
 * 这段代码定义了一个名为 `CheckOptions` 的类，该类位于 `bustub` 命名空间中。`CheckOptions` 类用于存储一组检查选项，
 * 这些选项用于测试执行器逻辑。这个类的主要功能是作为一个容器，用于存储和查询不同的检查选项。
以下是这段代码的详细分析：
1. `CheckOption` 枚举类型：这是一个枚举类，用于定义不同的检查选项。在这个例子中，有两个选项：
   - `ENABLE_NLJ_CHECK`：启用嵌套循环连接（Nested Loop Join）的检查。
   - `ENABLE_TOPN_CHECK`：启用 TOP-N 操作的检查。
2. `CheckOptions` 类：
   - `check_options_set_`：这是一个 `std::unordered_set`，用于存储 `CheckOption` 类型的选项。
   `std::unordered_set`
是一个容器，它存储唯一的元素，并且允许快速查找、插入和删除。在这个上下文中，它用于存储启用的检查选项，
   以确保每个选项只被设置一次。
这个类没有提供公共的成员函数，可能是因为它被设计为通过其成员变量 `check_options_set_` 直接访问。
 这意味着使用者可以直接修改这个集合来添加或移除检查选项。
整体而言，`CheckOptions` 类是一个简单的容器，用于在测试执行器逻辑时配置和存储检查选项。
 它可以通过添加或删除 `CheckOption` 枚举的值来启用或禁用特定的检查功能。
*/
#include <memory>
#include <unordered_set>

#pragma once

namespace bustub {

enum class CheckOption : uint8_t {
  ENABLE_NLJ_CHECK = 0,
  ENABLE_TOPN_CHECK = 1,
};

/**
 * The CheckOptions class contains the set of check options used for testing
 * executor logic.
 */
class CheckOptions {
 public:
  std::unordered_set<CheckOption> check_options_set_;
};

};  // namespace bustub
