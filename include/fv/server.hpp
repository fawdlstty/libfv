#ifndef __FV_TCP_SERVER_HPP___
#define __FV_TCP_SERVER_HPP___



#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_set>

#include "common.hpp"
#include "conn.hpp"



namespace fv {
struct TcpServer {
	void SetOnConnect (std::function<Task<void> (std::shared_ptr<IConn2>)> _on_connect) { OnConnect = _on_connect; }
	void RegisterClient (int64_t _id, std::shared_ptr<IConn2> _conn) { std::unique_lock _ul { Mutex }; Clients [_id] = _conn; }
	void UnregisterClient (int64_t _id, std::shared_ptr<IConn2> _conn) {
		std::unique_lock _ul { Mutex };
		if (Clients [_id].get () == _conn.get ())
			Clients.erase (_id);
	}
	Task<bool> SendData (int64_t _id, char *_data, size_t _size) {
		try {
			std::unique_lock _ul { Mutex };
			if (Clients.contains (_id)) {
				auto _conn = Clients [_id];
				_ul.unlock ();
				co_await _conn->Send (_data, _size);
				co_return true;
			}
		} catch (...) {
		}
		co_return false;
	}
	Task<size_t> BroadcastData (char *_data, size_t _size) {
		std::unique_lock _ul { Mutex };
		std::unordered_set<std::shared_ptr<IConn2>> _conns;
		for (auto [_key, _val] : Clients)
			_conns.emplace (_val);
		_ul.unlock ();
		size_t _count = 0;
		for (auto _conn : _conns) {
			try {
				co_await _conn->Send (_data, _size);
				_count++;
			} catch (...) {
			}
		}
		co_return _count;
	}
	Task<void> Run (std::string _ip, uint16_t _port) {
		if (IsRun.load ())
			co_return;
		IsRun.store (true);
		auto _executor = co_await asio::this_coro::executor;
		Tcp::endpoint _ep { asio::ip::address::from_string (_ip), _port };
		Acceptor = std::make_unique<Tcp::acceptor> (_executor, _ep, true);
		try {
			for (; IsRun.load ();) {
				std::shared_ptr<IConn2> _conn = std::shared_ptr<IConn2> ((IConn2 *) new TcpConn2 (co_await Acceptor->async_accept (UseAwaitable)));
				Tasks::RunAsync ([this, _conn] () -> Task<void> {
					co_await OnConnect (_conn);
				});
			}
		} catch (...) {
		}
	}
	Task<void> Run (uint16_t _port) {
		co_await Run ("0.0.0.0", _port);
	}
	void Stop () {
		IsRun.store (false);
		if (Acceptor)
			Acceptor->cancel ();
	}

private:
	std::function<Task<void> (std::shared_ptr<IConn2>)> OnConnect;
	std::mutex Mutex;
	std::unordered_map<int64_t, std::shared_ptr<IConn2>> Clients;

	std::unique_ptr<Tcp::acceptor> Acceptor;
	std::atomic_bool IsRun { false };
};




struct SslServer {
	void SetOnConnect (std::function<Task<void> (std::shared_ptr<IConn2>)> _on_connect) { OnConnect = _on_connect; }
	void RegisterClient (int64_t _id, std::shared_ptr<IConn2> _conn) { std::unique_lock _ul { Mutex }; Clients [_id] = _conn; }
	void UnregisterClient (int64_t _id, std::shared_ptr<IConn2> _conn) {
		std::unique_lock _ul { Mutex };
		if (Clients [_id].get () == _conn.get ())
			Clients.erase (_id);
	}
	Task<bool> SendData (int64_t _id, char *_data, size_t _size) {
		try {
			std::unique_lock _ul { Mutex };
			if (Clients.contains (_id)) {
				auto _conn = Clients [_id];
				_ul.unlock ();
				co_await _conn->Send (_data, _size);
				co_return true;
			}
		} catch (...) {
		}
		co_return false;
	}
	Task<size_t> BroadcastData (char *_data, size_t _size) {
		std::unique_lock _ul { Mutex };
		std::unordered_set<std::shared_ptr<IConn2>> _conns;
		for (auto [_key, _val] : Clients)
			_conns.emplace (_val);
		_ul.unlock ();
		size_t _count = 0;
		for (auto _conn : _conns) {
			try {
				co_await _conn->Send (_data, _size);
				_count++;
			} catch (...) {
			}
		}
		co_return _count;
	}
	Task<void> Run (std::string _ip, uint16_t _port, Ssl::context& _ssl_ctx) {
		if (IsRun.load ())
			co_return;
		IsRun.store (true);
		auto _executor = co_await asio::this_coro::executor;
		Tcp::endpoint _ep { asio::ip::address::from_string (_ip), _port };
		Acceptor = std::make_unique<Tcp::acceptor> (_executor, _ep, true);
		try {
			for (; IsRun.load ();) {
				try {
					Tcp::socket _socket(co_await Acceptor->async_accept (UseAwaitable));
					Ssl::stream<Tcp::socket> _ssl_socket(std::move(_socket), _ssl_ctx);
					// Perform SSL handshake
					co_await _ssl_socket.async_handshake(Ssl::stream_base::server, UseAwaitable);
					std::shared_ptr<IConn2> _conn = std::make_shared<SslConn2>(std::move(_ssl_socket));
					Tasks::RunAsync ([this, _conn] () -> Task<void> {
						co_await OnConnect (_conn);
					});
				} catch (...) {
					// Handle SSL handshake errors properly
					// Continue to next iteration to accept new connections
					continue;
				}
			}
		} catch (...) {
			// Log error or handle outer exception as needed
		}
	}
	Task<void> Run (uint16_t _port, Ssl::context& _ssl_ctx) {
		co_await Run ("0.0.0.0", _port, _ssl_ctx);
	}
	void Stop () {
		IsRun.store (false);
		if (Acceptor)
			Acceptor->cancel ();
	}

private:
	std::function<Task<void> (std::shared_ptr<IConn2>)> OnConnect;
	std::mutex Mutex;
	std::unordered_map<int64_t, std::shared_ptr<IConn2>> Clients;

