# TCP & SSL Server

**Note: SSL server is not supported now**

## Create a server object

```cpp
fv::TcpServer _tcpserver {};
```

## Sets the new link handler function

```cpp
m_tcpserver.SetOnConnect ([&m_tcpserver] (std::shared_ptr<IConn2> _conn) -> Task<void> {
	// Free play here, return then link close, usually `while (true)`

	// You can register client or unregister, specify the ID based on your profession
	m_tcpserver.RegisterClient (123, _conn);
	m_tcpserver.UnregisterClient (123, _conn);
});

// If the handler function has registered internally, the external can send or broadcast messages directly to the corresponding client
std::string _data = "hello";
bool _success = co_await m_tcpserver.SendData (123, _data.data (), _data.size ());
size_t _count = co_await m_tcpserver.BroadcastData (_data.data (), _data.size ());
```

## Start listen

```cpp
co_await _server.Run (8080);
```
