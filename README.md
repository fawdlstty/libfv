# libfv

English | [简体中文](./README.zh.md)

A fast C++20 async http library based on boost.asio.

Example:

```cpp
// include headers
#include <fv/fv.hpp>

// global init
Tasks::Start (true);

// HttpGet
fv::Response _r = co_await fv::Get ("http://www.fawdlstty.com");
std::cout << _r.Content;

// global release
Tasks::Stop ();
```

Roadmap:

- [x] TCP
- [x] HttpGet
- [ ] HttpPost
- [ ] UDP
- [ ] Websocket
- [ ] SSL
