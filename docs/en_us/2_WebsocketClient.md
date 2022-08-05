# Websocket Client

## Create connection

```cpp
std::shared_ptr<fv::WsConn> _conn = co_await fv::ConnectWS ("wss://t.cn/ws");
```

To create `Websocket` connection, you can attach `HTTP` arguments, reference `HTTP Client` section. Example:

```cpp
std::shared_ptr<fv::WsConn> _conn = co_await fv::ConnectWS ("wss://t.cn/ws", fv::header ("Origin", "https://a.cn"));
```

## Loop to receive content and print

```cpp
// throw exception means link is broken
// It can receive two types, `fv::WsType::Text`, `fv::WsType::Binary`
while (true) {
	auto [_data, _type] = co_await _conn->Recv ();
	std::cout << _data << std::endl;
}
```

## Send data

```cpp
// throw exception means link is broken
std::string _str = "hello";
co_await _conn->SendText (_str.data (), _str.size ());
co_await _conn->SendBinary (_str.data (), _str.size ());
```

## Close connection

Actively close the connection:

```cpp
co_await _conn->Close ();
```

In addition to active closing, the connection is automatically closed as long as the connection object is not referenced by the code and is automatically freed by the smart pointer.

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
