#ifndef __FV_H__
#define __FV_H__



#include <chrono>
#include <map>
#include <semaphore>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

#define BOOST_ASIO_HAS_CO_AWAIT
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>



namespace fv {
#define Task boost::asio::awaitable
using Tcp = boost::asio::ip::tcp;
using Udp = boost::asio::ip::udp;
namespace Ssl = boost::asio::ssl;
using IoContext = boost::asio::io_context;
using TimeSpan = std::chrono::system_clock::duration;
using SslCheckCb = std::function<bool (bool, Ssl::verify_context &)>;

enum class Method { Head, Option, Get, Post, Put, Delete };

struct CaseInsensitiveHash { size_t operator() (const std::string &str) const noexcept; };
struct CaseInsensitiveEqual { bool operator() (const std::string &str1, const std::string &str2) const noexcept; };
using CaseInsensitiveMap = std::unordered_map<std::string, std::string, CaseInsensitiveHash, CaseInsensitiveEqual>;



struct Config {
	static SslCheckCb SslVerifyFunc;
	static bool NoDelay;
};



struct AsyncSemaphore {
	AsyncSemaphore (int _init_count): m_sp (_init_count) {}
	void Release () { m_sp.release (); }
	bool TryAcquire () { return m_sp.try_acquire (); }
	void Acquire () { m_sp.acquire (); }
	Task<void> AcquireAsync ();
	Task<bool> AcquireForAsync (TimeSpan _span);
	Task<bool> AcquireUntilAsync (std::chrono::system_clock::time_point _until);

private:
	std::counting_semaphore<> m_sp;
};

struct CancelToken {
	CancelToken (std::chrono::system_clock::time_point _cancel_time) { m_cancel_time = _cancel_time; }
	CancelToken (TimeSpan _expire) { m_cancel_time = std::chrono::system_clock::now () + _expire; }
	void Cancel () { m_cancel_time = std::chrono::system_clock::now (); }
	bool IsCancel () { return std::chrono::system_clock::now () <= m_cancel_time; }
	TimeSpan GetRemaining () { return m_cancel_time - std::chrono::system_clock::now (); }

private:
	std::chrono::system_clock::time_point m_cancel_time;
};

struct Tasks {
	template<typename F>
	static void RunAsync (F &&f) {
		using TRet = decltype (f ());
		if constexpr (std::is_void<TRet>::value) {
			m_ctx.post (f);
		} else if constexpr (std::is_same<TRet, Task<void>>::value) {
			boost::asio::co_spawn (m_ctx, f, boost::asio::detached);
		} else {
			static_assert (false, "返回类型只能为 void 或 Task<void>");
		}
	}

	template<typename F, typename... Args>
	static void RunAsync (F &&f, Args... args) { return RunAsync (std::bind (f, args...)); }
	static Task<void> Delay (TimeSpan _dt);
	static void Start (bool _new_thread);
	static void Stop ();

	static IoContext &GetContext () { return m_ctx; }

private:
	static IoContext m_ctx;
	static std::atomic_bool m_run, m_running;
};

struct AsyncTimer {
	AsyncTimer () { m_sema.Acquire (); }
	~AsyncTimer () { Cancel (); }

	Task<bool> WaitTimeoutAsync (TimeSpan _elapse) {
		co_return !co_await m_sema.AcquireForAsync (_elapse);
	}

	template<typename F>
	void WaitCallback (TimeSpan _elapse, F _cb) {
		Tasks::RunAsync ([this] (TimeSpan _elapse, F _cb) -> Task<void> {
			if (co_await WaitTimeoutAsync (_elapse)) {
				using _TRet = typename std::decay<decltype (_cb ())>;
				if constexpr (std::is_same<_TRet, void>::value) {
					_cb ();
				} else if constexpr (std::is_same<_TRet, Task<void>>::value) {
					co_await _cb ();
				} else {
					throw std::exception ("不支持的回调函数返回类型");
				}
			}
		}, _elapse, _cb);
	}

	void Cancel () {
		m_sema.Release ();
	}

private:
	AsyncSemaphore m_sema { 1 };
};



struct Exception: public std::exception {
	Exception (std::string _err): m_err (_err) {}
	template<typename ...Args>
	Exception (std::string _err, Args ..._args): m_err (std::format (_err, _args...)) {}
	char const *what () const override { return m_err.c_str (); }

private:
	std::string m_err = "";
};



struct body_kv {
	std::string Name;
	std::string Value;
	body_kv (std::string _name, std::string _value): Name (_name), Value (_value) {}
};
struct body_file {
	std::string Name;
	std::string FileName;
	std::string FileContent;
	body_file (std::string _name, std::string _filename, std::string _content): Name (_name), FileName (_filename), FileContent (_content) {}
};
struct body_raw {
	std::string ContentType;
	std::string Content;
	body_raw (std::string _content_type, std::string _content): ContentType (_content_type), Content (_content) {}
};



struct Request {
	TimeSpan Timeout = std::chrono::seconds (0);
	std::string Server = "";
	//
	std::string Url = "";
	std::string Content = "";
	std::vector<std::variant<body_kv, body_file>> ContentRaw;
	CaseInsensitiveMap Headers = DefaultHeaders ();
	std::map<std::string, std::string> Cookies;

