# Startup

## Configure vcpkg

libfv depends on the VCPKG environment. Therefore, you need to configure it before using libfv.

```
# Installing dependency packages
vcpkg install asio fmt gzip-hpp nlohmann-json openssl zlib
# `asio` can be changed to `boost-asio`
```

## Configure libfv

First, clone the libfv repository

```
git clone git@github.com:fawdlstty/libfv.git
```

Then add the include folder in the repository to your project's header search path. If this step does not work, copy the `include/fv` folder from the repository into your project and reference it later via `#include "fv/fv.hpp"`.

## Initialize

```cpp
// Import header file (Macro need to be defined if use standalone asio)
#define ASIO_STANDALONE
#include <fv/fv.hpp>

// Main function
int main () {
	// Global initialize (you can specified the pointer of asio::io_context)
	fv::Tasks::Init ();

	// ...

	// Loop processing task (or quit when another code call `fv::Tasks::Stop ()`)
	fv::Tasks::LoopRun ();

	// Global release
	fv::Tasks::Release ();
	return 0;
}
```

## Entry asynchronous

When an asynchronous function has called, it is added to the task pool with `fv::Tasks::RunAsync`

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

## Global configuration Settings

```cpp
// Set SSL verification function (default no verification)
fv::Config::SslVerifyFunc = [] (bool preverified, fv::Ssl::verify_context &ctx) { return true; };

// Setting global TCP transmission without delay (Used in scenarios requiring high real-time performance)
fv::Config::NoDelay = true;

// Setting the global HTTP header (client)
fv::Request::SetDefaultHeader ("User-Agent", "libfv-0.0.1");

// Setting the global Websocket ping interval
fv::Config::WebsocketAutoPing = std::chrono::minutes (1);

// Setting SSL version
fv::Config::SslClientVer = fv::Ssl::context::tls;
fv::Config::SslServerVer = fv::Ssl::context::tls;

// Setting DNS resolve function
fv::Config::DnsResolve = [] (std::string _host) -> Task<std::vector<std::string>> {
	std::vector<std::string> _v;
	try {
		fv::Tcp::resolver _resolver { fv::Tasks::GetContext () };
		auto _it = co_await _resolver.async_resolve (_host, "", fv::seAwaitable);
		for (auto _i : _it)
			_v.push_back (_i.endpoint ().address ().to_string ());
	} catch (...) {
	}
	co_return _v;
};

// Set the local client IP binding query function
fv::Config::BindClientIP = [] () -> Task<std::string> {
	std::string _ip = co_await fv::Config::DnsResolve (asio::ip::host_name ());
	co_return _ip;
};
```
