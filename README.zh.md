# libfv

[English](./README.md) | 简体中文

libfv 是 C++20 纯头文件网络库，支持 TCP/SSL/Http/websocket 服务器端与客户端

你可以通过它使用纯异步的网络功能，当然你也能完全不使用网络，仅使用异步包装功能，让你的项目支持异步开发。

库除了提供网络功能外，还提供多种异步工具，比如定时器、信号量等。

技术交流：[点击链接加入群【1018390466】](https://jq.qq.com/?_wv=1027&k=7ZQLihbT)

## 文档

[简体中文文档](https://libfv.fawdlstty.com/zh_hans/)

[English Document](https://libfv.fawdlstty.com/en_us/)

[Github 在线文档](docs/)

## 项目描述

libfv 相对于其他网络库来说，最大的优势是支持纯异步的开发方式。C++是一个非常有年代感的语言，对于开发者来说，主要体现在，C++20出了异步协程语法后，支持的库迟迟没能跟上，使得异步开发选择范围很少。libfv 为新C++的异步协程网络开发框架的选项之一，可以从库的层面使得异步协程开发更为轻松愉快。

## 选择 libfv 的理由

C++ 较老的 HTTP 库有两种主要的实现方式，第一种是同步 HTTP 网络访问，比如这样的代码：

```cpp
// 伪代码
Response _r = HttpGet ("https://t.cn");
std::cout << _t.text;
```

这样的代码写起来很简单，但它存在一个问题：HTTP 网络访问比较耗时，可能需要几百毫秒，这么长时间，这个线程将阻塞在这里，比较消耗线程资源。假如遇到需要同时发起几十、几百个请求，将较大消耗系统资源。很显然，它不是一个较好的设计。

第二种是回调通知，比如这样的代码：

```cpp
// 伪代码
HttpGet ("https://t.cn", [] (Response _r) {
	std::cout << _t.text;
});
```

这种方式解决了线程问题，也就是，几十、几百个请求可以同时发起，只需要极少量或者一个线程就行，HTTP 库内部实现了请求的内部管理，在收到请求的回复后，调用回调函数，从而实现请求的高效处理。

但这种方式有个问题，假如我们需要根据请求结果内容转给下一个请求，这会带来一个回调地狱问题，比如这样的代码：

```cpp
// 伪代码
HttpGet ("https://t.cn", [] (Response _r) {
    HttpGet (_t.text, [] (Response _r) {
        HttpGet (_t.text, [] (Response _r) {
            HttpGet (_t.text, [] (Response _r) {
                HttpGet (_t.text, [] (Response _r) {
                    std::cout << _t.text;
                });
            });
        });
    });
});
```

那么，libfv 有哪些改进呢？看下面的代码

```cpp
fv::Response _r = co_await fv::Get ("https://t.cn");
```

一方面它能获得回调方式的好处，也就是少量线程支撑同时大量的请求任务，同时它不会带来回调地狱问题。上面的代码通过libfv实现，代码可以这样写：

```cpp
fv::Response _r = co_await fv::Get ("https://t.cn");
_r = co_await fv::Get (_r.text);
_r = co_await fv::Get (_r.text);
_r = co_await fv::Get (_r.text);
_r = co_await fv::Get (_r.text);
std::cout << _t.text;
```

同时请求两个接口，并将结果拼接。以前怎样处理？代码大概如下：

```cpp
// 同步伪代码
std::string _ret1, _ret2;
std::thread _t1 ([&_ret1] () {
    _ret1 = HttpGet ("https://t.cn/1");
});
std::thread _t2 ([&_ret2] () {
    _ret2 = HttpGet ("https://t.cn/2");
});
_t1.join ();
_t2.join ();
std::string _ret = _ret1 + _ret2;


// 回调伪代码
std::string _ret1 = "", _ret2 = "";
bool _bret1 = false, _bret2 = false;
HttpGet ("https://t.cn/1", [&_ret1, &_bret1] (std::string _r) {
    _ret1 = _r;
    _bret1 = true;
});
HttpGet ("https://t.cn/2", [&_ret2, &_bret2] (std::string _r) {
    _ret2 = _r;
    _bret2 = true;
});
while (!_bret1 || !_bret2)
    std::this_thread::sleep_for (std::chrono::milliseconds (1));
std::string _ret = _ret1 + _ret2;
```

libfv 处理方式：

```cpp
Task<std::string> _ret1 = fv::Get ("https://t.cn/1");
Task<std::string> _ret2 = fv::Get ("https://t.cn/2");
std::string _ret = (co_await _ret1) + (co_await _ret2);
```

不仅代码简单很多，还节省了额外的创建线程成本。

## License

MIT
