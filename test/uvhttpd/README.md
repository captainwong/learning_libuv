# uvhttpd

inspired by Ryan Dahl's great [video](https://vimeo.com/24713213).

using:

* [libuv-1.44.2](https://github.com/libuv/libuv)
* [llhttp-8.1.0](https://github.com/nodejs/llhttp)


## 一个发现

客户端执行 `curl -X POST http://localhost:8000/api/disable_print -F "NAME=ASDF"` 的一次输出：

```
on_connected
on_alloc
suggested_size=65536
on_read
nread= 203
POST /api/disable_print HTTP/1.1
Host: localhost:8000
User-Agent: curl/7.58.0
Accept: */*
Content-Length: 143
Content-Type: multipart/form-data; boundary=------------------------34c5d99a8d767553


on_message_begin
on_method
on_method_complete
on_url
on_url_complete
on_version
on_version_complete
on_header_field
on_header_field_complete
on_header_value
on_header_value_complete
on_header_field
on_header_field_complete
on_header_value
on_header_value_complete
on_header_field
on_header_field_complete
on_header_value
on_header_value_complete
on_header_field
on_header_field_complete
on_header_value
on_header_value_complete
on_header_field
on_header_field_complete
on_header_value
on_header_value_complete
on_headers_complete
on_alloc
suggested_size=65536
on_read
nread= 143
--------------------------34c5d99a8d767553
Content-Disposition: form-data; name="NAME"

ASDF
--------------------------34c5d99a8d767553--

on_body
on_message_complete
on_write
on_alloc
suggested_size=65536
on_read
nread= -4095
on_close

```

可以看到，一次 `http` 请求有可能会分成多个 `tcp` 包，即 `on_read` 内调用 `llhttp_execute` 时，有可能不带 `BODY`，在下一次 `on_read` 时再次调用 `llhttp_execute` 时，`llhttp` 是可以正常继续处理的.

但由于我们的 `on_alloc` 每次都会 `malloc` 新内存，且在 `on_read` 内调用 `llhttp_execute` 后就立即 `free` 掉了 `buf`，所以在 `on_headers_complete` 之前的回调函数比如 `on_url, on_version, on_header_field, on_header_value` 内保存指针和偏移量，在 `on_message_complete` 回调时有可能是已经被 `free` 掉了的.

解决办法是要么在 `on_url, on_version, on_header_field, on_header_value` 等回调内复制一份，或者使用改进的 `on_alloc`，自己管理 `tcp` 接收缓冲区.