#ifndef __FV_IMPL_HPP__
#define __FV_IMPL_HPP__



#include "fv.h"

#include <format>
#include <regex>
#include <sstream>

#ifndef ZLIB_CONST
#pragma warning (push)
#pragma warning (disable: 4068)
#include <gzip/config.hpp>
#include <gzip/compress.hpp>
#include <gzip/decompress.hpp>
#include <gzip/utils.hpp>
#include <gzip/version.hpp>
#pragma warning (pop)
#endif

#include <nlohmann/json.hpp>
#include <openssl/sha.h>



using boost::asio::socket_base;
using boost::asio::use_awaitable;



namespace fv {
SslCheckCb Config::SslVerifyFunc = [] (bool preverified, Ssl::verify_context &ctx) { return true; };
bool Config::NoDelay = false;



static std::string _to_lower (std::string _s) {
	std::transform (_s.begin (), _s.end (), _s.begin (), ::tolower);
	return _s;
}

static std::tuple<std::string, std::string, std::string, std::string> _parse_url (std::string _url) {
	size_t _p = _url.find ('#');
	if (_p != std::string::npos)
		_url = _url.substr (0, _p);
	std::string _schema = "http";
	_p = _url.find ("://");
	if (_p != std::string::npos) {
		_schema = _to_lower (_url.substr (0, _p));
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
		if (_schema == "http" || _schema == "ws") {
			_port = "80";
		} else if (_schema == "https" || _schema == "wss") {
			_port = "443";
		} else {
			throw Exception ("未知端口");
		}
	}
	return { _schema, _host, _port, _path };
}



bool CaseInsensitiveEqual::operator() (const std::string &str1, const std::string &str2) const noexcept {
	return str1.size () == str2.size () && std::equal (str1.begin (), str1.end (), str2.begin (), [] (char a, char b) {
		return tolower (a) == tolower (b);
	});
}



size_t CaseInsensitiveHash::operator() (const std::string &str) const noexcept {
	size_t h = 0;
	std::hash<int> hash {};
	for (auto c : str)
		h ^= hash (::tolower (c)) + 0x9e3779b9 + (h << 6) + (h >> 2);
	return h;
}



static std::string percent_encode (std::string_view data) {
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

static std::string base64_encode (std::string_view data) {
	static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string ret;
	int i = 0, j = 0;
	unsigned char char_3 [3], char_4 [4];
	unsigned int in_len = data.size ();
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

static std::string _trim (std::string _s) {
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



Task<void> AsyncSemaphore::AcquireAsync () {
	while (!m_sp.try_acquire ()) {
		boost::asio::steady_timer timer (co_await boost::asio::this_coro::executor);
		timer.expires_after (std::chrono::milliseconds (1));
		co_await timer.async_wait (use_awaitable);
	}
}
Task<bool> AsyncSemaphore::AcquireForAsync (TimeSpan _span) {
	co_return co_await AcquireUntilAsync (std::chrono::system_clock::now () + _span);
}
Task<bool> AsyncSemaphore::AcquireUntilAsync (std::chrono::system_clock::time_point _until) {
	while (!m_sp.try_acquire ()) {
		if (std::chrono::system_clock::now () >= _until)
			co_return false;
		boost::asio::steady_timer timer (co_await boost::asio::this_coro::executor);
		timer.expires_after (std::chrono::milliseconds (1));
		co_await timer.async_wait (use_awaitable);
	}
	co_return true;
}



Task<void> Tasks::Delay (TimeSpan _dt) {
	boost::asio::steady_timer timer (co_await boost::asio::this_coro::executor);
	timer.expires_after (_dt);
	co_await timer.async_wait (use_awaitable);
}

// 启动异步任务池
void Tasks::Start (bool _new_thread) {
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
void Tasks::Stop () {
	m_run.store (false);
	while (m_running.load ())
		std::this_thread::sleep_for (std::chrono::milliseconds (1));
}

IoContext Tasks::m_ctx { 1 };
std::atomic_bool Tasks::m_run = true, Tasks::m_running = false;



static std::string base64_encode (std::string data) {
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



Task<Request> Request::GetFromConn (std::shared_ptr<IConn> _conn, uint16_t _listen_port) {
	std::string _line = co_await _conn->ReadLine ();
	size_t _p = _line.find (' ');
	static std::unordered_map<std::string, MethodType> s_method_vals { { "HEAD", MethodType::Head }, { "OPTION", MethodType::Option }, { "GET", MethodType::Get }, { "POST", MethodType::Post }, { "PUT", MethodType::Put }, { "DELETE", MethodType::Delete } };
	std::string _tmp = _p != std::string::npos ? _line.substr (0, _p) : "";
	if (!s_method_vals.contains (_tmp))
		throw Exception ("Unrecognized request type");
	Request _r {};
	_r.Schema = dynamic_cast<SslConn *> (_conn.get ()) ? "https" : "http";
	_r.Method = s_method_vals [_tmp];
	_line = _line.erase (0, _p + 1);
	_p = _line.find (' ');
	if (_p == std::string::npos)
		throw Exception ("Unrecognized request path");
	_r.UrlPath = _line.substr (0, _p);
	while ((_line = co_await _conn->ReadLine ()) != "") {
		size_t _p = _line.find (':');
		std::string _key = _trim (_line.substr (0, _p));
		std::string _value = _trim (_line.substr (_p + 1));
		_r.Headers [_key] = _value;
	}
	_tmp = _to_lower (_r.Headers ["Connection"]);
	if (_tmp == "upgrade")
		_r.Schema = _r.Schema == "https" ? "wss" : "ws";
	std::string _port = "";
	if (!((_listen_port == 80 && (_r.Schema == "http" || _r.Schema == "ws")) || (_listen_port == 443 && (_r.Schema == "https" || _r.Schema == "wss"))))
		_port = std::format (":{}", _listen_port);
	_r.Url = std::format ("{}://{}{}{}", _r.Schema, _r.Headers ["Host"], _port, _r.UrlPath);
	if (_r.Headers.contains ("Content-Length")) {
		size_t _p = std::stoi (_r.Headers ["Content-Length"]);
		_r.Content = co_await _conn->ReadCount (_p);
	}
	_r.Conn = _conn;
	co_return _r;
}

std::string Request::Serilize (MethodType _method, std::string _host, std::string _path) {
	std::stringstream _ss;
	static std::unordered_map<MethodType, std::string> s_method_names { { MethodType::Head, "HEAD" }, { MethodType::Option, "OPTION" }, { MethodType::Get, "GET" }, { MethodType::Post, "POST" }, { MethodType::Put, "PUT" }, { MethodType::Delete, "DELETE" } };
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
				_ss1 << percent_encode (_data.Name) << '=' << percent_encode (_data.Value);
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

bool Request::IsWebsocket () {
	return _to_lower (Headers ["Connection"]) == "upgrade" && Headers ["Sec-WebSocket-Version"] == "13" && Headers ["Sec-WebSocket-Key"].size () > 0;
}

Task<std::shared_ptr<WsConn>> Request::UpgradeWebsocket () {
	if (!IsWebsocket ())
		throw Exception ("请求非 Websocket 请求，升级Websocket失败");
	Response _res = Response::FromUpgradeWebsocket (*this);
	std::string _res_str = _res.Serilize ();
	co_await Conn->Send (_res_str.data (), _res_str.size ());
	Upgrade = true;
	co_return std::make_shared<WsConn> (Conn, false);
}



bool Request::_content_raw_contains_files () {
	for (size_t i = 0; i < ContentRaw.size (); ++i) {
		if (ContentRaw [i].index () == 1) {
			return true;
		}
	}
	return false;
}



Task<Response> Response::GetFromConn (std::shared_ptr<IConn> _conn) {
	std::string _line = co_await _conn->ReadLine ();
	Response _r {};
	::sscanf_s (_line.data (), "HTTP/%*[0-9.] %d", &_r.HttpCode);
	if (_r.HttpCode == -1)
		throw Exception ("无法解析的标识：{}", _line);
	while ((_line = co_await _conn->ReadLine ()) != "") {
		size_t _p = _line.find (':');
		std::string _key = _trim (_line.substr (0, _p));
		std::string _value = _trim (_line.substr (_p + 1));
		_r.Headers [_key] = _value;
	}
	if (_r.Headers.contains ("Content-Length")) {
		int _sz = std::stoi (_r.Headers ["Content-Length"]);
		if (_sz > 0) {
			_r.Content = co_await _conn->ReadCount (_sz);
			if (_r.Headers.contains ("Content-Encoding")) {
				_line = _to_lower (_r.Headers ["Content-Encoding"]);
				if (_line == "gzip") {
					_r.Content = gzip::decompress (_r.Content.data (), _r.Content.size ());
				}
			}
		}
	}
	co_return _r;
}

Response Response::FromNotFound () {
	auto _res = Response { .HttpCode = 404, .Content = "404 Not Found" };
	Response::InitDefaultHeaders (_res.Headers);
	return _res;
}

Response Response::FromText (std::string _text) {
	auto _res = Response { .HttpCode = 200, .Content = _text };
	Response::InitDefaultHeaders (_res.Headers);
	return _res;
}

Response Response::FromUpgradeWebsocket (Request &_r) {
	auto _res = Response { .HttpCode = 101 };
	Response::InitDefaultHeaders (_res.Headers);
	std::string _tmp = std::format ("{}258EAFA5-E914-47DA-95CA-C5AB0DC85B11", _r.Headers ["Sec-WebSocket-Key"]);
	char _buf [20];
	::SHA1 ((const unsigned char *) _tmp.data (), _tmp.size (), (unsigned char *) _buf);
	_tmp = std::string (_buf, sizeof (_buf));
	_res.Headers ["Sec-WebSocket-Accept"] = base64_encode (_tmp);
	_res.Headers ["Connection"] = "Upgrade";
	_res.Headers ["Upgrade"] = _r.Headers ["Upgrade"];
	return _res;
}

std::string Response::Serilize () {
	std::string _cnt = Content;
	if (_cnt.size () > 0) {
		std::string _cnt_enc = _to_lower (Headers ["Content-Encoding"]);
		if (_cnt_enc == "gzip") {
			_cnt = gzip::compress (_cnt.data (), _cnt.size ());
		} else if (_cnt_enc != "") {
			throw Exception (std::format ("Unrecognized content encoding type [{}]", _cnt_enc));
		}
		Headers ["Content-Length"] = std::format ("{}", _cnt.size ());
	}

	std::stringstream _ss;
	std::unordered_map<int, std::string> s_httpcode { { 100, "Continue" }, { 101, "Switching Protocols" }, { 200, "OK" }, { 201, "Created" }, { 202, "Accepted" }, { 203, "Non-Authoritative Information" }, { 204, "No Content" }, { 205, "Reset Content" }, { 206, "Partial Content" }, { 300, "Multiple Choices" }, { 301, "Moved Permanently" }, { 302, "Found" }, { 303, "See Other" }, { 304, "Not Modified" }, { 305, "Use Proxy" }, { 306, "Unused" }, { 307, "Temporary Redirect" }, { 400, "Bad Request" }, { 401, "Unauthorized" }, { 402, "Payment Required" }, { 403, "Forbidden" }, { 404, "Not Found" }, { 405, "Method Not Allowed" }, { 406, "Not Acceptable" }, { 407, "Proxy Authentication Required" }, { 408, "Request Time-out" }, { 409, "Conflict" }, { 410, "Gone" }, { 411, "Length Required" }, { 412, "Precondition Failed" }, { 413, "Request Entity Too Large" }, { 414, "Request-URI Too Large" }, { 415, "Unsupported Media Type" }, { 416, "Requested range not satisfiable" }, { 417, "Expectation Failed" }, { 500, "Internal Server Error" }, { 501, "Not Implemented" }, { 502, "Bad Gateway" }, { 503, "Service Unavailable" }, { 504, "Gateway Time-out" }, { 505, "HTTP Version not supported" } };
	_ss << std::format ("HTTP/1.1 {} {}\r\n", HttpCode, s_httpcode.contains (HttpCode) ? s_httpcode [HttpCode] : "Unknown");
	for (auto [_key, _val] : Headers)
		_ss << _key << ": " << _val << "\r\n";
	_ss << "\r\n";
	_ss << _cnt;
	return _ss.str ();
}

void Response::InitDefaultHeaders (CaseInsensitiveMap &_map) {
	// TODO
}



Task<char> IConn::ReadChar () {
	if (TmpData.size () == 0) {
		char _buf [1024];
		size_t _n = co_await RecvImpl (_buf, sizeof (_buf));
		TmpData.resize (_n);
		::memcpy_s (&TmpData [0], _n, _buf, _n);
	}
	char _ret = TmpData [0];
	TmpData.erase (0);
	co_return _ret;
}

Task<std::string> IConn::ReadLine () {
	std::string _tmp = "";
	size_t _p;
	while ((_p = TmpData.find ('\n')) == std::string::npos) {
		char _buf [1024];
		size_t _readed = co_await RecvImpl (_buf, sizeof (_buf));
		TmpData += std::string_view { _buf, _readed };
	}
	_tmp = TmpData.substr (0, _p);
	if (_tmp [_p - 1] == '\r')
		_tmp.erase (_p - 1);
	TmpData.erase (0, _p + 1);
	co_return _tmp;
}

Task<std::string> IConn::ReadCount (size_t _count) {
	std::string _tmp = "";
	while (TmpData.size () < _count) {
		char _buf [1024];
		size_t _readed = co_await RecvImpl (_buf, sizeof (_buf));
		TmpData += std::string_view { _buf, _readed };
	}
	_tmp = TmpData.substr (0, _count);
	TmpData.erase (0, _count);
	co_return _tmp;
}

Task<std::vector<uint8_t>> IConn::ReadCountVec (size_t _count) {
	std::string _tmp = co_await ReadCount (_count);
	std::vector<uint8_t> _vec;
	_vec.assign ((uint8_t *) &_tmp [0], (uint8_t *) &_tmp [_count]);
	co_return _vec;
}

Task<std::string> IConn::ReadSome () {
	if (TmpData.size () > 0) {
		std::string _ret = std::move (TmpData);
		TmpData = "";
		co_return _ret;
	} else {
		char _buf [1024];
		size_t _len = co_await RecvImpl (_buf, sizeof (_buf));
		co_return std::string (_buf, _len);
	}
}



Task<void> TcpConn::Connect (std::string _host, std::string _port) {
	Close ();
	std::regex _r { "(\\d+\\.){3}\\d+" };
	if (std::regex_match (_host, _r)) {
		uint16_t _sport = (uint16_t) std::stoi (_port);
		co_await Socket.async_connect (Tcp::endpoint { boost::asio::ip::address::from_string (_host), _sport }, use_awaitable);
	} else {
		std::string _sport = std::format ("{}", _port);
		auto _it = co_await ResolverImpl.async_resolve (_host, _sport, use_awaitable);
		co_await Socket.async_connect (_it->endpoint (), use_awaitable);
	}
	if (!Socket.is_open ())
		throw Exception ("无法连接至目标服务器 {}", _host);
	if (Config::NoDelay)
		Socket.set_option (Tcp::no_delay { true });
}

void TcpConn::Close () {
	if (Socket.is_open ()) {
		Socket.shutdown (socket_base::shutdown_both);
		Socket.close ();
	}
}

Task<void> TcpConn::Send (char *_data, size_t _size) {
	if (!Socket.is_open ())
		throw Exception ("Cannot send data to a closed connection.");
	size_t _sended = 0;
	while (_sended < _size) {
		size_t _tmp_send = co_await Socket.async_send (boost::asio::buffer (&_data [_sended], _size - _sended), use_awaitable);
		if (_tmp_send == 0)
			throw Exception ("Connection temp closed.");
		_sended += _tmp_send;
	}
}

Task<size_t> TcpConn::RecvImpl (char *_data, size_t _size) {
	if (!Socket.is_open ())
		co_return 0;
	size_t _count = co_await Socket.async_read_some (boost::asio::buffer (_data, _size), use_awaitable);
	co_return _count;
}

void TcpConn::Cancel () {
	ResolverImpl.cancel ();
	Socket.cancel ();
}



TcpConn2::TcpConn2 (Tcp::socket _sock): Socket (std::move (_sock)) {
	if (Config::NoDelay)
		Socket.set_option (Tcp::no_delay { true });
}

void TcpConn2::Close () {
	if (Socket.is_open ()) {
		Socket.shutdown (socket_base::shutdown_both);
		Socket.close ();
	}
}

Task<void> TcpConn2::Send (char *_data, size_t _size) {
	if (!Socket.is_open ())
		throw Exception ("Cannot send data to a closed connection.");
	size_t _sended = 0;
	while (_sended < _size) {
		size_t _tmp_send = co_await Socket.async_send (boost::asio::buffer (&_data [_sended], _size - _sended), use_awaitable);
		if (_tmp_send == 0)
			throw Exception ("Connection temp closed.");
		_sended += _tmp_send;
	}
}

Task<size_t> TcpConn2::RecvImpl (char *_data, size_t _size) {
	if (!Socket.is_open ())
		co_return 0;
	size_t _count = co_await Socket.async_read_some (boost::asio::buffer (_data, _size), use_awaitable);
	co_return _count;
}



Task<void> SslConn::Connect (std::string _host, std::string _port) {
	Close ();
	SslSocket.set_verify_mode (Ssl::verify_peer);
	SslSocket.set_verify_callback (Config::SslVerifyFunc);
	std::regex _r { "(\\d+\\.){3}\\d+" };
	if (std::regex_match (_host, _r)) {
		uint16_t _sport = (uint16_t) std::stoi (_port);
		co_await SslSocket.next_layer ().async_connect (Tcp::endpoint { boost::asio::ip::address::from_string (_host), _sport }, use_awaitable);
	} else {
		std::string _sport = std::format ("{}", _port);
		auto _it = co_await ResolverImpl.async_resolve (_host, _sport, use_awaitable);
		co_await SslSocket.next_layer ().async_connect (_it->endpoint (), use_awaitable);
	}
	if (!SslSocket.next_layer ().is_open ())
		throw Exception ("Cannot connect to server {}", _host);
	if (Config::NoDelay)
		SslSocket.next_layer ().set_option (Tcp::no_delay { true });
	co_await SslSocket.async_handshake (Ssl::stream_base::client, use_awaitable);
}

void SslConn::Close () {
	if (SslSocket.next_layer ().is_open ()) {
		SslSocket.next_layer ().shutdown (socket_base::shutdown_both);
		SslSocket.next_layer ().close ();
	}
}

Task<void> SslConn::Send (char *_data, size_t _size) {
	if (!SslSocket.next_layer ().is_open ())
		throw Exception ("Cannot send data to a closed connection.");
	size_t _sended = 0;
	while (_sended < _size) {
		size_t _tmp_send = co_await SslSocket.async_write_some (boost::asio::buffer (&_data [_sended], _size - _sended), use_awaitable);
		if (_tmp_send == 0)
			throw Exception ("Connection temp closed.");
		_sended += _tmp_send;
	}
}

Task<size_t> SslConn::RecvImpl (char *_data, size_t _size) {
	co_return co_await SslSocket.async_read_some (boost::asio::buffer (_data, _size), use_awaitable);
}

void SslConn::Cancel () {
	ResolverImpl.cancel ();
	SslSocket.next_layer ().cancel ();
}



Task<std::tuple<std::string, WsType>> WsConn::Recv () {
	std::vector<uint8_t> _tmp = co_await Parent->ReadCountVec (2);
	bool _is_eof = (_tmp [0] & 0x80) != 0;
	WsType _type = (WsType) (_tmp [0] & 0xf);
	if (_type == WsType::Close) {
		Close ();
		throw Exception ("Remote send close msg.");
	} else if (_type == WsType::Ping) {
		co_await _Send (nullptr, 0, WsType::Pong);
		co_return co_await Recv ();
	} else if (_type == WsType::Pong) {
		co_return std::make_tuple (std::string (""), WsType::Pong);
	} else if (_type == WsType::Text || _type == WsType::Binary) {
		bool _mask = _tmp [1] & 0x80;
		long _payload_length = (long) (_tmp [1] & 0x7f);
		if (_payload_length == 126) {
			_tmp = co_await Parent->ReadCountVec (2);
			_payload_length = (((long) _tmp [0]) << 8) + _tmp [1];
		} else if (_payload_length == 127) {
			_tmp = co_await Parent->ReadCountVec (8);
			_payload_length = 0;
			for (int i = 0; i < 8; ++i)
				_payload_length = (_payload_length << 8) + _tmp [i];
		}
		if (_mask)
			_tmp = co_await Parent->ReadCountVec (4);
		std::string _s = co_await Parent->ReadCount ((size_t) _payload_length);
		if (_mask) {
			for (size_t i = 0; i < _s.size (); ++i)
				_s [i] ^= _tmp [i % 4];
		}
		if (_type == WsType::Continue) {
			auto [_s1, _type1] = co_await Recv ();
			_s += _s1;
			co_return std::make_tuple (_s, _type1);
		} else {
			co_return std::make_tuple (_s, _type);
		}
	} else {
		Parent = nullptr;
		throw Exception ("Unparsed websocket frame.");
	}
	co_return std::make_tuple (std::string (""), WsType::Close);
}

Task<void> WsConn::_Send (char *_data, size_t _size, WsType _type) {
	if (!IsConnect ()) {
		if (_type == WsType::Close)
			co_return;
		throw Exception ("Cannot send data to a closed connection.");
	}
	std::stringstream _ss;
	_ss << (char) (0x80 | (char) _type);
	static std::vector<char> _mask = { (char) 0xfa, (char) 0xfb, (char) 0xfc, (char) 0xfd };
	if (_size > 0) {
		if (_size < 126) {
			_ss << (char) (_size);
		} else if (_size < 0xffff) {
			_ss << (char) (126) << (char) ((_size >> 8) & 0xff) << (char) (_size & 0xff);
		} else {
			_ss << (char) (127);
			int64_t _size64 = (int64_t) _size;
			for (int i = 7; i >= 0; --i)
				_ss << (char) ((_size64 >> (i * 8)) & 0xff);
		}
		if (IsClient) {
			for (size_t i = 0; i < 4; ++i)
				_ss << _mask [i];
		}
		_ss << std::string_view { _data, _size };
	} else {
		_ss << '\0';
	}
	std::string _to_send = _ss.str ();
	if (IsClient) {
		for (size_t i = 6; i < _to_send.size (); ++i)
			_to_send [i] ^= _mask [i % 4];
	}
	if (_type == WsType::Close) {
		try {
			co_await Parent->Send (_to_send.data (), _to_send.size ());
		} catch (...) {
		}
	} else {
		co_await Parent->Send (_to_send.data (), _to_send.size ());
	}
}



Task<std::shared_ptr<IConn>> Connect (std::string _url) {
	auto [_schema, _host, _port, _path] = _parse_url (_url);
	//
	if (_schema == "tcp") {
		if (_path != "/")
			throw Exception ("url格式错误");
		auto _conn = std::shared_ptr<IConn> (new TcpConn { Tasks::GetContext () });
		co_await _conn->Connect (_host, _port);
		co_return _conn;
	} else {
		throw Exception ("未知协议：{}", _schema);
	}
}

Task<std::shared_ptr<WsConn>> ConnectWS (std::string _url) {
	auto [_schema, _host, _port, _path] = _parse_url (_url);
	if (_schema == "ws" || _schema == "wss") {
		// connect
		auto _conn = std::shared_ptr<IConn> (_schema == "ws" ?
			(IConn *) new TcpConn { Tasks::GetContext () } :
			new SslConn { Tasks::GetContext () });
		co_await _conn->Connect (_host, _port);

		// generate data
		Request _r { _url, MethodType::Get };
		_OptionApplys (_r, header ("Pragma", "no-cache"), connection ("Upgrade"), header ("Upgrade", "websocket"),
			header ("Sec-WebSocket-Version", "13"), header ("Sec-WebSocket-Key", "libfvlibfv=="));
		std::string _data = _r.Serilize (MethodType::Get, _host, _path);

		// send
		co_await _conn->Send (_data.data (), _data.size ());

		// recv
		co_await Response::GetFromConn (_conn);
		co_return std::make_shared<WsConn> (_conn, true);
	} else {
		throw Exception ("未知协议：{}", _schema);
	}
}



timeout::timeout (TimeSpan _exp): m_exp (_exp) {}
server::server (std::string _ip) : m_ip (_ip) {}
header::header (std::string _key, std::string _value) : m_key (_key), m_value (_value) {}
authorization::authorization (std::string _auth) : m_auth (_auth) {}
authorization::authorization (std::string _uid, std::string _pwd) : m_auth (std::format ("Basic {}", base64_encode (std::format ("{}:{}", _uid, _pwd)))) {}
connection::connection (std::string _co) : m_co (_co) {}
content_type::content_type (std::string _ct) : m_ct (_ct) {}
referer::referer (std::string _r) : m_r (_r) {}
user_agent::user_agent (std::string _ua) : m_ua (_ua) {}



Task<Response> DoMethod (Request _r) {
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



void TcpServer::SetOnConnect (std::function<Task<void> (std::shared_ptr<IConn>)> _on_connect) {
	OnConnect = _on_connect;
}

void TcpServer::RegisterClient (int64_t _id, std::shared_ptr<IConn> _conn) {
	std::unique_lock _ul { Mutex };
	Clients [_id] = _conn;
}

void TcpServer::UnregisterClient (int64_t _id, std::shared_ptr<IConn> _conn) {
	std::unique_lock _ul { Mutex };
	if (Clients [_id].get () == _conn.get ())
		Clients.erase (_id);
}

Task<bool> TcpServer::SendData (int64_t _id, char *_data, size_t _size) {
	try {
		std::unique_lock _ul { Mutex };
		if (Clients.contains (_id)) {
			auto _conn = Clients [_id];
			_ul.unlock ();
			co_await Clients [_id]->Send (_data, _size);
			co_return true;
		}
	} catch (...) {
	}
	co_return false;
}

Task<size_t> TcpServer::BroadcastData (char *_data, size_t _size) {
	std::unique_lock _ul { Mutex };
	std::unordered_set<std::shared_ptr<IConn>> _conns;
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

Task<void> TcpServer::Run (uint16_t _port) {
	if (IsRun.load ())
		co_return;
	IsRun.store (true);
	auto _executor = co_await boost::asio::this_coro::executor;
	Acceptor = std::make_unique<Tcp::acceptor> (_executor, Tcp::endpoint { Tcp::v4 (), _port }, true);
	try {
		for (; IsRun.load ();) {
			std::shared_ptr<IConn> _conn = std::shared_ptr<IConn> ((IConn *) new TcpConn2 (co_await Acceptor->async_accept (use_awaitable)));
			Tasks::RunAsync ([this, _conn] () -> Task<void> {
				try {
					co_await OnConnect (_conn);
				} catch (...) {
				}
			});
		}
	} catch (...) {
	}
}

void TcpServer::Stop () {
	IsRun.store (false);
	if (Acceptor)
		Acceptor->cancel ();
}



Task<void> HttpServer::Run (uint16_t _port) {
	m_tcpserver.SetOnConnect ([this, _port] (std::shared_ptr<IConn> _conn) -> Task<void> {
		while (true) {
			Request _req = co_await Request::GetFromConn (_conn, _port);
			if (m_before) {
				std::optional<Response> _ores = co_await m_before (_req);
				if (_ores.has_value ()) {
					if (m_after)
						co_await m_after (_req, _ores.value ());
					std::string _str_res = _ores.value ().Serilize ();
					co_await _conn->Send (_str_res.data (), _str_res.size ());
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
				}
			}
			if (_req.IsUpgraded ())
				break;
			if (_res.HttpCode == -1)
				_res = Response::FromNotFound ();
			if (m_after)
				co_await m_after (_req, _res);
			std::string _str_res = _res.Serilize ();
			co_await _conn->Send (_str_res.data (), _str_res.size ());
		}
	});
	co_await m_tcpserver.Run (_port);
}
}



#endif //__FV_IMPL_HPP__
