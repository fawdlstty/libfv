# Startup

## Configure environment

### Configure project

- Windows - VS2019
	1. Configuration Properties - General - C++ Language Standard, configure to Preview (/std:c++latest)
	2. Configuration Properties - C++ - Command Line, add `/await`
- Windows - VS2022
	1. Configuration Properties - General - C++ Language Standard, configure to Preview (/std:c++latest)
	2. Configuration Properties - C++ - Command Line, add `/await:strict`
- Linux - gcc11.2+
	1. TODO

### Configure vcpkg

First install `libfv` through `vcpkg`:

```
vcpkg install fawdlstty-libfv
```

Or, use the warehouse's latest code:

```
vcpkg remove fawdlstty-libfv
vcpkg install asio fmt gzip-hpp nlohmann-json openssl zlib
git clone git@github.com:fawdlstty/libfv.git
```

## Initialize

```cpp
// Import header file (default dependency on asio, macro need to be defined if use boost::asio)
//#define FV_USE_BOOST_ASIO
#include <fv/fv.h>

// Main function
int main () {
	// Global initialize (you can specified thread number, specify CPU thread number - 1)
	fv::Tasks::Init ();

	// ...

	// Loop processing task (or quit when another code call `fv::Tasks::Stop ()`)
	fv::Tasks::Run ();
	return 0;
}
```

## Entry asynchronous

When an asynchronous function has called, it is added to the task pool with `fv::Tasks::RunAsync` or `fv::Tasks::RunMainAsync`

The one without Main is the task pool wrapped by the thread pool, and the one with Main is the task pool wrapped by a single thread (the thread that calls Run).  It is recommended that the service category carry Main and other categories do not carry Main.

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

// To execute asynchronous functions
fv::Tasks::RunAsync (async_func);
fv::Tasks::RunMainAsync (async_func2, 5);
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

// Set the connection pool automatic close interval
fv::Config::SessionPoolTimeout = std::chrono::minutes (1);

// Setting SSL version
fv::Config::SslClientVer = fv::Ssl::context::tls;
fv::Config::SslServerVer = fv::Ssl::context::tls;

// Setting DNS resolve function
fv::Config::DnsResolve = [] (std::string _host) -> Task<std::string> {
	Tcp::resolver _resolver { Tasks::GetContext () };
	auto _it = co_await _resolver.async_resolve (_host, "", UseAwaitable);
	co_return _it.begin ()->endpoint ().address ().to_string ();
};

// Set the local client IP binding query function
fv::Config::BindClientIP = [] () -> Task<std::string> {
	std::string _ip = co_await fv::Config::DnsResolve (asio::ip::host_name ());
	co_return _ip;
};
```
