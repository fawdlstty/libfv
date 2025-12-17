# TCP 及 SSL 服务器端

## 创建TCP服务器端对象

```cpp
fv::TcpServer _tcpserver {};
```

## 创建SSL服务器端对象

```cpp
fv::SslServer _sslserver {};
```

## 设置新链接处理函数

```cpp
_tcpserver.SetOnConnect ([&m_tcpserver] (std::shared_ptr<IConn2> _conn) -> Task<void> {
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

## 配置SSL上下文并设置SSL服务器连接处理函数

```cpp
// 设置SSL服务器的新连接处理函数
_sslserver.SetOnConnect([&_sslserver](std::shared_ptr<IConn2> _conn) -> Task<void> {
    // 处理SSL连接
    // 发送欢迎消息
    std::string welcome_msg = "Welcome to SSL server!\n";
    co_await _conn->Send(welcome_msg.data(), welcome_msg.size());
    
    // 注册客户端
    static int64_t client_id = 1000;
    _sslserver.RegisterClient(client_id++, _conn);
    
    // 接收数据循环
    while (true) {
        try {
            char buffer[1024] = {0};
            size_t received = co_await _conn->Recv(buffer, sizeof(buffer) - 1);
            if (received > 0) {
                // 回显接收到的数据
                co_await _conn->Send(buffer, received);
            }
        } catch (...) {
            break; // 连接断开或其他错误
        }
    }
    
    // 取消注册客户端
    // 注意：实际应用中应根据具体业务逻辑维护client_id映射关系
});
```

## 开始监听并启动TCP服务

```cpp
co_await _tcpserver.Run (8080);
```

## 配置SSL上下文并启动SSL服务

```cpp
// 创建并配置SSL上下文
fv::Ssl::context _ssl_ctx(fv::Ssl::context::tlsv12);
_ssl_ctx.set_options(
    fv::Ssl::context::default_workarounds |
    fv::Ssl::context::no_sslv2 |
    fv::Ssl::context::single_dh_use
);
_ssl_ctx.use_certificate_chain_file("server.crt");
_ssl_ctx.use_private_key_file("server.key", fv::Ssl::context::pem);

// 启动SSL服务器
co_await _sslserver.Run(8443, _ssl_ctx);
```

## 示例

```cpp
#ifdef _MSC_VER
#   define _WIN32_WINNT 0x0601
#   pragma warning (disable: 4068)
#   pragma comment (lib, "Crypt32.lib")
#endif

#include <iostream>
#include <string>
#include <fv/fv.h>

Task<void> test_tcp_server() {
    fv::TcpServer _tcpserver{};
    
    _tcpserver.SetOnConnect([](std::shared_ptr<fv::IConn2> _conn) -> Task<void> {
        std::cout << "New TCP client connected\n";
        
        // 发送欢迎消息
        std::string welcome_msg = "Welcome to TCP server!\n";
        co_await _conn->Send(welcome_msg.data(), welcome_msg.size());
        
        // 接收并回显数据
        while (true) {
            try {
                char buffer[1024] = {0};
                size_t received = co_await _conn->Recv(buffer, sizeof(buffer) - 1);
                if (received > 0) {
                    std::cout << "Received: " << std::string(buffer, received) << "\n";
                    // 回显
                    co_await _conn->Send(buffer, received);
                }
            } catch (...) {
                break;
            }
        }
        
        std::cout << "TCP client disconnected\n";
    });
    
    std::cout << "TCP Server listening on port 8080\n";
    co_await _tcpserver.Run(8080);
}

Task<void> test_ssl_server() {
    fv::SslServer _sslserver{};
    
    // 配置SSL上下文
    fv::Ssl::context _ssl_ctx(fv::Ssl::context::tlsv12);
    _ssl_ctx.set_options(
        fv::Ssl::context::default_workarounds |
        fv::Ssl::context::no_sslv2 |
        fv::Ssl::context::single_dh_use
    );
    // 注意：在实际使用中，请提供有效的证书和私钥文件路径
    _ssl_ctx.use_certificate_chain_file("server.crt");
    _ssl_ctx.use_private_key_file("server.key", fv::Ssl::context::pem);
    
    _sslserver.SetOnConnect([](std::shared_ptr<fv::IConn2> _conn) -> Task<void> {
        std::cout << "New SSL client connected\n";
        
        // 发送欢迎消息
        std::string welcome_msg = "Welcome to SSL server!\n";
        co_await _conn->Send(welcome_msg.data(), welcome_msg.size());
        
        // 接收并回显数据
        while (true) {
            try {
                char buffer[1024] = {0};
                size_t received = co_await _conn->Recv(buffer, sizeof(buffer) - 1);
                if (received > 0) {
                    std::cout << "Received: " << std::string(buffer, received) << "\n";
                    // 回显
                    co_await _conn->Send(buffer, received);
                }
            } catch (...) {
                break;
            }
        }
        
        std::cout << "SSL client disconnected\n";
    });
    
    std::cout << "SSL Server listening on port 8443\n";
    co_await _sslserver.Run(8443, _ssl_ctx);
}

int main() {
    fv::Tasks::Init();
    
    // 运行TCP服务器
    fv::Tasks::RunMainAsync(test_tcp_server);
    
    // 或者运行SSL服务器
    // fv::Tasks::RunMainAsync(test_ssl_server);
    
    fv::Tasks::Run();
    return 0;
}
```