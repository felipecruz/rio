CFLAGS=-std=c99

all: http_parser.o
	gcc http-parser/http_parser.o src/utils.c src/dispatch.c src/static.c src/network.c src/rio.c -lzmq -lmsgpack -std=c99 -o rio -DDEBUG=0
debug:
	gcc http-parser/http_parser.o -g src/utils.c src/dispatch.c src/static.c src/network.c src/rio.c -lzmq -lmsgpack -std=c99 -o rio -DDEBUG=1 
test:
	gcc http-parser/http_parser.o -g src/utils.c src/dispatch.c src/static.c src/network.c src/tests.c -lzmq -lmsgpack -lcunit -std=c99 -o test_rio -DDEBUG=1 -DTEST=1
	./test_rio

HTTP_PARSER_DIR=http-parser

http_parser.o:
	cd $(HTTP_PARSER_DIR); $(MAKE) http_parser.o
