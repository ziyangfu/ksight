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
    std::cout << "mmap_client pid :" << getpid() << std::endl;
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    std::cout << "shm_fd :" << shm_fd << std::endl;
    if (shm_fd == -1) {
        perror("shm_open");
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
    std::cout << "mmap client shm vaddr is: " << shm << std::endl;

    std::cout << "Client connected to shared memory." << std::endl;


    int count = 0;
    // 写入请求
    //char *large_str = generate_large_string(TARGET_SIZE_BYTES); // MB

    while (cycle != 0) {
        pthread_mutex_lock(&shm->mutex);

        // 等待服务端空闲
        while (shm->ready != 1) {
            pthread_cond_wait(&shm->cond_response, &shm->mutex);
        }

        
        snprintf(shm->request, BUFFER_SIZE, "Message %d", ++count);
        //snprintf(shm->request, BUFFER_SIZE, large_str);
        std::cout << "Sending to server: XXX" << std::endl;
        //std::cout << "Sending to server: " << shm->request << std::endl;

        shm->ready = 0;
        pthread_cond_signal(&shm->cond_request);  // 通知服务端有新请求

        // 等待服务端响应
        while (shm->ready != 1) {
            pthread_cond_wait(&shm->cond_response, &shm->mutex);
        }

        std::cout << "Response from server,size: " << sizeof(shm->response) << std::endl;
        //std::cout << "Response from server: " << shm->response << std::endl;
        pthread_mutex_unlock(&shm->mutex);

        sleep(1);

        cycle--;
    }

    //free(large_str);

    // 清理资源
    // munmap(shm, sizeof(SharedMemory));
    close(shm_fd);

    return 0;
}