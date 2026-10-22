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
#include <signal.h>

#define PORT 12345
#define MAX_CLIENTS  FD_SETSIZE  // or define your own limit
#define BUFFER_SIZE 1024

/*
 * Explanation:
 *  - We use select() so that the server doesn't block on any single operation
 *    (accepting new connections or reading from existing sockets).
 *  - This allows multiple client connections to be handled in a single thread.
 */

/*
 * DRAGON #4: Ctrl+C (SIGINT) should shut down the server cleanly.
 * Remember last week's lab: sig_atomic_t, signal(), close(), errno.
 *
 * A handler should only request shutdown and do async-signal-safe work.
 * The actual cleanup of client sockets belongs after the main loop.
 */
static volatile sig_atomic_t stop_requested = 0;
static volatile sig_atomic_t listening_fd = -1;

static void on_sigint(int signum)
{
    int saved_errno = errno;
    (void)signum;

    /* DRAGON #4 TODO:
     * 1. Set stop_requested.
     * 2. Close the listening socket (if it is open), and mark it closed.
     *    Why might closing it help if SIGINT arrives just BEFORE select()?
     * 3. Do not use printf() here.
     */

    errno = saved_errno;
}

int main(void)
{
    int listen_fd, new_fd, max_fd, i;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;
    char buffer[BUFFER_SIZE];
    int client_sockets[MAX_CLIENTS];
    fd_set read_fds;

    /* DRAGON #4 TODO: Register on_sigint as the handler for SIGINT.
     * Hint: look at signal(SIGINT, ...) in last week's server.c.
     */

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
    // DRAGON #2 - the following string won't work. It won't listen on the port you want.  Fix it.
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

    /* DRAGON #4: Let the handler know which descriptor is listening.
     * See how the previous lab dealt with its listening socket.
     */
    listening_fd = listen_fd;

    printf("Server listening on 127.0.0.1:%d\n", PORT);

    // 4. Server loop
    // DRAGON #4 TODO: Stop this loop when SIGINT requests shutdown.
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
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL); // This is select!
        if (activity < 0) {
            /* DRAGON #4 TODO:
             * - A signal may interrupt select() (errno == EINTR).
             * - Closing the listening socket may cause errno == EBADF.
             * - If stop_requested, leave the loop without reporting an
             *   expected shutdown as an error.
             * - Otherwise retry on EINTR, or report real errors.
             * Never use FD_ISSET() when select() has failed.
             */
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            break;
        }
        // DRAGON #4 TODO: Check stop_requested again before using read_fds.

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

    // Cleanup (ordinary code, not inside the signal handler).
    // DRAGON #4: The SIGINT handler may ALREADY have closed listen_fd.
    if (listening_fd >= 0) {
        listening_fd = -1;
        close(listen_fd);
    }
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != -1) {
            close(client_sockets[i]);
        }
    }

    if (stop_requested) {
        puts("Server: SIGINT caught; sockets closed, exiting");
    }
    return 0;
}

