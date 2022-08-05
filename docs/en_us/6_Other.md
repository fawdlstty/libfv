# Another asynchronous functionality

## Sleep task

```cpp
// Sleep 10 seconds
co_await fv::Tasks::Delay (std::chrono::seconds (10));
```

## If external needs io_context

```cpp
fv::IoContext &_ctx = fv::Tasks::GetContext ();
```

## Asynchronous mutex

Asynchronous mutex is a mutex suitable for asynchronous environments. In contrast to `std::mutex`, there are the following features:

- Supports asynchronous wait locking
- Locking and unlocking do not require the same thread

Create mutex

```cpp
fv::AsyncMutex _mtx {}; // Pass the true argument to lock during initialization
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
bool _locked = _mtx.IsLocked ();
```

## Asynchronous semaphore

An asynchronous semaphore is a semaphore suitable for asynchrony.  In contrast to the library's `std::counting_semaphore`, there are the following features:

- Supports asynchronous wait for acquire

Create semaphore:

```cpp
fv::AsyncSemaphore _sema { 1 }; // Parameter means the initial number of resources
```

Acquire resources:

```cpp
// try acquire resources
bool _acq = _sema.TryAcquire ();

// asynchronous acquire resources
co_await _mtx.Acquire ();

// asynchronous timeout acquire resources
bool _acq = co_await _mtx.Acquire (std::chrono::seconds (1));
```

Release resources:

```cpp
_mtx.Release ();
```

Get the count of available resources:

```cpp
size_t _count = _mtx.GetResCount ();
```

## Example

TODO
