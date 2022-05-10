#ifndef __FV_BASE_HPP__
#define __FV_BASE_HPP__



#include <string>

#include "common.hpp"
#include "common_funcs.hpp"



namespace fv {
enum class MethodType { Head, Option, Get, Post, Put, Delete };
enum class WsType { Continue = 0, Text = 1, Binary = 2, Close = 8, Ping = 9, Pong = 10 };



struct Config {
	inline static SslCheckCb SslVerifyFunc = [] (bool preverified, Ssl::verify_context &ctx) { return true; };
	inline static bool NoDelay = false;
};



struct timeout { TimeSpan m_exp; timeout (TimeSpan _exp): m_exp (_exp) {} };
struct server { std::string m_ip; server (std::string _ip): m_ip (_ip) {} };
struct header {
	std::string m_key, m_value;
	header (std::string _key, std::string _value): m_key (_key), m_value (_value) {}
};
struct authorization: public header {
	authorization (std::string _auth): header ("Authorization", _auth) {}
	authorization (std::string _uid, std::string _pwd): header ("Authorization", fmt::format ("Basic {}", base64_encode (fmt::format ("{}:{}", _uid, _pwd)))) {}
};
struct connection: public header { connection (std::string _co): header ("Connection", _co) {} };
struct content_type: public header { content_type (std::string _co): header ("Content-Type", _co) {} };
struct referer: public header { referer (std::string _r): header ("Referer", _r) {} };
struct user_agent: public header { user_agent (std::string _ua): header ("User-Agent", _ua) {} };
struct url_kv {
	std::string Name, Value;
	url_kv (std::string _name, std::string _value): Name (_name), Value (_value) {}
};
struct body_kv {
	std::string Name, Value;
	body_kv (std::string _name, std::string _value): Name (_name), Value (_value) {}
};
struct body_file {
	std::string Name, FileName, FileContent;
	body_file (std::string _name, std::string _filename, std::string _content): Name (_name), FileName (_filename), FileContent (_content) {}
};
struct body_raw {
	std::string ContentType, Content;
	body_raw (std::string _content_type, std::string _content): ContentType (_content_type), Content (_content) {}
};
template<typename T>
concept TOption = std::is_same<T, timeout>::value || std::is_same<T, server>::value ||
std::is_same<T, header>::value || std::is_same<T, authorization>::value || std::is_same<T, connection>::value ||
std::is_same<T, content_type>::value || std::is_same<T, referer>::value || std::is_same<T, user_agent>::value ||
std::is_same<T, url_kv>::value;
template<typename T>
concept TFormOption = std::is_same<T, timeout>::value || std::is_same<T, server>::value ||
std::is_same<T, header>::value || std::is_same<T, authorization>::value || std::is_same<T, connection>::value ||
std::is_same<T, content_type>::value || std::is_same<T, referer>::value || std::is_same<T, user_agent>::value ||
std::is_same<T, url_kv>::value || std::is_same<T, body_kv>::value || std::is_same<T, body_file>::value;
}



#endif //__FV_BASE_HPP__
