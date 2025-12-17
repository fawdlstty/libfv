# HTTP/HTTPS 服务器端

## 创建HTTP服务器端对象

```cpp
fv::HttpServer _server {};
```

## 创建HTTPS服务器端对象

```cpp
fv::HttpsServer _ssl_server {};
```

## 指定 HTTP 及 websocket 请求处理回调

```cpp
_server.SetHttpHandler ("/hello", [] (fv::Request &_req) -> Task<fv::Response> {
	co_return fv::Response::FromText ("hello world");
});

_server.SetHttpHandler ("/ws", [] (fv::Request &_req) -> Task<fv::Response> {
	// 检查是否为 websocket 请求
	if (_req.IsWebsocket ()) {
		// 升级为 websocket
		auto _conn = co_await _req.UpgradeWebsocket ();
		while (true) {
			auto [_data, _type] = co_await _conn->Recv ();
			if (_type == fv::WsType::Text) {
				co_await _conn->SendText (_data.data (), _data.size ());
			} else if (_type == fv::WsType::Binary) {
				co_await _conn->SendBinary (_data.data (), _data.size ());
			}
		}
		// 请求完成 websocket 升级后返回空即可
		co_return fv::Response::Empty ();
	} else {
		co_return fv::Response::FromText ("please use websocket");
	}
});
```

## 设置前置请求过滤

```cpp
_server.OnBefore ([] (fv::Request &_req) -> std::optional<Task<fv::Response>> {
	// 此处返回 std::nullopt 代表通过过滤，否则代表拦截请求（不进入 SetHttpHandler 处理回调）
	co_return std::nullopt;
});
```

## 设置后置请求过滤

```cpp
_server.OnAfter ([] (fv::Request &_req, fv::Response &_res) -> Task<void> {
	// 此处可对返回内容做一些处理
	co_return std::nullopt;
});
```

## 设置未 handle 的请求处理函数

```cpp
_server.OnUnhandled ([] (fv::Request &_req) -> Task<fv::Response> {
	co_return fv::Response::FromText ("not handled!");
});
```

## 开始监听并启动HTTP服务

```cpp
co_await _server.Run (8080);
```

## 配置SSL上下文并启动HTTPS服务

```cpp
// 创建并配置SSL上下文
fv::Ssl::context _ssl_ctx(fv::Ssl::context::tlsv12);
_ssl_ctx.set_options(
    fv::Ssl::context::default_workarounds |
    fv::Ssl::context::no_sslv2 |
    fv::Ssl::context::single_dh_use
);
_ssl_ctx.use_certificate_chain_file("server.crt");
_ssl_ctx.use_private_key_file("server.key", fv::Ssl::context::pem);

// 启动HTTPS服务器
co_await _ssl_server.Run(8443, _ssl_ctx);
```

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

Task<void> test_http_server () {
	fv::HttpServer _server {};
	_server.SetHttpHandler ("/hello", [] (fv::Request &_req) -> Task<fv::Response> {
		co_return fv::Response::FromText ("hello world");
	});
	std::cout << "You can access from browser: http://127.0.0.1:8080/hello\n";
	co_await _server.Run (8080);
}

Task<void> test_https_server () {
	fv::HttpsServer _ssl_server {};

	// 设置HTTP处理器
	_ssl_server.SetHttpHandler("/hello", [](fv::Request& _req) -> Task<fv::Response> {
		co_return fv::Response::FromText("Hello from HTTPS server!");
	});

	// 配置SSL上下文
	fv::Ssl::context _ssl_ctx(fv::Ssl::context::tlsv12);
	_ssl_ctx.set_options(
		fv::Ssl::context::default_workarounds |
		fv::Ssl::context::no_sslv2 |
		fv::Ssl::context::single_dh_use
	);
	// 注意：需要提供有效的证书和私钥文件
	_ssl_ctx.use_certificate_chain_file("server.crt");
	_ssl_ctx.use_private_key_file("server.key", fv::Ssl::context::pem);

	std::cout << "You can access from browser: https://127.0.0.1:8443/hello\n";
	co_await _ssl_server.Run(8443, _ssl_ctx);
}

int main () {
	fv::Tasks::Init ();
	fv::Tasks::RunMainAsync (test_http_server);
	// 或者运行HTTPS服务器
	// fv::Tasks::RunMainAsync (test_https_server);
	fv::Tasks::Run ();
	return 0;
}
```