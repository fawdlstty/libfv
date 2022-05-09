#ifndef __FV_HTTP_CLIENT_HPP__
#define __FV_HTTP_CLIENT_HPP__



#include "base.hpp"
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



Task<Response> DoMethod (Request _r);



inline Task<Response> Head (std::string _url) {
	co_return co_await DoMethod (Request { _url, MethodType::Head });
}
template<TOption ..._Ops>
inline Task<Response> Head (std::string _url, _Ops ..._ops) {
	Request _r { .Url = _url, .Method = MethodType::Head };
	_OptionApplys (_r, _ops...);
	co_return co_await DoMethod (_r);
}



inline Task<Response> Option (std::string _url) {
	co_return co_await DoMethod (Request { _url, MethodType::Option });
}
template<TOption ..._Ops>
inline Task<Response> Option (std::string _url, _Ops ..._ops) {
	Request _r { .Url = _url, .Method = MethodType::Option };
	_OptionApplys (_r, _ops...);
	co_return co_await DoMethod (_r);
}



inline Task<Response> Get (std::string _url) {
	co_return co_await DoMethod (Request { _url, MethodType::Get });
}
template<TOption ..._Ops>
inline Task<Response> Get (std::string _url, _Ops ..._ops) {
	Request _r { .Url = _url, .Method = MethodType::Get };
	_OptionApplys (_r, _ops...);
	co_return co_await DoMethod (_r);
}



template<TFormOption ..._Ops>
inline Task<Response> Post (std::string _url, _Ops ..._ops) {
	Request _r { .Url = _url, .Method = MethodType::Post };
	_OptionApplys (_r, _ops...);
	co_return co_await DoMethod (_r);
}
template<TOption ..._Ops>
inline Task<Response> Post (std::string _url, body_raw _data, _Ops ..._ops) {
	Request _r { .Url = _url, .Method = MethodType::Post, .Content = _data };
	_r.Headers ["Content-Type"] = _data.ContentType;
	_r.Content = _data.Content;
	_OptionApplys (_r, _ops...);
	co_return co_await DoMethod (_r);
}



template<TFormOption ..._Ops>
inline Task<Response> Put (std::string _url, _Ops ..._ops) {
	Request _r { .Url = _url, .Method = MethodType::Put };
	_OptionApplys (_r, _ops...);
	co_return co_await DoMethod (_r);
}
template<TOption ..._Ops>
inline Task<Response> Put (std::string _url, body_raw _data, _Ops ..._ops) {
	Request _r { .Url = _url, .Method = MethodType::Put, .Content = _data };
	_r.Headers ["Content-Type"] = _data.ContentType;
	_r.Content = _data.Content;
	_OptionApplys (_r, _ops...);
	co_return co_await DoMethod (_r);
}



template<TFormOption ..._Ops>
inline Task<Response> Delete (std::string _url) {
	co_return co_await DoMethod (Request { _url, MethodType::Delete });
}
template<TOption ..._Ops>
inline Task<Response> Delete (std::string _url, _Ops ..._ops) {
	Request _r { .Url = _url, .Method = MethodType::Delete };
	_OptionApplys (_r, _ops...);
	co_return co_await DoMethod (_r);
}
}



#endif //__FV_HTTP_CLIENT_HPP__
