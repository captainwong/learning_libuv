#ifndef __HTTPD_H__
#define __HTTPD_H__

#pragma once

#include <uv.h>
#include <llhttp.h>


typedef struct {
	size_t offset;
	size_t len;
}uvhttpd_string_t;

typedef struct {
	uvhttpd_string_t key;
	uvhttpd_string_t value;
}uvhttpd_header_t;

typedef struct {
	size_t n;
	uvhttpd_header_t* headers; 
}uvhttpd_headers_t;

typedef struct {
	const char* base; // base address for offset/len
	uvhttpd_string_t remote; // remote address ip:port
	llhttp_method_t method;
	uvhttpd_string_t url;
	uvhttpd_string_t version;
	uvhttpd_headers_t headers; // user should NOT free
	uvhttpd_string_t body;
}uvhttpd_request_t;

typedef struct uvhttpd_client_s uvhttpd_client_t;
typedef struct uvhttpd_server_s uvhttpd_server_t;

typedef void(*on_request_t)(uvhttpd_client_t* client, uvhttpd_request_t* req);

void nprintf(const char* msg, size_t len, int newline);
int string_ncmp(const char* s1, size_t len1, const char* s2, size_t len2);
int string0_ncmp(const char* s1, const char* s2, size_t len2);
// enable `printf`s, default is disabled
void uvhttpd_enable_printf(int enable);
// return 0 for success, otherwise it is `uv_errno_t`
// if your want to use a existing `uv_loop_t`, pass it by `loop`
// otherwise a new `uv_loop_t` will be created.
int uvhttpd_init(uvhttpd_server_t** server, uv_loop_t* loop, on_request_t on_request);
void uvhttpd_free(uvhttpd_server_t* server);
// return 0 for success, otherwise it is uv_errno_t
// if you want to call uv_run() here, pass `runuv` by 1, otherwise you have to call `uv_run` yourself.
int uvhttpd_listen(uvhttpd_server_t* server, const char* ip, int port, int runuv);
// return 0 for success, otherwise it is uv_errno_t
int uvhttpd_write_response(uvhttpd_client_t* client, char* response, size_t len);



#endif
