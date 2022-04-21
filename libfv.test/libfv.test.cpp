#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING
#define _SILENCE_CXX20_CODECVT_FACETS_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SILENCE_ALL_CXX20_DEPRECATION_WARNINGS
#define PIO_APC_ROUTINE_DEFINED
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WIN32_LEAN_AND_MEAN



#include <fv/fv.hpp>

#include <conio.h>
#include <iostream>

#pragma comment (lib, "Crypt32.lib")



Task<void> _test_async () {
	fv::Response _r = co_await fv::Get ("http://www.fawdlstty.com");
	std::cout << _r.Content;
}

int main () {
	// 控制台utf8支持
	::SetConsoleOutputCP (65001);
	//
	Tasks::Start (true);
	Tasks::RunAsync (_test_async);
	::_getch ();
	Tasks::Stop ();
	return 0;
}
