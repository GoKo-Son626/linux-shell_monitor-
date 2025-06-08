# 定义编译器
CC = gcc
CFLAGS = -Wall -g -Isrc/include

SYSMON_SRCS = src/main.c src/cpu.c src/memory.c src/disk.c src/network.c
SYSMON_EXEC = sys_monitor

SHELL_SRCS = shell.c
SHELL_EXEC = shell # 可以是 'shell' 或 'my_shell'，这里用 my_shell

.PHONY: all clean

all: $(SYSMON_EXEC) $(SHELL_EXEC)

$(SYSMON_EXEC): $(SYSMON_SRCS)
	$(CC) $(CFLAGS) $(SYSMON_SRCS) -o $(SYSMON_EXEC)

$(SHELL_EXEC): $(SHELL_SRCS)
	$(CC) $(CFLAGS) $(SHELL_SRCS) -o $(SHELL_EXEC)

# 清理目标：
# 移除 sys_monitor 和 my_shell 两个可执行文件。
clean:
	rm -f $(SYSMON_EXEC) $(SHELL_EXEC)
