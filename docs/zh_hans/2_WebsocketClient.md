# Websocket 客户端

## 建立连接

```cpp
std::shared_ptr<fv::WsConn> _conn = co_await fv::ConnectWS ("wss://t.cn/ws");
```

建立 `Websocket` 连接可附加 `HTTP` 参数，参考 `Http 客户端` 章节。示例：

```cpp
std::shared_ptr<fv::WsConn> _conn = co_await fv::ConnectWS ("wss://t.cn/ws", fv::header ("Origin", "https://a.cn"));
```

## 循环接收数据

```cpp
// 抛异常说明连接断开
// 能接收到两种类型，`fv::WsType::Text`，`fv::WsType::Binary`
while (true) {
	auto [_data, _type] = co_await _conn->Recv ();
	std::cout << _data << std::endl;
}
```

## 发送数据

```cpp
// 抛异常说明连接断开
std::string _str = "hello";
co_await _conn->SendText (_str.data (), _str.size ());
co_await _conn->SendBinary (_str.data (), _str.size ());
```

## 关闭连接

主动关闭连接：

```cpp
co_await _conn->Close ();
```

除了主动关闭外，只要连接对象不被代码所引用，受智能指针自动释放，也会自动关闭链接。

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
	try {
		std::shared_ptr<fv::WsConn> _conn = co_await fv::ConnectWS ("ws://82.157.123.54:9010/ajaxchattest", fv::header ("Origin", "http://coolaf.com"));
		while (true) {
			std::cout << "press any char to continue (q to exit)\n";
			char ch;
			std::cin >> ch;
			if (ch == 'q')
				break;

			std::string _str = "hello";
			std::cout << "send" << std::endl;
			co_await _conn->SendText (_str.data (), _str.size ());

			std::cout << "recv: ";
			auto [_data, _type] = co_await _conn->Recv ();
			std::cout << _data << std::endl;
		}
	} catch (std::exception &_e) {
		std::cout << "catch exception: " << _e.what () << std::endl;
	} catch (...) {
		std::cout << "catch exception" << std::endl;
	}
    fv::Tasks::Stop ();
}

int main () {
	fv::Tasks::Init ();
	fv::Tasks::RunAsync (test_client);
	fv::Tasks::Run ();
	return 0;
}
```
