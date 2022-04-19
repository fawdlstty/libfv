#include <fv/fv.hpp>

#include <conio.h>
#include <iostream>



Task<void> _test_async () {
	fv::Resp _r = co_await fv::Get ("http://www.baidu.com");
}

int main () {
	Tasks::Start (true);
	auto _r = _test_async ();
	std::cout << "press any key to exit.\n";
	::_getch ();
	Tasks::Stop ();
	return 0;
}
