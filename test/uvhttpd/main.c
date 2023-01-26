#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <llhttp.h>
#include <uv.h>

static int enable_print = 1;

#define LISTEN_ADDR "0.0.0.0"
#define LISTEN_PORT 8000
#define RESPONSE \
  "HTTP/1.1 200 OK\r\n" \
  "Content-Type: text/plain\r\n" \
  "Content-Length: 12\r\n" \
  "\r\n" \
  "hello world\n"

#define dprintf if (enable_print) printf
#define print_func dprintf("%s\n", __FUNCTION__);
#define uv_check_ret(r) do { if ((r)) { fprintf(stderr, "%d: %s\n", (r), uv_err_name((r))); abort(); } } while(0);
#define uv_check_ret_msg(r, msg) do { if ((r)) { fprintf(stderr, "%s, %d: %s\n", (msg), (r), uv_err_name((r))); abort(); } } while(0);
#define check_not_null(p) do { if (!p) { fprintf(stderr, "No memory!\n"); abort(); } } while (0);


uv_loop_t* uvloop;
uv_tcp_t http_server;
llhttp_settings_t http_settings;
uv_buf_t resbuf;

typedef struct {
	uv_tcp_t tcp;
	llhttp_t parser;
	uv_write_t wreq;
}client_t;

int on_message_begin(llhttp_t* llhttp) {
	print_func;
	return 0;
}

int on_url(llhttp_t* llhttp, const char* at, size_t length) {
	print_func;
	return 0;
}

int on_status(llhttp_t* llhttp, const char* at, size_t length) {
	print_func;
	return 0;
}

int on_method(llhttp_t* llhttp, const char* at, size_t length) {
	print_func;
	return 0;
}

int on_version(llhttp_t* llhttp, const char* at, size_t length) {
	print_func;
	return 0;
}

int on_header_field(llhttp_t* llhttp, const char* at, size_t length) {
	print_func;
	return 0;
}

int on_header_value(llhttp_t* llhttp, const char* at, size_t length) {
	print_func;
	return 0;
}

int on_chunk_extension_name(llhttp_t* llhttp, const char* at, size_t length) {
	print_func;
	return 0;
}

int on_chunk_extension_value(llhttp_t* llhttp, const char* at, size_t length) {
	print_func;
	return 0;
}

int on_headers_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

int on_body(llhttp_t* llhttp, const char* at, size_t length) {
	print_func;
	return 0;
}

void on_write(uv_write_t* req, int status) {
	print_func;
	client_t* client = req->data;
	assert(req == &client->wreq);
	assert(status == 0);
}

int on_message_complete(llhttp_t* llhttp) {
	print_func;
	client_t* client = llhttp->data;
	client->wreq.data = client;
	uv_write(&client->wreq, (uv_stream_t*)&client->tcp, &resbuf, 1, on_write);
	return 0;
}

int on_url_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

int on_status_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

int on_method_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

int on_version_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

int on_header_field_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

int on_header_value_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

int on_chunk_extension_name_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

int on_chunk_extension_value_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

int on_chunk_header(llhttp_t* llhttp) {
	print_func;
	return 0;
}

int on_chunk_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

int on_reset(llhttp_t* llhttp) {
	print_func;
	return 0;
}

void on_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	print_func; dprintf("suggested_size=%zu\n", suggested_size);
	buf->base = malloc(suggested_size);
	check_not_null(buf->base);
#ifdef _WIN32
	buf->len = (ULONG)suggested_size;
#else
	buf->len = suggested_size;
#endif
}

void on_close(uv_handle_t* peer) {
	print_func;
	free(peer); // since our client_t's first member is uv_tcp_t, so peer's addr IS our client's addr, just free it.
}

void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
	print_func; dprintf("nread= %Id\n", nread);
	client_t* client = stream->data;
	enum llhttp_errno parse_ret;

	if (nread < 0) {
		free(buf->base);
		uv_close((uv_handle_t*)stream, on_close);
		return;
	} else if (nread == 0) {
		free(buf->base);
		return;
	}

	parse_ret = llhttp_execute(&client->parser, buf->base, nread);
	if (parse_ret != HPE_OK) {
		fprintf(stderr, "Parse error: %s %s\n", llhttp_errno_name(parse_ret),
				client->parser.reason);
		uv_close((uv_handle_t*)stream, on_close);
	} else {
		
	}
	free(buf->base);
}

void on_connected(uv_stream_t* stream, int status) {
	print_func;
	assert(stream == (uv_stream_t*)&http_server);
	assert(status == 0);

	client_t* client = malloc(sizeof * client);
	check_not_null(client);
	int r = uv_tcp_init(stream->loop, &client->tcp);
	uv_check_ret(r);
	r = uv_accept(stream, (uv_stream_t*)&client->tcp);
	uv_check_ret_msg(r, "Accept error");

	client->tcp.data = client;
	llhttp_init(&client->parser, HTTP_REQUEST, &http_settings);
	client->parser.data = client;
	uv_read_start((uv_stream_t*)&client->tcp, on_alloc, on_read);
}

int main()
{
	int r;
	struct sockaddr_in addr;

	uvloop = uv_default_loop();

	resbuf.base = RESPONSE;
	resbuf.len = sizeof RESPONSE - 1;

	http_settings.on_message_begin = on_message_begin;
	http_settings.on_url = on_url;
	http_settings.on_status = on_status;
	http_settings.on_method = on_method;
	http_settings.on_version = on_version;
	http_settings.on_header_field = on_header_field;
	http_settings.on_header_value = on_header_value;
	http_settings.on_chunk_extension_name = on_chunk_extension_name;
	http_settings.on_chunk_extension_value = on_chunk_extension_value;
	http_settings.on_headers_complete = on_headers_complete;
	http_settings.on_body = on_body;
	http_settings.on_message_complete = on_message_complete;
	http_settings.on_url_complete = on_url_complete;
	http_settings.on_status_complete = on_status_complete;
	http_settings.on_method_complete = on_method_complete;
	http_settings.on_version_complete = on_version_complete;
	http_settings.on_header_field_complete = on_header_field_complete;
	http_settings.on_header_value_complete = on_header_value_complete;
	http_settings.on_chunk_extension_name_complete = on_chunk_extension_name_complete;
	http_settings.on_chunk_header = on_chunk_header;
	http_settings.on_chunk_complete = on_chunk_complete;
	http_settings.on_reset = on_reset;

	r = uv_tcp_init(uvloop, &http_server);
	uv_check_ret(r);
	r = uv_ip4_addr(LISTEN_ADDR, LISTEN_PORT, &addr);
	uv_check_ret(r);
	r = uv_tcp_bind(&http_server, (const struct sockaddr*)&addr, 0);
	uv_check_ret_msg(r, "Bind error");
	r = uv_listen((uv_stream_t*)&http_server, SOMAXCONN, on_connected);
	uv_check_ret_msg(r, "Listen error");
	r = uv_run(uvloop, UV_RUN_DEFAULT);
	return r;
}
