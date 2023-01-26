#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>


#include "G:/dev_libs/uv.h"

uv_loop_t* loop;
uv_async_t async;

double percentage;

void fake_download(uv_work_t* req) {
    printf("fake_download thread is %p\n", uv_thread_self());
    int size = *((int*)req->data);
    int downloaded = 0;
    while (downloaded < size) {
        percentage = downloaded * 100.0 / size;
        async.data = (void*)&percentage;
        uv_async_send(&async);

        uv_sleep(1000);
        downloaded += (200 + rand()) % 1000; // can only download max 1000bytes/sec,
                                           // but at least a 200;
    }
}

void after(uv_work_t* req, int status) {
    printf("after thread is %p\n", uv_thread_self());
    fprintf(stderr, "Download complete\n");
    uv_close((uv_handle_t*)&async, NULL);
}

void print_progress(uv_async_t* handle) {
    printf("print_progress thread is %p\n", uv_thread_self());
    double percentage = *((double*)handle->data);
    fprintf(stderr, "Downloaded %.2f%%\n", percentage);
}

int main() {
    loop = uv_default_loop();

    uv_work_t req;
    int size = 10240;
    req.data = (void*)&size;

    printf("Main thread is %p\n", uv_thread_self());
    uv_async_init(loop, &async, print_progress);
    uv_queue_work(loop, &req, fake_download, after);

    return uv_run(loop, UV_RUN_DEFAULT);
}
