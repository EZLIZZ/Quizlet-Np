#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 5555
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("connect");
        exit(1);
    }

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = read(sock, buffer, sizeof(buffer)-1);
        if (bytes <= 0) {
            printf("Server closed connection.\n");
            break;
        }

        buffer[bytes] = '\0';
        printf("%s", buffer);

        // Only respond if input is expected
        if (strstr(buffer, "Enter option") || strstr(buffer, "Enter your name")) {
            fgets(buffer, sizeof(buffer), stdin);
            write(sock, buffer, strlen(buffer));
        }
    }

    close(sock);
    return 0;
}
