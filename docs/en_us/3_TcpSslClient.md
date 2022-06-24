# TCP & SSL Client

## Create connection

```cpp
// tcp
std::shared_ptr<fv::IConn> _conn = co_await fv::Connect ("tcp://127.0.0.1:1234");

// ssl
std::shared_ptr<fv::IConn> _conn2 = co_await fv::Connect ("ssl://127.0.0.1:1235");
```

## Loop to receive content and print

Both TCP and SSL are streaming protocols. The length of a single packet cannot be accurately obtained. You need to specify the length in a customized format.

```cpp
// throw exception means link is broken
char _ch = co_await _conn->ReadChar ();
std::string _line = co_await _conn->ReadLine ();

// ReadCount and ReadCountVec will not return until specified length data has been received
std::string _buf = co_await _conn->ReadCount (1024);
std::vector<uint8_t> _buf2 = co_await _conn->ReadCountVec (1024);
```

## Send data

```cpp
// throw exception means link is broken
std::string _str = "hello";
co_await _conn->Send (_str.data (), _str.size ());
```

## Close connection

As long as the connection object is not referenced by the code, it is automatically freed by the smart pointer and the link is closed automatically. 
