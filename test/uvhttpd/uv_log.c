#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <uv.h>
#include "uv_log.h"

static uv_log_level_t g_level = uv_log_level_info;

void uv_log_set_level(uv_log_level_t level) {
	g_level = level;
}

uv_log_level_t uv_log_get_level() {
	return g_level;
}

static void log_raw(uv_log_level_t level, const char* msg) {
	size_t off;
	uv_timeval64_t now;
	char buf[64];
	const char* c = "ADIWEF";

	if (level < g_level || level > uv_log_level_fatal) return;
	
	uv_gettimeofday(&now);
	off = strftime(buf, sizeof(buf), "%b %d %H:%M:%S.", localtime(&now.tv_sec));
	snprintf(buf + off, sizeof(buf) - off, "%03d", (int)(now.tv_usec / 1000));
	fprintf(stdout, "%s %c %s\n", buf, c[level], msg);
}

static void uv_log_vprintf(uv_log_level_t level, const char* fmt, va_list ap) {
	va_list cpy;
	char msg[1024], * p = msg;
	size_t len = sizeof(msg);
	int l = 0;

	while (1) {
		va_copy(cpy, ap);
		l = vsnprintf(p, len, fmt, cpy);
		va_end(cpy);

		if (l < 0) {
			return;
		} else if ((size_t)l >= len) {
			if (p != msg) free(p);
			len = (size_t)l + 1;
			p = malloc(len);
			if (p == NULL) {
				return;
			}
			continue;
		}
		break;
	}

	log_raw(level, p);
	if (p != msg)
		free(p);
}

void uv_log(uv_log_level_t level, const char* fmt, ...) {
	va_list ap;	
	if (level < g_level) return;
	va_start(ap, fmt);
	uv_log_vprintf(level, fmt, ap);
	va_end(ap);
	
}
