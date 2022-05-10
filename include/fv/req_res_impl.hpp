#ifndef __FV_REQ_RES_IMPL_HPP__
#define __FV_REQ_RES_IMPL_HPP__



#include <string>
#include <unordered_map>

#include <fmt/core.h>
#ifndef ZLIB_CONST
//#	pragma warning (push)
//#	pragma warning (disable: 4068)
#	include <gzip/config.hpp>
#	include <gzip/compress.hpp>
#	include <gzip/decompress.hpp>
#	include <gzip/utils.hpp>
#	include <gzip/version.hpp>
//#	pragma warning (pop)
#endif
#include <nlohmann/json.hpp>

#include "common.hpp"
#include "base.hpp"
#include "req_res.hpp"



namespace fv {
inline Task<Request> Request::GetFromConn (std::shared_ptr<IConn> _conn, uint16_t _listen_port) {
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
		_port = fmt::format (":{}", _listen_port);
	_r.Url = fmt::format ("{}://{}{}{}", _r.Schema, _r.Headers ["Host"], _port, _r.UrlPath);
	if (_r.Headers.contains ("Content-Length")) {
		size_t _p = std::stoi (_r.Headers ["Content-Length"]);
		_r.Content = co_await _conn->ReadCount (_p);
	}
	_r.Conn = _conn;
	co_return _r;
}

inline std::string Request::Serilize (MethodType _method, std::string _host, std::string _path) {
	std::stringstream _ss;
	static std::unordered_map<MethodType, std::string> s_method_names { { MethodType::Head, "HEAD" }, { MethodType::Option, "OPTION" }, { MethodType::Get, "GET" }, { MethodType::Post, "POST" }, { MethodType::Put, "PUT" }, { MethodType::Delete, "DELETE" } };
	_ss << s_method_names [_method] << " " << _path << " HTTP/1.1\r\n";
	if (!Headers.contains ("Host"))
		Headers ["Host"] = _host;
	if (Content.size () == 0 && ContentItems.size () > 0) {
		std::string _content_type = Headers ["Content-Type"], _boundary = "";
		if (_content_type == "") {
			// 生成新content type
			if (_content_raw_contains_files ()) {
				_boundary = fmt::format ("------libfv-{}", random_str (8));
				_content_type = fmt::format ("multipart/form-data; boundary={}", _boundary);
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
					throw Exception (fmt::format ("ContentType 为 {} 时，无法提交文件内容", _content_type));
				}
			} else {
				// 都行
			}
		}
		//
		std::stringstream _ss1;
		if (_boundary != "") {
			// multipart/form-data; boundary=
			for (auto _item : ContentItems) {
				_ss1 << _boundary << "\r\n";
				if (_item.index () == 0) {
					body_kv &_data = std::get<0> (_item);
					_ss1 << fmt::format ("Content-Disposition: form-data; name=\"{}\"\r\n", _data.Name);
					_ss1 << "\r\n";
					_ss1 << _data.Value << "\r\n";
				} else {
					body_file &_file = std::get<1> (_item);
					_ss1 << fmt::format ("Content-Disposition: form-data; name=\"{}\"; filename=\"{}\"\r\n", _file.Name, _file.FileName);
					_ss1 << "\r\n";
					_ss1 << _file.FileContent << "\r\n";
				}
			}
			_ss1 << _boundary << "--";
		} else if (_content_type == "application/json") {
			nlohmann::json _j;
			for (auto _item : ContentItems) {
				body_kv &_data = std::get<0> (_item);
				_j [_data.Name] = _data.Value;
			}
			_ss1 << _j.dump ();
		} else {
			// application/x-www-form-urlencoded
			for (size_t i = 0; i < ContentItems.size (); ++i) {
				if (i > 0)
					_ss1 << '&';
				body_kv &_data = std::get<0> (ContentItems [i]);
				_ss1 << percent_encode (_data.Name) << '=' << percent_encode (_data.Value);
			}
		}
		Content = _ss1.str ();
	}
	if (Content.size () > 0) {
		Headers ["Content-Length"] = fmt::format ("{}", Content.size ());
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

inline bool Request::IsWebsocket () {
	return _to_lower (Headers ["Connection"]) == "upgrade" && Headers ["Sec-WebSocket-Version"] == "13" && Headers ["Sec-WebSocket-Key"].size () > 0;
}

inline Task<std::shared_ptr<WsConn>> Request::UpgradeWebsocket () {
	if (!IsWebsocket ())
		throw Exception ("请求非 Websocket 请求，升级Websocket失败");
	Response _res = Response::FromUpgradeWebsocket (*this);
	std::string _res_str = _res.Serilize ();
	co_await Conn->Send (_res_str.data (), _res_str.size ());
	Upgrade = true;
	co_return std::make_shared<WsConn> (Conn, false);
}

inline bool Request::_content_raw_contains_files () {
	for (size_t i = 0; i < ContentItems.size (); ++i) {
		if (ContentItems [i].index () == 1) {
			return true;
		}
	}
	return false;
}



inline Task<Response> Response::GetFromConn (std::shared_ptr<IConn> _conn) {
	std::string _line = co_await _conn->ReadLine ();
	Response _r {};
	::sscanf_s (_line.data (), "HTTP/%*[0-9.] %d", &_r.HttpCode);
	if (_r.HttpCode == -1)
		throw Exception (fmt::format ("无法解析的标识：{}", _line));
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

inline Response Response::FromNotFound () {
	auto _res = Response { .HttpCode = 404, .Content = "404 Not Found" };
	Response::InitDefaultHeaders (_res.Headers);
	return _res;
}

inline Response Response::FromText (std::string _text) {
	auto _res = Response { .HttpCode = 200, .Content = _text };
	Response::InitDefaultHeaders (_res.Headers);
	return _res;
}

inline Response Response::FromUpgradeWebsocket (Request &_r) {
	auto _res = Response { .HttpCode = 101 };
	Response::InitDefaultHeaders (_res.Headers);
	std::string _tmp = fmt::format ("{}258EAFA5-E914-47DA-95CA-C5AB0DC85B11", _r.Headers ["Sec-WebSocket-Key"]);
	char _buf [20];
	::SHA1 ((const unsigned char *) _tmp.data (), _tmp.size (), (unsigned char *) _buf);
	_tmp = std::string (_buf, sizeof (_buf));
	_res.Headers ["Sec-WebSocket-Accept"] = base64_encode (_tmp);
	_res.Headers ["Connection"] = "Upgrade";
	_res.Headers ["Upgrade"] = _r.Headers ["Upgrade"];
	return _res;
}

inline std::string Response::Serilize () {
	std::string _cnt = Content;
	if (_cnt.size () > 0) {
		std::string _cnt_enc = _to_lower (Headers ["Content-Encoding"]);
		if (_cnt_enc == "gzip") {
			_cnt = gzip::compress (_cnt.data (), _cnt.size ());
		} else if (_cnt_enc != "") {
			throw Exception (fmt::format ("Unrecognized content encoding type [{}]", _cnt_enc));
		}
		Headers ["Content-Length"] = fmt::format ("{}", _cnt.size ());
	}

	std::stringstream _ss;
	std::unordered_map<int, std::string> s_httpcode { { 100, "Continue" }, { 101, "Switching Protocols" }, { 200, "OK" }, { 201, "Created" }, { 202, "Accepted" }, { 203, "Non-Authoritative Information" }, { 204, "No Content" }, { 205, "Reset Content" }, { 206, "Partial Content" }, { 300, "Multiple Choices" }, { 301, "Moved Permanently" }, { 302, "Found" }, { 303, "See Other" }, { 304, "Not Modified" }, { 305, "Use Proxy" }, { 306, "Unused" }, { 307, "Temporary Redirect" }, { 400, "Bad Request" }, { 401, "Unauthorized" }, { 402, "Payment Required" }, { 403, "Forbidden" }, { 404, "Not Found" }, { 405, "Method Not Allowed" }, { 406, "Not Acceptable" }, { 407, "Proxy Authentication Required" }, { 408, "Request Time-out" }, { 409, "Conflict" }, { 410, "Gone" }, { 411, "Length Required" }, { 412, "Precondition Failed" }, { 413, "Request Entity Too Large" }, { 414, "Request-URI Too Large" }, { 415, "Unsupported Media Type" }, { 416, "Requested range not satisfiable" }, { 417, "Expectation Failed" }, { 500, "Internal Server Error" }, { 501, "Not Implemented" }, { 502, "Bad Gateway" }, { 503, "Service Unavailable" }, { 504, "Gateway Time-out" }, { 505, "HTTP Version not supported" } };
	_ss << fmt::format ("HTTP/1.1 {} {}\r\n", HttpCode, s_httpcode.contains (HttpCode) ? s_httpcode [HttpCode] : "Unknown");
	for (auto [_key, _val] : Headers)
		_ss << _key << ": " << _val << "\r\n";
	_ss << "\r\n";
	_ss << _cnt;
	return _ss.str ();
}

inline void Response::InitDefaultHeaders (CaseInsensitiveMap &_map) {
	// TODO
}
}



#endif //__FV_REQ_RES_IMPL_HPP__
