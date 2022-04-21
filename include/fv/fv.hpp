#ifndef __FV_HPP__
#define __FV_HPP__



#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <tuple>
#include <variant>

#include "Tasks.hpp"

#pragma warning (push)
#pragma warning (disable: 4068)

#include <gzip/compress.hpp>
#include <gzip/config.hpp>
#include <gzip/decompress.hpp>
#include <gzip/utils.hpp>
#include <gzip/version.hpp>

#pragma warning (pop)



using boost::asio::socket_base;



namespace fv {
enum class ContentType { KvPair, Json, Form };
class CaseInsensitiveEqual {
public:
	bool operator() (const std::string &str1, const std::string &str2) const noexcept {
		return str1.size () == str2.size () &&
			std::equal (str1.begin (), str1.end (), str2.begin (), [] (char a, char b) {
			return tolower (a) == tolower (b);
		});
	}
};
// Based on https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x/2595226#2595226
class CaseInsensitiveHash {
public:
	size_t operator() (const std::string &str) const noexcept {
		size_t h = 0;
		std::hash<int> hash {};
		for (auto c : str)
			h ^= hash (::tolower (c)) + 0x9e3779b9 + (h << 6) + (h >> 2);
		return h;
	}
};
using CaseInsensitiveMap = std::unordered_map<std::string, std::string, CaseInsensitiveHash, CaseInsensitiveEqual>;

struct Exception: public std::exception {
	Exception (std::string _err): m_err (_err) {}
	template<typename ...Args>
	Exception (std::string _err, Args ..._args): m_err (std::format (_err, _args...)) {}
	char const *what () const override { return m_err.c_str (); }

private:
	std::string m_err = "";
};



struct Request {
	std::string Url = "";
	std::string Content = "";
	CaseInsensitiveMap Headers = DefaultHeaders ();
	std::map<std::string, std::string> Cookies;

	static CaseInsensitiveMap DefaultHeaders () { return m_def_headers; }
	static void SetDefaultHeader (std::string _key, std::string _value) { m_def_headers [_key] = _value; }

private:
	inline static CaseInsensitiveMap m_def_headers { { "User-Agent", "libfv-0.0.1" }, { "Accept-Encoding", "gzip" } };
};



struct Response {
	int HttpCode = -1;
	std::string Content = "";
	std::string Error = "";
	CaseInsensitiveMap Headers;
};



struct IConn {
	IConn () = default;
	virtual ~IConn () = default;
	virtual Task<void> Connect (std::string _host, std::string _port, bool _delay) = 0;
	virtual void Close () = 0;
	virtual Task<size_t> Send (char *_data, size_t _size) = 0;
	virtual Task<size_t> Recv (char *_data, size_t _size) = 0;
	virtual void Cancel () = 0;

	static Task<std::shared_ptr<IConn>> Create (std::string _url);
};



struct TcpConnection: IConn {
	asio_tcp::resolver ResolverImpl;
	asio_tcp::socket Socket;

	TcpConnection (boost::asio::io_context &_ctx): ResolverImpl (_ctx), Socket (_ctx) {}

	virtual ~TcpConnection () { Cancel (); }

	Task<void> Connect (std::string _host, std::string _port, bool _delay) override {
		Close ();
		std::regex _r { "(\\d+\\.){3}\\d+" };
		try {
			if (std::regex_match (_host, _r)) {
				uint16_t _sport = (uint16_t) std::stoi (_port);
				co_await Socket.async_connect (asio_tcp::endpoint { boost::asio::ip::address::from_string (_host), _sport }, use_awaitable);
			} else {
				std::string _sport = std::format ("{}", _port);
				auto _it = co_await ResolverImpl.async_resolve (_host, _sport, use_awaitable);
				co_await Socket.async_connect (_it->endpoint (), use_awaitable);
			}
		} catch (Exception &) {
			throw;
		} catch (...) {
		}
		if (!Socket.is_open ())
			throw Exception ("无法连接至目标服务器 {}", _host);
		if (!_delay)
			Socket.set_option (asio_tcp::no_delay { true });
	}

	void Close () override {
		if (Socket.is_open ()) {
			Socket.shutdown (socket_base::shutdown_both);
			Socket.close ();
		}
	}

