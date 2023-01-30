#include "httpd.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int enable_print = 0;

#define dprintf if (enable_print) printf
#define dnprintf if (enable_print) nprintf
#define print_func dprintf("%s\n", __FUNCTION__);
#define uv_check_ret(r) do { if ((r)) { fprintf(stderr, "%d: %s\n", (r), uv_err_name((r))); abort(); } } while(0);
#define uv_check_ret_msg(r, msg) do { if ((r)) { fprintf(stderr, "%s, %d: %s\n", (msg), (r), uv_err_name((r))); abort(); } } while(0);
#define check_not_null(p) do { if (!p) { fprintf(stderr, "No memory!\n"); abort(); } } while (0);


#define HEADERS_DEFAULT_LENGTH 16
#define DEFAULT_BUFF_SIZE 1024


typedef struct {
	char* buf;
	char mybuf[DEFAULT_BUFF_SIZE];
	size_t size, capacity;
}mybuf_t;

struct uvhttpd_server_s {
	uv_tcp_t tcp;
	llhttp_settings_t http_settings;
	on_request_t on_request;
	uv_loop_t* myloop;
};

struct uvhttpd_client_s {
	uv_tcp_t tcp;
	llhttp_t parser;
	on_request_t on_request;
	uvhttpd_request_t req;
	uvhttpd_header_t headers[HEADERS_DEFAULT_LENGTH];
	mybuf_t buf;
	mybuf_t pkt;
};

struct write_req_t {
	uv_write_t req;
	uv_buf_t buf;
};

static void on_close(uv_handle_t* peer);


/*************************** mybuf_t functions ****************/

static void mybuf_init(mybuf_t* buf) {
	buf->buf = buf->mybuf;
	buf->size = 0;
	buf->capacity = DEFAULT_BUFF_SIZE;
}

static size_t mybuf_space(mybuf_t* buf) {
	return buf->capacity - buf->size;
}

static void mybuf_reserve(mybuf_t* buf, size_t size) {
	if (mybuf_space(buf) < size) {
		dprintf("WARN: mybuf_t not enough, space=%zu, needed=%zu\n", mybuf_space(buf), size);
		while (buf->capacity < size) {
			buf->capacity *= 2;
		}
		if (buf->buf == buf->mybuf) {
			buf->buf = malloc(buf->capacity);
			check_not_null(buf->buf);
			memcpy(buf->buf, buf->mybuf, buf->size);
		} else {
			char* tmp = realloc(buf->buf, buf->capacity);
			check_not_null(tmp);
			buf->buf = tmp;
		}
	}
}

static void mybuf_append(mybuf_t* buf, const char* data, size_t len) {
	if (mybuf_space(buf) >= len) {
		memcpy(buf->buf + buf->size, data, len);
		buf->size += len;
	} else {
		dprintf("WARN: mybuf_t not enough, space=%zu, needed=%zu\n", mybuf_space(buf), len);
		while (buf->capacity < len) {
			buf->capacity *= 2;
		}
		if (buf->buf == buf->mybuf) {			
			buf->buf = malloc(buf->capacity);
			check_not_null(buf->buf);
			memcpy(buf->buf, buf->mybuf, buf->size);
		} else {
			char* tmp = realloc(buf->buf, buf->capacity);
			check_not_null(tmp);
			buf->buf = tmp;
		}
		memcpy(buf->buf + buf->size, data, len);
		buf->size += len;
	}
}

static void mybuf_clear(mybuf_t* buf) {
	if (buf->buf != buf->mybuf) {
		free(buf->buf);
		buf->buf = buf->mybuf;
	}
	buf->size = 0;
	buf->capacity = DEFAULT_BUFF_SIZE;
}


/*************************** helper functions ****************/

static void reset_request(uvhttpd_client_t* client) {
	if (client->req.headers.headers != client->headers) {
		free(client->req.headers.headers);
	}
	memset(&client->req, 0, sizeof(uvhttpd_request_t));
	client->req.headers.headers = client->headers;
}

