CC="gcc"

CFLAGS=-std=c99
LIBS=-lcrypto -lzmq -lczmq -lmsgpack

FAST=-O3
DEBUG=-g

all: http_parser.o websocket.o
	$(CC) -I cws/src http-parser/http_parser.o cws/b64.o cws/websocket.o \
          src/utils.c src/buffer.c src/dispatch.c \
          src/static.c src/network.c src/rio.c \
          $(FAST) $(LIBS) $(CFLAGS) -o rio -DDEBUG=0

debug: http_parser.o websocket.o
	$(CC) -I cws/src http-parser/http_parser.o cws/b64.o cws/websocket.o \
          -g src/utils.c src/buffer.c src/dispatch.c \
             src/static.c src/network.c src/rio.c \
             $(LIBS) $(CFLAGS) -o rio -DDEBUG=1

test: http_parser.o websocket.o
	$(CC) -I cws/src http-parser/http_parser.o cws/b64.o cws/websocket.o \
          -g src/utils.c src/buffer.c src/dispatch.c \
             src/static.c src/network.c src/rio.c \
             $(LIBS) $(CFLAGS) -o rio -DDEBUG=0
		     -lcunit -o test_rio -DDEBUG=1 -DTEST=1
	./test_rio

clean:
	rm -f rio
	rm -f test_rio

HTTP_PARSER_DIR=http-parser
CWS_DIR=cws

websocket.o:
	cd $(CWS_DIR); $(MAKE)

http_parser.o:
	cd $(HTTP_PARSER_DIR); $(MAKE) http_parser.o
