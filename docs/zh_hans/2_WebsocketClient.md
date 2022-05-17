# Websocket 客户端

## 建立连接

```cpp
std::shared_ptr<fv::WsConn> _conn = co_await fv::ConnectWS ("wss://t.cn/ws");
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
