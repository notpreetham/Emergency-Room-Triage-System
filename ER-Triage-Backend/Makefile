CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -std=c99

SRCS = src/main.c src/triage.c src/queue.c src/reports.c src/fileio.c src/utils.c src/json.c
OBJS = src/main.o src/triage.o src/queue.o src/reports.o src/fileio.o src/utils.o src/json.o
TARGET_DIR = build
TARGET = $(TARGET_DIR)/triage.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	@if not exist $(TARGET_DIR) mkdir $(TARGET_DIR) 2>nul || mkdir -p $(TARGET_DIR)
	$(CC) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@del /f /q src\*.o 2>nul || rm -f src/*.o
	@del /f /q $(TARGET_DIR)\* 2>nul || rm -rf $(TARGET_DIR)

.PHONY: all clean
