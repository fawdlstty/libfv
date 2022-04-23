#ifndef __FV_IMPL_HPP__
#define __FV_IMPL_HPP__



#include "fv.h"

#include <regex>

#include <nlohmann/json.hpp>

#pragma warning (push)
#pragma warning (disable: 4068)
#include <gzip/compress.hpp>
#include <gzip/config.hpp>
#include <gzip/decompress.hpp>
#include <gzip/utils.hpp>
#include <gzip/version.hpp>
#pragma warning (pop)

using boost::asio::socket_base;
using boost::asio::use_awaitable;



bool fv::CaseInsensitiveEqual::operator() (const std::string &str1, const std::string &str2) const noexcept {
	return str1.size () == str2.size () &&
		std::equal (str1.begin (), str1.end (), str2.begin (), [] (char a, char b) {
		return tolower (a) == tolower (b);
	});
}



size_t fv::CaseInsensitiveHash::operator() (const std::string &str) const noexcept {
	size_t h = 0;
	std::hash<int> hash {};
	for (auto c : str)
		h ^= hash (::tolower (c)) + 0x9e3779b9 + (h << 6) + (h >> 2);
	return h;
}



std::string percent_str_encode (std::string_view data) {
	const static char *hex_char = "0123456789ABCDEF";
	std::string ret = "";
	for (size_t i = 0; i < data.size (); ++i) {
		char ch = data [i];
		if (isalnum ((unsigned char) ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
			ret += ch;
		} else if (ch == ' ') {
			ret += "+";
		} else {
			ret += '%';
			ret += hex_char [((unsigned char) ch) >> 4];
			ret += hex_char [((unsigned char) ch) % 16];
		}
	}
	return ret;
}



Task<void> fv::AsyncSemaphore::AcquireAsync () {
	while (!m_sp.try_acquire ()) {
		boost::asio::steady_timer timer (co_await boost::asio::this_coro::executor);
		timer.expires_after (std::chrono::milliseconds (1));
		co_await timer.async_wait (use_awaitable);
	}
}
Task<bool> fv::AsyncSemaphore::AcquireForAsync (TimeSpan _span) {
	co_return co_await AcquireUntilAsync (std::chrono::system_clock::now () + _span);
}
Task<bool> fv::AsyncSemaphore::AcquireUntilAsync (std::chrono::system_clock::time_point _until) {
	while (!m_sp.try_acquire ()) {
		if (std::chrono::system_clock::now () >= _until)
			co_return false;
		boost::asio::steady_timer timer (co_await boost::asio::this_coro::executor);
		timer.expires_after (std::chrono::milliseconds (1));
		co_await timer.async_wait (use_awaitable);
	}
	co_return true;
}



Task<void> fv::Tasks::Delay (TimeSpan _dt) {
	boost::asio::steady_timer timer (co_await boost::asio::this_coro::executor);
	timer.expires_after (_dt);
	co_await timer.async_wait (use_awaitable);
}

// 启动异步任务池
void fv::Tasks::Start (bool _new_thread) {
	::srand ((unsigned int) ::time (NULL));
	static auto s_func = [] () {
		m_running.store (true);
		while (m_run.load ()) {
			if (m_ctx.run_one () == 0)
				std::this_thread::sleep_for (std::chrono::milliseconds (1));
		}
		m_running.store (false);
	};
	if (_new_thread) {
		std::thread ([] () { s_func (); }).detach ();
	} else {
		s_func ();
	}
}

// 停止异步任务池
void fv::Tasks::Stop () {
	m_run.store (false);
	while (m_running.load ())
		std::this_thread::sleep_for (std::chrono::milliseconds (1));
}

boost::asio::io_context fv::Tasks::m_ctx { 1 };
std::atomic_bool fv::Tasks::m_run = true, fv::Tasks::m_running = false;



std::string fv::base64_enc (std::string data) {
	static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string ret;
	int i = 0, j = 0;
	unsigned char char_3 [3], char_4 [4];
	size_t in_len = data.size ();
	unsigned char *bytes_to_encode = (unsigned char *) &data [0];
	while (in_len--) {
		char_3 [i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_4 [0] = (char_3 [0] & 0xfc) >> 2;
			char_4 [1] = ((char_3 [0] & 0x03) << 4) + ((char_3 [1] & 0xf0) >> 4);
			char_4 [2] = ((char_3 [1] & 0x0f) << 2) + ((char_3 [2] & 0xc0) >> 6);
			char_4 [3] = char_3 [2] & 0x3f;

			for (i = 0; i < 4; i++)
				ret += base64_chars [char_4 [i]];
			i = 0;
		}
	}
	if (i) {
		for (j = i; j < 3; j++)
			char_3 [j] = '\0';
		char_4 [0] = (char_3 [0] & 0xfc) >> 2;
		char_4 [1] = ((char_3 [0] & 0x03) << 4) + ((char_3 [1] & 0xf0) >> 4);
		char_4 [2] = ((char_3 [1] & 0x0f) << 2) + ((char_3 [2] & 0xc0) >> 6);
		for (j = 0; j < i + 1; j++)
			ret += base64_chars [char_4 [j]];
		while ((i++ < 3))
			ret += '=';
	}
	return ret;
}



static std::string random_str (size_t _len) {
	static const std::string s_chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	std::string _str = "";
	if (_len == 0 || _len == std::string::npos)
		return _str;
	_str.resize (_len);
	for (size_t i = 0; i < _len; ++i)
		_str [i] = s_chars [((size_t) ::rand ()) % s_chars.size ()];
	return _str;
}



std::string fv::Request::Serilize (Method _method, std::string _host, std::string _path) {
	std::stringstream _ss;
	static std::map<Method, std::string> s_method_names { { Method::Head, "HEAD" }, { Method::Option, "OPTION" }, { Method::Get, "GET" }, { Method::Post, "POST" }, { Method::Put, "PUT" }, { Method::Delete, "DELETE" } };
	_ss << s_method_names [_method] << " " << _path << " HTTP/1.1\r\n";
	if (!Headers.contains ("Host"))
		Headers ["Host"] = _host;
	if (Content.size () == 0 && ContentRaw.size () > 0) {
		std::string _content_type = Headers ["Content-Type"], _boundary = "";
		if (_content_type == "") {
			// 生成新content type
			if (_content_raw_contains_files ()) {
				_boundary = std::format ("------libfv-{}", random_str (8));
				_content_type = std::format ("multipart/form-data; boundary={}", _boundary);
			} else {
				_content_type = "application/json";
			}
			Headers ["Content-Type"] = _content_type;
		} else {
			// 校验content type是否合理
			if (_content_raw_contains_files ()) {
				if (_content_type.substr (0, 19) == "multipart/form-data") {
					size_t _p = _content_type.find ("boundary=");
					_boundary = _content_type.substr (_p + 9);
				} else {
					throw Exception (std::format ("ContentType 为 {} 时，无法提交文件内容", _content_type));
				}
			} else {
				// 都行
			}
		}
		//
		std::stringstream _ss1;
		if (_boundary != "") {
			// multipart/form-data; boundary=
			for (auto _item : ContentRaw) {
				_ss1 << _boundary << "\r\n";
				if (_item.index () == 0) {
					body_kv &_data = std::get<0> (_item);
					_ss1 << std::format ("Content-Disposition: form-data; name=\"{}\"\r\n", _data.Name);
					_ss1 << "\r\n";
					_ss1 << _data.Value << "\r\n";
				} else {
					body_file &_file = std::get<1> (_item);
					_ss1 << std::format ("Content-Disposition: form-data; name=\"{}\"; filename=\"{}\"\r\n", _file.Name, _file.FileName);
					_ss1 << "\r\n";
					_ss1 << _file.FileContent << "\r\n";
				}
			}
			_ss1 << _boundary << "--";
		} else if (_content_type == "application/json") {
			nlohmann::json _j;
			for (auto _item : ContentRaw) {
				body_kv &_data = std::get<0> (_item);
				_j [_data.Name] = _data.Value;
			}
			_ss1 << _j.dump ();
		} else {
			// application/x-www-form-urlencoded
			for (size_t i = 0; i < ContentRaw.size (); ++i) {
				if (i > 0)
					_ss1 << '&';
				body_kv &_data = std::get<0> (ContentRaw [i]);
				_ss1 << percent_str_encode (_data.Name) << '=' << percent_str_encode (_data.Value);
			}
		}
		Content = _ss1.str ();
	}
	if (Content.size () > 0) {
		Headers ["Content-Length"] = std::format ("{}", Content.size ());
		if (!Headers.contains ("Content-Type"))
			Headers ["Content-Type"] = Content [0] == '{' ? "application/json" : "application/x-www-form-urlencoded";
	}
	// TODO cookies -> headers
	for (auto &[_key, _value] : Headers)
		_ss << _key << ": " << _value << "\r\n";
	_ss << "\r\n";
	_ss << Content;
	return _ss.str ();
}



bool fv::Request::_content_raw_contains_files () {
	for (size_t i = 0; i < ContentRaw.size (); ++i) {
		if (ContentRaw [i].index () == 1) {
			return true;
		}
	}
	return false;
}



Task<std::tuple<fv::Response, std::string>> fv::Response::GetResponse (std::function<Task<size_t> (char *, size_t)> _cb) {
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
	static std::function<std::string (std::string)> _trim = [] (std::string _s) {
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
		int _sz = std::stoi (_r.Headers ["Content-Length"]);
		if (_sz > 0) {
			_r.Content = co_await _read_count (_sz);
			if (_r.Headers.contains ("Content-Encoding")) {
				_line = _r.Headers ["Content-Encoding"];
				std::transform (_line.begin (), _line.end (), _line.begin (), ::tolower);
				if (_line == "gzip") {
					_r.Content = gzip::decompress (_r.Content.data (), _r.Content.size ());
				}
			}
		}
	}
	co_return std::make_tuple (_r, _data);
}



Task<void> fv::TcpConnection::Connect (std::string _host, std::string _port, bool _nodelay) {
	Close ();
	std::regex _r { "(\\d+\\.){3}\\d+" };
	if (std::regex_match (_host, _r)) {
		uint16_t _sport = (uint16_t) std::stoi (_port);
		co_await Socket.async_connect (asio_tcp::endpoint { boost::asio::ip::address::from_string (_host), _sport }, use_awaitable);
	} else {
		std::string _sport = std::format ("{}", _port);
		auto _it = co_await ResolverImpl.async_resolve (_host, _sport, use_awaitable);
		co_await Socket.async_connect (_it->endpoint (), use_awaitable);
	}
	if (!Socket.is_open ())
		throw Exception ("无法连接至目标服务器 {}", _host);
	if (_nodelay)
		Socket.set_option (asio_tcp::no_delay { _nodelay });
}

void fv::TcpConnection::Close () {
	if (Socket.is_open ()) {
		Socket.shutdown (socket_base::shutdown_both);
		Socket.close ();
	}
}

Task<size_t> fv::TcpConnection::Send (char *_data, size_t _size) {
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

Task<size_t> fv::TcpConnection::Recv (char *_data, size_t _size) {
	if (!Socket.is_open ())
		co_return 0;
	size_t _count = co_await Socket.async_read_some (boost::asio::buffer (_data, _size), use_awaitable);
	co_return _count;
}

void fv::TcpConnection::Cancel () {
	ResolverImpl.cancel ();
	Socket.cancel ();
}



Task<void> fv::SslConnection::Connect (std::string _host, std::string _port, bool _nodelay) {
	Close ();
	SslSocket.set_verify_mode (asio_ssl::verify_peer);
	SslSocket.set_verify_callback ([] (bool preverified, asio_ssl::verify_context &ctx) {
		return true;
	});
	std::regex _r { "(\\d+\\.){3}\\d+" };
	if (std::regex_match (_host, _r)) {
		uint16_t _sport = (uint16_t) std::stoi (_port);
		co_await SslSocket.next_layer ().async_connect (asio_tcp::endpoint { boost::asio::ip::address::from_string (_host), _sport }, use_awaitable);
	} else {
		std::string _sport = std::format ("{}", _port);
		auto _it = co_await ResolverImpl.async_resolve (_host, _sport, use_awaitable);
		co_await SslSocket.next_layer ().async_connect (_it->endpoint (), use_awaitable);
	}
	if (!SslSocket.next_layer ().is_open ())
		throw Exception ("无法连接至目标服务器 {}", _host);
	if (_nodelay)
		SslSocket.next_layer ().set_option (asio_tcp::no_delay { _nodelay });
	//// TODO 握手
	co_await SslSocket.async_handshake (asio_ssl::stream_base::client, use_awaitable);
}

void fv::SslConnection::Close () {
	if (SslSocket.next_layer ().is_open ()) {
		SslSocket.next_layer ().shutdown (socket_base::shutdown_both);
		SslSocket.next_layer ().close ();
	}
}

Task<size_t> fv::SslConnection::Send (char *_data, size_t _size) {
	if (!SslSocket.next_layer ().is_open ())
		co_return 0;
	size_t _sended = 0;
	while (_sended < _size) {
		size_t _tmp_send = co_await SslSocket.async_write_some (boost::asio::buffer (&_data [_sended], _size - _sended), use_awaitable);
		if (_tmp_send == 0)
			co_return _sended;
		_sended += _tmp_send;
	}
	co_return _sended;
}

Task<size_t> fv::SslConnection::Recv (char *_data, size_t _size) {
	co_return co_await SslSocket.async_read_some (boost::asio::buffer (_data, _size), use_awaitable);
}

void fv::SslConnection::Cancel () {
	ResolverImpl.cancel ();
	SslSocket.next_layer ().cancel ();
}



fv::timeout::timeout (TimeSpan _exp): m_exp (_exp) {}
fv::server::server (std::string _ip): m_ip (_ip) {}
fv::no_delay::no_delay (bool _nd): m_nd (_nd) {}
fv::header::header (std::string _key, std::string _value): m_key (_key), m_value (_value) {}
fv::authorization::authorization (std::string _auth): m_auth (_auth) {}
fv::authorization::authorization (std::string _uid, std::string _pwd): m_auth (std::format ("Basic {}", base64_enc (std::format ("{}:{}", _uid, _pwd)))) {}
fv::connection::connection (std::string _co): m_co (_co) {}
fv::content_type::content_type (std::string _ct): m_ct (_ct) {}
fv::referer::referer (std::string _r): m_r (_r) {}
fv::user_agent::user_agent (std::string _ua): m_ua (_ua) {}



static std::tuple<std::string, std::string, std::string, std::string> _parse_url (std::string _url) {
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



Task<fv::Response> fv::DoMethod (Request _r, Method _method) {
	auto [_schema, _host, _port, _path] = _parse_url (_r.Url);
	std::shared_ptr<IConn> _conn;
	if (_schema == "https") {
		_conn = std::shared_ptr<IConn> (new SslConnection { Tasks::GetContext () });
	} else {
		_conn = std::shared_ptr<IConn> (new TcpConnection { Tasks::GetContext () });
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
	co_await _conn->Connect (_r.Server != "" ? _r.Server : _host, _port, _r.NoDelay);

	// generate data
	std::string _data = _r.Serilize (_method, _host, _path);

	// send
	size_t _count = co_await _conn->Send (_data.data (), _data.size ());
	if (_count < _data.size ())
		throw Exception ("发送信息不完全，应发 {} 字节，实发 {} 字节。", _data.size (), _count);

	// recv
	auto [_ret, _next_data] = co_await Response::GetResponse ([_conn] (char *_data, size_t _size) ->Task<size_t> {
		co_return co_await _conn->Recv (_data, _size);
	});
	co_return _ret;
}



#endif //__FV_IMPL_HPP__