	static CaseInsensitiveMap DefaultHeaders () { return m_def_headers; }
	static void SetDefaultHeader (std::string _key, std::string _value) { m_def_headers [_key] = _value; }

	std::string Serilize (Method _method, std::string _host, std::string _path);

private:
	bool _content_raw_contains_files ();

	inline static CaseInsensitiveMap m_def_headers { { "Accept", "*/*" }, { "Accept-Encoding", "gzip" }, { "Cache-Control", "no-cache" }, { "Connection", "keep-alive" }, { "User-Agent", "libfv-0.0.1" } };
};



struct IConn;
struct Response {
	int HttpCode = -1;
	std::string Content = "";
	CaseInsensitiveMap Headers;

	static Task<Response> GetResponse (std::shared_ptr<IConn> _conn);
};



struct IConn {
	IConn () = default;
	virtual ~IConn () = default;
	virtual Task<void> Connect (std::string _host, std::string _port) = 0;
	virtual bool IsConnect () = 0;
	virtual void Close () = 0;
	virtual Task<void> Send (char *_data, size_t _size) = 0;
	virtual void Cancel () = 0;

	Task<char> ReadChar ();
	Task<std::string> ReadLine ();
	Task<std::string> ReadCount (size_t _count);
	Task<std::vector<uint8_t>> ReadCountVec (size_t _count);

protected:
	virtual Task<size_t> RecvImpl (char *_data, size_t _size) = 0;
	std::string TmpData = "";
};



struct TcpConn: public IConn {
	Tcp::resolver ResolverImpl;
	Tcp::socket Socket;

	TcpConn (IoContext &_ctx): ResolverImpl (_ctx), Socket (_ctx) {}
	virtual ~TcpConn () { Cancel (); Close (); }
	Task<void> Connect (std::string _host, std::string _port) override;
	bool IsConnect () override { return Socket.is_open (); }
	void Close () override;
	Task<void> Send (char *_data, size_t _size) override;
	void Cancel () override;

protected:
	Task<size_t> RecvImpl (char *_data, size_t _size) override;
};



struct SslConn: public IConn {
	Tcp::resolver ResolverImpl;
	Ssl::context SslCtx { Ssl::context::tls };
	Ssl::stream<Tcp::socket> SslSocket;

	SslConn (IoContext &_ctx): ResolverImpl (_ctx), SslSocket (_ctx, SslCtx) {}
	virtual ~SslConn () { Cancel (); Close (); }
	Task<void> Connect (std::string _host, std::string _port) override;
	bool IsConnect () override { return SslSocket.next_layer ().is_open (); }
	void Close () override;
	Task<void> Send (char *_data, size_t _size) override;
	void Cancel () override;

protected:
	Task<size_t> RecvImpl (char *_data, size_t _size) override;
};



enum class WsType { Continue = 0, Text = 1, Binary = 2, Close = 8, Ping = 9, Pong = 10 };

struct WsConn {
	std::shared_ptr<IConn> Parent;

