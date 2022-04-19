#ifndef __FV_HPP__
#define __FV_HPP__



#include <cstdint>
#include <format>
#include <map>
#include <sstream>
#include <string>
#include <tuple>

#include "Tasks.hpp"



namespace fv {
enum class ContentType { KvPair, Json, Form };

struct Req {
	std::string Url = "";
	std::string Content = "";
	std::map<std::string, std::string> Headers = DefaultHeaders ();
	std::map<std::string, std::string> Cookies;

	static std::map<std::string, std::string> DefaultHeaders () { return m_def_headers; }
	static void SetDefaultHeader (std::string _key, std::string _value) { m_def_headers [_key] = _value; }

private:
	inline static std::map<std::string, std::string> m_def_headers { { "User-Agent", "libfv-0.0.1" }, { "Accept-Encoding", "gzip" } };
};

struct Resp {
	int HttpCode = -1;
	std::string Content = "";
	std::string Error = "";
	std::map<std::string, std::string> Headers;
};



inline std::tuple<std::string, std::string, std::string> _parse_url (std::string _url) {
	std::string _schema = "http";
	size_t _p = _url.find ("://");
	if (_p != std::string::npos) {
		_schema = _url.substr (0, _p);
		_url = _url.substr (_p + 3);
	}
	_p = _url.find ("/");
	if (_p != std::string::npos) {
		return { _schema, _url.substr (0, _p), _url.substr (_p) };
	} else {
		return { _schema, _url, "/" };
	}
}

inline Task<Resp> Get (std::string _url, const std::chrono::system_clock::duration &_exp = std::chrono::milliseconds (0)) {
	co_return co_await Get (Req { .Url = _url }, _exp);
}

inline Task<Resp> Get (Req _r, const std::chrono::system_clock::duration &_exp) {
	struct req_data_t {
		std::atomic<int32_t> _index;
		asio_udp::resolver _resolver;
		asio_tcp::socket _socket;
		AsyncTimer _timer;
		req_data_t (boost::asio::io_context &_ctx): _resolver (_ctx), _socket (_ctx) { _index.store (0); }
	};
	auto _req_data = std::make_shared<req_data_t> (Tasks::GetContext ());
	_req_data->_timer.WaitCallback (_exp, std::function ([_wdata = std::weak_ptr (_req_data)] (TimerType _ty) -> Task<void> {
		if (_ty == TimerType::Timeout) {
			auto _data = _wdata.lock ();
			if (_data) {
				int _index = _data->_index.load ();
				if (_index == 0) {
					_data->_resolver.cancel ();
				} else if (_index <= 3) {
					if (_data->_socket.is_open ())
						_data->_socket.cancel ();
				}
			}
		}
		co_return;
	}));
	std::string _schema = "", _host = "", _path = "";

	try {
		// make data
		std::tie (_schema, _host, _path) = _parse_url (_r.Url);
		std::stringstream _ss;
		_ss << "GET " << _path << " HTTP/1.1\r\n";
		if (!_r.Headers.contains ("Host"))
			_r.Headers ["Host"] = _host;
		// TODO cookies -> headers
		for (auto &[_key, _value] : _r.Headers)
			_ss << _key << ": " << _value << "\r\n";
		_ss << "\r\n";

		// dns resolve
		_req_data->_index.store (0);
		asio_udp::resolver::results_type _resolve_data = co_await _req_data->_resolver.async_resolve (_host, use_awaitable);

		// connect
		_req_data->_index.store (1);
		uint16_t _port = _schema == "http" ? 80 : 443;
		auto _dest = asio_tcp::endpoint (boost::asio::ip::address::from_string (_host), _port);
		co_await _req_data->_socket.async_connect (_dest, use_awaitable);
		if (!_req_data->_socket.is_open ()) {
			_req_data->_timer.Cancel ();
			co_return Resp {};
		}

		// send
		_req_data->_index.store (2);
		std::string _data = _ss.str ();
		size_t _count = co_await _req_data->_socket.async_send (_data, use_awaitable);
		if (_count < _data.size ()) {
			_req_data->_timer.Cancel ();
			co_return Resp { .Error = std::format ("发送信息不完全，应发 {} 字节，实发 {} 字节。", _data.size (), _count) };
		}

		// recv
		_req_data->_index.store (3);
		// TODO

		// 结束
		_req_data->_index.store (4);
		_req_data->_timer.Cancel ();
		co_return Resp {};
	} catch (...) {
		_req_data->_timer.Cancel ();
		int _index = _req_data->_index.load ();
		const static std::map<int, std::string> s_err_msg { { 0, "域名解析失败" }, { 1, "无法建立链接" }, { 2, "发送信息超时" }, { 3, "接收信息超时" } };
		co_return Resp { .Error = s_err_msg.contains (_index) ? s_err_msg [_index] : "未知异常" };
	}
}

inline Task<Resp> Post (std::string _url, std::string _data, ContentType _ctype) {
	co_return Resp {};
}
}



#endif //__FV_HPP__
