#ifndef __FV_SESSION_HPP__
#define __FV_SESSION_HPP__



#include <memory>

#include "common.hpp"
#include "common_funcs.hpp"
#include "conn.hpp"
#include "req_res.hpp"



namespace fv {
template<TFormOption _Op1>
inline void _OptionApply (Request &_r, _Op1 &_op) { throw Exception ("Unsupported dest type template instance"); }
template<> inline void _OptionApply (Request &_r, timeout &_t) { _r.Timeout = _t.m_exp; }
template<> inline void _OptionApply (Request &_r, server &_s) { _r.Server = _s.m_ip; }
template<> inline void _OptionApply (Request &_r, header &_hh) { _r.Headers [_hh.m_key] = _hh.m_value; }
template<> inline void _OptionApply (Request &_r, authorization &_auth) { _r.Headers [_auth.m_key] = _auth.m_value; }
template<> inline void _OptionApply (Request &_r, connection &_co) { _r.Headers [_co.m_key] = _co.m_value; }
template<> inline void _OptionApply (Request &_r, content_type &_ct) { _r.Headers [_ct.m_key] = _ct.m_value; }
template<> inline void _OptionApply (Request &_r, referer &_re) { _r.Headers [_re.m_key] = _re.m_value; }
template<> inline void _OptionApply (Request &_r, user_agent &_ua) { _r.Headers [_ua.m_key] = _ua.m_value; }
template<> inline void _OptionApply (Request &_r, url_kv &_pd) { _r.QueryItems.push_back (_pd); }
template<> inline void _OptionApply (Request &_r, body_kv &_pd) { _r.ContentItems.push_back (_pd); }
template<> inline void _OptionApply (Request &_r, body_file &_pf) { _r.ContentItems.push_back (_pf); }
template<> inline void _OptionApply (Request &_r, body_kvs &_body) {
	for (auto &[_k, _v] : _body.Kvs)
		_r.ContentItems.push_back (body_kv { _k, _v });
}

template<TBodyOption _Op1>
inline void _OptionApplyBody (Request &_r, _Op1 &_op) { throw Exception ("Unsupported dest type template instance"); }
template<> inline void _OptionApplyBody (Request &_r, body_json &_body) {
	_r.Headers ["Content-Type"] = "application/json";
	_r.Content = _body.Content;
}
template<> inline void _OptionApplyBody (Request &_r, body_raw &_body) {
	_r.Headers ["Content-Type"] = _body.ContentType;
	_r.Content = _body.Content;
}

template<TFormOption _Op1>
inline void _OptionApplys (Request &_r, _Op1 _op1) { _OptionApply (_r, _op1); }
template<TFormOption _Op1, TFormOption ..._Ops>
inline void _OptionApplys (Request &_r, _Op1 _op1, _Ops ..._ops) { _OptionApply (_r, _op1); _OptionApplys (_r, _ops...); }



struct Session {
	std::shared_ptr<IConn> Conn;
	std::string ConnFlag = "";
	AsyncMutex SessMtx {};

	Session () {}
	bool IsConnect () { return Conn && Conn->IsConnect (); }

	Task<Response> DoMethod (Request _r) {
		co_await SessMtx.Lock ();
		bool _is_lock = true;
		try {
			auto [_schema, _host, _port, _path] = _parse_url (_r.Url);
			std::string _conn_flag = fmt::format ("{}://{}:{}", _schema, _host, _port);
			if (!Conn || ConnFlag != _conn_flag) {
				ConnFlag = _conn_flag;
				if (_schema == "https") {
					Conn = std::shared_ptr<IConn> (new SslConn {});
				} else {
					Conn = std::shared_ptr<IConn> (new TcpConn {});
				}
				auto _ip = co_await Config::DnsResolve (_host);
				co_await Conn->Connect (_ip != "" ? _ip : _host, _port);
			}

			_r.Schema = _schema;
			_r.UrlPath = _path;

			//// cancel
			//AsyncTimer _timer {};
			//if (std::chrono::duration_cast<std::chrono::nanoseconds> (_r.Timeout).count () > 0) {
			//	_timer.WaitCallback (_r.Timeout, [_tconn = std::weak_ptr (Conn)] ()->Task<void> {
			//		auto _conn = _tconn.lock ();
			//		if (_conn)
			//			_conn->Cancel ();
			//	});
			//}

			// generate data
			std::string _data = _r.Serilize (_host, _port, _path);

			for (size_t i = 0; i < 2; i++) {
				try {
					// send
					co_await Conn->Send (_data.data (), _data.size ());
					break;
				} catch (...) {
					if (i == 1)
						throw;
				}
				co_await Conn->Reconnect ();
			}

			SessMtx.Unlock ();
			_is_lock = false;

			// recv
			Response _ret = co_await Response::GetFromConn (Conn);
			//_timer.Cancel ();
			co_return _ret;
		} catch (...) {
			if (_is_lock)
				SessMtx.Unlock ();
			Conn = nullptr;
			throw;
		}
	}

	Task<Response> Head (std::string _url) {
		co_return co_await DoMethod (Request { _url, MethodType::Head });
	}
	template<TOption ..._Ops>
	Task<Response> Head (std::string _url, _Ops ..._ops) {
		Request _r { _url, MethodType::Head };
		_OptionApplys (_r, _ops...);
		co_return co_await DoMethod (_r);
	}

