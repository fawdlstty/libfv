# libfv

[English](./README.md) | ��������

���� boost.asio �� C++ �첽 HTTP �⡣

ʾ����

```cpp
#include <fv/fv.hpp>

// ...

fv::Response _r = co_await fv::Get ("http://www.fawdlstty.com");
std::cout << _r.Content;
```

�ƻ���

- [x] HttpGet
- [ ] HttpPost
- [ ] TCP
- [ ] UDP
- [ ] Websocket
- [ ] SSL
