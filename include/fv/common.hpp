#ifndef __FV_COMMON_HPP__
#define __FV_COMMON_HPP__



#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#ifdef ASIO_STANDALONE
#ifndef ASIO_HAS_CO_AWAIT
#define ASIO_HAS_CO_AWAIT
#endif
#include <asio.hpp>
#include <asio/ssl.hpp>
#else
#ifndef BOOST_ASIO_HAS_CO_AWAIT
#define BOOST_ASIO_HAS_CO_AWAIT
#endif
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#endif



namespace fv {
#ifndef ASIO_STANDALONE
namespace asio = boost::asio;
#endif

#define Task asio::awaitable
using Tcp = asio::ip::tcp;
using Udp = asio::ip::udp;
namespace Ssl = asio::ssl;
using IoContext = asio::io_context;
using SocketBase = asio::socket_base;
using SslCheckCb = std::function<bool (bool, Ssl::verify_context &)>;
inline decltype (asio::use_awaitable) &UseAwaitable = asio::use_awaitable;
using TimeSpan = std::chrono::system_clock::duration;



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



struct Exception: public std::exception {
	Exception (std::string _err): m_err (_err) {}
	//template<typename ...Args>
	//Exception (std::string _err, Args ..._args): m_err (fmt::format (_err, _args...)) {}
	const char* what() const noexcept override { return m_err.c_str (); }

private:
	std::string m_err = "";
};



struct AsyncMutex {
	AsyncMutex (bool _init_locked = false): m_locked (_init_locked) {}

	bool IsLocked () {
		std::unique_lock _ul { m_mtx };
		return m_locked;
	}

	bool TryLock () {
		std::unique_lock _ul { m_mtx };
		if (!m_locked) {
			m_locked = true;
			return true;
		} else {
			return false;
		}
	}

	Task<void> Lock () {
		std::unique_lock _ul { m_mtx, std::defer_lock };
		while (true) {
			_ul.lock ();
			if (!m_locked) {
				m_locked = true;
				co_return;
			}
			_ul.unlock ();
			co_await _delay (std::chrono::milliseconds (1));
		}
	}

	Task<bool> Lock (TimeSpan _timeout) {
		std::unique_lock _ul { m_mtx, std::defer_lock };
		auto _elapsed = std::chrono::system_clock::now () + _timeout;
		while (_elapsed > std::chrono::system_clock::now ()) {
			_ul.lock ();
			if (!m_locked) {
				m_locked = true;
				co_return true;
			}
			_ul.unlock ();
			co_await _delay (std::chrono::milliseconds (1));
		}
		co_return false;
	}

	void LockSync () {
		std::unique_lock _ul { m_mtx, std::defer_lock };
		while (true) {
			_ul.lock ();
			if (!m_locked) {
				m_locked = true;
				return;
			}
			_ul.unlock ();
			std::this_thread::sleep_for (std::chrono::milliseconds (1));
		}
	}

	void Unlock () {
		std::unique_lock _ul { m_mtx };
		if (!m_locked)
			throw Exception ("Cannot unlock a unlocked mutex");
		m_locked = false;
	}

private:
	static Task<void> _delay (TimeSpan _dt) {
		asio::steady_timer timer (co_await asio::this_coro::executor);
		timer.expires_after (_dt);
		co_await timer.async_wait (UseAwaitable);
	}

	bool m_locked = false;
	std::mutex m_mtx;
};



struct AsyncSemaphore {
	AsyncSemaphore (size_t _init_count = 1): m_count (_init_count) {}

	size_t GetResCount () {
		std::unique_lock _ul { m_mtx };
		return m_count;
	}

	bool TryAcquire () {
		std::unique_lock _ul { m_mtx };
		if (m_count > 0) {
			--m_count;
			return true;
		} else {
			return false;
		}
	}

	Task<void> Acquire () {
		std::unique_lock _ul { m_mtx, std::defer_lock };
		while (true) {
			_ul.lock ();
			if (m_count > 0) {
				--m_count;
				co_return;
			}
			_ul.unlock ();
			co_await _delay (std::chrono::milliseconds (1));
		}
	}

	Task<bool> Acquire (TimeSpan _timeout) {
		std::unique_lock _ul { m_mtx, std::defer_lock };
		auto _elapsed = std::chrono::system_clock::now () + _timeout;
		while (_elapsed > std::chrono::system_clock::now ()) {
			_ul.lock ();
			if (m_count > 0) {
				--m_count;
				co_return true;
			}
			_ul.unlock ();
			co_await _delay (std::chrono::milliseconds (1));
		}
		co_return false;
	}

