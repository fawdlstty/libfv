#ifndef __FV_SESSION_HPP__
#define __FV_SESSION_HPP__



#include <memory>

#include "common.hpp"
#include "common_funcs.hpp"
#include "conn.hpp"
#include "req_res.hpp"



namespace fv {
template<TFormOption _Op1>
inline void _OptionApply (Request &_r, _Op1 _op) { throw Exception ("暂未支持目标类型特例化"); }
template<> inline void _OptionApply (Request &_r, timeout _t) { _r.Timeout = _t.m_exp; }
template<> inline void _OptionApply (Request &_r, server _s) { _r.Server = _s.m_ip; }
template<> inline void _OptionApply (Request &_r, header _hh) { _r.Headers [_hh.m_key] = _hh.m_value; }
template<> inline void _OptionApply (Request &_r, authorization _auth) { _r.Headers [_auth.m_key] = _auth.m_value; }
template<> inline void _OptionApply (Request &_r, connection _co) { _r.Headers [_co.m_key] = _co.m_value; }
template<> inline void _OptionApply (Request &_r, content_type _ct) { _r.Headers [_ct.m_key] = _ct.m_value; }
template<> inline void _OptionApply (Request &_r, referer _re) { _r.Headers [_re.m_key] = _re.m_value; }
template<> inline void _OptionApply (Request &_r, user_agent _ua) { _r.Headers [_ua.m_key] = _ua.m_value; }
template<> inline void _OptionApply (Request &_r, url_kv _pd) { _r.QueryItems.push_back (_pd); }
template<> inline void _OptionApply (Request &_r, body_kv _pd) { _r.ContentItems.push_back (_pd); }
template<> inline void _OptionApply (Request &_r, body_file _pf) { _r.ContentItems.push_back (_pf); }

template<TFormOption _Op1>
inline void _OptionApplys (Request &_r, _Op1 _op1) { _OptionApply (_r, _op1); }
template<TFormOption _Op1, TFormOption ..._Ops>
inline void _OptionApplys (Request &_r, _Op1 _op1, _Ops ..._ops) { _OptionApply (_r, _op1); _OptionApplys (_r, _ops...); }



struct Session {
	std::shared_ptr<IConn> Conn;

	Session (std::shared_ptr<IConn> _conn): Conn (_conn) {}
	static Task<Session> FromUrl (std::string _url, std::string _server_ip = "") {
		auto [_schema, _host, _port, _path] = _parse_url (_url);
		std::shared_ptr<IConn> _conn;
		if (_schema == "https") {
			_conn = std::shared_ptr<IConn> (new SslConn { Tasks::GetContext () });
		} else {
			_conn = std::shared_ptr<IConn> (new TcpConn { Tasks::GetContext () });
		}

		// TODO
		//AsyncTimer _timer {};
		//if (std::chrono::duration_cast<std::chrono::nanoseconds> (Config::ConnectTimeout).count () > 0) {
		//	_timer.WaitCallback (Config::ConnectTimeout, [_tconn = std::weak_ptr (_conn)] ()->Task<void> {
		//		auto _conn = _tconn.lock ();
		//		if (_conn)
		//			_conn->Cancel ();
		//	});
		//}

		// connect
		co_await _conn->Connect (_server_ip != "" ? _server_ip : _host, _port);

		//_timer.Cancel ();
		if (!_conn->IsConnect ())
			throw Exception ("Connect failure");
		co_return Session { _conn };
	}