	Task<Response> Option (std::string _url) {
		co_return co_await DoMethod (Request { _url, MethodType::Option });
	}
	template<TOption ..._Ops>
	Task<Response> Option (std::string _url, _Ops ..._ops) {
		Request _r { _url, MethodType::Option };
		_OptionApplys (_r, _ops...);
		co_return co_await DoMethod (_r);
	}

	Task<Response> Get (std::string _url) {
		co_return co_await DoMethod (Request { _url, MethodType::Get });
	}
	template<TOption ..._Ops>
	Task<Response> Get (std::string _url, _Ops ..._ops) {
		Request _r { _url, MethodType::Get };
		_OptionApplys (_r, _ops...);
		co_return co_await DoMethod (_r);
	}

	template<TFormOption ..._Ops>
	Task<Response> Post (std::string _url, _Ops ..._ops) {
		Request _r { _url, MethodType::Post };
		_OptionApplys (_r, _ops...);
		co_return co_await DoMethod (_r);
	}
	template<TBodyOption _Body>
	Task<Response> Post (std::string _url, _Body _body) {
		Request _r { _url, MethodType::Post };
		_OptionApplyBody (_r, _body);
		co_return co_await DoMethod (_r);
	}
	template<TBodyOption _Body, TOption ..._Ops>
	Task<Response> Post (std::string _url, _Body _body, _Ops ..._ops) {
		Request _r { _url, MethodType::Post };
		_OptionApplyBody (_r, _body);
		_OptionApplys (_r, _ops...);
		co_return co_await DoMethod (_r);
	}

	template<TFormOption ..._Ops>
	Task<Response> Put (std::string _url, _Ops ..._ops) {
		Request _r { _url, MethodType::Put };
		_OptionApplys (_r, _ops...);
		co_return co_await DoMethod (_r);
	}
	template<TBodyOption _Body>
	Task<Response> Put (std::string _url, _Body _body) {
		Request _r { _url, MethodType::Put };
		_OptionApplyBody (_r, _body);
		co_return co_await DoMethod (_r);
	}
	template<TBodyOption _Body, TOption ..._Ops>
	Task<Response> Put (std::string _url, _Body _body, _Ops ..._ops) {
		Request _r { _url, MethodType::Put };
		_OptionApplyBody (_r, _body);
		_OptionApplys (_r, _ops...);
		co_return co_await DoMethod (_r);
	}

	Task<Response> Delete (std::string _url) {
		co_return co_await DoMethod (Request { _url, MethodType::Delete });
	}
	template<TOption ..._Ops>
	Task<Response> Delete (std::string _url, _Ops ..._ops) {
		Request _r { _url, MethodType::Delete };
		_OptionApplys (_r, _ops...);
		co_return co_await DoMethod (_r);
	}
};



inline Task<Response> Head (std::string _url) {
	Session _sess {};
	co_return co_await _sess.Head (_url);
}
template<TOption ..._Ops>
inline Task<Response> Head (std::string _url, _Ops ..._ops) {
	Session _sess {};
	co_return co_await _sess.Head (_url, std::forward<_Ops> (_ops)...);
}

inline Task<Response> Option (std::string _url) {
	Session _sess {};
	co_return co_await _sess.Option (_url);
}
template<TOption ..._Ops>
inline Task<Response> Option (std::string _url, _Ops ..._ops) {
	Session _sess {};
	co_return co_await _sess.Option (_url, std::forward<_Ops> (_ops)...);
}

inline Task<Response> Get (std::string _url) {
	Session _sess {};
	co_return co_await _sess.Get (_url);
}
template<TOption ..._Ops>
inline Task<Response> Get (std::string _url, _Ops ..._ops) {
	Session _sess {};
	co_return co_await _sess.Get (_url, std::forward<_Ops> (_ops)...);
}

template<TFormOption ..._Ops>
inline Task<Response> Post (std::string _url, _Ops ..._ops) {
	Session _sess {};
	co_return co_await _sess.Post (_url, std::forward<_Ops> (_ops)...);
}
template<TBodyOption _Body>
inline Task<Response> Post (std::string _url, _Body _body) {
	Session _sess {};
	co_return co_await _sess.Post (_url, std::forward<_Body> (_body));
}
template<TBodyOption _Body, TOption ..._Ops>
inline Task<Response> Post (std::string _url, _Body _body, _Ops ..._ops) {
	Session _sess {};
	co_return co_await _sess.Post (_url, std::forward<_Body> (_body), std::forward<_Ops> (_ops)...);
}

template<TFormOption ..._Ops>
inline Task<Response> Put (std::string _url, _Ops ..._ops) {
	Session _sess {};
	co_return co_await _sess.Put (_url, std::forward<_Ops> (_ops)...);
}
template<TBodyOption _Body>
inline Task<Response> Put (std::string _url, _Body _body) {
	Session _sess {};
	co_return co_await _sess.Put (_url, std::forward<_Body> (_body));
}
template<TBodyOption _Body, TOption ..._Ops>
inline Task<Response> Put (std::string _url, _Body _body, _Ops ..._ops) {
	Session _sess {};
	co_return co_await _sess.Put (_url, std::forward<_Body> (_body), std::forward<_Ops> (_ops)...);
}

inline Task<Response> Delete (std::string _url) {
	Session _sess {};
	co_return co_await _sess.Delete (_url);
}
template<TOption ..._Ops>
inline Task<Response> Delete (std::string _url, _Ops ..._ops) {
	Session _sess {};
	co_return co_await _sess.Delete (_url, std::forward<_Ops> (_ops)...);
}
}



#endif //__FV_SESSION_HPP__
