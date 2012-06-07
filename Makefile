CFLAGS=-std=c99 

CC="gcc"

all: http_parser.o
	$(CC) http-parser/http_parser.o src/b64.c src/websocket.c \
          src/utils.c src/buffer.c  \
          src/dispatch.c src/static.c \
          src/network.c src/rio.c \
          -O2 -Wall -lcrypto -lzmq -lczmq -lmsgpack -std=c99 -o rio -DDEBUG=0

debug:
	$(CC) http-parser/http_parser.o -g src/b64.c src/websocket.c \
		  src/utils.c src/buffer.c \
		  src/dispatch.c src/static.c \
		  src/network.c src/rio.c \
		  -O2 -lcrypto -lzmq -lczmq -lmsgpack -std=c99 -o rio -DDEBUG=1 
test:
	$(CC) http-parser/http_parser.o -g src/b64.c src/websocket.c \
		  src/utils.c src/buffer.c \
		  src/dispatch.c src/static.c \
		  src/network.c src/tests.c \
		  -O2 -lcrypto -lzmq -lczmq -lmsgpack -lcunit -std=c99 -o test_rio -DDEBUG=1 -DTEST=1
	      ./test_rio

HTTP_PARSER_DIR=http-parser

http_parser.o:
	cd $(HTTP_PARSER_DIR); $(MAKE) http_parser.o
