# HTTP 服务器端

**注：暂不支持 HTTPS 服务器端**

## 创建服务器端对象

```cpp
fv::HttpServer _server {};
```

## 指定 HTTP 请求处理回调

```cpp
_server.SetHttpHandler ("/hello", [] (fv::Request &_req) -> Task<fv::Response> {
	co_return fv::Response::FromText ("hello world");
});
```

## 指定 websocket 请求处理回调

```cpp
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
```

## 设置前置请求过滤

```cpp
_server.OnBefore ([] (fv::Request &_req) -> std::optional<Task<fv::Response>> {
	// 此处返回 std::nullopt 代表通过过滤，否则代表拦截请求（不进入 SetHttpHandler 处理回调）
	co_return std::nullopt;
});
```

## 设置后置请求过滤

```cpp
_server.OnAfter ([] (fv::Request &_req, fv::Response &_res) -> Task<void> {
	// 此处可对返回内容做一些处理
	co_return std::nullopt;
});
```

## 设置未 handle 的请求处理函数

```cpp
_server.OnUnhandled ([] (fv::Request &_req) -> Task<fv::Response> {
	co_return fv::Response::FromText ("not handled!");
});
```

## 开始监听并启动服务

```cpp
co_await _server.Run (8080);
```
