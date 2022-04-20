#ifndef __FV_HPP__
#define __FV_HPP__



#include <cstdint>
#include <exception>
#include <format>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <tuple>
#include <variant>

#include "Tasks.hpp"



namespace fv {
enum class ContentType { KvPair, Json, Form };

struct Exception: public std::exception {
	Exception (std::string _err): m_err (_err) {}
	char const *what () const override { return m_err.c_str (); }

private:
	std::string m_err = "";
};



struct Request {
	std::string Url = "";
	std::string Content = "";
	std::map<std::string, std::string> Headers = DefaultHeaders ();
	std::map<std::string, std::string> Cookies;

	static std::map<std::string, std::string> DefaultHeaders () { return m_def_headers; }
	static void SetDefaultHeader (std::string _key, std::string _value) { m_def_headers [_key] = _value; }

private:
	inline static std::map<std::string, std::string> m_def_headers { { "User-Agent", "libfv-0.0.1" }, { "Accept-Encoding", "gzip" } };
};



struct Response {
	int HttpCode = -1;
	std::string Content = "";
	std::string Error = "";
	std::map<std::string, std::string> Headers;
};



struct Connection {
	asio_tcp::resolver ResolverImpl;
	asio_tcp::socket Socket;
	//std::optional<boost::asio::ssl::stream<asio_tcp::socket>> Ssl;

	Connection (boost::asio::io_context &_ctx): ResolverImpl (_ctx), Socket (_ctx) {}
	~Connection () { Cancel (); }

	Task<bool> Connect (std::string _host, uint16_t _port/*, bool _ssl*/) {
		std::regex _r { "(\\d+\\.){3}\\d+" };
		if (std::regex_match (_host, _r)) {
			co_await Socket.async_connect (asio_tcp::endpoint { boost::asio::ip::address::from_string (_host), _port }, use_awaitable);
		} else {
			asio_tcp::endpoint _endpoint = (co_await ResolverImpl.async_resolve (_host, use_awaitable))->endpoint ();
			co_await Socket.async_connect (asio_tcp::endpoint { _endpoint.address (), _port }, use_awaitable);
		}
		co_return Socket.is_open ();
	}

	Task<size_t> Send (char *_data, size_t _size) {
		if (!Socket.is_open ())
			co_return 0;
		size_t _sended = 0;
		while (_sended < _size) {
			size_t _tmp_send = co_await Socket.async_send (boost::asio::buffer (&_data [_sended], _size - _sended), use_awaitable);
			if (_tmp_send == 0)
				co_return _sended;
			_sended += _tmp_send;
		}
		co_return _sended;
	}

	Task<size_t> Recv (char *_data, size_t _size) {
		if (!Socket.is_open ())
			co_return 0;
		co_return co_await Socket.async_read_some (boost::asio::buffer (_data, _size), use_awaitable);
	}

	void Cancel () {
		ResolverImpl.cancel ();
		Socket.cancel ();
	}
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

inline Task<Response> Get (std::string _url, const std::chrono::system_clock::duration &_exp = std::chrono::milliseconds (0)) {
	co_return co_await Get (Request { .Url = _url }, _exp);
}

inline Task<Response> Get (Request _r, const std::chrono::system_clock::duration &_exp) {
	auto _conn = std::make_shared<Connection> (Tasks::GetContext ());
	AsyncTimer _timer {};
	_timer.WaitCallback (_exp, [_tconn = std::weak_ptr (_conn)] ()->Task<void> {
		auto _conn = _tconn.lock ();
		if (_conn)
			_conn->Cancel ();
	});

	auto [_schema, _host, _path] = _parse_url (_r.Url);
	uint16_t _port = _schema == "http" ? 80 : 443;
	try {
		// connect
		if (!co_await _conn->Connect (_host, _port))
			throw std::exception ("无法连接远程服务器");

		// generate data
		std::stringstream _ss;
		_ss << "GET " << _path << " HTTP/1.1\r\n";
		if (!_r.Headers.contains ("Host"))
			_r.Headers ["Host"] = _host;
		// TODO cookies -> headers
		for (auto &[_key, _value] : _r.Headers)
			_ss << _key << ": " << _value << "\r\n";
		_ss << "\r\n";

		// send
		std::string _data = _ss.str ();
		size_t _count = co_await _conn->Send (_data.data (), _data.size ());
		if (_count < _data.size ())
			throw Exception (std::format ("发送信息不完全，应发 {} 字节，实发 {} 字节。", _data.size (), _count));

		// recv
		char _buf [1024];
		// TODO

		// make
		// TODO
		co_return Response {};
	} catch (std::exception &_e) {
		_timer.Cancel ();
		co_return Response { .Error = _e.what () };
	} catch (...) {
		_timer.Cancel ();
		co_return Response { .Error = "未知异常" };
	}
}

inline Task<Response> Post (std::string _url, std::string _data, ContentType _ctype) {
	co_return Response {};
}
}



#endif //__FV_HPP__
