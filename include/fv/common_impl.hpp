#ifndef __FV_COMMON_IMPL_HPP__
#define __FV_COMMON_IMPL_HPP__



#include "common.hpp"
#include "session.hpp"



namespace fv {
void Tasks::LoopRun () {
	std::unique_lock _ul { m_mtx, std::defer_lock };
	_ul.lock ();
	m_run = m_running = true;
	auto _interval = Config::SessionPoolTimeout / 5;
	auto _t = std::chrono::steady_clock::now () + _interval;
	while (m_run) {
		_ul.unlock ();
		if (m_ctx->run_one () == 0)
			std::this_thread::sleep_for (std::chrono::milliseconds (1));
		if (std::chrono::steady_clock::now () >= _t) {
			SessionPool::TimeClear ();
			_t = std::chrono::steady_clock::now () + _interval;
		}
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
}



#endif //__FV_COMMON_IMPL_HPP__