	void Release () {
		std::unique_lock _ul { m_mtx };
		++m_count;
	}

private:
	static Task<void> _delay (TimeSpan _dt) {
		asio::steady_timer timer (co_await asio::this_coro::executor);
		timer.expires_after (_dt);
		co_await timer.async_wait (UseAwaitable);
	}

	size_t m_count;
	std::mutex m_mtx;
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
		std::unique_lock _ul { m_mtx };
		if (!m_ctx)
			throw Exception ("You should invoke Init method first");

		using TRet = decltype (f ());
		if constexpr (std::is_void<TRet>::value) {
			m_ctx->post (std::forward<F> (f));
		} else if constexpr (std::is_same<TRet, Task<void>>::value) {
			asio::co_spawn (*m_ctx, std::forward<F> (f), asio::detached);
		} else {
			static_assert (std::is_void<TRet>::value || std::is_same<TRet, Task<void>>::value, "Unsupported returns type");
		}
	}

	template<typename F, typename... Args>
	static void RunAsync (F &&f, Args... args) { return RunAsync (std::bind (f, args...)); }
	static Task<void> Delay (TimeSpan _dt) {
		asio::steady_timer timer (co_await asio::this_coro::executor);
		timer.expires_after (_dt);
		co_await timer.async_wait (UseAwaitable);
	}
	static void Init (IoContext *_ctx = nullptr) {
		::srand ((unsigned int) ::time (NULL));
		std::unique_lock _ul { m_mtx };
		if (m_ctx)
			throw Exception ("You should only invoke Init method once");
		if (_ctx) {
			m_ctx = _ctx;
			m_extern_ctx = true;
		} else {
			m_ctx = new IoContext { 1 };
		}
	}
	static void Release () {
		if (!m_extern_ctx)
			delete m_ctx;
		m_ctx = nullptr;
	}
	static void Stop () {
		std::unique_lock _ul { m_mtx };
		m_run = false;
	}
	static void LoopRun () {
		std::unique_lock _ul { m_mtx };
		m_run = m_running = true;
		while (m_run) {
			_ul.unlock ();
			if (m_ctx->run_one () == 0)
				std::this_thread::sleep_for (std::chrono::milliseconds (1));
			_ul.lock ();
		}
		m_running = false;
	}
	//static void LoopRun (size_t _thread = 1) {
	//	std::unique_lock _ul { m_mtx };
	//	m_run = m_running = true;
	//	auto _loop_run = [] () {
	//		std::unique_lock _ul { m_mtx };
	//		while (m_run) {
	//			_ul.unlock ();
	//			if (m_ctx->run_one () == 0)
	//				std::this_thread::sleep_for (std::chrono::milliseconds (1));
	//			_ul.lock ();
	//		}
	//	};
	//	if (_thread > 1) {
	//		std::vector<std::unique_ptr<std::thread>> _threads;
	//		for (size_t i = 1; i < _thread; ++i) {
	//			_threads.emplace_back (std::make_unique<std::thread> ());
	//		}
	//	} else {
	//		_loop_run ();
	//	}
	//	m_running = false;
	//}
	static IoContext &GetContext () {
		std::unique_lock _ul { m_mtx };
		if (!m_ctx)
			Init ();
		return *m_ctx;
	}

private:
	inline static std::mutex m_mtx;
	inline static IoContext *m_ctx = nullptr;
	inline static bool m_run = false, m_running = false, m_extern_ctx = false;
};

struct AsyncTimer {
	AsyncTimer () { }
	~AsyncTimer () { Cancel (); }

	Task<bool> WaitTimeoutAsync (TimeSpan _elapse) {
		co_return !co_await m_mtx.Lock (_elapse);
	}

	template<typename F>
	void WaitCallback (TimeSpan _elapse, F _cb) {
		if (!m_mtx.IsLocked ())
			m_mtx.LockSync ();
		Tasks::RunAsync ([this] (TimeSpan _elapse, F _cb) -> Task<void> {
			bool _lock = co_await m_mtx.Lock (_elapse);
			if (!_lock) {
				using TRet = typename std::decay<decltype (_cb ())>;
				if constexpr (std::is_same<TRet, void>::value) {
					_cb ();
				} else if constexpr (std::is_same<TRet, Task<void>>::value) {
					co_await _cb ();
				} else {
					static_assert (std::is_void<TRet>::value || std::is_same<TRet, Task<void>>::value, "Unsupported returns type");
				}
			}
		}, _elapse, _cb);
	}

	void Cancel () {
		if (m_mtx.IsLocked ())
			m_mtx.Unlock ();
	}

private:
	AsyncMutex m_mtx {};
};
}



#endif //__FV_COMMON_HPP__
