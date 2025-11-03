# === Project Settings === 
CC       := gcc
CFLAGS   := -Wall -Wextra -Ilibs/include

# OBS: smw.c (inte sm_worker.c)
LIB_SRCS := libs/src/smw.c libs/src/TCP_client.c libs/src/TCP_server.c
LIB_OBJS := $(LIB_SRCS:.c=.o)

CLIENT_SRC := client/src/main.c
CLIENT_OBJ := $(CLIENT_SRC:.c=.o)
CLIENT_INC := -Iclient/include

SERVER_SRC := server/src/main.c
SERVER_OBJ := $(SERVER_SRC:.c=.o)
SERVER_INC := -Iserver/include

CLIENT_BIN := client_app
SERVER_BIN := server_app

# === Build Rules ===
.PHONY: all clean client server

all: $(CLIENT_BIN) $(SERVER_BIN)

# --- Client ---
$(CLIENT_BIN): $(CLIENT_OBJ) $(LIB_OBJS)
	@echo "🔧 Linking $@"
	$(CC) $(CFLAGS) -o $@ $^

# --- Server ---
$(SERVER_BIN): $(SERVER_OBJ) $(LIB_OBJS)
	@echo "🔧 Linking $@"
	$(CC) $(CFLAGS) -o $@ $^

# --- Target-specifika flaggor (för include-sökvägar) ---
client/src/%.o: CFLAGS += $(CLIENT_INC)
server/src/%.o: CFLAGS += $(SERVER_INC)
libs/src/%.o:   CFLAGS += -Ilibs/include

# --- Generisk objekregel ---
%.o: %.c
	@echo "🧩 Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

# --- Individual Builds ---
client: $(CLIENT_BIN)
server: $(SERVER_BIN)

# --- Cleanup ---
clean:
	@echo "🧹 Cleaning build files..."
	rm -f $(CLIENT_OBJ) $(SERVER_OBJ) $(LIB_OBJS) $(CLIENT_BIN) $(SERVER_BIN)