static void headers_append_key(uvhttpd_client_t* client, size_t offset, size_t len) {
	if (client->req.headers.n == HEADERS_DEFAULT_LENGTH) {
		uvhttpd_header_t* headers = malloc((client->req.headers.n + 1) * sizeof(uvhttpd_header_t));
		check_not_null(headers);
		memcpy(headers, client->req.headers.headers, (client->req.headers.n) * sizeof(uvhttpd_header_t));
		client->req.headers.headers = headers;
	} else if (client->req.headers.n > HEADERS_DEFAULT_LENGTH) {
		uvhttpd_header_t* headers = realloc(client->req.headers.headers, (client->req.headers.n + 1) * sizeof(uvhttpd_header_t));
		check_not_null(headers);
		client->req.headers.headers = headers;
	}
	client->req.headers.headers[client->req.headers.n].key.offset = offset;
	client->req.headers.headers[client->req.headers.n].key.len = len;
}

static void headers_append_value(uvhttpd_client_t* client, size_t offset, size_t len) {
	client->req.headers.headers[client->req.headers.n].value.offset = offset;
	client->req.headers.headers[client->req.headers.n].value.len = len;
	client->req.headers.n++;
}

static int headers_contains(uvhttpd_client_t* client, const char* key, const char* value) {
	for (size_t i = 0; i < client->req.headers.n; i++) {
		uvhttpd_header_t header = client->req.headers.headers[i];
		if(0 == string0_ncmp(key, client->req.base + header.key.offset, header.key.len)
		   && 0 == string0_ncmp(value, client->req.base + header.value.offset, header.value.len)) {
			return 1;
		}
	}
	return 0;
}


/*************************** llhttp callback functions ****************/

static int on_message_begin(llhttp_t* llhttp) {
	print_func;
	uvhttpd_client_t* client = llhttp->data; 
	mybuf_clear(&client->pkt); 
	reset_request(client);
	return 0;
}

static int on_url(llhttp_t* llhttp, const char* at, size_t length) {
	print_func;
	dnprintf(at, length, 1);
	uvhttpd_client_t* client = llhttp->data;
	client->req.url.offset = client->pkt.size;
	client->req.url.len = length;
	mybuf_append(&client->pkt, at, length);
	return 0;
}

static int on_status(llhttp_t* llhttp, const char* at, size_t length) {
	print_func;
	dnprintf(at, length, 1);
	return 0;
}

static int on_method(llhttp_t* llhttp, const char* at, size_t length) {
	print_func;
	dnprintf(at, length, 1);
	return 0;
}

static int on_version(llhttp_t* llhttp, const char* at, size_t length) {
	print_func;
	dnprintf(at, length, 1);
	uvhttpd_client_t* client = llhttp->data;
	client->req.version.offset = client->pkt.size;
	client->req.version.len = length;
	mybuf_append(&client->pkt, at, length);
	return 0;
}

static int on_header_field(llhttp_t* llhttp, const char* at, size_t length) {
	print_func;
	dnprintf(at, length, 1);
	uvhttpd_client_t* client = llhttp->data;
	headers_append_key(client, client->pkt.size, length);
	mybuf_append(&client->pkt, at, length);
	return 0;
}

static int on_header_value(llhttp_t* llhttp, const char* at, size_t length) {
	print_func;
	dnprintf(at, length, 1);
	uvhttpd_client_t* client = llhttp->data;
	headers_append_value(client, client->pkt.size, length);
	mybuf_append(&client->pkt, at, length);
	return 0;
}

static int on_chunk_extension_name(llhttp_t* llhttp, const char* at, size_t length) {
	print_func;
	dnprintf(at, length, 1);
	return 0;
}

static int on_chunk_extension_value(llhttp_t* llhttp, const char* at, size_t length) {
	print_func;
	dnprintf(at, length, 1);
	return 0;
}

static int on_headers_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

static int on_body(llhttp_t* llhttp, const char* at, size_t length) {
	print_func;
	dnprintf(at, length, 1);
	uvhttpd_client_t* client = llhttp->data;
	client->req.body.offset = client->pkt.size;
	client->req.body.len = length;
	mybuf_append(&client->pkt, at, length);
	return 0;
}

