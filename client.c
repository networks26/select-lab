#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 12345
#define BUFFER_SIZE 1024
#define XOR_KEY     42

/*
 * Explanation:
 *  - The client connects to the server and reads user input in a loop.
 *  - Before sending, it XORs the message with the constant XOR_KEY.
 *  - This simulates sending "encrypted" data to the server.
 */

int main(void)
{
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // 1. Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 2. Server address setup
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port        = htons(SERVER_PORT);

    // 3. Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Connected to server %s:%d\n", SERVER_IP, SERVER_PORT);

    // 4. Send data in a loop
    while (1) {
        printf("Enter message (type 'exit' to quit): ");
        fflush(stdout);

        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;  // EOF or error
        }

        // Remove trailing newline
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        // If user typed "exit", break
        if (strcmp(buffer, "exit") == 0) {
            printf("Exiting...\n");
            break;
        }

        // XOR the buffer before sending
        for (size_t i = 0; i < len; i++) {
            buffer[i] ^= XOR_KEY;
        }

        // Send data
        ssize_t bytes_sent = send(sockfd, buffer, len, 0);
        if (bytes_sent < 0) {
            perror("send");
            break;
        }
    }

    close(sockfd);
    return 0;
}

