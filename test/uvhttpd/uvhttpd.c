#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "httpd.h"

static int enable_print = 0;

#define LISTEN_ADDR "0.0.0.0"
#define LISTEN_PORT 8000
#define RESPONSE \
  "HTTP/1.1 200 OK\r\n" \
  "Content-Type: text/plain\r\n" \
  "Content-Length: 12\r\n" \
  "\r\n" \
  "hello world\n"


//uv_loop_t* uvloop;
//uv_tcp_t http_server;
//llhttp_settings_t http_settings;
//uv_buf_t resbuf;

void on_request(uvhttpd_client_t* client, uvhttpd_request_t* req) {
	if (enable_print) {
		printf("METHOD: %s\n", llhttp_method_name(req->method));
		printf("URL: "); nprintf(req->base + req->url.offset, req->url.len, 1);
		printf("VERSION: "); nprintf(req->base + req->version.offset, req->version.len, 1);
		printf("HEADERS: %zu\n", req->headers.n);
		for (size_t i = 0; i < req->headers.n; i++) {
			printf("  ");  nprintf(req->base + req->headers.headers[i].key.offset, req->headers.headers[i].key.len, 0); printf(": ");
			nprintf(req->base + req->headers.headers[i].value.offset, req->headers.headers[i].value.len, 1);
		}
		printf("BODY: \n"); nprintf(req->base + req->body.offset, req->body.len, 1);
	}

	if (string0_ncmp("/api/enable_print", req->base + req->url.offset, req->url.len) == 0) {
		uvhttpd_enable_printf(1);
	} else if (string0_ncmp("/api/disable_print", req->base + req->url.offset, req->url.len) == 0) {
		uvhttpd_enable_printf(0);
	}

	uvhttpd_write_response(client, RESPONSE, sizeof RESPONSE - 1);
}

int main()
{
	/*int r;

	uvloop = uv_default_loop();

	resbuf.base = RESPONSE;
	resbuf.len = sizeof RESPONSE - 1;
	
	r = uv_run(uvloop, UV_RUN_DEFAULT);
	return r;*/
	 
	uvhttpd_server_t* server;
	uvhttpd_enable_printf(0);
	uv_default_loop();
	int r = uvhttpd_init(&server, uv_default_loop(), on_request);
	if (r) {
		fprintf(stderr, "%d %s\n", r, uv_err_name(r));
		return r;
	}

	r = uvhttpd_listen(server, LISTEN_ADDR, LISTEN_PORT, 0);
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	return r;
}
