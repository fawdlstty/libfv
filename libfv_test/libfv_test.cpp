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
#define _SILENCE_CXX23_ALIGNED_STORAGE_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX23_DEPRECATION_WARNINGS
#endif

#include <iostream>
#include <string>
#include <fv/fv.h>



Task<void> test_client() {
    try {
        std::shared_ptr<fv::WsConn> _conn = co_await fv::ConnectWS("ws://127.0.0.1:8080/websocket");
        std::cout << "";
        while (true) {
            auto [_data, _type] = co_await _conn->Recv();
            std::cout << _data << std::endl;
        }
    } catch (std::exception &_e) {
        std::cout << "catch exception: " << _e.what() << std::endl;
    } catch (...) {
        std::cout << "catch exception" << std::endl;
    }
    fv::Tasks::Stop();
}

int main () {
	fv::Tasks::Init ();
	fv::Tasks::RunAsync (test_client);
	fv::Tasks::Run ();
	return 0;
}