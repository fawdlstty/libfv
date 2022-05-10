# libfv

[English](./README.md) | 简体中文

libfv 是 C++20 纯头文件网络库，支持 TCP/SSL/Http/websocket 服务器端与客户端

你可以通过它使用纯异步的网络功能，当然你也能完全不使用网络，仅使用异步包装功能，让你的项目支持异步开发。

库除了提供网络功能外，还提供多种异步工具，比如定时器、信号量等。

## 配置环境

1. 安装并配置 `vcpkg` 环境
	```
	vcpkg install boost-asio fmt gzip-hpp nlohmann-json openssl zlib
	# boost-asio 可换成 asio
	```
2. 初始化
	```cpp
	// 引入库头文件（使用独立asio则需定义宏）
	#define ASIO_STANDALONE
	#include <fv/fv.hpp>

	// 主函数
	int main () {
		// 全局初始化
		fv::Tasks::Start (true);

		// ...

		// 全局释放
		fv::Tasks::Stop ();
		return 0;
	}
	```
3. 调用异步函数时，通过 `fv::Tasks::RunAsync` 将其加入异步任务队列
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

现在我们已经创建好了异步函数环境，可以自由在里面编写异步代码啦！

## 使用手册

### 全局配置设置

```cpp
// 设置SSL校验函数（默认不校验）
fv::Config::SslVerifyFunc = [] (bool preverified, fv::Ssl::verify_context &ctx) { return true; };

// 设置全局TCP不延迟发送（对实时性要求较高场合使用）
fv::Config::NoDelay = true;

// 设置全局 http 头（客户端）
fv::Request::SetDefaultHeader ("User-Agent", "libfv-0.0.1");
```

### HTTP(S) Client

共支持6种HTTP请求，可使用 `fv::Head`、`fv::Option`、`fv::Get`、`fv::Post`、`fv::Put`、`fv::Delete` 方法。

```cpp
// 发送 HttpGet 请求
fv::Response _r = co_await fv::Get ("https://t.cn");

// 发送 HttpPost 请求，提交 application/json 格式（默认）
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_kv ("a", "aaa"));

// 发送 HttpPost 请求，提交 application/x-www-form-urlencoded 格式
fv::Response _r2 = co_await fv::Post ("https://t.cn", fv::body_kv ("a", "aaa"), fv::content_type ("application/x-www-form-urlencoded"));

// 提交文件
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_file ("a", "filename.txt", "content..."));

// 发送 HttpPost 请求并提交原始内容
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_raw ("application/octet-stream", "aaa"));

// 指定请求超时时长
fv::Response _r = co_await fv::Get ("https://t.cn", fv::timeout (std::chrono::seconds (10)));

// 向指定服务器发送请求
fv::Response _r = co_await fv::Get ("https://t.cn", fv::server ("106.75.237.200"));

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

### Websocket Client

```cpp
// 建立链接
std::shared_ptr<fv::WsConn> _conn = co_await fv::ConnectWS ("wss://t.cn/ws");

// 循环接收内容并打印（抛异常说明链接断开）
// 能接收到三种类型，`fv::WsType::Text`，`fv::WsType::Binary`，`fv::WsType::Pong`
while (true) {
	auto [_data, _type] = co_await _conn->Recv ();
	std::cout << _data << std::endl;
}

// 发送数据（抛异常说明链接断开）
std::string _str = "hello";
co_await _conn->SendText (_str.data (), _str.size ());
co_await _conn->SendBinary (_str.data (), _str.size ());
co_await _conn->SendPing ();

// 关闭链接
co_await _conn->Close ();
```

### TCP/SSL Client

```cpp
// 建立链接（ssl链接可换成：ssl://127.0.0.1:1234）
std::shared_ptr<fv::IConn> _conn = co_await fv::Connect ("tcp://127.0.0.1:1234");

// 接收内容并打印（抛异常说明链接断开）
// 备注：TCP与SSL均为流式协议，无法准确获取单个数据包长度，请自定格式指定长度信息
// 备注：ReadCount 与 ReadCountVec 必须待接收到那么长数据之后才会返回
char _ch = co_await _conn->ReadChar ();
std::string _line = co_await _conn->ReadLine ();
std::string _buf = co_await _conn->ReadCount (1024);
std::vector<uint8_t> _buf2 = co_await _conn->ReadCountVec (1024);

// 发送数据（抛异常说明链接断开）
std::string _str = "hello";
co_await _conn->Send (_str.data (), _str.size ());

// 关闭链接
co_await _conn->Close ();
```

### HTTP Server (Websocket Server)

```cpp
// 服务器对象
fv::HttpServer _server {};

// 指定 HTTP 请求处理回调
_server.SetHttpHandler ("/hello", [] (fv::Request &_req) -> Task<fv::Response> {
	co_return fv::Response::FromText ("hello world");
});

// 指定 websocket 请求处理回调
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

// 设置前置请求过滤（可不设置）
_server.OnBefore ([] (fv::Request &_req) -> std::optional<Task<fv::Response>> {
	// 此处返回 std::nullopt 代表通过过滤，否则代表拦截请求（不进入 SetHttpHandler 处理回调）
	co_return std::nullopt;
});

// 设置后置请求过滤（可不设置）
_server.OnAfter ([] (fv::Request &_req, fv::Response &_res) -> Task<void> {
	// 此处可对返回内容做一些处理
	co_return std::nullopt;
});

// 设置未 handle 的请求处理函数（可不设置）
_server.OnUnhandled ([] (fv::Request &_req) -> Task<fv::Response> {
	co_return fv::Response::FromText ("not handled!");
});

// 开始监听
co_await _server.Run (8080);
```

### TCP Server

```cpp
// 服务器对象
TcpServer m_tcpserver {};

// 设置新链接处理函数
m_tcpserver.SetOnConnect ([&m_tcpserver] (std::shared_ptr<IConn> _conn) -> Task<void> {
	// 此处自由发挥，退出函数则链接断开，通常 `while (true)`

	// 可考虑注册客户端及取消注册，此处自己根据业务指定ID
	m_tcpserver.RegisterClient (123, _conn);
	m_tcpserver.UnregisterClient (123, _conn);
});

// 假如处理函数内部注册后，外部可直接给对应客户端发消息或者广播消息
std::string _data = "hello";
co_await m_tcpserver.SendData (123, _data.data (), _data.size ());
co_await m_tcpserver.BroadcastData (_data.data (), _data.size ());

// 开始监听
co_await m_tcpserver.Run (2233);
```

## 附带的其他异步功能

```cpp
// 暂停 10 秒
co_await fv::Tasks::Delay (std::chrono::seconds (10));

// 外部需要用到 io_context
boost::asio::io_context &_ctx = fv::Tasks::GetContext ();

// 异步方法内同步
// 创建信号量
AsyncSemaphore _sema { 1 };
// 尝试获取信号
bool _success = _sema.TryAcquire ();
// 异步等待信号
co_await _sema.AcquireAsync ();
 // 超时异步等待信号
bool _success = co_await _sema.AcquireForAsync (std::chrono::seconds (10));
bool _success = co_await _sema.AcquireForAsync (std::chrono::system_clock::now () + std::chrono::seconds (10));
// 释放信号
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