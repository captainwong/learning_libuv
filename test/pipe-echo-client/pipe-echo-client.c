#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../libuv.h"


#ifdef _WIN32
#define PIPENAME "\\\\?\\pipe\\echo.sock"
#define strdup _strdup
#else
#define PIPENAME "/tmp/echo.sock"
#endif

uv_loop_t* loop;


typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

void free_write_req(uv_write_t* req) {
    write_req_t* wr = (write_req_t*)req;
    free(wr->buf.base);
    free(wr);
}

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

void read_cb(uv_stream_t* stream,
             ssize_t nread,
             const uv_buf_t* buf)
{
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t*)stream, NULL);
    } else {
        if (nread < 8) {
            char msg[8];
            memcpy(msg, buf->base, nread);
            msg[nread] = '\0';
            printf("Server response: %s\n", msg);
        } else {
            printf("Server response: %zu\n", nread);
        }
    }

    uv_close((uv_handle_t*)stream, NULL);
    free(buf->base);
}

void write_cb(uv_write_t* req, int status) {
    if (status < 0) {
        fprintf(stderr, "Write error %s\n", uv_err_name(status));
    }

    uv_read_start(req->handle, alloc_buffer, read_cb);
    free_write_req(req);
}

static void connect_cb(uv_connect_t* connect_req, int status) {
    //ASSERT(status == UV_ENOENT);
    //uv_close((uv_handle_t*)connect_req->handle, close_cb);
    //connect_cb_called++;

    if (status < 0) {
        fprintf(stderr, "connect to %s failed: %s\n", PIPENAME, uv_strerror(status));
        return;
    }

    write_req_t* req = (write_req_t*)malloc(sizeof(write_req_t));
    req->buf = uv_buf_init(strdup("hello\n"), strlen("hello\n"));
    int r = uv_write(&req->req, connect_req->handle, &req->buf, 1, write_cb);
    if (r < 0) {
        fprintf(stderr, "uv_write failed: %s\n", uv_strerror(status));
    }
}

int main() {
    loop = uv_default_loop();

    uv_pipe_t client;
    uv_connect_t req;
    uv_pipe_init(loop, &client, 0);
    uv_pipe_connect(&req, &client, PIPENAME, connect_cb);

    printf("connecting to %s\n", PIPENAME);
    return uv_run(loop, UV_RUN_DEFAULT);
}
