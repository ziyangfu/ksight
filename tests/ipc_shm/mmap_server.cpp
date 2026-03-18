/*!
 * \brief mmap +shm_open，客户端每隔一秒发送消息，服务端echo
 * compile
 *          g++ -std=c++11 -o mmap_server ./mmap_server.cpp -lrt -lpthread
 *          g++ -std=c++11 -o mmap_client ./mmap_client.cpp -lrt -lpthread
 */

#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include "shm_struct.h"
int cycle = 100;
int main() {
    std::cout << "mmap_server pid :" << getpid() << std::endl;
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    std::cout << "shm_fd :" << shm_fd << std::endl;
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    // 设置共享内存大小
    if (ftruncate(shm_fd, sizeof(SharedMemory)) == -1) {
        perror("ftruncate");
        close(shm_fd);
        return 1;
    }

    // 映射共享内存
    SharedMemory* shm = static_cast<SharedMemory*>(
            mmap(nullptr, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
    if (shm == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        return 1;
    }
    std::cout << "mmap server shm vaddr is: " << shm << std::endl;

    // 初始化互斥锁和条件变量
    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;

    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);

    pthread_condattr_init(&cattr);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&shm->mutex, &mattr);
    pthread_cond_init(&shm->cond_request, &cattr);
    pthread_cond_init(&shm->cond_response, &cattr);
    shm->ready = 1;  // 初始状态为就绪

    std::cout << "Server is running..." << std::endl;
    //char *large_str = generate_large_string(TARGET_SIZE_BYTES); // MB

//    while (true) {
    while (cycle != 0) {
        pthread_mutex_lock(&shm->mutex);

        // 等待客户端写入请求
        while (shm->ready == 1) {
            pthread_cond_wait(&shm->cond_request, &shm->mutex);
        }

        //std::cout << "Received from client: " << shm->request << std::endl;
        std::cout << "Received from client, size:  " << sizeof(shm->request) << std::endl;
        // 构造响应
        snprintf(shm->response, BUFFER_SIZE, "Echo: %s", shm->request);
        //snprintf(shm->response, BUFFER_SIZE, large_str);

        // 标记为已处理，唤醒客户端
        shm->ready = 1;
        pthread_cond_signal(&shm->cond_response);

        pthread_mutex_unlock(&shm->mutex);

        sleep(1);  // 避免 CPU 占用过高
        cycle--;
    }

    // 清理资源（正常退出时执行）
    //free(large_str);
    
    // munmap(shm, sizeof(SharedMemory));
    close(shm_fd);
    shm_unlink(SHM_NAME);

    return 0;
}