static int on_message_complete(llhttp_t* llhttp) {
	print_func;
	uvhttpd_client_t* client = llhttp->data;
	client->req.base = client->pkt.buf;
	client->on_request(client, &client->req);

	if (!headers_contains(client, "Connection", "Keep-Alive")) {
		uv_close((uv_handle_t*)&client->tcp, on_close);
	}

	reset_request(client);

	return 0;
}

static int on_url_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

static int on_status_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

static int on_method_complete(llhttp_t* llhttp) {
	print_func;
	uvhttpd_client_t* client = llhttp->data;
	client->req.method = llhttp->method;
	return 0;
}

static int on_version_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

static int on_header_field_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

static int on_header_value_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

static int on_chunk_extension_name_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

static int on_chunk_extension_value_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

static int on_chunk_header(llhttp_t* llhttp) {
	print_func;
	return 0;
}

static int on_chunk_complete(llhttp_t* llhttp) {
	print_func;
	return 0;
}

static int on_reset(llhttp_t* llhttp) {
	print_func;
	return 0;
}

static void setup_default_llhttp_settings(llhttp_settings_t* settings) {
	settings->on_message_begin = on_message_begin;
	settings->on_url = on_url;
	settings->on_status = on_status;
	settings->on_method = on_method;
	settings->on_version = on_version;
	settings->on_header_field = on_header_field;
	settings->on_header_value = on_header_value;
	settings->on_chunk_extension_name = on_chunk_extension_name;
	settings->on_chunk_extension_value = on_chunk_extension_value;
	settings->on_headers_complete = on_headers_complete;
	settings->on_body = on_body;
	settings->on_message_complete = on_message_complete;
	settings->on_url_complete = on_url_complete;
	settings->on_status_complete = on_status_complete;
	settings->on_method_complete = on_method_complete;
	settings->on_version_complete = on_version_complete;
	settings->on_header_field_complete = on_header_field_complete;
	settings->on_header_value_complete = on_header_value_complete;
	settings->on_chunk_extension_name_complete = on_chunk_extension_name_complete;
	settings->on_chunk_header = on_chunk_header;
	settings->on_chunk_complete = on_chunk_complete;
	settings->on_reset = on_reset;
}

/*************************** uv callback functions ****************/

static void on_write(uv_write_t* req, int status) {
	print_func;
	assert(status == 0);
	struct write_req_t* wr = req->data;
	free(wr->buf.base);
	free(wr);
}

static void on_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	print_func; dprintf("suggested_size=%zu\n", suggested_size);
	uvhttpd_client_t* client = handle->data;
	mybuf_reserve(&client->buf, DEFAULT_BUFF_SIZE);
	buf->base = client->buf.buf;
#ifdef _WIN32
	buf->len = (ULONG)mybuf_space(&client->buf);
#else
	buf->len = mybuf_space(&client->buf);
#endif
}

static void on_close(uv_handle_t* peer) {
	print_func;
	uvhttpd_client_t* client = peer->data;
	mybuf_clear(&client->buf);
	mybuf_clear(&client->pkt);
	reset_request(client);
	free(peer); // since our client_t's first member is uv_tcp_t, so peer's addr IS our client's addr, just free it.
}

static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
	print_func; dprintf("nread= %zd\n", nread);
	uvhttpd_client_t* client = stream->data;
	enum llhttp_errno parse_ret;

	if (nread < 0) {
		uv_close((uv_handle_t*)stream, on_close);
		return;
	} else if (nread == 0) {
		return;
	}

	dprintf("before llhttp_execute\n");
	dnprintf(buf->base, nread, 1);
	parse_ret = llhttp_execute(&client->parser, client->buf.buf, nread);
	if (parse_ret != HPE_OK) {
		fprintf(stderr, "Parse error: %s %s\n", llhttp_errno_name(parse_ret),
				client->parser.reason);
		uv_close((uv_handle_t*)stream, on_close);
	} else {
		// parse succeed, on_request_t should be called in on_message_complete		
	}
	dprintf("after llhttp_execute\n");
	mybuf_clear(&client->buf);
}

