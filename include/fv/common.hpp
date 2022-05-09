#ifndef __FV_COMMON_HPP__
#define __FV_COMMON_HPP__



#include <chrono>
#include <semaphore>
#include <string>
#include <functional>
#include <unordered_map>

#ifndef BOOST_ASIO_HAS_CO_AWAIT
#define BOOST_ASIO_HAS_CO_AWAIT
#endif
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>



namespace fv {
#define Task boost::asio::awaitable
using Tcp = boost::asio::ip::tcp;
using Udp = boost::asio::ip::udp;
namespace Ssl = boost::asio::ssl;
using IoContext = boost::asio::io_context;
using TimeSpan = std::chrono::system_clock::duration;
using SocketBase = boost::asio::socket_base;
using SslCheckCb = std::function<bool (bool, Ssl::verify_context &)>;
inline decltype (boost::asio::use_awaitable) &UseAwaitable = boost::asio::use_awaitable;



struct CaseInsensitiveHash {
	size_t operator() (const std::string &str) const noexcept {
		size_t h = 0;
		std::hash<int> hash {};
		for (auto c : str)
			h ^= hash (::tolower (c)) + 0x9e3779b9 + (h << 6) + (h >> 2);
		return h;
	}
};
struct CaseInsensitiveEqual {
	bool operator() (const std::string &str1, const std::string &str2) const noexcept {
		return str1.size () == str2.size () && std::equal (str1.begin (), str1.end (), str2.begin (), [] (char a, char b) {
			return tolower (a) == tolower (b);
		});
	}
};
using CaseInsensitiveMap = std::unordered_map<std::string, std::string, CaseInsensitiveHash, CaseInsensitiveEqual>;



struct AsyncSemaphore {
	AsyncSemaphore (int _init_count): m_sp (_init_count) {}
	void Release () { m_sp.release (); }
	bool TryAcquire () { return m_sp.try_acquire (); }
	void Acquire () { m_sp.acquire (); }
	Task<void> AcquireAsync () {
		while (!m_sp.try_acquire ()) {
			boost::asio::steady_timer timer (co_await boost::asio::this_coro::executor);
			timer.expires_after (std::chrono::milliseconds (1));
			co_await timer.async_wait (UseAwaitable);
		}
	}
	Task<bool> AcquireForAsync (TimeSpan _span) { co_return co_await AcquireUntilAsync (std::chrono::system_clock::now () + _span); }
	Task<bool> AcquireUntilAsync (std::chrono::system_clock::time_point _until) {
		while (!m_sp.try_acquire ()) {
			if (std::chrono::system_clock::now () >= _until)
				co_return false;
			boost::asio::steady_timer timer (co_await boost::asio::this_coro::executor);
			timer.expires_after (std::chrono::milliseconds (1));
			co_await timer.async_wait (UseAwaitable);
		}
		co_return true;
	}

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
	static Task<void> Delay (TimeSpan _dt) {
		boost::asio::steady_timer timer (co_await boost::asio::this_coro::executor);
		timer.expires_after (_dt);
		co_await timer.async_wait (UseAwaitable);
	}
	static void Start (bool _new_thread) {
		::srand ((unsigned int) ::time (NULL));
		m_run.store (true);
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
	static void Stop () {
		m_run.store (false);
		while (m_running.load ())
			std::this_thread::sleep_for (std::chrono::milliseconds (1));
	}

	static IoContext &GetContext () { return m_ctx; }

private:
	inline static IoContext m_ctx { 1 };
	inline static std::atomic_bool m_run { false }, m_running { false };
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
}



#endif //__FV_COMMON_HPP__
