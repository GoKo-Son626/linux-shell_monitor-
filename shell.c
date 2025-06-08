#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> // 引入布尔类型
#include <sys/wait.h> // 用于获取 system() 的返回值状态

// 定义一个结构体，包括一个路径、名称，以及种类（文件还是文件夹），三个路径指针。
typedef struct FileFolder {
	char pwd[100]; // 路径长度增加
	struct FileFolder* Child;
	struct FileFolder* Father;
	struct FileFolder* Brother;
	char name[50]; // 名称长度增加
	char type[20]; // 类型长度增加
	char content[200]; // 内容长度增加
} FileFolder;

// 辅助函数：安全地分配内存
FileFolder* create_node() {
	FileFolder* node = (FileFolder*)malloc(1 * sizeof(FileFolder));
	if (node == NULL) {
		perror("Failed to allocate memory for FileFolder node");
		exit(EXIT_FAILURE); // 内存分配失败，直接退出
	}
	// 初始化新节点
	strcpy(node->pwd, "");
	node->Child = NULL;
	node->Father = NULL;
	node->Brother = NULL;
	strcpy(node->name, "");
	strcpy(node->type, "");
	strcpy(node->content, "");
	return node;
}

// 辅助函数：将字符串复制到目标，并确保不会溢出
void safe_strcpy(char* dest, const char* src, size_t dest_size) {
	strncpy(dest, src, dest_size - 1);
	dest[dest_size - 1] = '\0'; // 确保字符串以 null 结尾
}

// 分割字符串函数
void split(char *src, const char *separator, char **dest, int *count) {
	*count = 0;
	char *pNext;
	if (src == NULL || strlen(src) == 0) return;
	if (separator == NULL || strlen(separator) == 0) return;

	// strtok 会修改原始字符串，所以复制一份
	char *temp_src = strdup(src);
	if (temp_src == NULL) {
		perror("Failed to allocate memory for temp_src");
		exit(EXIT_FAILURE);
	}

	pNext = strtok(temp_src, separator);
	while (pNext != NULL) {
		dest[*count] = pNext;
		(*count)++;
		pNext = strtok(NULL, separator);
	}
}

// 初始化根目录
FileFolder* init() {
	FileFolder* rootFileFolder = create_node();
	safe_strcpy(rootFileFolder->pwd, "/", sizeof(rootFileFolder->pwd)); // 根目录路径为 /
	rootFileFolder->Father = rootFileFolder; // 根目录的父目录是它自己
	safe_strcpy(rootFileFolder->name, "root", sizeof(rootFileFolder->name));
	safe_strcpy(rootFileFolder->type, "FileFolder", sizeof(rootFileFolder->type));
	return rootFileFolder;
}

// 查找该目录下最后一个子文件/文件夹
FileFolder* putpToEndBrother(FileFolder* p) {
	if (p == NULL) return NULL;
	FileFolder* q = p;
	while (q->Brother != NULL) {
		q = q->Brother;
	}
	return q;
}

// 检查在当前目录下是否存在同名文件或文件夹
bool is_name_exist(FileFolder* current_dir, const char* name) {
	FileFolder* p = current_dir->Child;
	while (p != NULL) {
		if (strcmp(p->name, name) == 0) {
			return true;
		}
		p = p->Brother;
	}
	return false;
}

// 创建文件夹
void makdir(FileFolder* now, char** command, int argc) {
	if (argc < 2) {
		printf("Usage: makdir <folder_name>\n");
		return;
	}
	const char* folder_name = command[1];
	if (is_name_exist(now, folder_name)) {
		printf("Error: Directory '%s' already exists.\n", folder_name);
		return;
	}

	FileFolder* x = create_node();
	safe_strcpy(x->name, folder_name, sizeof(x->name));
	safe_strcpy(x->type, "FileFolder", sizeof(x->type));

	// 构建新文件夹的路径
	safe_strcpy(x->pwd, now->pwd, sizeof(x->pwd));
	// 如果当前目录不是根目录，才需要追加斜杠
	if (strcmp(now->pwd, "/") != 0) {
		strcat(x->pwd, "/");
	}
	strcat(x->pwd, x->name);

	x->Father = now;

	if (now->Child == NULL) {
		now->Child = x;
	} else {
		FileFolder* p = putpToEndBrother(now->Child);
		p->Brother = x;
	}
}

