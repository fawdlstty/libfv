# HTTP 客户端

## 发起请求

共支持6种HTTP请求，可使用 `fv::Head`、`fv::Option`、`fv::Get`、`fv::Post`、`fv::Put`、`fv::Delete` 方法。

```cpp
// 发送 HttpGet 请求
fv::Response _r = co_await fv::Get ("https://t.cn");
```

可在请求时指定超时时长或目标服务器地址：

```cpp
// 指定请求超时时长
fv::Response _r = co_await fv::Get ("https://t.cn", fv::timeout (std::chrono::seconds (10)));

// 向指定服务器发送请求
fv::Response _r = co_await fv::Get ("https://t.cn", fv::server ("106.75.237.200"));
```

对 POST 或 PUT 请求可在请求时附带参数：

```cpp
// 发送 HttpPost 请求，提交 application/json 格式（默认）
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_kv ("a", "aaa"));

// 发送 HttpPost 请求，提交 application/x-www-form-urlencoded 格式
fv::Response _r2 = co_await fv::Post ("https://t.cn", fv::body_kv ("a", "aaa"), fv::content_type ("application/x-www-form-urlencoded"));

// 提交文件
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_file ("a", "filename.txt", "content..."));

// 发送 HttpPost 请求并提交 Key-Value Pair 内容
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_kvs ({{ "a", "b" }, { "c", "d" }}));

// 发送 HttpPost 请求并提交 json 内容
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_json ("{\"a\":\"b\"}"));

// 发送 HttpPost 请求并提交原始内容
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_raw ("application/octet-stream", "aaa"));

```

可在请求时指定 HTTP 头：

```cpp
// 自定义http头
fv::Response _r = co_await fv::Get ("https://t.cn", fv::header ("X-WWW-Router", "123456789"));

// 设置http头 `Authorization` 值之 jwt bearer 鉴权
fv::Response _r = co_await fv::Get ("https://t.cn", fv::authorization ("Bearer XXXXXXXXXXXXX=="));

// 设置http头 `Authorization` 值之用户名密码鉴权
fv::Response _r1 = co_await fv::Get ("https://t.cn", fv::authorization ("admin", "123456"));

// 设置http头 `Connection` 值
fv::Response _r = co_await fv::Get ("https://t.cn", fv::connection ("keep-alive"));

// 设置http头 `Content-Type` 值
fv::Response _r = co_await fv::Get ("https://t.cn", fv::content_type ("application/octet-stream"));

// 设置http头 `Referer` 值
fv::Response _r = co_await fv::Get ("https://t.cn", fv::referer ("https://t.cn"));

// 设置http头 `User-Agent` 值
fv::Response _r = co_await fv::Get ("https://t.cn", fv::user_agent ("Mozilla/4.0 Chrome 2333"));
```

## HTTP pipeline

libfv内部将维护一个链接池，提供对相同服务地址（协议、域名、端口均相同）的请求的复用，无需手工干预

## 示例

```cpp
#ifdef _MSC_VER
#   define _WIN32_WINNT 0x0601
#   pragma warning (disable: 4068)
#   pragma comment (lib, "Crypt32.lib")
//#	ifdef _RESUMABLE_FUNCTIONS_SUPPORTED
//#		undef _RESUMABLE_FUNCTIONS_SUPPORTED
//#	endif
//#	ifndef __cpp_lib_coroutine
//#		define __cpp_lib_coroutine
//#	endif
#endif

#include <iostream>
#include <string>
#include <fv/fv.h>



Task<void> test_client () {
	fv::Response _r = co_await fv::Get ("https://www.fawdlstty.com");
	std::cout << "received content length: " << _r.Content.size () << '\n';
	std::cout << "press any key to exit\n";
    char ch;
    std::cin >> ch;
    fv::Tasks::Stop ();
}

int main () {
	fv::Tasks::Init ();
	fv::Tasks::RunAsync (test_client);
	fv::Tasks::Run ();
	return 0;
}
```
