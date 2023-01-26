uvhttpd: uvhttpd.c
	gcc \
	uvhttpd.c \
	llhttp/src/api.c llhttp/src/http.c llhttp/src/llhttp.c \
	-o uvhttpd \
	-I./llhttp/include -I/usr/local/include/uv \
	-luv -lpthread -ldl -lrt -lm


clean:
	rm uvhttpd


