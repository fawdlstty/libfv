#ifndef __TASKS_HPP__
#define __TASKS_HPP__



#include <atomic>
#include <chrono>
#include <functional>
#include <semaphore>
#include <string>
#include <thread>

#define BOOST_ASIO_HAS_CO_AWAIT
#include <boost/asio.hpp>
#define Task boost::asio::awaitable
using asio_tcp = boost::asio::ip::tcp;
using asio_udp = boost::asio::ip::udp;
using asio_address = boost::asio::ip::address;
using boost::asio::use_awaitable;



//
// 异步信号量
//
struct AsyncSemaphore {
	AsyncSemaphore (int _init_count): m_sp (_init_count) {}

	void Release () { m_sp.release (); }

	bool TryAcquire () { return m_sp.try_acquire (); }

	void Acquire () { m_sp.acquire (); }

	Task<void> AcquireAsync () {
		while (!m_sp.try_acquire ()) {
			boost::asio::steady_timer timer (co_await boost::asio::this_coro::executor);
			timer.expires_after (std::chrono::milliseconds (1));
			co_await timer.async_wait (use_awaitable);
		}
	}

	Task<bool> AcquireForAsync (std::chrono::system_clock::duration _span) {
		co_return co_await AcquireUntilAsync (std::chrono::system_clock::now () + _span);
	}

	Task<bool> AcquireUntilAsync (std::chrono::system_clock::time_point _until) {
		while (!m_sp.try_acquire ()) {
			if (std::chrono::system_clock::now () >= _until)
				co_return false;
			boost::asio::steady_timer timer (co_await boost::asio::this_coro::executor);
			timer.expires_after (std::chrono::milliseconds (1));
			co_await timer.async_wait (use_awaitable);
		}
		co_return true;
	}

private:
	std::counting_semaphore<> m_sp;
};



//
// 超时管理
//
struct CancelToken {
	CancelToken (std::chrono::system_clock::time_point _cancel_time) { m_cancel_time = _cancel_time; }
	CancelToken (std::chrono::system_clock::duration _expire) { m_cancel_time = std::chrono::system_clock::now () + _expire; }
	void Cancel () { m_cancel_time = std::chrono::system_clock::now (); }
	bool IsCancel () { return std::chrono::system_clock::now () <= m_cancel_time; }
	std::chrono::system_clock::duration GetRemaining () { return m_cancel_time - std::chrono::system_clock::now (); }

private:
	std::chrono::system_clock::time_point m_cancel_time;
};



//
// 任务池
//
struct Tasks {
	//// 运行同步方法
	//static void Run (std::function<void ()> _cb) { m_ctx.post (_cb); }

	//// 运行异步方法
	//static void RunAsync (std::function<Task<void> ()> _cb) { boost::asio::co_spawn (m_ctx, _cb, boost::asio::detached); }

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

	// 异步延时
	static Task<void> Delay (const std::chrono::system_clock::duration &_dt) {
		boost::asio::steady_timer timer (co_await boost::asio::this_coro::executor);
		timer.expires_after (_dt);
		co_await timer.async_wait (use_awaitable);
	}

	// 启动异步任务池
	static void Start (bool _new_thread) {
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
	static void Stop () {
		m_run.store (false);
		while (m_running.load ())
			std::this_thread::sleep_for (std::chrono::milliseconds (1));
	}

	static boost::asio::io_context &GetContext () { return m_ctx; }

private:
	inline static boost::asio::io_context m_ctx { 1 };
	inline static std::atomic_bool m_run = true, m_running = false;
};



//
// 定时器
//
enum class TimerType { Timeout, Cancel };
struct AsyncTimer {
	AsyncTimer () { m_sema.Acquire (); }
	~AsyncTimer () { Cancel (); }

	Task<TimerType> WaitAsync (std::chrono::system_clock::duration _elapse) {
		bool _acq = co_await m_sema.AcquireForAsync (_elapse);
		co_return _acq ? TimerType::Cancel : TimerType::Timeout;
	}

	void WaitCallback (std::chrono::system_clock::duration _elapse, std::function<void (TimerType)> _cb) {
		Tasks::RunAsync ([this] (std::chrono::system_clock::duration _elapse, std::function<void (TimerType)> _cb) -> Task<void> {
			_cb (co_await WaitAsync (_elapse));
		}, _elapse, _cb);
	}

	void WaitCallback (std::chrono::system_clock::duration _elapse, std::function<Task<void> (TimerType)> _cb) {
		Tasks::RunAsync ([this] (std::chrono::system_clock::duration _elapse, std::function<Task<void> (TimerType)> _cb) -> Task<void> {
			co_await _cb (co_await WaitAsync (_elapse));
		}, _elapse, _cb);
	}

	void Cancel () {
		m_sema.Release ();
	}

private:
	AsyncSemaphore m_sema { 1 };
};



#endif //__TASKS_HPP__
