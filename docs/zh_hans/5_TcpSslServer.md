# TCP 及 SSL 服务器端

**注：暂不支持 SSL 服务器端**

## 创建服务器端对象

```cpp
fv::TcpServer _tcpserver {};
```

## 设置新链接处理函数

```cpp
m_tcpserver.SetOnConnect ([&m_tcpserver] (std::shared_ptr<IConn2> _conn) -> Task<void> {
	// 此处自由发挥，退出函数则链接断开，通常 `while (true)`

	// 可考虑注册客户端及取消注册，此处自己根据业务指定ID
	m_tcpserver.RegisterClient (123, _conn);
	m_tcpserver.UnregisterClient (123, _conn);
});

// 假如处理函数内部注册后，外部可直接给对应客户端发消息或者广播消息
std::string _data = "hello";
bool _success = co_await m_tcpserver.SendData (123, _data.data (), _data.size ());
size_t _count = co_await m_tcpserver.BroadcastData (_data.data (), _data.size ());
```

## 开始监听并启动服务

```cpp
co_await _server.Run (8080);
```

## 示例

TODO
