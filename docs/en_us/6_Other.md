# Another asynchronous functionality

## Sleep task

```cpp
// Sleep 10 seconds
co_await fv::Tasks::Delay (std::chrono::seconds (10));
```

## If external needs io_context

```cpp
boost::asio::io_context &_ctx = fv::Tasks::GetContext ();
```

## Asynchronous mutex

Asynchronous mutex is a mutex suitable for asynchronous environments. In contrast to std::mutex, there are the following features:

- Supports asynchronous wait locking
- Locking and unlocking do not require the same thread

Create mutex

```cpp
AsyncMutex _mtx {}; // Pass the true argument to lock during initialization
```

Lock:

```cpp
// try lock
bool _locked = _mtx.TryLock ();

// asynchronous lock
co_await _mtx.Lock ();

// asynchronous timeout lock
bool _locked = co_await _mtx.Lock (std::chrono::seconds (1));
```

Unlock:

```cpp
_mtx.Unlock ();
```

To know if it is locked:

```cpp
_mtx.IsLocked ();
```
