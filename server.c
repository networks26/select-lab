#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <errno.h>

#define PORT 12345
#define MAX_CLIENTS  FD_SETSIZE  // or define your own limit
#define BUFFER_SIZE 1024

/*
 * Explanation:
 *  - We use select() so that the server doesn't block on any single operation
 *    (accepting new connections or reading from existing sockets).
 *  - This allows multiple client connections to be handled in a single thread.
 */

int main(void)
{
    int listen_fd, new_fd, max_fd, i;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;
    char buffer[BUFFER_SIZE];
    int client_sockets[MAX_CLIENTS];
    fd_set read_fds;

    // 1. Create the listening socket
    // DRAGON #1 - 111 on the next line won't work. What number to use for TCP?
    listen_fd = socket(AF_INET, SOCK_STREAM, 111);
    if (listen_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 2. Bind to 127.0.0.1:PORT
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    // DRAGON #2 - the following string won't work. It won't listen on the port you want. Fix it.
    server_addr.sin_port        = PORT;

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // 3. Listen for incoming connections
    if (listen(listen_fd, 5) < 0) {
        perror("listen");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // Initialize client_sockets array
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = -1;  // -1 indicates available slot
    }

    printf("Server listening on 127.0.0.1:%d\n", PORT);

    // 4. Server loop
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(listen_fd, &read_fds);
        max_fd = listen_fd;

        // Add client sockets to set
        for (i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &read_fds);
                if (sd > max_fd) {
                    max_fd = sd;
                }
            }
        }

        // 5. Use select() to monitor sockets
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL); \\ <-- This is select
        if (activity < 0 && errno != EINTR) {
            perror("select");
            break;
        }

        // 6. Check if we have a new connection on listening socket
        if (FD_ISSET(listen_fd, &read_fds)) {
            addr_len = sizeof(client_addr);
            new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
            if (new_fd < 0) {
                perror("accept");
                continue;
            }

            // Add new socket to array
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == -1) {
                    client_sockets[i] = new_fd;
                    printf("New connection accepted (fd=%d)\n", new_fd);
                    break;
                }
            }

            if (i == MAX_CLIENTS) {
                // No space for new connection
                fprintf(stderr, "Max clients reached, rejecting new connection.\n");
                close(new_fd);
            }
        }

        // 7. Check activity on existing clients
        for (i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd < 0)
                continue;

            if (FD_ISSET(sd, &read_fds)) {
                // 8. Read data
                memset(buffer, 0, BUFFER_SIZE);
                ssize_t bytes_read = recv(sd, buffer, BUFFER_SIZE - 1, 0);
                if (bytes_read <= 0) {
                    // Connection closed or error
                    if (bytes_read == 0) {
                        printf("Client (fd=%d) disconnected.\n", sd);
                    } else {
                        perror("recv");
                    }
                    close(sd);
                    client_sockets[i] = -1;
                } else {
                    // We got data; buffer is encrypted with XOR_KEY
                    // Print encrypted data (hex or as-is)
                    printf("Encrypted data (fd=%d, %zd bytes): ", sd, bytes_read);
                    for (int j = 0; j < bytes_read; j++) {
                        printf("%02X ", (unsigned char)buffer[j]);
                    }
                    printf("\n");
                    printf("or like this: %s\n", buffer);
                    // Dragon #3: Decrypt the received string
                    // Decryption code here (:

                    // Print decrypted data as a string
                    printf("Decrypted data: %s\n", buffer);
                }
            }
        }
    }

    // Cleanup
    close(listen_fd);
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != -1) {
            close(client_sockets[i]);
        }
    }

    return 0;
}

