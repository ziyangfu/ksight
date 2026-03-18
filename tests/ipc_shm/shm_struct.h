

#ifndef MAGICEYES_SHM_STRUCT_H
#define MAGICEYES_SHM_STRUCT_H


// shm_struct.h
#ifndef SHM_STRUCT_H
#define SHM_STRUCT_H

#include <pthread.h>
#include <string.h>

#define SHM_NAME "/my_shared_memory"
#define BUFFER_SIZE 1024
//#define BUFFER_SIZE 1024 * 1024 * 64   // 64 MB

#define BUFFER_SIZE_MB 1024 * 1024 * 1   // 1 MB

#define TARGET_SIZE_MB 60
#define TARGET_SIZE_BYTES (TARGET_SIZE_MB * 1024 * 1024)


struct SharedMemory {
    char request[BUFFER_SIZE];       // 客户端写入的内容
    char response[BUFFER_SIZE];      // 服务端返回的内容
    pthread_mutex_t mutex;           // 同步锁
    pthread_cond_t cond_request;     // 客户端请求到达通知
    pthread_cond_t cond_response;    // 服务端响应完成通知
    int ready;                       // 是否准备好进行下一次通信
};



// 生成一个随机字母或数字字符
inline char rand_char() {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    int index = rand() % (sizeof(charset) - 1);
    return charset[index];
}

// 生成一个大小为 size 的随机字符串（字符在A-Z, a-z, 0-9）
inline char* generate_large_string(size_t size) {
    // 分配内存
    char *str = (char *)malloc(size + 1);  // +1用于字符串终止符（如果需要）
    if (!str) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    size_t generated = 0;
    while (generated < size) {
        size_t chunk = (size - generated) < BUFFER_SIZE_MB ? (size - generated) : BUFFER_SIZE_MB;
        for (size_t i = 0; i < chunk; i++) {
            str[generated + i] = rand_char();
        }
        generated += chunk;
    }
    str[size] = '\0';  // 终止字符串（可选）
    return str;
}

#endif // SHM_STRUCT_H


#endif //MAGICEYES_SHM_STRUCT_H
