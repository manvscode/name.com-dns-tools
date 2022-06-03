##################################################################################
#                                                                                #
#  Copyright (C) 2016 by Joseph A. Marrero. http://www.joemarrero.com/           #
#                                                                                #
#  Permission is hereby granted, free of charge, to any person obtaining a copy  #
#  of this software and associated documentation files (the "Software"), to deal #
#  in the Software without restriction, including without limitation the rights  #
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell     #
#  copies of the Software, and to permit persons to whom the Software is         #
#  furnished to do so, subject to the following conditions:                      #
#                                                                                #
#  The above copyright notice and this permission notice shall be included in    #
#  all copies or substantial portions of the Software.                           #
#                                                                                #
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR    #
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,      #
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE   #
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER        #
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, #
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN     #
#  THE SOFTWARE.                                                                 #
#                                                                                #
##################################################################################
#
#
# To build for Linux, FreeBSD, etc..:
# 	make OS=unix
#
# To build for Windows x86:
# 	make OS=windows-x86
#
# To build for Windows x86_64:
# 	make OS=windows-x86_64


ifndef $(OS)
	OS=unix
endif

ifndef $(DEBUG)
	DEBUG=false
endif

CWD = $(shell pwd)


# Dynamic DNS tool.
DYNDNS_BIN = namecom_dyndns
DYNDNS_SOURCES = src/dyndns.c src/ipify.c src/namecom_api.c

# DNS record tool.
DNS_BIN = namecom_dns
DNS_SOURCES = src/dns.c src/namecom_api.c

CFLAGS = -std=c99 -Wall \
		 -Iextern/include/collections-1.0.0/ \
		 -Iextern/include/xtd-1.0.0/ \
		 -Iextern/include/ \
		 -I/usr/local/include
LDFLAGS = 


ifeq ($(DEBUG), true)
	CFLAGS += -O2
else
	CFLAGS += -O0 -g
endif


ifeq ($(OS),unix)
	CC = gcc
	HOST=
	CFLAGS += -fsanitize=undefined
	LDFLAGS += -L$(CWD)/extern/lib/ \
			   -L/usr/local/lib \
			   -lcurl \
			   -ljansson \
			   -lxtd \
			   -lcollections
endif

ifeq ($(OS),windows-x86)
	DYNDNS_BIN = $(DYNDNS_BIN,-x86.exe)
	DNS_BIN = $(DNS_BIN,-x86.exe)
	CC=i686-w64-mingw32-gcc
	HOST=i686-w64-mingw32
	CFLAGS += -D_POSIX -mconsole
	LDFLAGS += -Wl,-no-undefined -L$(CWD)/extern/lib/ -L/usr/i686-w64-mingw32/lib/ \
			   -lcurl \
			   -ljansson \
			   -lxtd \
			   -lcollections \
			   \
			   -lmingw32 \
			   -lgcc \
			   -lgcc_eh \
			   -lmoldname \
			   -lmingwex \
			   -lmsvcrt \
			   -ladvapi32 \
			   -lshell32 \
			   -luser32 \
			   -lkernel32
endif

ifeq ($(OS),windows-x86_64)
	DYNDNS_BIN = $(DYNDNS_BIN,-x64.exe)
	DNS_BIN = $(DNS_BIN,-x64.exe)
	CC=x86_64-w64-mingw32-gcc
	HOST=x86_64-w64-mingw32
	CFLAGS += -D_POSIX -mconsole
	LDFLAGS += -Wl,-no-undefined -L$(CWD)/extern/lib/ -L/usr/x86_64-w64-mingw32/lib/ \
			   -lcurl \
			   -ljansson \
			   -lxtd \
			   -lcollections \
			   \
			   -lmingw32 \
			   -lgcc \
			   -lgcc_eh \
			   -lmoldname \
			   -lmingwex \
			   -lmsvcrt \
			   -ladvapi32 \
			   -lshell32 \
			   -luser32 \
			   -lkernel32
endif


all: extern/libxtd \
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
extern/libxtd:
	@mkdir -p extern/libxtd/
	@git clone https://bitbucket.org/manvscode/libxtd.git extern/libxtd/
	@cd extern/libxtd && \
		autoreconf -i && \
		PKG_CONFIG_LIBDIR=$(CWD)/extern/lib/pkgconfig \
		./configure --libdir=$(CWD)/extern/lib/ \
		            --includedir=$(CWD)/extern/include/ \
		            --disable-shared \
		            --enable-static \
		            --host=$(HOST) && \
		make && \
		make install

extern/libcollections:
	@mkdir -p extern/libcollections/
	@git clone https://bitbucket.org/manvscode/libcollections.git extern/libcollections/
	@cd extern/libcollections && \
		autoreconf -i && \
		PKG_CONFIG_LIBDIR=$(CWD)/extern/lib/pkgconfig \
		./configure --libdir=$(CWD)/extern/lib/ \
		            --includedir=$(CWD)/extern/include/ \
		            --disable-shared \
		            --enable-static \
		            --host=$(HOST) && \
		make && \
		make install

extern/libjansson:
	@mkdir -p extern/libjansson/
	@git clone https://github.com/akheron/jansson extern/libjansson
	@cd extern/libjansson && \
		autoreconf -i && \
		PKG_CONFIG_LIBDIR=$(CWD)/extern/lib/pkgconfig \
		./configure --libdir=$(CWD)/extern/lib/ \
		            --includedir=$(CWD)/extern/include/ \
		            --disable-shared \
		            --enable-static \
		            --host=$(HOST) && \
		make && \
		make install

#################################################
# Cleaning                                      #
#################################################
clean_extern:
	@rm -rf extern

clean:
	@rm -rf src/*.o
	@rm -rf bin


#################################################
# Installing                                    #
#################################################
install:
ifeq ("$(INSTALL_PATH)","")
	$(error INSTALL_PATH is not set.)
endif
	@echo "Installing ${CWD}/bin/${BIN_NAME} to ${INSTALL_PATH}"
	@cp bin/$(BIN_NAME) $(INSTALL_PATH)/$(BIN_NAME)
