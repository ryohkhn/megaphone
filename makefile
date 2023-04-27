# Compiler
CC=gcc
# Compiler flags
CFLAGS= -Wall -g
# Libraries to link
LIBS= -lpthread
# Combined compiler and flags
COMPILE=$(CC) $(CFLAGS)

# Generate list of source files for client, server, and utilities
CLIENT_SRC=$(wildcard src/client/*.c)
SERVER_SRC=$(wildcard src/server/*.c)
COMMON_SRC=$(wildcard src/*.c)

# Generate list of corresponding object files for client, server, and utilities
CLIENT_OBJECTS=$(patsubst src/client/%.c, build/%.o, $(CLIENT_SRC))
SERVER_OBJECTS=$(patsubst src/server/%.c, build/%.o, $(SERVER_SRC))
COMMON_OBJECTS=$(patsubst src/%.c, build/%.o, $(COMMON_SRC))

# Rules for building client and server object files with corresponding header files
build/%.o: src/client/%.c include/%.h
	mkdir -p build
	$(COMPILE) -c $< -o $@

build/%.o: src/server/%.c include/%.h
	mkdir -p build
	$(COMPILE) -c $< -o $@

build/%.o: src/%.c include/%.h
	mkdir -p build
	$(COMPILE) -c $< -o $@

# Rules for building client and server object files without corresponding header files
build/%.o: src/%.c
	mkdir -p build
	$(COMPILE) -c $< -o $@

# Build the client binary using the generated object files
build/client: $(CLIENT_OBJECTS) $(COMMON_OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# Build the server binary using the generated object files
build/server: $(SERVER_OBJECTS) $(COMMON_OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# Build both client and server binaries
build: build/client build/server
	@printf "\033[32mYou can now run ./build/client and ./build/server\n\033[0m"

# Alias for the build target
all: build

# Remove build artifacts
clean :
	rm -rf build
