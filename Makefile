all: http_parser.o
	gcc http-parser/http_parser.o src/utils.c src/dispatch.c src/network.c src/rio.c -o rio
debug:
	gcc http-parser/http_parser.o -g src/utils.c src/dispatch.c src/network.c src/rio.c -o rio


HTTP_PARSER_DIR=http-parser

http_parser.o:
	cd $(HTTP_PARSER_DIR); $(MAKE) http_parser.o

#network.o: http_parser.o
#	cd src; gcc ../$(HTTP_PARSER_DIR)/http_parser.o -fPIC -c network.c -o network.o

