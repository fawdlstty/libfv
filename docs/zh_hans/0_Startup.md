# 开始使用

## 配置环境

### 配置项目环境

- Windows - VS2019
	1. 配置属性 - 常规 - C++语言标准，设置为最新（`/std:c++latest`）
	2. 配置属性 - C++ - 命令行，新增：`/await`
- Windows - VS2022
	1. 配置属性 - 常规 - C++语言标准，设置为最新（`/std:c++latest`）
	2. 配置属性 - C++ - 命令行，新增：`/await:strict`
- Linux - gcc11.2+
	1. TODO

### 配置 vcpkg 环境

通过 `vcpkg` 安装 `libfv`：

```
vcpkg install fawdlstty-libfv
```

或者，使用仓库最新代码：

```
vcpkg remove fawdlstty-libfv
vcpkg install asio fmt gzip-hpp nlohmann-json openssl zlib
git clone git@github.com:fawdlstty/libfv.git
```

## 初始化

```cpp
// 引入库头文件（默认依赖独立asio，改用boost::asio则需定义宏）
//#define FV_USE_BOOST_ASIO
#include <fv/fv.h>

// 主函数
int main () {
	// 全局初始化（参数可指线程数，核心多的建议CPU线程数-1）
	fv::Tasks::Init ();

	// ...

	// 循环处理任务（其他地方调用 `fv::Tasks::Stop ()` 可退出）
	fv::Tasks::Run ();
	return 0;
}
```

## 进入异步

调用异步函数时，通过 `fv::Tasks::RunAsync` 或 `fv::Tasks::RunMainAsync` 将其加入异步任务队列

其中不带Main的是线程池包装的任务池，带Main的是独立的单线程（调用Run的线程）包装的任务池。建议服务类别带Main，其他类别不带Main

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

// 执行异步函数
fv::Tasks::RunAsync (async_func);
fv::Tasks::RunMainAsync (async_func2, 5);
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
fv::Config::DnsResolve = [] (std::string _host) -> Task<std::string> {
	Tcp::resolver _resolver { Tasks::GetContext () };
	auto _it = co_await _resolver.async_resolve (_host, "", UseAwaitable);
	co_return _it.begin ()->endpoint ().address ().to_string ();
};

// 设置本地客户端IP绑定查询函数
fv::Config::BindClientIP = [] () -> Task<std::string> {
	std::string _ip = co_await fv::Config::DnsResolve (asio::ip::host_name ());
	co_return _ip;
};
```