	WsConn (std::shared_ptr<IConn> _parent): Parent (_parent) {}
	~WsConn () { Close (); }
	bool IsConnect () { return Parent && Parent->IsConnect (); }
	Task<void> SendText (char *_data, size_t _size) { co_await _Send (_data, _size, WsType::Text); }
	Task<void> SendBinary (char *_data, size_t _size) { co_await _Send (_data, _size, WsType::Binary); }
	Task<void> SendPing () { co_await _Send (nullptr, 0, WsType::Ping); }
	Task<void> Close () { co_await _Send (nullptr, 0, WsType::Close); Parent = nullptr; }
	Task<std::tuple<std::string, WsType>> Recv ();

private:
	Task<void> _Send (char *_data, size_t _size, WsType _type);
};

Task<std::shared_ptr<IConn>> Connect (std::string _url);
Task<std::shared_ptr<WsConn>> ConnectWS (std::string _url);



struct timeout { timeout (TimeSpan _exp); TimeSpan m_exp; };
struct server { server (std::string _ip); std::string m_ip; };
struct header { header (std::string _key, std::string _value); std::string m_key, m_value; };
struct authorization { authorization (std::string _auth); authorization (std::string _uid, std::string _pwd); std::string m_auth; };
struct connection { connection (std::string _co); std::string m_co; };
struct content_type { content_type (std::string _ct); std::string m_ct; };
struct referer { referer (std::string _r); std::string m_r; };
struct user_agent { user_agent (std::string _ua); std::string m_ua; };
template<typename T>
concept TOption = std::is_same<T, timeout>::value || std::is_same<T, server>::value ||
	std::is_same<T, header>::value || std::is_same<T, authorization>::value || std::is_same<T, connection>::value ||
	std::is_same<T, content_type>::value || std::is_same<T, referer>::value || std::is_same<T, user_agent>::value;
template<typename T>
concept TFormOption = std::is_same<T, timeout>::value || std::is_same<T, server>::value ||
	std::is_same<T, header>::value || std::is_same<T, authorization>::value || std::is_same<T, connection>::value ||
	std::is_same<T, content_type>::value || std::is_same<T, referer>::value || std::is_same<T, user_agent>::value ||
	std::is_same<T, body_kv>::value || std::is_same<T, body_file>::value;

template<TFormOption _Op1>
inline void _OptionApply (Request &_r, _Op1 _op) { throw Exception ("暂未支持目标类型特例化"); }
template<> inline void _OptionApply (Request &_r, timeout _t) { _r.Timeout = _t.m_exp; }
template<> inline void _OptionApply (Request &_r, server _s) { _r.Server = _s.m_ip; }
template<> inline void _OptionApply (Request &_r, header _hh) { _r.Headers [_hh.m_key] = _hh.m_value; }
template<> inline void _OptionApply (Request &_r, authorization _auth) { _r.Headers ["Authorization"] = _auth.m_auth; }
template<> inline void _OptionApply (Request &_r, connection _co) { _r.Headers ["Connection"] = _co.m_co; }
template<> inline void _OptionApply (Request &_r, content_type _ct) { _r.Headers ["Content-Type"] = _ct.m_ct; }
template<> inline void _OptionApply (Request &_r, referer _re) { _r.Headers ["Referer"] = _re.m_r; }
template<> inline void _OptionApply (Request &_r, user_agent _ua) { _r.Headers ["User-Agent"] = _ua.m_ua; }
template<> inline void _OptionApply (Request &_r, body_kv _pd) { _r.ContentRaw.push_back (_pd); }
template<> inline void _OptionApply (Request &_r, body_file _pf) { _r.ContentRaw.push_back (_pf); }

template<TFormOption _Op1>
inline void _OptionApplys (Request &_r, _Op1 _op1) { _OptionApply (_r, _op1); }
template<TFormOption _Op1, TFormOption ..._Ops>
inline void _OptionApplys (Request &_r, _Op1 _op1, _Ops ..._ops) { _OptionApply (_r, _op1); _OptionApplys (_r, _ops...); }



Task<Response> DoMethod (Request _r, Method _method);
inline Task<Response> Head (std::string _url) {
	co_return co_await DoMethod (Request { .Url = _url }, Method::Head);
}
template<TOption ..._Ops>
inline Task<Response> Head (std::string _url, _Ops ..._ops) {
	Request _r { .Url = _url };
	_OptionApplys (_r, _ops...);
	co_return co_await DoMethod (_r, Method::Head);
}
inline Task<Response> Option (std::string _url) {
	co_return co_await DoMethod (Request { .Url = _url }, Method::Option);
}
template<TOption ..._Ops>
inline Task<Response> Option (std::string _url, _Ops ..._ops) {
	Request _r { .Url = _url };
	_OptionApplys (_r, _ops...);
	co_return co_await DoMethod (_r, Method::Option);
}
inline Task<Response> Get (std::string _url) {
	co_return co_await DoMethod (Request { .Url = _url }, Method::Get);
}
template<TOption ..._Ops>
inline Task<Response> Get (std::string _url, _Ops ..._ops) {
	Request _r { .Url = _url };
	_OptionApplys (_r, _ops...);
	co_return co_await DoMethod (_r, Method::Get);
}
template<TFormOption ..._Ops>
inline Task<Response> Post (std::string _url, _Ops ..._ops) {
	Request _r { .Url = _url };
	_OptionApplys (_r, _ops...);
	co_return co_await DoMethod (_r, Method::Post);
}
template<TOption ..._Ops>
inline Task<Response> Post (std::string _url, body_raw _data, _Ops ..._ops) {
	Request _r { .Url = _url, .Content = _data };
	_r.Headers ["Content-Type"] = _data.ContentType;
	_r.Content = _data.Content;
	_OptionApplys (_r, _ops...);
	co_return co_await DoMethod (_r, Method::Post);
}
template<TFormOption ..._Ops>
inline Task<Response> Put (std::string _url, _Ops ..._ops) {
	Request _r { .Url = _url };
	_OptionApplys (_r, _ops...);
	co_return co_await DoMethod (_r, Method::Put);
}
template<TOption ..._Ops>
inline Task<Response> Put (std::string _url, body_raw _data, _Ops ..._ops) {
	Request _r { .Url = _url, .Content = _data };
	_r.Headers ["Content-Type"] = _data.ContentType;
	_r.Content = _data.Content;
	_OptionApplys (_r, _ops...);
	co_return co_await DoMethod (_r, Method::Put);
}
template<TFormOption ..._Ops>
inline Task<Response> Delete (std::string _url) {
	co_return co_await DoMethod (Request { .Url = _url }, Method::Delete);
}
template<TOption ..._Ops>
inline Task<Response> Delete (std::string _url, _Ops ..._ops) {
	Request _r { .Url = _url };
	_OptionApplys (_r, _ops...);
	co_return co_await DoMethod (_r, Method::Delete);
}
}



#endif //__FV_H__