// 创建文件
void touch(FileFolder* now, char** command, int argc) {
	if (argc < 2) {
		printf("Usage: touch <file_name>\n");
		return;
	}
	const char* file_name = command[1];
	if (is_name_exist(now, file_name)) {
		printf("Error: File '%s' already exists.\n", file_name);
		return;
	}

	FileFolder* x = create_node();
	safe_strcpy(x->name, file_name, sizeof(x->name));
	safe_strcpy(x->type, "File", sizeof(x->type));
	safe_strcpy(x->content, "", sizeof(x->content)); // 初始化内容为空

	// 文件也需要有完整的路径，虽然它不能被 cd
	safe_strcpy(x->pwd, now->pwd, sizeof(x->pwd));
	if (strcmp(now->pwd, "/") != 0) {
		strcat(x->pwd, "/");
	}
	strcat(x->pwd, x->name);

	x->Father = now;

	if (now->Child == NULL) {
		now->Child = x;
	} else {
		FileFolder* p = putpToEndBrother(now->Child); // 修正：应该在子节点链表上找最后一个兄弟
		p->Brother = x;
	}
}

// 按创建顺序展示该目录下的文件、文件夹
void ls(FileFolder* now) {
	FileFolder* p = now->Child;
	if (p == NULL) {
		printf("\n"); // 目录为空
	} else {
		while (p != NULL) {
			printf("%s ", p->name);
			p = p->Brother;
		}
		printf("\n");
	}
}

// cd 操作
FileFolder* cd(const char* cmd_full, FileFolder* now, char** command, int argc, FileFolder* root) {
	if (argc < 2) {
		printf("Usage: cd <path> or cd .. or cd /\n");
		return now;
	}

	const char* target_path = command[1];

	if (strcmp(target_path, "/") == 0) { // cd / 返回根目录
		return root;
	} else if (strcmp(target_path, "..") == 0) { // cd .. 返回父目录
		if (now->Father == now) { // 已经是根目录
			return now;
		}
		return now->Father;
	} else if (target_path[0] == '/') { // 绝对路径 cd /path/to/folder
		FileFolder* current_ptr = root;
		char temp_path[200];
		safe_strcpy(temp_path, target_path + 1, sizeof(temp_path)); // 跳过开头的 '/'

		char* path_segments[50]; // 假设路径最多50段
		int segment_count = 0;
		char *token;
		char *rest = temp_path;

		while ((token = strtok_r(rest, "/", &rest)) != NULL) {
			path_segments[segment_count++] = token;
		}

		for (int i = 0; i < segment_count; i++) {
			FileFolder* found_child = NULL;
			FileFolder* child_iter = current_ptr->Child;
			while (child_iter != NULL) {
				if (strcmp(child_iter->name, path_segments[i]) == 0) {
					found_child = child_iter;
					break;
				}
				child_iter = child_iter->Brother;
			}

			if (found_child == NULL || strcmp(found_child->type, "File") == 0) {
				printf("Error: No such directory or it's a file: '%s'\n", target_path);
				return now; // 路径无效或目标是文件，返回当前目录
			}
			current_ptr = found_child;
		}
		return current_ptr;

	} else { // 相对路径 cd folder_name
		FileFolder* p = now->Child;
		while (p != NULL && strcmp(p->name, target_path) != 0) {
			p = p->Brother;
		}
		if (p == NULL) {
			printf("Error: No such directory: '%s'\n", target_path);
			return now;
		} else if (strcmp(p->type, "File") == 0) {
			printf("Error: Cannot 'cd' into a file: '%s'\n", target_path);
			return now;
		} else {
			return p;
		}
	}
}

