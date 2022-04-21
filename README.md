# libfv

English | [简体中文](./README.zh.md)

A fast C++20 async http library based on boost.asio.

Example:

```cpp
// include headers
#include <fv/fv.hpp>

// init (once)
Tasks::Start (true);

// request
fv::Response _r = co_await fv::Get ("http://www.fawdlstty.com");
std::cout << _r.Content;

// release (once)
Tasks::Stop ();
```

Roadmap:

- [x] HttpGet
- [ ] HttpPost
- [ ] TCP
- [ ] UDP
- [ ] Websocket
- [ ] SSL
