#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING
#define _SILENCE_CXX20_CODECVT_FACETS_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SILENCE_ALL_CXX20_DEPRECATION_WARNINGS
#define PIO_APC_ROUTINE_DEFINED
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WIN32_LEAN_AND_MEAN



#include <fv/fv.h>

#include <conio.h>
#include <iostream>

#pragma comment (lib, "Crypt32.lib")



using boost::asio::use_awaitable;
using namespace fv;

struct TcpServer {
	void SetOnConnect (std::function<Task<int64_t> (std::shared_ptr<IConn>)> _on_connect) { OnConnect = _on_connect; }
	void SetOnRecv (std::function<Task<void> (int64_t, std::string)> _on_recv) { OnRecv = _on_recv; }

	Task<void> Run (uint16_t _port) {
		if (IsRun.load ())
			co_return;
		IsRun.store (true);
		auto _executor = co_await boost::asio::this_coro::executor;
		Acceptor = std::make_unique<Tcp::acceptor> (_executor, Tcp::endpoint { Tcp::v4 (), _port }, true);
		try {
			for (; IsRun.load ();) {
				std::shared_ptr<IConn> _conn = std::shared_ptr<IConn> ((IConn *) new TcpConn2 (co_await Acceptor->async_accept (use_awaitable)));
				Tasks::RunAsync ([this, _conn] () -> Task<void> {
					std::unique_lock _ul { Mutex, std::defer_lock };
					TcpConn2 *_raw_conn = (TcpConn2 *) _conn.get ();
					int64_t _id = -1;
					try {
						_id = co_await OnConnect (_conn);
						_raw_conn->Id = _id;
						_ul.lock ();
						if (Clients.contains (_id)) {
							auto _last_conn = Clients [_id];
							Clients [_id] = _conn;
							_ul.unlock ();
							_last_conn->Cancel ();
						} else {
							Clients [_id] = _conn;
							_ul.unlock ();
						}

						while (true) {
							std::string _data = co_await _conn->ReadSome ();
							co_await OnRecv (_id, std::move (_data));
						}
					} catch (...) {
					}
					_ul.lock ();
					if (Clients.contains (_id)) {
						if (Clients [_id].get () == _raw_conn)
							Clients.erase (_id);
					}
					_ul.unlock ();
				});
			}
		} catch (...) {
		}
	}

	void Stop () {
		IsRun.store (false);
		if (Acceptor)
			Acceptor->cancel ();
	}

private:
	std::function<Task<int64_t> (std::shared_ptr<IConn>)> OnConnect;
	std::function<Task<void> (int64_t, std::string)> OnRecv;
	std::mutex Mutex;
	std::unordered_map<int64_t, std::shared_ptr<IConn>> Clients;

	std::unique_ptr<Tcp::acceptor> Acceptor;
	std::atomic_bool IsRun { false };
};

struct HttpServer {
	void OnBefore (std::function<Task<std::optional<fv::Response>> (fv::Request&)> _cb) { m_before = _cb; }
	void OnRequest (std::string _path, std::function<Task<fv::Response> (fv::Request&)> _cb) { m_map_proc [_path] = _cb; }
	void OnAfter (std::function<Task<void> (fv::Request&, fv::Response&)> _cb) { m_after = _cb; }

	

private:

	std::function<Task<std::optional<fv::Response>> (fv::Request &)> m_before;
	std::unordered_map<std::string, std::function<Task<fv::Response> (fv::Request &)>> m_map_proc;
	std::function<Task<void> (fv::Request &, fv::Response &)> m_after;
};

Task<void> run_server () {
	HttpServer _server {};
	_server.OnBefore ([] (fv::Request &_req) -> Task<std::optional<fv::Response>> {
		if (_req.Url == "http://127.0.0.1/haha") {
			co_return fv::Response::FromText ("hello");
		} else {
			co_return std::nullopt;
		}
	});
	_server.OnAfter ([] (fv::Request &_req, fv::Response &_res) -> Task<void> {
		// TODO
		co_return;
	});
	_server.OnRequest ("/hello", [] (fv::Request &_req) -> Task<fv::Response> {
		co_return fv::Response::FromText ("hello world");
	});
	// TODO
	co_return;
}

int main () {
	fv::Tasks::RunAsync (run_server);
	::_getch ();
	return 0;
}



//std::shared_ptr<fv::WsConn> g_conn;
//Task<void> async_func () {
//	//fv::Response _r = co_await fv::Get ("http://www.fawdlstty.com"
//	//	//,fv::server ("106.75.237.200"),
//	//	//fv::user_agent ("Mozilla 4.0 Chromiiii 100 (Windows 12)"),
//	//	//fv::http_header ("MyHeader", "MyValue"),
//	//	//fv::auth ("admin", "123456")
//	//);
//	//fv::Response _r = co_await fv::Get ("https://www.fawdlstty.com");
//	//std::cout << _r.Content;
//	try {
//		g_conn = co_await fv::ConnectWS ("ws://127.0.0.1:1234/ws");
//		std::cout << "connected." << std::endl;
//		while (true) {
//			auto [_data, _type] = co_await g_conn->Recv ();
//			std::cout << _data << std::endl;
//		}
//	} catch (std::exception &_ex) {
//		std::cout << "[async_func] catch exception: " << _ex.what () << std::endl;
//	} catch (...) {
//		std::cout << "[async_func] catch exception." << std::endl;
//	}
//}
//
//int main () {
//	// 控制台utf8支持
//	::SetConsoleOutputCP (65001);
//	//
//	fv::Tasks::Start (true);
//	fv::Config::SslVerifyFunc = [] (bool preverified, fv::Ssl::verify_context &ctx) { return true; };
//	fv::Tasks::RunAsync (async_func);
//	while (true) {
//		::_getch ();
//		fv::Tasks::RunAsync ([] () -> Task<void> {
//			static std::string _str = "{\"cmd\":\"hello\",\"seq\":\"0\"}";
//			try {
//				co_await g_conn->SendText (_str.data (), _str.size ());
//			} catch (std::exception &_ex) {
//				std::cout << "[main] catch exception: " << _ex.what () << std::endl;
//			} catch (...) {
//				std::cout << "[main] catch exception." << std::endl;
//			}
//		});
//	}
//	fv::Tasks::Stop ();
//	return 0;
//}
