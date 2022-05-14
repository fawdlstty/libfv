# 其他异步功能

## 暂停任务

```cpp
// 暂停 10 秒
co_await fv::Tasks::Delay (std::chrono::seconds (10));
```

## 外部需要用到 io_context

```cpp
boost::asio::io_context &_ctx = fv::Tasks::GetContext ();
```

## 异步锁

异步锁是一款适用于异步的锁。相对于标准库的 std::mutex 来说，有以下特性：

- 支持异步等待加锁
- 加锁与解锁不要求同一线程

创建锁：

```cpp
AsyncMutex _mtx {}; // 参数传 true 代表初始化时加锁
```

加锁：

```cpp
// 尝试加锁
bool _locked = _mtx.TryLock ();

// 异步加锁
co_await _mtx.Lock ();

// 异步超时锁
bool _locked = co_await _mtx.Lock (std::chrono::seconds (1));
```

解锁：

```cpp
_mtx.Unlock ();
```

获知是否已锁：

```cpp
_mtx.IsLocked ();
```
