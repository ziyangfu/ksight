#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include <string>
#include <iostream>

/** 这种方法能获取 shm_open创建的fd对应的path，对于memfd_create的，就不行 */
char* get_path_from_fd(int pid, int fd) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "/proc/%d/fd/%d", pid, fd);

    char resolved_path[PATH_MAX];
    if (realpath(path, resolved_path)) {
        char *result = strdup(resolved_path);
        return result;
    }
    return NULL;
}

int main(int argc, char **argv) {
    int pid = atoi(argv[1]);
    int fd = atoi(argv[2]);
	std::string str = get_path_from_fd(pid, fd);
	std::cout << "path: " << str << std::endl;
	return 0;
}
