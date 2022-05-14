# 开始使用

## 配置 `vcpkg` 环境

libfv 依赖 vcpkg 环境，因此在使用 libfv 之前需配置

```
# 安装依赖包
vcpkg install asio fmt gzip-hpp nlohmann-json openssl zlib
# `asio` 可换成 `boost-asio`
```

## 配置 libfv 环境

首先克隆 libfv 仓库

```
git clone git@github.com:fawdlstty/libfv.git
```

然后将仓库内 include 文件夹添加到你的项目的头文件搜索路径。如果这步不会，那么将仓库内 `include/fv` 文件夹拷贝到你的项目内，后续通过 `#include "fv/fv.hpp"` 引用即可。

## 初始化

```cpp
// 引入库头文件（使用独立asio则需定义宏）
#define ASIO_STANDALONE
#include <fv/fv.hpp>

// 主函数
int main () {
	// 全局初始化（参数可指定外部 asio::io_context 指针）
	fv::Tasks::Init ();

	// ...

	// 循环处理任务（其他地方调用 `fv::Tasks::Stop ()` 可退出）
	fv::Tasks::LoopRun ();

	// 全局释放
	fv::Tasks::Release ();
	return 0;
}
```

## 进入异步

调用异步函数时，通过 `fv::Tasks::RunAsync` 将其加入异步任务队列

```cpp
// 异步函数
Task<void> async_func () {
	fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_kv ("a", "aaa"));
	std::cout << _r.Content;
}
// 带参异步函数
Task<void> async_func2 (int n) {
	std::cout << n << std::endl;
	co_return;
}

// 执行异步函数，可以把 std::function<Task<void> ()> 类型方法塞进去
fv::Tasks::RunAsync (async_func);
fv::Tasks::RunAsync (async_func2, 5);
```

现在我们已经创建好了异步函数环境，可以自由在里面编写异步代码啦！
