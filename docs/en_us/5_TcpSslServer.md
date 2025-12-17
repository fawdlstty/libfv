# TCP & SSL Server

## Create TCP server object

```cpp
fv::TcpServer _tcpserver {};
```

## Create SSL server object

```cpp
fv::SslServer _sslserver {};
```

## Sets the new link handler function for TCP server

```cpp
_tcpserver.SetOnConnect ([&_tcpserver] (std::shared_ptr<IConn2> _conn) -> Task<void> {
	// Free play here, return then link close, usually `while (true)`

	// You can register client or unregister, specify the ID based on your profession
	_tcpserver.RegisterClient (123, _conn);
	_tcpserver.UnregisterClient (123, _conn);
});

// If the handler function has registered internally, the external can send or broadcast messages directly to the corresponding client
std::string _data = "hello";
bool _success = co_await _tcpserver.SendData (123, _data.data (), _data.size ());
size_t _count = co_await _tcpserver.BroadcastData (_data.data (), _data.size ());
```

## Configure SSL context and set new connection handler for SSL server

```cpp
// Set new connection handler for SSL server
_sslserver.SetOnConnect([&_sslserver](std::shared_ptr<IConn2> _conn) -> Task<void> {
    // Handle SSL connection
    // Send welcome message
    std::string welcome_msg = "Welcome to SSL server!\n";
    co_await _conn->Send(welcome_msg.data(), welcome_msg.size());
    
    // Register client
    static int64_t client_id = 1000;
    _sslserver.RegisterClient(client_id++, _conn);
    
    // Receive data loop
    while (true) {
        try {
            char buffer[1024] = {0};
            size_t received = co_await _conn->Recv(buffer, sizeof(buffer) - 1);
            if (received > 0) {
                // Echo received data
                co_await _conn->Send(buffer, received);
            }
        } catch (...) {
            break; // Connection closed or other error
        }
    }
    
    // Unregister client
    // Note: In real applications, you should maintain client_id mapping according to your business logic
});
```

## Start TCP server

```cpp
co_await _tcpserver.Run (8080);
```

## Configure SSL context and start SSL server

```cpp
// Create and configure SSL context
fv::Ssl::context _ssl_ctx(fv::Ssl::context::tlsv12);
_ssl_ctx.set_options(
    fv::Ssl::context::default_workarounds |
    fv::Ssl::context::no_sslv2 |
    fv::Ssl::context::single_dh_use
);
_ssl_ctx.use_certificate_chain_file("server.crt");
_ssl_ctx.use_private_key_file("server.key", fv::Ssl::context::pem);

// Start SSL server
co_await _sslserver.Run(8443, _ssl_ctx);
```

## Example

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
        
        // Send welcome message
        std::string welcome_msg = "Welcome to TCP server!\n";
        co_await _conn->Send(welcome_msg.data(), welcome_msg.size());
        
        // Receive and echo data
        while (true) {
            try {
                char buffer[1024] = {0};
                size_t received = co_await _conn->Recv(buffer, sizeof(buffer) - 1);
                if (received > 0) {
                    std::cout << "Received: " << std::string(buffer, received) << "\n";
                    // Echo
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
    
    // Configure SSL context
    fv::Ssl::context _ssl_ctx(fv::Ssl::context::tlsv12);
    _ssl_ctx.set_options(
        fv::Ssl::context::default_workarounds |
        fv::Ssl::context::no_sslv2 |
        fv::Ssl::context::single_dh_use
    );
    // Note: Please provide valid certificate and private key file paths in actual use
    _ssl_ctx.use_certificate_chain_file("server.crt");
    _ssl_ctx.use_private_key_file("server.key", fv::Ssl::context::pem);
    
    _sslserver.SetOnConnect([](std::shared_ptr<fv::IConn2> _conn) -> Task<void> {
        std::cout << "New SSL client connected\n";
        
        // Send welcome message
        std::string welcome_msg = "Welcome to SSL server!\n";
        co_await _conn->Send(welcome_msg.data(), welcome_msg.size());
        
        // Receive and echo data
        while (true) {
            try {
                char buffer[1024] = {0};
                size_t received = co_await _conn->Recv(buffer, sizeof(buffer) - 1);
                if (received > 0) {
                    std::cout << "Received: " << std::string(buffer, received) << "\n";
                    // Echo
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
    
    // Run TCP server
    fv::Tasks::RunMainAsync(test_tcp_server);
    
    // Or run SSL server
    // fv::Tasks::RunMainAsync(test_ssl_server);
    
    fv::Tasks::Run();
    return 0;
}
```