	Task<size_t> Send (char *_data, size_t _size) override {
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

	Task<size_t> Recv (char *_data, size_t _size) override {
		if (!Socket.is_open ())
			co_return 0;
		size_t _count = co_await Socket.async_read_some (boost::asio::buffer (_data, _size), use_awaitable);
		co_return _count;
	}

	void Cancel () override {
		ResolverImpl.cancel ();
		Socket.cancel ();
	}
};



//struct SslConnection: IConn {
//	asio_tcp::resolver ResolverImpl;
//	asio_ssl::context SslCtx { asio_ssl::context::tls };
//	asio_ssl::stream<asio_tcp::socket> SslStream;
//
//	SslConnection (boost::asio::io_context &_ctx): ResolverImpl (_ctx), SslStream (_ctx, SslCtx) {}
//
//	virtual ~SslConnection () { Cancel (); }
//
//	Task<void> Connect (std::string _host, std::string _port, bool _delay) override {
//		Close ();
//		std::regex _r { "(\\d+\\.){3}\\d+" };
//		try {
//			if (std::regex_match (_host, _r)) {
//				uint16_t _sport = (uint16_t) std::stoi (_port);
//				co_await SslStream.next_layer ().async_connect (asio_tcp::endpoint { boost::asio::ip::address::from_string (_host), _sport }, use_awaitable);
//			} else {
//				std::string _sport = std::format ("{}", _port);
//				auto _it = co_await ResolverImpl.async_resolve (_host, _sport, use_awaitable);
//				co_await SslStream.next_layer ().async_connect (_it->endpoint (), use_awaitable);
//			}
//		} catch (Exception &) {
//			throw;
//		} catch (...) {
//		}
//		if (!SslStream.next_layer ().is_open ())
//			throw Exception ("无法连接至目标服务器 {}", _host);
//		if (!_delay)
//			SslStream.next_layer ().set_option (asio_tcp::no_delay { true });
//		// TODO 握手
//	}
//
//	void Close () override {}
//
//	Task<size_t> Send (char *_data, size_t _size) override {
//		auto _ret = co_await SslStream.async_write_some (boost::asio::buffer (_data, _size), use_awaitable);
//		//if (!Socket.is_open ())
//		//	co_return 0;
//		//size_t _sended = 0;
//		//while (_sended < _size) {
//		//	size_t _tmp_send = co_await Socket.async_send (boost::asio::buffer (&_data [_sended], _size - _sended), use_awaitable);
//		//	if (_tmp_send == 0)
//		//		co_return _sended;
//		//	_sended += _tmp_send;
//		//}
//		//co_return _sended;
//	}
//
//	Task<size_t> Recv (char *_data, size_t _size) override {}
//
//	void Cancel () override {}
//};



struct HttpReader {
	static Task<std::tuple<Response, std::string>> GetResponse (std::function<Task<size_t> (char *, size_t)> _cb) {
		std::string _data = "";
		char _buf [1024];
		size_t _p;
		auto _read_line = [&] () -> Task<std::string> {
			std::string _tmp = "";
			while ((_p = _data.find ('\n')) == std::string::npos) {
				size_t _readed = co_await _cb (_buf, 1024);
				_data += std::string_view { _buf, _readed };
			}
			_tmp = _data.substr (0, _p);
			if (_tmp [_p - 1] == '\r')
				_tmp.erase (_p - 1);
			_data.erase (0, _p + 1);
			co_return _tmp;
		};
		auto _read_count = [&] (size_t _count) ->Task<std::string> {
			std::string _tmp = "";
			while (_data.size () < _count) {
				size_t _readed = co_await _cb (_buf, 1024);
				_data += std::string_view { _buf, _readed };
			}
			_tmp = _data.substr (0, _count);
			_data.erase (0, _count);
			co_return _tmp;
		};
		//
		std::string _line = co_await _read_line ();
		Response _r {};
		::sscanf_s (_line.data (), "HTTP/%*[0-9.] %d", &_r.HttpCode);
		if (_r.HttpCode == -1)
			throw Exception ("无法解析的标识：{}", _line);
		auto _trim = [] (std::string _s) {
			size_t _start = 0, _stop = _s.size ();
			while (_start < _stop) {
				char _ch = _s [_start];
				if (_ch != ' ' && _ch != '　' && _ch != '\r')
					break;
				_start++;
			}
			while (_start < _stop) {
				char _ch = _s [_stop - 1];
				if (_ch != ' ' && _ch != '　' && _ch != '\r')
					break;
				_stop--;
			}
			return _s.substr (_start, _stop - _start);
		};
		while ((_line = co_await _read_line ()) != "") {
			_p = _line.find (':');
			std::string _key = _trim (_line.substr (0, _p));
			std::string _value = _trim (_line.substr (_p + 1));
			_r.Headers [_key] = _value;
		}
		if (_r.Headers.contains ("Content-Length")) {
			size_t _sz = (size_t) std::stoi (_r.Headers ["Content-Length"]);
			_r.Content = co_await _read_count (_sz);
		}
		if (_r.Headers.contains ("Content-Encoding")) {
			_line = _r.Headers ["Content-Encoding"];
			std::transform (_line.begin (), _line.end (), _line.begin (), ::tolower);
			if (_line == "gzip") {
				_r.Content = gzip::decompress (_r.Content.data (), _r.Content.size ());
			}
		}
		co_return std::make_tuple (_r, _data);
	}
};



inline std::tuple<std::string, std::string, std::string, std::string> _parse_url (std::string _url) {
	std::string _schema = "http";
	size_t _p = _url.find ("://");
	if (_p != std::string::npos) {
		_schema = _url.substr (0, _p);
		std::transform (_schema.begin (), _schema.end (), _schema.begin (), ::tolower);
		_url = _url.substr (_p + 3);
	}
	//
	_p = _url.find ('/');
	std::string _path = "/";
	if (_p != std::string::npos) {
		_path = _url.substr (_p);
		_url = _url.substr (0, _p);
	}
	//
	_p = _url.find (':');
	std::string _host = "", _port = "";
	if (_p != std::string::npos) {
		_host = _url.substr (0, _p);
		_port = _url.substr (_p + 1);
	} else {
		_host = _url;
		_port = _schema == "http" ? "80" : "443";
	}
	return { _schema, _host, _port, _path };
}

inline Task<Response> Get (Request _r, const std::chrono::system_clock::duration &_exp) {
	auto _conn = std::shared_ptr<IConn> (new TcpConnection { Tasks::GetContext () });
	AsyncTimer _timer {};
	if (std::chrono::duration_cast<std::chrono::nanoseconds> (_exp).count () > 0) {
		_timer.WaitCallback (_exp, [_tconn = std::weak_ptr (_conn)] ()->Task<void> {
			auto _conn = _tconn.lock ();
			if (_conn)
				_conn->Cancel ();
		});
	}

	auto [_schema, _host, _port, _path] = _parse_url (_r.Url);
	try {
		// connect
		co_await _conn->Connect (_host, _port, false);

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
			throw Exception ("发送信息不完全，应发 {} 字节，实发 {} 字节。", _data.size (), _count);

		// recv
		auto [_ret, _next_data] = co_await HttpReader::GetResponse ([_conn] (char *_data, size_t _size) ->Task<size_t> {
			co_return co_await _conn->Recv (_data, _size);
		});
		co_return _ret;
	} catch (std::exception &_e) {
		_timer.Cancel ();
		co_return Response { .Error = _e.what () };
	} catch (...) {
		_timer.Cancel ();
		co_return Response { .Error = "未知异常" };
	}
}

inline Task<Response> Get (std::string _url, const std::chrono::system_clock::duration &_exp = std::chrono::milliseconds (0)) {
	co_return co_await Get (Request { .Url = _url }, _exp);
}

inline Task<Response> Post (std::string _url, std::string _data, ContentType _ctype) {
	co_return Response {};
}

inline Task<std::shared_ptr<IConn>> IConn::Create (std::string _url) {
	size_t _p = _url.find ("://");
	if (_p == std::string::npos)
		throw Exception ("URL格式错误：{}", _url);
	std::string _schema = _url.substr (0, _p);
	_p += 3;
	//
	std::string _host = "", _port = "";
	size_t _q = _url.find (':', _p);
	if (_q != std::string::npos) {
		_host = _url.substr (_p, _q);
		_port = _url.substr (_q + 1);
	} else {
		_host = _url.substr (_p);
		_port = _schema == "ws" ? "80" : ("wss" ? "443" : "");
		if (_port == "")
			throw Exception ("URL格式错误：{}", _url);
	}
	//
	if (_schema == "tcp") {
		auto _conn = std::shared_ptr<IConn> (new TcpConnection { Tasks::GetContext () });
		co_await _conn->Connect (_host, _port, false);
		co_return _conn;
	} else if (_schema == "udp") {
		throw Exception ("暂不支持的协议：{}", _schema);
	} else if (_schema == "ssl") {
		throw Exception ("暂不支持的协议：{}", _schema);
	} else if (_schema == "quic") {
		throw Exception ("暂不支持的协议：{}", _schema);
	} else if (_schema == "ws") {
		throw Exception ("暂不支持的协议：{}", _schema);
	} else if (_schema == "wss") {
		throw Exception ("暂不支持的协议：{}", _schema);
	} else {
		throw Exception ("未知协议：{}", _schema);
	}
}
}



#endif //__FV_HPP__