	Task<Response> DoMethod (Request _r) {
		auto [_schema, _host, _port, _path] = _parse_url (_r.Url);
		_r.Schema = _schema;
		_r.UrlPath = _path;
		// TODO
		//AsyncTimer _timer {};
		//if (std::chrono::duration_cast<std::chrono::nanoseconds> (_r.Timeout).count () > 0) {
		//	_timer.WaitCallback (_r.Timeout, [_tconn = std::weak_ptr (Conn)] ()->Task<void> {
		//		auto _conn = _tconn.lock ();
		//		if (_conn)
		//			_conn->Cancel ();
		//	});
		//}

		// generate data
		std::string _data = _r.Serilize (_host, _path);

		// send
		co_await Conn->Send (_data.data (), _data.size ());

		// recv
		Response _ret = co_await Response::GetFromConn (Conn);
		//_timer.Cancel ();
		co_return _ret;
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
	template<TOption ..._Ops>
	Task<Response> Post (std::string _url, body_raw _data, _Ops ..._ops) {
		Request _r { _url, MethodType::Post };
		_r.Headers ["Content-Type"] = _data.ContentType;
		_r.Content = _data.Content;
		_OptionApplys (_r, _ops...);
		co_return co_await DoMethod (_r);
	}

	template<TFormOption ..._Ops>
	Task<Response> Put (std::string _url, _Ops ..._ops) {
		Request _r { _url, MethodType::Put };
		_OptionApplys (_r, _ops...);
		co_return co_await DoMethod (_r);
	}
	template<TOption ..._Ops>
	Task<Response> Put (std::string _url, body_raw _data, _Ops ..._ops) {
		Request _r { _url, MethodType::Put };
		_r.Headers ["Content-Type"] = _data.ContentType;
		_r.Content = _data.Content;
		_OptionApplys (_r, _ops...);
		co_return co_await DoMethod (_r);
	}

	template<TFormOption ..._Ops>
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



Task<Response> Head (std::string _url) {
	Session _sess = co_await Session::FromUrl (_url, "");
	co_return co_await _sess.DoMethod (Request { _url, MethodType::Head });
}
template<TOption ..._Ops>
Task<Response> Head (std::string _url, _Ops ..._ops) {
	Request _r { _url, MethodType::Head };
	Session _sess = co_await Session::FromUrl (_url, _r.Server);
	_OptionApplys (_r, _ops...);
	co_return co_await _sess.DoMethod (_r);
}

Task<Response> Option (std::string _url) {
	Session _sess = co_await Session::FromUrl (_url, "");
	co_return co_await _sess.DoMethod (Request { _url, MethodType::Option });
}
template<TOption ..._Ops>
Task<Response> Option (std::string _url, _Ops ..._ops) {
	Request _r { _url, MethodType::Option };
	Session _sess = co_await Session::FromUrl (_url, _r.Server);
	_OptionApplys (_r, _ops...);
	co_return co_await _sess.DoMethod (_r);
}

Task<Response> Get (std::string _url) {
	Session _sess = co_await Session::FromUrl (_url, "");
	co_return co_await _sess.DoMethod (Request { _url, MethodType::Get });
}
template<TOption ..._Ops>
Task<Response> Get (std::string _url, _Ops ..._ops) {
	Request _r { _url, MethodType::Get };
	Session _sess = co_await Session::FromUrl (_url, _r.Server);
	_OptionApplys (_r, _ops...);
	co_return co_await _sess.DoMethod (_r);
}

template<TFormOption ..._Ops>
Task<Response> Post (std::string _url, _Ops ..._ops) {
	Request _r { _url, MethodType::Post };
	Session _sess = co_await Session::FromUrl (_url, _r.Server);
	_OptionApplys (_r, _ops...);
	co_return co_await _sess.DoMethod (_r);
}
template<TOption ..._Ops>
Task<Response> Post (std::string _url, body_raw _data, _Ops ..._ops) {
	Request _r { _url, MethodType::Post };
	Session _sess = co_await Session::FromUrl (_url, _r.Server);
	_r.Headers ["Content-Type"] = _data.ContentType;
	_r.Content = _data.Content;
	_OptionApplys (_r, _ops...);
	co_return co_await _sess.DoMethod (_r);
}

template<TFormOption ..._Ops>
Task<Response> Put (std::string _url, _Ops ..._ops) {
	Request _r { _url, MethodType::Put };
	Session _sess = co_await Session::FromUrl (_url, _r.Server);
	_OptionApplys (_r, _ops...);
	co_return co_await _sess.DoMethod (_r);
}
template<TOption ..._Ops>
Task<Response> Put (std::string _url, body_raw _data, _Ops ..._ops) {
	Request _r { _url, MethodType::Put };
	Session _sess = co_await Session::FromUrl (_url, _r.Server);
	_r.Headers ["Content-Type"] = _data.ContentType;
	_r.Content = _data.Content;
	_OptionApplys (_r, _ops...);
	co_return co_await _sess.DoMethod (_r);
}

Task<Response> Delete (std::string _url) {
	Session _sess = co_await Session::FromUrl (_url, "");
	co_return co_await _sess.DoMethod (Request { _url, MethodType::Delete });
}
template<TOption ..._Ops>
Task<Response> Delete (std::string _url, _Ops ..._ops) {
	Request _r { _url, MethodType::Delete };
	Session _sess = co_await Session::FromUrl (_url, _r.Server);
	_OptionApplys (_r, _ops...);
	co_return co_await _sess.DoMethod (_r);
}
}



#endif //__FV_SESSION_HPP__
