CFLAGS += -Wall -Wextra -pedantic
CPPFLAGS += -I./include
LDLIBS += -lblobmsg_json -lubox -lubus
BIN = bin/netconflictscand
BUILD_DIR := build
SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=bin/%.o)

.PHONY: clean all objects run

all: $(BIN)


$(BIN): $(OBJS)

	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDLIBS)   -o $@ $^ 

build: $(BIN)

objects: $(OBJS)

bin/%.o: src/%.c
	mkdir -p bin/
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDLIBS)  -c -o $@ $^

clean:
	${RM} $(BIN) $(OBJS)

run: build
	$(BIN)
