#include <iostream>

#define ASIO_STANDALONE
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
	fv::Session _sess = co_await fv::Session::FromUrl ("https://www.fawdlstty.com");
	fv::Response _r = co_await _sess.Get ("https://www.fawdlstty.com");
	std::cout << _r.Content.size () << '\n';
	_r = co_await _sess.Post ("https://www.fawdlstty.com", fv::body_json ("{\"a\":\"b\"}"));
	std::cout << _r.Content.size () << '\n';
	//
	std::this_thread::sleep_for (std::chrono::seconds (10));
	fv::Tasks::Stop ();
}



int main () {
	fv::Tasks::Init ();
	fv::Tasks::RunAsync (test_client);
	fv::Tasks::LoopRun ();
	fv::Tasks::Release ();
	return 0;
}
