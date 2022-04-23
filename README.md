# libfv

基于 boost.asio 的 C++20 纯头文件异步 HTTP 库

## 配置环境

步骤0：配置 `vcpkg` 环境

```
vcpkg install boost-asio:x86-windows-static
vcpkg install nlohmann-json:x86-windows-static
```

步骤1：克隆项目并初始化子模块

```
git clone https://github.com/fawdlstty/libfv
git.exe submodule update --init --recursive
```

步骤2：项目头文件搜索路径加入 `仓库根目录/include`、`仓库根目录/gzip-hpp/include`

步骤3：创建一个空cpp文件并引入libfv实现文件。这个文件最好不要加入其他代码

```cpp
#include <fv/fv-impl.hpp>
```

步骤4：在需要使用api的源码文件里引入库头文件

```cpp
#include <fv/fv.hpp>
```

步骤5：主函数进入及退出时调用初始化、释放函数

```cpp
// 初始化
fv::Tasks::Start (true);

// 释放
fv::Tasks::Stop ();
```

步骤6：创建异步函数时，通过 `fv::Tasks::RunAsync` 加入异步执行队列

```cpp
// 异步函数
Task<void> async_func () {
	fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_kv ("a", "aaa"));
	std::cout << _r.Content;
}

// 执行异步函数
fv::Tasks::RunAsync (async_func);
```

现在我们已经创建好了异步函数，可以自由在里面编写异步代码啦！

## 使用手册

### 发起 HttpGet 请求

```cpp
fv::Response _r = co_await fv::Get ("https://t.cn");
```

### 发起 HttpPost 请求

```cpp
// 提交 application/json 格式
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_kv ("a", "aaa"));

// 提交 application/x-www-form-urlencoded 格式
fv::Response _r2 = co_await fv::Post ("https://t.cn", fv::body_kv ("a", "aaa"), fv::content_type ("application/x-www-form-urlencoded"));
```

### 提交文件

```cpp
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_file ("a", "filename.txt", "content..."));
```

### 发起 HttpPost 请求并提交原始内容

```cpp
fv::Response _r = co_await fv::Post ("https://t.cn", fv::body_raw ("application/octet-stream", "aaa"));
// 备注：指定了原始内容后，无法再通过 fv::body_kv、fv::body_file指定格式化内容
```

共支持6种HTTP请求，还可使用 `fv::Head`、`fv::Option`、`fv::Put`、`fv::Delete` 方法。其中 `fv::body_raw` 参数只能用于 `fv::Post` 和 `fv::Put`。

### 指定请求超时时长

```cpp
fv::Response _r = co_await fv::Get ("https://t.cn", fv::timeout (std::chrono::seconds (10)));
```

### 向指定服务器发起请求

```cpp
fv::Response _r = co_await fv::Get ("https://t.cn", fv::server ("106.75.237.200"));
```

### 禁用tcp延迟

```cpp
fv::Response _r = co_await fv::Get ("https://t.cn", fv::no_delay (true));
```

### 自定义http头

```cpp
fv::Response _r = co_await fv::Get ("https://t.cn", fv::header ("X-WWW-Router", "123456789"));
```

### 自定义鉴权

```cpp
// jwt bearer 鉴权
fv::Response _r = co_await fv::Get ("https://t.cn", fv::authorization ("Bearer XXXXXXXXXXXXX=="));
// 用户名密码鉴权
fv::Response _r1 = co_await fv::Get ("https://t.cn", fv::authorization ("admin", "123456"));
```

### 设置http头 `Connection` 属性

```cpp
fv::Response _r = co_await fv::Get ("https://t.cn", fv::connection ("keep-alive"));
```

### 设置http头 `Content-Type` 属性

```cpp
fv::Response _r = co_await fv::Get ("https://t.cn", fv::content_type ("application/octet-stream"));
```

### 设置http头 `Referer` 属性

```cpp
fv::Response _r = co_await fv::Get ("https://t.cn", fv::referer ("https://t.cn"));
```

### 设置http头 `User-Agent` 属性

```cpp
fv::Response _r = co_await fv::Get ("https://t.cn", fv::user_agent ("Mozilla/4.0 Chrome 2333"));
```

## 计划

- [ ] TCP
- [x] Http(s)
- [ ] UDP
- [ ] Websocket
- [ ] SSL
