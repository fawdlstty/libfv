# libfv

English | [简体中文](./README.zh.md)

libfv is C++20 header-only network library, support TCP/SSL/Http/websocket server and client

You can use pure asynchronous networking with it, or you can use asynchronous wrappers without networking at all and support asynchronous development in your projects.

In addition to providing network functions, the library also provides a variety of asynchronous tools, such as timers, semaphores, etc.

## Started

1. Install and config `vcpkg` environment
	```
	vcpkg install asio fmt gzip-hpp nlohmann-json openssl zlib
	# `asio` can be changed to `boost-asio`
	```
2. Initialize
	```cpp
	// Import header file (Macro need to be defined if use standalone asio)
	#define ASIO_STANDALONE
	#include <fv/fv.hpp>

	// Main function
	int main () {
		// Global initialize
		fv::Tasks::Start (true);

		// ...

		// Global release
		fv::Tasks::Stop ();
		return 0;
	}
	```
3. When an asynchronous function has called, it is added to the task pool with 'fv::Tasks::RunAsync'
	```cpp
	// Asynchronous function
	Task<void> async_func () {
		fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_kv ("a", "aaa"));
		std::cout << _r.Content;
	}
	// Asynchronous function with parameter
	Task<void> async_func2 (int n) {
		std::cout << n << std::endl;
		co_return;
	}

	// To execute asynchronous functions, you can transfer std::function<Task<void> ()> type
	fv::Tasks::RunAsync (async_func);
	fv::Tasks::RunAsync (async_func2, 5);
	```

Now that we have created the asynchronous function environment, we can write asynchronous code in it.

## Manual

### Global configuration Settings

```cpp
// Set SSL verification function (default no verification)
fv::Config::SslVerifyFunc = [] (bool preverified, fv::Ssl::verify_context &ctx) { return true; };

// Setting global TCP transmission without delay (Used in scenarios requiring high real-time performance)
fv::Config::NoDelay = true;

// Setting the global HTTP header (client)
fv::Request::SetDefaultHeader ("User-Agent", "libfv-0.0.1");
```

### HTTP(S) Client

A total of six HTTP requests are supported. You can use `fv::Head`、`fv::Option`、`fv::Get`、`fv::Post`、`fv::Put`、`fv::Delete` methods.

```cpp
// Send HttpGet request
fv::Response _r = co_await fv::Get ("https://t.cn");

// Send HttpPost request with `application/json` content type (default)
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_kv ("a", "aaa"));

// Send HttpPost request with `application/x-www-form-urlencoded` content type
fv::Response _r2 = co_await fv::Post ("https://t.cn", fv::body_kv ("a", "aaa"), fv::content_type ("application/x-www-form-urlencoded"));

// Commit file
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_file ("a", "filename.txt", "content..."));

// Send HttpPost request with raw data
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_raw ("application/octet-stream", "aaa"));

// Specifies the request timeout period
fv::Response _r = co_await fv::Get ("https://t.cn", fv::timeout (std::chrono::seconds (10)));

// Send request to the specified server
fv::Response _r = co_await fv::Get ("https://t.cn", fv::server ("106.75.237.200"));

// Custom http header
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

### Websocket Client

```cpp
// Create connect
std::shared_ptr<fv::WsConn> _conn = co_await fv::ConnectWS ("wss://t.cn/ws");

// Loop to receive content and print (throw exception means link is broken)
// It can receive three types, `fv::WsType::Text`, `fv::WsType::Binary`, `fv::WsType::Pong`
while (true) {
	auto [_data, _type] = co_await _conn->Recv ();
	std::cout << _data << std::endl;
}

// Send data (throw exception means link is broken)
std::string _str = "hello";
co_await _conn->SendText (_str.data (), _str.size ());
co_await _conn->SendBinary (_str.data (), _str.size ());
co_await _conn->SendPing ();

// Close connect
co_await _conn->Close ();
```

### TCP/SSL Client

```cpp
// Create connect (SSL link can be changed to: ssl://127.0.0.1:1234)
std::shared_ptr<fv::IConn> _conn = co_await fv::Connect ("tcp://127.0.0.1:1234");

// Receive content and print (throw exception means link is broken)
// Note: TCP and SSL are streaming protocols, so the length of a single packet cannot be accurately obtained. Please specify the length in a customized format
// Note: ReadCount and ReadCountVec will not return until specified length data has been received
char _ch = co_await _conn->ReadChar ();
std::string _line = co_await _conn->ReadLine ();
std::string _buf = co_await _conn->ReadCount (1024);
std::vector<uint8_t> _buf2 = co_await _conn->ReadCountVec (1024);

// Send data (throw exception means link is broken)
std::string _str = "hello";
co_await _conn->Send (_str.data (), _str.size ());

// Close connect
co_await _conn->Close ();
```

### HTTP Server (Websocket Server)

```cpp
// Server object
fv::HttpServer _server {};

// Specifies the http request handling callback
_server.SetHttpHandler ("/hello", [] (fv::Request &_req) -> Task<fv::Response> {
	co_return fv::Response::FromText ("hello world");
});

// Specifies the websocket request handling callback
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

// Set before-request filtering (optional)
_server.OnBefore ([] (fv::Request &_req) -> std::optional<Task<fv::Response>> {
	// Return std::nullopt to indicate passing, otherwise intercept and return the currently returned result (Do not enter SetHttpHandler handling callback)
	co_return std::nullopt;
});

// Set after-request filtering (optional)
_server.OnAfter ([] (fv::Request &_req, fv::Response &_res) -> Task<void> {
	// Here you can do something with the returned content
	co_return std::nullopt;
});

// Set unhandled request handling callback (optional)
_server.OnUnhandled ([] (fv::Request &_req) -> Task<fv::Response> {
	co_return fv::Response::FromText ("not handled!");
});

// Start listen
co_await _server.Run (8080);
```

### TCP Server

```cpp
// Server object
TcpServer m_tcpserver {};

// Sets the new link handler function
m_tcpserver.SetOnConnect ([&m_tcpserver] (std::shared_ptr<IConn> _conn) -> Task<void> {
	// Free play here, return then link close, usually `while (true)`

	// You can register client or unregister, specify the ID based on your profession
	m_tcpserver.RegisterClient (123, _conn);
	m_tcpserver.UnregisterClient (123, _conn);
});

// If the handler function has registered internally, the external can send or broadcast messages directly to the corresponding client
std::string _data = "hello";
co_await m_tcpserver.SendData (123, _data.data (), _data.size ());
co_await m_tcpserver.BroadcastData (_data.data (), _data.size ());

// Start listen
co_await m_tcpserver.Run (2233);
```

## Another asynchronous functionality

```cpp
// Sleep 10 seconds
co_await fv::Tasks::Delay (std::chrono::seconds (10));

// If external needs io_context
boost::asio::io_context &_ctx = fv::Tasks::GetContext ();

// Synchronization within asynchronous methods
// Create semaphore
AsyncSemaphore _sema { 1 };
// Attempt to acquire signal
bool _success = _sema.TryAcquire ();
// Asynchronous wait signal
co_await _sema.AcquireAsync ();
 // Timeout asynchronously waits for a signal
bool _success = co_await _sema.AcquireForAsync (std::chrono::seconds (10));
bool _success = co_await _sema.AcquireForAsync (std::chrono::system_clock::now () + std::chrono::seconds (10));
// Release signal
_sema.Release ();
```

## Roadmap

- [x] HTTP(S) Server/Client
- [x] TCP Server/Client
- [x] SSL Client
- [x] Websocket Server/Client
- [ ] UDP Server/Client
- [ ] Cancellation
- [ ] Dns Cache
