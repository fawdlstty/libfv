# HTTP Server

**Note: HTTPS server is not supported now**

## Create a server object

```cpp
fv::HttpServer _server {};
```

## Specify HTTP and Websocket request processing callbacks

```cpp
_server.SetHttpHandler ("/hello", [] (fv::Request &_req) -> Task<fv::Response> {
	co_return fv::Response::FromText ("hello world");
});

_server.SetHttpHandler ("/ws", [] (fv::Request &_req) -> Task<fv::Response> {
	// Check whether it is a websocket request
	if (_req.IsWebsocket ()) {
		// Upgrade to websocket connect
		auto _conn = co_await _req.UpgradeWebsocket ();
		while (true) {
			auto [_data, _type] = co_await _conn->Recv ();
			if (_type == fv::WsType::Text) {
				co_await _conn->SendText (_data.data (), _data.size ());
			} else if (_type == fv::WsType::Binary) {
				co_await _conn->SendBinary (_data.data (), _data.size ());
			}
		}
		// Return empty after websocket upgrade
		co_return fv::Response::Empty ();
	} else {
		co_return fv::Response::FromText ("please use websocket");
	}
});
```

## Set before-request filtering

```cpp
_server.OnBefore ([] (fv::Request &_req) -> std::optional<Task<fv::Response>> {
	// Return std::nullopt to indicate passing, otherwise intercept and return the currently returned result (Do not enter SetHttpHandler handling callback)
	co_return std::nullopt;
});
```

## Set after-request filtering

```cpp
_server.OnAfter ([] (fv::Request &_req, fv::Response &_res) -> Task<void> {
	// Here you can do something with the returned content
	co_return std::nullopt;
});
```

## Set unhandled request handling callback

```cpp
_server.OnUnhandled ([] (fv::Request &_req) -> Task<fv::Response> {
	co_return fv::Response::FromText ("not handled!");
});
```

## Start listen

```cpp
co_await _server.Run (8080);
```