	std::unique_ptr<Tcp::acceptor> Acceptor;
	std::atomic_bool IsRun { false };
};




template<typename ServerType>
struct HttpServerBase {
	void OnBefore (std::function<Task<std::optional<fv::Response>> (fv::Request &)> _cb) { m_before = _cb; }
	void SetHttpHandler (std::string _path, std::function<Task<fv::Response> (fv::Request &)> _cb) { m_map_proc [_path] = _cb; }
	void OnUnhandled (std::function<Task<fv::Response> (fv::Request &)> _cb) { m_unhandled_proc = _cb; }
	void OnAfter (std::function<Task<void> (fv::Request &, fv::Response &)> _cb) { m_after = _cb; }

	// Common request processing logic to avoid duplication
	Task<void> ProcessRequests(std::shared_ptr<IConn2> _conn, uint16_t _port) {
		while (true) {
			Request _req = co_await Request::GetFromConn (_conn, _port);
			if (m_before) {
				std::optional<Response> _ores = co_await m_before (_req);
				if (_ores.has_value ()) {
					if (m_after)
						co_await m_after (_req, _ores.value ());
					std::string _str_res = _ores.value ().Serilize ();
					try {
						co_await _conn->Send (_str_res.data (), _str_res.size ());
					} catch (...) {
						break;
					}
					continue;
				}
			}
			Response _res {};
			if (m_map_proc.contains (_req.UrlPath)) {
				try {
					_res = co_await m_map_proc [_req.UrlPath] (_req);
				} catch (...) {
				}
			}
			if (_res.HttpCode == -1) {
				try {
					_res = co_await m_unhandled_proc (_req);
				} catch (...) {
					// 如果未处理的请求处理器也失败了，默认返回404
					_res = Response::FromNotFound();
				}
			}
			if (_req.IsUpgraded ())
				break;
			if (_res.HttpCode == -1)
				_res = Response::FromNotFound ();
			if (m_after)
				co_await m_after (_req, _res);
			std::string _str_res = _res.Serilize ();
			try {
				co_await _conn->Send (_str_res.data (), _str_res.size ());
			} catch (...) {
				break;
			}
		}
	}

	// Primary template declaration for Run method
	template<typename T = ServerType>
	typename std::enable_if<std::is_same_v<T, TcpServer>, Task<void>>::type
	Run (uint16_t _port) {
		m_server.SetOnConnect ([_port, this] (std::shared_ptr<IConn2> _conn) -> Task<void> {
			co_await ProcessRequests(_conn, _port);
		});
		co_await m_server.Run(_port);
	}
	
	// Overload for SSL server that takes SSL context
	template<typename T = ServerType>
	typename std::enable_if<std::is_same_v<T, SslServer>, Task<void>>::type
	Run (uint16_t _port, Ssl::context& _ssl_ctx) {
		m_server.SetOnConnect ([_port, this] (std::shared_ptr<IConn2> _conn) -> Task<void> {
			co_await ProcessRequests(_conn, _port);
		});
		co_await m_server.Run(_port, _ssl_ctx);
	}

	void Stop () { m_server.Stop (); }

private:
	ServerType m_server {};
	std::function<Task<std::optional<Response>> (Request &)> m_before;
	std::unordered_map<std::string, std::function<Task<Response> (Request &)>> m_map_proc;
	std::function<Task<Response> (Request &)> m_unhandled_proc = [] (Request &) -> Task<Response> { co_return Response::FromNotFound (); };
	std::function<Task<void> (Request &, Response &)> m_after;
};

// Type aliases for convenience
using HttpServer = HttpServerBase<TcpServer>;
using HttpsServer = HttpServerBase<SslServer>;
}



#endif //__FV_TCP_SERVER_HPP___
