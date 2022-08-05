#ifdef _MSC_VER
#   define _WIN32_WINNT 0x0601
#   pragma warning (disable: 4068)
#   pragma comment (lib, "Crypt32.lib")
//#	ifdef _RESUMABLE_FUNCTIONS_SUPPORTED
//#		undef _RESUMABLE_FUNCTIONS_SUPPORTED
//#	endif
//#	ifndef __cpp_lib_coroutine
//#		define __cpp_lib_coroutine
//#	endif
#endif

#include <iostream>
#include <string>
#include <fv/fv.h>



Task<void> test_server () {
	fv::HttpServer _server {};
	_server.SetHttpHandler ("/hello", [] (fv::Request &_req) -> Task<fv::Response> {
		co_return fv::Response::FromText ("hello world");
	});
	std::cout << "You can access from browser: http://127.0.0.1:8080/hello\n";
	co_await _server.Run (8080);
}

int main () {
	fv::Tasks::Init ();
	fv::Tasks::RunMainAsync (test_server);
	fv::Tasks::Run ();
	return 0;
}