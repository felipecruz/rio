CC="gcc"

CFLAGS=-std=c99
LIBS=-lcrypto -lzmq -lczmq -lmsgpack

FAST=-O3
DEBUG=-g

all: http_parser.o cws.o
	$(CC) -I cws/src http-parser/http_parser.o cws/b64.o cws/cws.o \
          src/utils.c src/buffer.c src/dispatch.c \
          src/static.c src/network.c src/rio.c \
          $(FAST) $(LIBS) $(CFLAGS) -o rio -DDEBUG=0

debug: http_parser.o cws.o
	$(CC) -I cws/src http-parser/http_parser.o cws/b64.o cws/cws.o \
          -g src/utils.c src/buffer.c src/dispatch.c \
             src/static.c src/network.c src/rio.c \
             $(LIBS) $(CFLAGS) -o rio -DDEBUG=1

leak: http_parser.o cws.o
	$(CC) -I cws/src http-parser/http_parser.o cws/b64.o cws/cws.o \
          -g src/utils.c src/buffer.c src/dispatch.c \
             src/static.c src/network.c src/rio.c \
             $(LIBS) $(CFLAGS) -o rio -DDEBUG=1 -DVALGRIND=1
		   sudo valgrind --leak-check=full ./rio

test: http_parser.o cws.o
	$(CC) -I cws/src -I src/ http-parser/http_parser.o cws/b64.o cws/cws.o \
          -g src/utils.c src/buffer.c src/dispatch.c \
             src/static.c src/network.c tests/tests.c \
             $(LIBS) $(CFLAGS) -lcunit -o test_rio -DDEBUG=1 -DTEST=1
	./test_rio

clean:
	rm -f rio
	rm -f test_rio

HTTP_PARSER_DIR=http-parser
CWS_DIR=cws

cws.o:
	cd $(CWS_DIR); $(MAKE)

http_parser.o:
	cd $(HTTP_PARSER_DIR); $(MAKE) http_parser.o
