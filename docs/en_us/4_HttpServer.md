# HTTP Server

**Note: HTTPS server is not supported now**

## Create a server object

```cpp
fv::HttpServer _server {};
```

## Specify HTTP and Websocket request processing callbacks

```cpp
_server.SetHttpHandler ("/hello", [] (fv::Request &_req) -> Task<fv::Response> {
	co_return fv::Response::FromText ("hello world");
});

_server.SetHttpHandler ("/ws", [] (fv::Request &_req) -> Task<fv::Response> {
	// Check whether it is a websocket request
	if (_req.IsWebsocket ()) {
		// Upgrade to websocket connect
		auto _conn = co_await _req.UpgradeWebsocket ();
		while (true) {
			auto [_data, _type] = co_await _conn->Recv ();
			if (_type == fv::WsType::Text) {
				co_await _conn->SendText (_data.data (), _data.size ());
			} else if (_type == fv::WsType::Binary) {
				co_await _conn->SendBinary (_data.data (), _data.size ());
			}
		}
		// Return empty after websocket upgrade
		co_return fv::Response::Empty ();
	} else {
		co_return fv::Response::FromText ("please use websocket");
	}
});
```

## Set before-request filtering

```cpp
_server.OnBefore ([] (fv::Request &_req) -> std::optional<Task<fv::Response>> {
	// Return std::nullopt to indicate passing, otherwise intercept and return the currently returned result (Do not enter SetHttpHandler handling callback)
	co_return std::nullopt;
});
```

## Set after-request filtering

```cpp
_server.OnAfter ([] (fv::Request &_req, fv::Response &_res) -> Task<void> {
	// Here you can do something with the returned content
	co_return std::nullopt;
});
```

## Set unhandled request handling callback

```cpp
_server.OnUnhandled ([] (fv::Request &_req) -> Task<fv::Response> {
	co_return fv::Response::FromText ("not handled!");
});
```

## Start listen

```cpp
co_await _server.Run (8080);
```

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



Task<void> test_server () {
	fv::HttpServer _server {};
	_server.SetHttpHandler ("/hello", [] (fv::Request &_req) -> Task<fv::Response> {
		co_return fv::Response::FromText ("hello world");
	});
	std::cout << "You can access from browser: http://127.0.0.1:8080/hello\n";
	co_await _server.Run (8080);
}

int main () {
	fv::Tasks::Init ();
	fv::Tasks::RunMainAsync (test_server);
	fv::Tasks::Run ();
	return 0;
}
```
