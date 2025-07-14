#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 5555
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server;
    char buffer[BUFFER_SIZE];
    char input[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr*)&server, sizeof(server));

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(sock, buffer, sizeof(buffer)-1);
        if (valread <= 0) {
            break;
        }
        buffer[valread] = '\0';
        printf("%s", buffer);

        if (strstr(buffer, "Enter")) {
            fgets(input, sizeof(input), stdin);
            write(sock, input, strlen(input));
        }
    }

    close(sock);
    return 0;
}
