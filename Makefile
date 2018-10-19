#
#  Copyright (C) 2016 by Joseph A. Marrero. http://www.joemarrero.com/
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#  THE SOFTWARE.
#

#CFLAGS = -O0 -g -Wall -fsanitize=undefined -D_POSIX_C_SOURCE -I /usr/local/include -I extern/include/
CFLAGS = -std=c99 -O2 -Wall -fsanitize=undefined -D_POSIX_C_SOURCE -I /usr/local/include -I extern/include/
LDFLAGS = extern/lib/libutility.a extern/lib/libcollections.a extern/lib/libjansson.a -L /usr/local/lib -L extern/lib/ -lcurl
CWD = $(shell pwd)

# Dynamic DNS tool.
DYNDNS_BIN = namecom_dyndns
DYNDNS_SOURCES = src/dyndns.c src/ipify.c src/namecom_api.c

# DNS record tool.
DNS_BIN = namecom_dns
DNS_SOURCES = src/dns.c src/namecom_api.c

all: extern/libutility \
	 extern/libcollections \
	 extern/libjansson \
	 bin/$(DYNDNS_BIN) \
	 bin/$(DNS_BIN)

bin/$(DYNDNS_BIN): $(DYNDNS_SOURCES:.c=.o)
	@mkdir -p bin
	@echo "Linking: $^"
	@$(CC) $(CFLAGS) -o bin/$(DYNDNS_BIN) $^ $(LDFLAGS)
	@echo "Created $@"

bin/$(DNS_BIN): $(DNS_SOURCES:.c=.o)
	@mkdir -p bin
	@echo "Linking: $^"
	@$(CC) $(CFLAGS) -o bin/$(DNS_BIN) $^ $(LDFLAGS)
	@echo "Created $@"

src/%.o: src/%.c
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS) -c $< -o $@

#################################################
# Dependencies                                  #
#################################################
extern/libutility:
	@mkdir -p extern/libutility/
	@git clone https://bitbucket.org/manvscode/libutility.git extern/libutility/
	@cd extern/libutility && autoreconf -i && ./configure --libdir=$(CWD)/extern/lib/ --includedir=$(CWD)/extern/include/ && make && make install

extern/libcollections:
	@mkdir -p extern/libcollections/
	@git clone https://bitbucket.org/manvscode/libcollections.git extern/libcollections/
	@cd extern/libcollections && autoreconf -i && ./configure --libdir=$(CWD)/extern/lib/ --includedir=$(CWD)/extern/include/ && make && make install

extern/libjansson:
	@mkdir -p extern/libjansson/
	@git clone https://github.com/akheron/jansson extern/libjansson
	@cd extern/libjansson && autoreconf -i && ./configure --libdir=$(CWD)/extern/lib/ --includedir=$(CWD)/extern/include/ && make && make install



#################################################
# Cleaning                                      #
#################################################
clean_extern:
	@rm -rf extern

clean:
	@rm -rf src/*.o
	@rm -rf bin
