#ifndef __FV_HTTP_CLIENT_IMPL_HPP__
#define __FV_HTTP_CLIENT_IMPL_HPP__



#include <chrono>
#include <memory>

#include "common.hpp"
#include "conn.hpp"
#include "req_res.hpp"



namespace fv {
inline Task<Response> DoMethod (Request _r) {
	auto [_schema, _host, _port, _path] = _parse_url (_r.Url);
	_r.Schema = _schema;
	_r.UrlPath = _path;
	std::shared_ptr<IConn> _conn;
	if (_schema == "https") {
		_conn = std::shared_ptr<IConn> (new SslConn { Tasks::GetContext () });
	} else {
		_conn = std::shared_ptr<IConn> (new TcpConn { Tasks::GetContext () });
	}
	AsyncTimer _timer {};
	if (std::chrono::duration_cast<std::chrono::nanoseconds> (_r.Timeout).count () > 0) {
		_timer.WaitCallback (_r.Timeout, [_tconn = std::weak_ptr (_conn)] ()->Task<void> {
			auto _conn = _tconn.lock ();
			if (_conn)
				_conn->Cancel ();
		});
	}

	// connect
	co_await _conn->Connect (_r.Server != "" ? _r.Server : _host, _port);

	// generate data
	std::string _data = _r.Serilize (_r.Method, _host, _path);

	// send
	co_await _conn->Send (_data.data (), _data.size ());

	// recv
	co_return co_await Response::GetFromConn (_conn);
}
}



#endif //__FV_HTTP_CLIENT_IMPL_HPP__
