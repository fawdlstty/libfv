# Home

Repository URL: <https://github.com/fawdlstty/libfv>

## Introduction

libfv is C++20 header-only network library, support TCP/SSL/Http/websocket server and client

## Why libfv

Compared with other network libraries, libfv's biggest advantage is that it supports pure asynchronous development mode. C++ is a very generational language for developers, mainly because C++20 introduced the asynchronous coroutine syntax, and the support for libraries has been slow to catch up, leaving asynchronous development options limited. libfv is one of the options of the new C++ asynchronous coroutine network development framework, which can make asynchronous coroutine development more pleasant from the library level.  

## Scene

The older HTTP libraries of C++ have two main implementations. The first is synchronous HTTP network access, such as code like this:

```cpp
// pseudocode
Response _r = HttpGet ("https://t.cn");
std::cout << _t.text;
```

Such code is simple to write, but there is a problem with it: HTTP network access is time-consuming, perhaps hundreds of milliseconds, so long that the thread will block here, consuming thread resources. If you need to initiate dozens or hundreds of requests at the same time, the system resources will be consumed. Obviously, it's not a good design.

The second is callback notification, such as code like this:

```cpp
// pseudocode
HttpGet ("https://t.cn", [] (Response _r) {
	std::cout << _t.text;
});
```

This way to solve the threading problem, that is, dozens or hundreds of requests can be launched at the same time, only a small amount or a thread on the line, HTTP library internal implementation of the request internal management, after receiving the response to the request, call the callback function, so as to achieve efficient processing of the request.  The problem with this approach is that if we need to forward the content of the request to the next request, this can cause a callback hell problem, such as code like this:

```cpp
// pseudocode
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

So, what are the improvements in libfv? Look at the code below:

```cpp
fv::Response _r = co_await fv::Get ("https://t.cn");
```

On the one hand it gets the benefit of the callback approach, where a small number of threads support a large number of requests at the same time, without the problem of callback hell. The above code is implemented via libfv and can be written as follows:

```cpp
fv::Response _r = co_await fv::Get ("https://t.cn");
_r = co_await fv::Get (_r.text);
_r = co_await fv::Get (_r.text);
_r = co_await fv::Get (_r.text);
_r = co_await fv::Get (_r.text);
std::cout << _t.text;
```

The code implementation seems to be simpler with synchronous requests, but because synchronization still causes a waste of thread computing power, synchronization is not optimal.

Request both interfaces at the same time and concatenate the results.  How was it handled before?  The code looks like this:

```cpp
// synchronize pseudocode
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


// callback pseudocode
std::string _ret1 = "", _ret2 = "";
bool _bret1 = false, _bret2 = false;
HttpGet ("https://t.cn/1", [] (std::string _r) {
    _ret1 = _r;
    _bret1 = true;
});
HttpGet ("https://t.cn/2", [] (std::string _r) {
    _ret2 = _r;
    _bret2 = true;
});
while (!_bret1 || !_bret2)
    std::this_thread::sleep_for (std::chrono::milliseconds (1));
std::string _ret = _ret1 + _ret2;
```

Use libfv:

```cpp
Task<std::string> _ret1 = fv::Get ("https://t.cn/1");
Task<std::string> _ret2 = fv::Get ("https://t.cn/2");
std::string _ret = (co_await _ret1) + (co_await _ret2);
```

Not only is the code much simpler, it also saves extra thread creation costs.

## License

MIT
