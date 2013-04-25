################################################################################
#
# proxenet Makefile
# 
#


VERSION         =       0.01
ARCH            =       $(shell uname)
DEBUG           =       1

CC              =       cc
BIN             =       proxenet
DEFINES         =       -DVERSION=$(VERSION)
# HARDEN		=	-Wl,-z,relro -pie -fstack-protector-all -fPIE
LDFLAGS         =       $(HARDEN) -lpthread 
SRC		=	$(wildcard *.c)
OBJECTS         =       $(patsubst %.c, %.o, $(SRC))
INC             =       -I/usr/include
CFLAGS          =       -O2 -Wall $(DEFINES) $(INC) 
LIB		= 	-L/lib


# DEBUG
ifeq ($(DEBUG), 1)
DBGFLAGS        =       -ggdb -DDEBUG
CFLAGS          +=      $(DBGFLAGS)
endif


# SSL 
INC			+=	-Ipolarssl/include
LIB			+= 	-Lpolarssl/library
LDFLAGS			+=	-lpolarssl
# CFLAGS			+=	-DDEBUG_SSL

# PLUGINS 
WITH_C_PLUGIN		=	1
WITH_PYTHON_PLUGIN	=	1
WITH_RUBY_PLUGIN	=	1

ifeq ($(WITH_C_PLUGIN), 1)
DEFINES			+=	-D_C_PLUGIN 
LDFLAGS			+=	-ldl
endif

ifeq ($(WITH_PYTHON_PLUGIN), 1)
DEFINES			+=	-D_PYTHON_PLUGIN
LDFLAGS			+=	-lpython2.7
INC			+=	-I/usr/include/python2.7
endif

ifeq ($(WITH_RUBY_PLUGIN), 1)
DEFINES			+=	-D_RUBY_PLUGIN
LDFLAGS			+=	-lruby
endif


# TEST
TEST_ARGS		= 	-4 -vvvv --nb-threads=10


# Compile rules
.PHONY : all check-syntax clean keys tags purge ssl sslclean test 

.c.o :
	@echo "CC $< -> $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

all : $(BIN)

$(BIN): $(OBJECTS) ssl
	@echo "LINK with $(LDFLAGS)"
	@$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LIB) $(LDFLAGS)

purge:
	@echo "RM objects"
	@rm -fr $(OBJECTS) ./core-$(BIN)-*

clean: purge
	@echo "RM $(BIN)"
	@rm -fr $(BIN)

keys:
	@make -C keys all

ssl:	polarssl/library/libpolarssl.a

polarssl/library/libpolarssl.a:
	@echo "Building PolarSSL library"
	@make -C polarssl lib

sslclean: clean
	@make -C polarssl clean

# Packaging
snapshot: clean
	git add . && \
	git ci -m "$(shell date): Generating snapshot release" && \
        git archive --format=tar --prefix=$(BIN)-$(VERSION)/ HEAD \
	|gzip > /tmp/$(PROGNAME)-latest.tgz

stable: clean
	git add . && \
	git ci -m "$(shell date): Generating stable release" && \
	git archive --format=tar --prefix=$(BIN)-$(VERSION)/ master \
	|gzip > /tmp/${PROGNAME}-${PROGVERS}.tgz

# Tests
check-syntax:
	$(CC) $(CFLAGS) -fsyntax-only $(CHK_SOURCES)

test: clean $(BIN)
	./$(BIN) $(TEST_ARGS)

valgrind: $(BIN)
	valgrind -v --leak-check=full --show-reachable=yes ./$(BIN) $(TEST_ARGS)