# HTTP Client

## Launch request

A total of six HTTP requests are supported. You can use `fv::Head`、`fv::Option`、`fv::Get`、`fv::Post`、`fv::Put`、`fv::Delete` methods.

```cpp
// Send HttpGet request
fv::Response _r = co_await fv::Get ("https://t.cn");
```

You can specify a timeout period or target server address at request time:

```cpp
// specify a timeout period
fv::Response _r = co_await fv::Get ("https://t.cn", fv::timeout (std::chrono::seconds (10)));

// specify a target server address
fv::Response _r = co_await fv::Get ("https://t.cn", fv::server ("106.75.237.200"));
```

A POST or PUT request can be requested with parameters:

```cpp
// Send HttpPost request with `application/json` content type (default)
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_kv ("a", "aaa"));

// Send HttpPost request with `application/x-www-form-urlencoded` content type
fv::Response _r2 = co_await fv::Post ("https://t.cn", fv::body_kv ("a", "aaa"), fv::content_type ("application/x-www-form-urlencoded"));

// Commit file
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_file ("a", "filename.txt", "content..."));

// Send HttpPost request with Key-Value pair data
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_kvs ({{ "a", "b" }, { "c", "d" }}));

// Send HttpPost request with json data
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_json ("{\"a\":\"b\"}"));

// Send HttpPost request with raw data
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_raw ("application/octet-stream", "aaa"));
```

You can specify HTTP headers at request time:

```cpp
// Set custom http header
fv::Response _r = co_await fv::Get ("https://t.cn", fv::header ("X-WWW-Router", "123456789"));

// Set http header `Authorization` value with jwt bearer authorization
fv::Response _r = co_await fv::Get ("https://t.cn", fv::authorization ("Bearer XXXXXXXXXXXXX=="));

// Set http header `Authorization` value with username and password
fv::Response _r1 = co_await fv::Get ("https://t.cn", fv::authorization ("admin", "123456"));

// Set http header `Connection` value
fv::Response _r = co_await fv::Get ("https://t.cn", fv::connection ("keep-alive"));

// Set http header `Content-Type` value
fv::Response _r = co_await fv::Get ("https://t.cn", fv::content_type ("application/octet-stream"));

// Set http header `Referer` value
fv::Response _r = co_await fv::Get ("https://t.cn", fv::referer ("https://t.cn"));

// Set http header `User-Agent` value
fv::Response _r = co_await fv::Get ("https://t.cn", fv::user_agent ("Mozilla/4.0 Chrome 2333"));
```

## HTTP pipeline

Libfv will maintain a link pool internally, providing reuse of requests for the same service address (same schema, domain name, port) without manual intervention.

## Example

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