// 显示路径
void pwd(FileFolder* p) {
	if (strcmp(p->type, "FileFolder") == 0) {
		printf("%s\n", p->pwd);
	} else { // 如果是文件，显示其父目录的路径
		printf("%s\n", p->Father->pwd);
	}
}

// 查找文件或文件夹
FileFolder* find_path_node(const char* full_path, FileFolder* current_dir, FileFolder* root) {
	FileFolder* p = NULL;
	char temp_path[200];
	safe_strcpy(temp_path, full_path, sizeof(temp_path));

	// 判断是绝对路径还是相对路径
	if (temp_path[0] == '/') { // 绝对路径
		p = root;
		char* path_segments[50];
		int segment_count = 0;
		char *token;
		char *rest = temp_path + 1; // 跳过开头的 '/'

		while ((token = strtok_r(rest, "/", &rest)) != NULL) {
			path_segments[segment_count++] = token;
		}

		for (int i = 0; i < segment_count; i++) {
			FileFolder* found_child = NULL;
			FileFolder* child_iter = p->Child;
			while (child_iter != NULL) {
				if (strcmp(child_iter->name, path_segments[i]) == 0) {
					found_child = child_iter;
					break;
				}
				child_iter = child_iter->Brother;
			}
			if (found_child == NULL) {
				return NULL; // 路径不存在
			}
			p = found_child;
		}
	} else { // 相对路径
		p = current_dir;
		char* path_segments[50];
		int segment_count = 0;
		char *token;
		char *rest = temp_path;

		while ((token = strtok_r(rest, "/", &rest)) != NULL) {
			path_segments[segment_count++] = token;
		}

		for (int i = 0; i < segment_count; i++) {
			FileFolder* found_child = NULL;
			FileFolder* child_iter = p->Child;
			while (child_iter != NULL) {
				if (strcmp(child_iter->name, path_segments[i]) == 0) {
					found_child = child_iter;
					break;
				}
				child_iter = child_iter->Brother;
			}
			if (found_child == NULL) {
				return NULL; // 路径不存在
			}
			p = found_child;
		}
	}
	return p;
}


// 查看一个指定文件的内容
void cat(const char* cmd_full, char **command, int argc, FileFolder* now, FileFolder* root) {
	if (argc < 2) {
		printf("Usage: cat <file_path>\n");
		return;
	}

	// 构建完整的路径字符串
	char path_buffer[200];
	safe_strcpy(path_buffer, command[1], sizeof(path_buffer));

	FileFolder* target_node = find_path_node(path_buffer, now, root);

	if (target_node == NULL) {
		printf("Error: No such file or directory: '%s'\n", path_buffer);
		return;
	}
	if (strcmp(target_node->type, "FileFolder") == 0) {
		printf("Error: '%s' is a directory.\n", target_node->name);
		return;
	}

	printf("%s\n", target_node->content);
}

// 修改一个指定文件的内容
void tac(const char* cmd_full, char **command, int argc, FileFolder* now, FileFolder* root) {
	if (argc < 3) { // tac <file_path> <content>
		printf("Usage: tac <file_path> <content>\n");
		return;
	}

	// 找到文件路径和内容的分界点
	// 假设第一个参数是文件路径，从第二个参数开始都是文件内容
	char file_path[100];
	safe_strcpy(file_path, command[1], sizeof(file_path));

	char new_content[200];
	new_content[0] = '\0'; // 初始化为空字符串

	// 拼接文件内容
	for (int i = 2; i < argc; i++) {
		strcat(new_content, command[i]);
		if (i < argc - 1) {
			strcat(new_content, " "); // 在内容之间添加空格
		}
	}

	FileFolder* target_node = find_path_node(file_path, now, root);

	if (target_node == NULL) {
		printf("Error: No such file or directory: '%s'\n", file_path);
		return;
	}
	if (strcmp(target_node->type, "FileFolder") == 0) {
		printf("Error: '%s' is a directory.\n", target_node->name);
		return;
	}

	safe_strcpy(target_node->content, new_content, sizeof(target_node->content));
}


