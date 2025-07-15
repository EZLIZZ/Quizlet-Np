#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <ctype.h>

#define PORT 5555
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define TIMEOUT_SEC 10
#define REQUIRED_CLIENTS 2   // How many clients before quiz starts

typedef struct {
    int socket;
    int score;
    char name[50];
    int active;
    int ready;
} ClientInfo;

typedef struct {
    char question[256];
    char options[4][128];
    char correct_option;
} QuizQuestion;

// Quiz questions
QuizQuestion quiz[] = {
    {"Capital of France?",
     {"A. Paris", "B. Rome", "C. Berlin", "D. Madrid"}, 'A'},
    {"2 + 2 = ?",
     {"A. 3", "B. 4", "C. 5", "D. 6"}, 'B'},
    {"Unix socket programming language?",
     {"A. Python", "B. Java", "C. C/C++", "D. HTML"}, 'C'}
};

ClientInfo clients[MAX_CLIENTS];
pthread_t clientThreads[MAX_CLIENTS];
pthread_mutex_t clientsMutex = PTHREAD_MUTEX_INITIALIZER;

void *clientHandler(void *arg);
void broadcast(const char *message);
void sendToClient(int sock, const char *message);
void updateScoreboard();
unsigned long getMillis();

int clientCount = 0;

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind");
        exit(1);
    }

    listen(serverSocket, 5);
    printf("Server running on port %d...\n", PORT);

    // Accept clients in a loop
    while (1) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);
        if (clientSocket < 0) {
            perror("accept");
            continue;
        }

        printf("New client connected: %s:%d\n",
            inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

        pthread_mutex_lock(&clientsMutex);
        int idx = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) {
                clients[i].socket = clientSocket;
                clients[i].score = 0;
                clients[i].active = 1;
                clients[i].ready = 0;
                strcpy(clients[i].name, "Anonymous");
                idx = i;
                break;
            }
        }
        pthread_mutex_unlock(&clientsMutex);

        if (idx != -1) {
            pthread_create(&clientThreads[idx], NULL, clientHandler, (void*)(long)idx);
            clientCount++;
        } else {
            sendToClient(clientSocket, "Server full.\n");
            close(clientSocket);
        }

        // Check if we have enough clients to start
        if (clientCount >= REQUIRED_CLIENTS) {
            break;
        }
    }

    // Wait until all clients have entered their names
    while (1) {
        pthread_mutex_lock(&clientsMutex);
        int readyClients = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active && clients[i].ready) {
                readyClients++;
            }
        }
        pthread_mutex_unlock(&clientsMutex);

        if (readyClients >= REQUIRED_CLIENTS) {
            printf("Starting quiz...\n");
            break;
        }
        usleep(500 * 1000);
    }

    // Broadcast quiz questions
    for (int q = 0; q < sizeof(quiz)/sizeof(QuizQuestion); q++) {
        char questionBuf[BUFFER_SIZE];
        snprintf(questionBuf, sizeof(questionBuf),
            "\nQ%d: %s\n%s\n%s\n%s\n%s\nEnter option (A/B/C/D): ",
            q+1,
            quiz[q].question,
            quiz[q].options[0],
            quiz[q].options[1],
            quiz[q].options[2],
            quiz[q].options[3]);

        broadcast(questionBuf);

        unsigned long startTime = getMillis();
        while (getMillis() - startTime < TIMEOUT_SEC * 1000) {
            usleep(500 * 1000);
        }

        updateScoreboard();
    }

    broadcast("Quiz finished!\n");

    close(serverSocket);
    return 0;
}

void *clientHandler(void *arg) {
    int idx = (int)(long)arg;
    char buffer[BUFFER_SIZE];
    int sock = clients[idx].socket;

    sendToClient(sock, "Enter your name: ");
    int len = read(sock, buffer, sizeof(buffer)-1);
    if (len > 0) {
        buffer[len-1] = '\0';
        pthread_mutex_lock(&clientsMutex);
        strcpy(clients[idx].name, buffer);
        clients[idx].ready = 1;
        pthread_mutex_unlock(&clientsMutex);
        printf("Client set name: %s\n", clients[idx].name);
    } else {
        clients[idx].active = 0;
        close(sock);
        return NULL;
    }

    for (int q = 0; q < sizeof(quiz)/sizeof(QuizQuestion); q++) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = read(sock, buffer, sizeof(buffer)-1);
        if (bytes <= 0) {
            printf("Client disconnected: %s\n", clients[idx].name);
            clients[idx].active = 0;
            close(sock);
            return NULL;
        }

        char answer = toupper(buffer[0]);
        printf("Client %s answered: %c\n", clients[idx].name, answer);

        pthread_mutex_lock(&clientsMutex);
        if (answer == quiz[q].correct_option) {
            clients[idx].score++;
        }
        pthread_mutex_unlock(&clientsMutex);
    }

    return NULL;
}

void broadcast(const char *message) {
    pthread_mutex_lock(&clientsMutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            write(clients[i].socket, message, strlen(message));
        }
    }
    pthread_mutex_unlock(&clientsMutex);
}

void sendToClient(int sock, const char *message) {
    write(sock, message, strlen(message));
}

void updateScoreboard() {
    char scoreBuf[BUFFER_SIZE];
    strcpy(scoreBuf, "\n=== Scoreboard ===\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            char line[128];
            snprintf(line, sizeof(line), "%s: %d\n", clients[i].name, clients[i].score);
            strcat(scoreBuf, line);
        }
    }
    broadcast(scoreBuf);
}

unsigned long getMillis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}
