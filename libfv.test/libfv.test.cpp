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



Task<void> run_server () {
	fv::HttpServer _server {};
	//_server.OnBefore ([] (fv::Request &_req) -> Task<std::optional<fv::Response>> {
	//	if (_req.Url == "http://127.0.0.1/haha") {
	//		co_return fv::Response::FromText ("hello");
	//	} else {
	//		co_return std::nullopt;
	//	}
	//});
	//_server.OnAfter ([] (fv::Request &_req, fv::Response &_res) -> Task<void> {
	//	// TODO
	//	co_return;
	//});
	_server.SetHttpHandler ("/hello", [] (fv::Request &_req) -> Task<fv::Response> {
		co_return fv::Response::FromText ("hello world");
	});
	_server.SetHttpHandler ("/ws", [] (fv::Request &_req) -> Task<fv::Response> {
		if (_req.IsWebsocket ()) {
			auto _conn = co_await _req.UpgradeWebsocket ();
			try {
				while (true) {
					auto [_data, _type] = co_await _conn->Recv ();
					if (_type == fv::WsType::Text) {
						std::cout << "server recv and reply: " << _data << std::endl;
						co_await _conn->SendText (_data.data (), _data.size ());
					} else if (_type == fv::WsType::Binary) {
						co_await _conn->SendBinary (_data.data (), _data.size ());
					}
				}
			} catch (std::exception &_e) {
				std::cout << "server exception: " << _e.what () << std::endl;
			}
			co_return fv::Response::Empty ();
		} else {
			co_return fv::Response::FromText ("please use websocket");
		}
	});
	co_await _server.Run (8080);
	co_return;
}


int main () {
	fv::Tasks::Start (true);
	fv::Tasks::RunAsync (run_server);
	//static std::shared_ptr<fv::WsConn> _conn;
	//std::cout << "press any key to connect websocket" << std::endl;
	//::_getch ();

	//fv::Tasks::RunAsync ([] () -> Task<void> {
	//	_conn = co_await fv::ConnectWS ("ws://127.0.0.1:8080/ws");
	//});
	//std::cout << "press any key to send hello" << std::endl;
	//::_getch ();
	//fv::Tasks::RunAsync ([] () -> Task<void> {
	//	std::string _s = "hello";
	//	co_await _conn->SendText (_s.data (), _s.size ());
	//});

	std::cout << "press any key to exit" << std::endl;
	::_getch ();
	fv::Tasks::Stop ();
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