static void on_connected(uv_stream_t* stream, int status) {
	print_func;
	assert(status == 0);
	uvhttpd_server_t* server = stream->data;
	uvhttpd_client_t* client = malloc(sizeof * client);
	check_not_null(client);
	int r = uv_tcp_init(stream->loop, &client->tcp);
	uv_check_ret(r);
	r = uv_accept(stream, (uv_stream_t*)&client->tcp);
	uv_check_ret_msg(r, "Accept error");

	client->on_request = server->on_request;
	client->tcp.data = client;
	llhttp_init(&client->parser, HTTP_REQUEST, &server->http_settings);
	client->parser.data = client;
	mybuf_init(&client->buf);
	mybuf_init(&client->pkt);
	client->req.headers.headers = client->headers;
	client->req.headers.n = 0;
	uv_read_start((uv_stream_t*)&client->tcp, on_alloc, on_read);
}


/*************************** public functions ****************/

void nprintf(const char* at, size_t len, int newline) {
	char buf[4096];
	char* p = buf;
	if (len < sizeof buf) {
		p = buf;
	} else {
		p = malloc(len + 1);
		check_not_null(p);
	}
	memcpy(p, at, len);
	p[len] = '\0';
	printf("%s", p);
	if (newline)
		printf("\n");

	if (p != buf) {
		free(p);
	}
}

int string_ncmp(const char* s1, size_t len1, const char* s2, size_t len2) {
	if (len1 == len2) {
		return memcmp(s1, s2, len1);
	}
	return 1;
}

int string0_ncmp(const char* s1, const char* s2, size_t len2) {
	size_t len1 = strlen(s1);
	if (len1 == len2) {
		return memcmp(s1, s2, len1);
	}
	return 1;
}

void uvhttpd_enable_printf(int enable) {
	enable_print = enable;
}

int uvhttpd_init(uvhttpd_server_t** server, uv_loop_t* loop, on_request_t on_request) {
	int r = UV_ENOMEM;
	uvhttpd_server_t* s;
	uv_loop_t* myloop = loop;

	if (myloop == NULL) {
		myloop = uv_loop_new();
		if (myloop == NULL) {
			return UV_ENOMEM;
		}
	}

	s = malloc(sizeof(*s));
	if (!s) {
		goto failed;
	}
	if (myloop != loop) {
		s->myloop = myloop;
	} else {
		s->myloop = NULL;
	}

	r = uv_tcp_init(myloop, &s->tcp);
	if (r) {
		goto failed;
	}
	s->tcp.data = s;
	s->on_request = on_request;
	setup_default_llhttp_settings(&s->http_settings);

	*server = s;
	return 0;

failed:
	uvhttpd_free(s);
	return r;
}

void uvhttpd_free(uvhttpd_server_t* server) {
	if (server->myloop) {
		uv_loop_close(server->myloop);
	}
	free(server);
}

int uvhttpd_listen(uvhttpd_server_t* server, const char* ip, int port, int runuv)
{
	int r;
	struct sockaddr_in addr;

	r = uv_ip4_addr(ip, port, &addr);
	if (r) return r;

	r = uv_tcp_bind(&server->tcp, (const struct sockaddr*)&addr, 0);
	if (r) return r;

	r = uv_listen((uv_stream_t*)&server->tcp, SOMAXCONN, on_connected);
	if (r)return r;

	if (runuv) {
		r = uv_run(server->tcp.loop, UV_RUN_DEFAULT);
	}

	return r;
}

int uvhttpd_write_response(uvhttpd_client_t* client, char* response, size_t len)
{
	struct write_req_t* req = malloc(sizeof * req);
	if (!req) return UV_ENOMEM;
	req->buf.base = malloc(len);
	if (!req->buf.base) {
		free(req);
		return UV_ENOMEM;
	}
	memcpy(req->buf.base, response, len);
#ifdef _WIN32
	req->buf.len = (ULONG)len;
#else
	req->buf.len = len;
#endif
	req->req.data = req;
	return uv_write(&req->req, (uv_stream_t*)&client->tcp, &req->buf, 1, on_write);
}
