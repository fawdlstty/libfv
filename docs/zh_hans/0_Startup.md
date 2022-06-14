# 开始使用

## 配置 `vcpkg` 环境

首先通过 `vcpkg` 安装 `libfv`

```
vcpkg install fawdlstty-libfv
```

## 初始化

```cpp
// 引入库头文件（默认依赖独立asio，改用boost::asio则需定义宏）
//#define FV_USE_BOOST_ASIO
#include <fv/fv.hpp>

// 主函数
int main () {
	// 全局初始化（参数可指定外部 asio::io_context 指针）
	fv::Tasks::Init ();

	// ...

	// 循环处理任务（其他地方调用 `fv::Tasks::Stop ()` 可退出）
	fv::Tasks::LoopRun ();

	// 全局释放
	fv::Tasks::Release ();
	return 0;
}
```

## 进入异步

调用异步函数时，通过 `fv::Tasks::RunAsync` 将其加入异步任务队列

```cpp
// 异步函数
Task<void> async_func () {
	fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_kv ("a", "aaa"));
	std::cout << _r.Content;
}
// 带参异步函数
Task<void> async_func2 (int n) {
	std::cout << n << std::endl;
	co_return;
}

// 执行异步函数，可以把 std::function<Task<void> ()> 类型方法塞进去
fv::Tasks::RunAsync (async_func);
fv::Tasks::RunAsync (async_func2, 5);
```

## 全局配置设置

```cpp
// 设置SSL校验函数（默认不校验）
fv::Config::SslVerifyFunc = [] (bool preverified, fv::Ssl::verify_context &ctx) { return true; };

// 设置全局TCP不延迟发送（对实时性要求较高场合使用）
fv::Config::NoDelay = true;

// 设置全局 http 头
fv::Request::SetDefaultHeader ("User-Agent", "libfv-0.0.1");

// 设置全局 Websocket 自动 ping 时间间隔
fv::Config::WebsocketAutoPing = std::chrono::minutes (1);

// 设置连接池自动释放超时时长
fv::Config::SessionPoolTimeout = std::chrono::minutes (1);

// 设置 SSL 版本
fv::Config::SslClientVer = fv::Ssl::context::tls;
fv::Config::SslServerVer = fv::Ssl::context::tls;

// 设置 DNS 查询函数
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

// 设置本地客户端IP绑定查询函数
fv::Config::BindClientIP = [] () -> Task<std::string> {
	std::string _ip = co_await fv::Config::DnsResolve (asio::ip::host_name ());
	co_return _ip;
};
```
