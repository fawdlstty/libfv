# libfv

[English](./README.md) | ��������

���� boost.asio �� C++ �첽 HTTP �⡣

ʾ����

```cpp
// ����ͷ�ļ�
#include <fv/fv.hpp>

// ��ʼ��������һ�Σ�
Tasks::Start (true);

// ��������
fv::Response _r = co_await fv::Get ("http://www.fawdlstty.com");
std::cout << _r.Content;

// �ͷţ�����һ�Σ�
Tasks::Stop ();
```

�ƻ���

- [x] HttpGet
- [ ] HttpPost
- [ ] TCP
- [ ] UDP
- [ ] Websocket
- [ ] SSL
