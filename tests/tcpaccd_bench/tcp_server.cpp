#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/tcp.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

int main(int argc, char *argv[]) {
  int port = 9999;
  if (argc > 1)
    port = std::stoi(argv[1]);

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("socket");
    return 1;
  }

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr("127.0.0.1");
  address.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind");
    return 1;
  }

  if (listen(server_fd, 3) < 0) {
    perror("listen");
    return 1;
  }

  std::cout << "Server listening on 127.0.0.1:" << port << std::endl;

  while (true) {
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    int client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
    if (client_fd < 0) {
      perror("accept");
      continue;
    }

    // Set TCP_NODELAY to minimize latency
    int nodelay = 1;
    setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

    char buffer[4096];
    while (true) {
      ssize_t valread = read(client_fd, buffer, sizeof(buffer));
      if (valread <= 0)
        break;
      send(client_fd, buffer, valread, 0);
    }
    close(client_fd);
  }

  close(server_fd);
  return 0;
}