// 与用户进行交互操作
void getcommand(FileFolder* root) {
	FileFolder* now = root;
	char cmd_line[256]; // 增加输入缓冲区大小
	char cmd_line_copy[256]; // 用于 strtok_r
	char* command_args[50]; // 最多支持50个命令参数
	int arg_count;

	printf("Welcome to the shell!\n");
	printf("support command: ls, pwd, touch, mkdir, \ncd, cat, tac, sysinfo, exit\n");
	printf("command sysinfo can display system resource status\n");
	printf("Type 'exit' to quit.\n");
	printf("%s > ", now->pwd);

	while (fgets(cmd_line, sizeof(cmd_line), stdin) != NULL) {
		// 移除换行符
		cmd_line[strcspn(cmd_line, "\n")] = 0;

		if (strlen(cmd_line) == 0) { // 处理空输入
			printf("%s > ", now->pwd);
			continue;
		}

		safe_strcpy(cmd_line_copy, cmd_line, sizeof(cmd_line_copy));

		// 使用 strtok_r (线程安全版本)
		char *token;
		char *rest = cmd_line_copy;
		arg_count = 0;

		// 获取第一个命令（主命令）
		token = strtok_r(rest, " ", &rest);
		if (token != NULL) {
			command_args[arg_count++] = token;
		}

		// 获取后续参数
		while (token != NULL && arg_count < 50) {
			token = strtok_r(rest, " ", &rest);
			if (token != NULL) {
				command_args[arg_count++] = token;
			}
		}

		if (arg_count == 0) { // 空命令
			printf("%s > ", now->pwd);
			continue;
		}

		if (strcmp(command_args[0], "pwd") == 0) {
			pwd(now);
		} else if (strcmp(command_args[0], "mkdir") == 0) {
			makdir(now, command_args, arg_count);
		} else if (strcmp(command_args[0], "touch") == 0) {
			touch(now, command_args, arg_count);
		} else if (strcmp(command_args[0], "ls") == 0) {
			ls(now);
		} else if (strcmp(command_args[0], "cd") == 0) {
			now = cd(cmd_line, now, command_args, arg_count, root);
		} else if (strcmp(command_args[0], "cat") == 0) {
			cat(cmd_line, command_args, arg_count, now, root);
		} else if (strcmp(command_args[0], "tac") == 0) {
			tac(cmd_line, command_args, arg_count, now, root);
		} else if (strcmp(command_args[0], "sysinfo") == 0) { // <-- 新增的命令
			printf("Gathering system information...\n");
			// 调用外部的 sys_monitor 程序
			// 假设 sys_monitor 可执行文件在当前目录或 PATH 中
			int status = system("./sys_monitor"); 
			if (status == -1) {
				perror("Error executing sys_monitor");
			} else if (WIFEXITED(status)) {
				//printf("sys_monitor exited with status %d\n", WEXITSTATUS(status));
			} else {
				//printf("sys_monitor did not exit normally.\n");
			}
		} else if (strcmp(command_args[0], "exit") == 0) {
			printf("Exiting file system.\n");
			// 这里可以添加清理内存的逻辑
			exit(0);
		} else {
			printf("Error: Unknown command '%s'\n", command_args[0]);
		}
		printf("%s > ", now->pwd);
	}
}

int main() {
	FileFolder* root = init();
	getcommand(root);
	// 在这里，如果程序退出，root 及其所有子节点分配的内存将不会被 free
	// 对于一个简单的模拟器，这通常不是大问题，因为操作系统会回收内存。
	// 但在生产级代码中，需要实现一个递归释放所有节点的函数。
	return 0;
}
