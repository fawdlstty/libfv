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
