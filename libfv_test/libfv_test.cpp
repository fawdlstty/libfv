#define _WIN32_WINNT 0x0601
#include <iostream>
#include <conio.h>

#ifdef _MSC_VER
#	ifdef _RESUMABLE_FUNCTIONS_SUPPORTED
#		undef _RESUMABLE_FUNCTIONS_SUPPORTED
#	endif
#	ifndef __cpp_lib_coroutine
#		define __cpp_lib_coroutine
#	endif
#	include <coroutine>
#	pragma warning (disable: 4068)
#	pragma comment (lib, "Crypt32.lib")
#endif

#include <fv/fv.h>



//Task<void> run_server () {
//	fv::HttpServer _server {};
//	_server.SetHttpHandler ("/hello", [] (fv::Request &_req) -> Task<fv::Response> {
//		co_return fv::Response::FromText ("hello world");
//	});
//	_server.SetHttpHandler ("/ws", [] (fv::Request &_req) -> Task<fv::Response> {
//		if (_req.IsWebsocket ()) {
//			auto _conn = co_await _req.UpgradeWebsocket ();
//			try {
//				while (true) {
//					auto [_data, _type] = co_await _conn->Recv ();
//					if (_type == fv::WsType::Text) {
//						std::cout << "server recv and reply: " << _data << std::endl;
//						co_await _conn->SendText (_data.data (), _data.size ());
//					} else if (_type == fv::WsType::Binary) {
//						co_await _conn->SendBinary (_data.data (), _data.size ());
//					}
//				}
//			} catch (std::exception &_e) {
//				std::cout << "server exception: " << _e.what () << std::endl;
//			}
//			co_return fv::Response::Empty ();
//		} else {
//			co_return fv::Response::FromText ("please use websocket");
//		}
//	});
//	co_await _server.Run (8080);
//	co_return;
//}



Task<void> test_client () {
	////std::string _ip = co_await fv::Config::DnsResolve ("www.baidu.com");
	//fv::Response _r = co_await fv::Get ("https://t.cn");
	//_r = co_await fv::Get ("https://t.cn", fv::timeout (std::chrono::seconds (10)));
	//_r = co_await fv::Get ("https://t.cn", fv::server ("106.75.237.200"));
	//_r = co_await fv::Post ("https://t.cn", fv::body_kv ("a", "aaa"));
	//_r = co_await fv::Post ("https://t.cn", fv::body_kv ("a", "aaa"), fv::content_type ("application/x-www-form-urlencoded"));
	//_r = co_await fv::Post ("https://t.cn", fv::body_file ("a", "filename.txt", "content..."));
	//_r = co_await fv::Post ("https://t.cn", fv::body_kvs ({{ "a", "b" }, { "c", "d" }}));
	//_r = co_await fv::Post ("https://t.cn", fv::body_json ("{\"a\":\"b\"}"));
	//_r = co_await fv::Post ("https://t.cn", fv::body_raw ("application/octet-stream", "aaa"));
	//_r = co_await fv::Get ("https://t.cn", fv::header ("X-WWW-Router", "123456789"));
	//_r = co_await fv::Get ("https://t.cn", fv::authorization ("Bearer XXXXXXXXXXXXX=="));
	//_r = co_await fv::Get ("https://t.cn", fv::authorization ("admin", "123456"));
	//_r = co_await fv::Get ("https://t.cn", fv::connection ("keep-alive"));
	//_r = co_await fv::Get ("https://t.cn", fv::content_type ("application/octet-stream"));
	//_r = co_await fv::Get ("https://t.cn", fv::referer ("https://t.cn"));
	//_r = co_await fv::Get ("https://t.cn", fv::user_agent ("Mozilla/4.0 Chrome 2333"));

	while (true) {
		std::cout << "1\n";
		fv::Response _r = co_await fv::Get ("http://www.fawdlstty.com");
		std::cout << _r.Content.size () << '\n';
		std::cout << "press any key to continue\n";
		::_getch ();
	}
	std::this_thread::sleep_for (std::chrono::seconds (10));
	fv::Tasks::Stop ();

	//try {
	//	std::shared_ptr<fv::WsConn> _conn = co_await fv::ConnectWS ("ws://82.157.123.54:9010/ajaxchattest", fv::header ("Origin", "http://coolaf.com"));
	//	while (true) {
	//		std::cout << "press any key to continue\n";
	//		::_getch ();

	//		std::string _str = "hello";
	//		std::cout << "send" << std::endl;
	//		co_await _conn->SendText (_str.data (), _str.size ());

	//		std::cout << "recv: ";
	//		auto [_data, _type] = co_await _conn->Recv ();
	//		std::cout << _data << std::endl;

	//	}
	//} catch (std::exception &_e) {
	//	std::cout << "catch exception: " << _e.what () << std::endl;
	//} catch (...) {
	//	std::cout << "catch exception" << std::endl;
	//}
}



int main () {
	fv::Tasks::Init ();
	fv::Tasks::RunAsync (test_client);
	fv::Tasks::LoopRun ();
	fv::Tasks::Release ();
	return 0;
}
