#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netinet/tcp.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

int main(int argc, char *argv[]) {
  std::string server_ip = "127.0.0.1";
  int port = 9999;
  int iterations = 1000000;
  size_t msg_size = 6400;

  if (argc > 1)
    iterations = std::stoi(argv[1]);
  if (argc > 2)
    msg_size = std::stoi(argv[2]);
  if (argc > 3)
    port = std::stoi(argv[3]);

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("socket");
    return 1;
  }

  // Set TCP_NODELAY to minimize latency
  int nodelay = 1;
  setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
    perror("inet_pton");
    return 1;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("connect");
    return 1;
  }

  std::vector<char> send_buf(msg_size, 'a');
  std::vector<char> recv_buf(msg_size);

  std::cout << "Starting benchmark: " << iterations << " iterations, "
            << msg_size << " bytes per message." << std::endl;

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; ++i) {
    if (send(sock, send_buf.data(), msg_size, 0) != (ssize_t)msg_size) {
      perror("send");
      break;
    }

    size_t total_received = 0;
    while (total_received < msg_size) {
      ssize_t n = read(sock, recv_buf.data() + total_received,
                       msg_size - total_received);
      if (n <= 0) {
        perror("read");
        goto end;
      }
      total_received += n;
    }
  }

end:
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end - start;

  std::cout << "Total time: " << diff.count() << " seconds" << std::endl;
  std::cout << "Average latency: " << (diff.count() * 1000000.0 / iterations)
            << " us" << std::endl;
  std::cout << "Throughput: " << (iterations / diff.count()) << " msg/s"
            << std::endl;

  close(sock);
  return 0;
}
