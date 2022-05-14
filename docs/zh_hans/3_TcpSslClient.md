# TCP 及 SSL 客户端

## 建立连接

```cpp
// tcp
std::shared_ptr<fv::IConn> _conn = co_await fv::Connect ("tcp://127.0.0.1:1234");

// ssl
std::shared_ptr<fv::IConn> _conn2 = co_await fv::Connect ("ssl://127.0.0.1:1235");
```

## 循环接收数据

TCP与SSL均为流式协议，无法准确获取单个数据包长度，请自定格式指定长度信息。

```cpp
// 抛异常说明连接断开
char _ch = co_await _conn->ReadChar ();
std::string _line = co_await _conn->ReadLine ();

// ReadCount 与 ReadCountVec 必须待接收到那么长数据之后才会返回
std::string _buf = co_await _conn->ReadCount (1024);
std::vector<uint8_t> _buf2 = co_await _conn->ReadCountVec (1024);
```

## 发送数据

```cpp
// 抛异常说明连接断开
std::string _str = "hello";
co_await _conn->Send (_str.data (), _str.size ());
```

## 关闭连接

主动关闭连接：

```cpp
co_await _conn->Close ();
```

除了主动关闭外，只要连接对象不被代码所引用，受智能指针自动释放，也会自动关闭链接。
