uvhttpd: uvhttpd.c httpd.h httpd.c
	gcc \
	uvhttpd.c httpd.c \
	llhttp/src/api.c llhttp/src/http.c llhttp/src/llhttp.c \
	-o uvhttpd \
	-I./llhttp/include -I/usr/local/include/uv \
	-luv -lpthread -ldl -lrt -lm


clean:
	rm uvhttpd


