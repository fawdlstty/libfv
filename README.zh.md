# libfv

[English](./README.md) | 简体中文

基于 boost.asio 的 C++ 异步 HTTP 库。

示例：

```cpp
// 引入头文件
#include <fv/fv.hpp>

// 初始化（调用一次）
Tasks::Start (true);

// 发起请求
fv::Response _r = co_await fv::Get ("http://www.fawdlstty.com");
std::cout << _r.Content;

// 释放（调用一次）
Tasks::Stop ();
```

计划：

- [x] HttpGet
- [ ] HttpPost
- [ ] TCP
- [ ] UDP
- [ ] Websocket
- [ ] SSL
