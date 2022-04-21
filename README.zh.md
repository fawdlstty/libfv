# libfv

[English](./README.md) | 简体中文

基于 boost.asio 的 C++ 异步 HTTP 库。

示例：

```cpp
#include <fv/fv.hpp>

// ...

fv::Response _r = co_await fv::Get ("http://www.fawdlstty.com");
std::cout << _r.Content;
```

计划：

- [x] HttpGet
- [ ] HttpPost
- [ ] TCP
- [ ] UDP
- [ ] Websocket
- [ ] SSL
