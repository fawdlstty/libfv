# libfv

[English](./README.md) | 简体中文

基于 boost.asio 的 C++20 异步 HTTP 库。

示例：

```cpp
// 引入头文件
#include <fv/fv.hpp>

// 全局初始化
Tasks::Start (true);

// HttpGet
fv::Response _r = co_await fv::Get ("http://www.fawdlstty.com");
std::cout << _r.Content;

// 全局释放
Tasks::Stop ();
```

计划：

- [x] TCP
- [x] HttpGet
- [ ] HttpPost
- [ ] UDP
- [ ] Websocket
- [ ] SSL
