#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/stat.h>

#ifdef MORPHOS
#include <sys/time.h>
#include <sys/ioctl.h>
// Define missing constants for MorphOS
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif
#endif

#include <sys/time.h>

#define DEFAULT_PORT 2324
#define BACKLOG 10
#define BUFFER_SIZE 1024
#define MAX_PATH 512
#define EXTENDED_PROTOCOL_MAGIC "NETSHELL_EXTENDED_V1\n"
#define EXTENDED_ACK "EXTENDED_ACK\n"

volatile sig_atomic_t server_running = 1;

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    server_running = 0;
    // Reinstall the signal handler (needed for some systems)
    signal(sig, signal_handler);
}

// Define socklen_t if not available on MorphOS
#ifdef MORPHOS
#ifndef socklen_t
typedef unsigned int socklen_t;
#endif
#endif

// Check if the connection should use extended protocol
int check_extended_protocol(int socket_fd) {
    char buffer[BUFFER_SIZE];
    fd_set read_fds;
    struct timeval timeout;

    // Set up timeout for 2 seconds to receive the magic string
    FD_ZERO(&read_fds);
    FD_SET(socket_fd, &read_fds);
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    if (select(socket_fd + 1, &read_fds, NULL, NULL, &timeout) > 0) {
        // Check if data is available
        if (FD_ISSET(socket_fd, &read_fds)) {
            // Peek at the data first to see if it matches the magic string
            ssize_t bytes_read = recv(socket_fd, buffer, strlen(EXTENDED_PROTOCOL_MAGIC), MSG_PEEK);
            
            if (bytes_read >= (ssize_t)strlen(EXTENDED_PROTOCOL_MAGIC) - 1) {
                // Now actually read the data
                bytes_read = recv(socket_fd, buffer, strlen(EXTENDED_PROTOCOL_MAGIC), 0);
                
                if (bytes_read >= (ssize_t)strlen(EXTENDED_PROTOCOL_MAGIC) - 1) {
                    buffer[bytes_read] = '\0';
                    
                    if (strncmp(buffer, EXTENDED_PROTOCOL_MAGIC, strlen(EXTENDED_PROTOCOL_MAGIC) - 1) == 0) {
                        // Send ACK for extended protocol
                        send(socket_fd, EXTENDED_ACK, strlen(EXTENDED_ACK), 0);
                        return 1; // Extended protocol active
                    }
                }
            }
        }
    }
    
    return 0; // Basic protocol
}

// Handle file transfer commands
int handle_extended_commands(int socket_fd, const char* command) {
    char cmd[MAX_PATH];
    char filename[MAX_PATH];
    char response[BUFFER_SIZE];
    
    // Parse the command
    if (sscanf(command, "%s", cmd) == 1) {
        if (strcmp(cmd, "SEND_FILE") == 0) {
            char size_str[32];
            if (sscanf(command, "%s %s %s", cmd, filename, size_str) == 3) {
                long file_size = atol(size_str);
                if (file_size > 0) {
                    // Send ready message
                    send(socket_fd, "READY\n", 6, 0);
                    
                    // Read the file data
                    FILE *file = fopen(filename, "wb");
                    if (file) {
                        char *file_buffer = malloc(file_size);
                        if (file_buffer) {
                            ssize_t bytes_read = 0;
                            ssize_t total_read = 0;
                            
                            while (total_read < file_size) {
                                bytes_read = recv(socket_fd, 
                                    file_buffer + total_read, 
                                    file_size - total_read, 0);
                                if (bytes_read <= 0) break;
                                total_read += bytes_read;
                            }
                            
                            if (fwrite(file_buffer, 1, file_size, file) == (size_t)file_size) {
                                send(socket_fd, "OK\n", 3, 0);
                            } else {
                                send(socket_fd, "ERROR\n", 6, 0);
                            }
                            
                            free(file_buffer);
                        } else {
                            send(socket_fd, "ERROR\n", 6, 0);
                        }
                        fclose(file);
                    } else {
                        send(socket_fd, "DENY\n", 5, 0);
                    }
                } else {
                    send(socket_fd, "DENY\n", 5, 0);
                }
            } else {
                send(socket_fd, "DENY\n", 5, 0);
            }
            return 1;
        } else if (strcmp(cmd, "GET_FILE") == 0) {
            if (sscanf(command, "%s %s", cmd, filename) == 2) {
                struct stat file_stat;
                if (stat(filename, &file_stat) == 0) {
                    // Send file size
                    snprintf(response, sizeof(response), "SIZE %ld\n", (long)file_stat.st_size);
                    send(socket_fd, response, strlen(response), 0);
                    
                    // Open and send the file
                    FILE *file = fopen(filename, "rb");
                    if (file) {
                        char *file_buffer = malloc(file_stat.st_size);
                        if (file_buffer && fread(file_buffer, 1, file_stat.st_size, file) == (size_t)file_stat.st_size) {
                            send(socket_fd, file_buffer, file_stat.st_size, 0);
                        } else {
                            // Send error if unable to read
                            send(socket_fd, "ERROR\n", 6, 0);
                        }
                        if (file_buffer) free(file_buffer);
                        fclose(file);
                    } else {
                        send(socket_fd, "ERROR\n", 6, 0);
                    }
                } else {
                    send(socket_fd, "NOT_FOUND\n", 10, 0);
                }
            } else {
                send(socket_fd, "ERROR\n", 6, 0);
            }
            return 1;
        }
    }
    return 0; // Not a recognized extended command
}

// Function to handle each client connection in basic mode
void handle_basic_client(int client_fd) {
    pid_t pid;
    int status;
    
    // Fork to create shell process (use vfork on MorphOS)
#ifdef MORPHOS
    pid = vfork();
#else
    pid = fork();
#endif
    
    if (pid == 0) {
        // Child process - set up shell
        // Redirect stdin, stdout, stderr to client socket
        dup2(client_fd, STDIN_FILENO);
        dup2(client_fd, STDOUT_FILENO);
        dup2(client_fd, STDERR_FILENO);
        
        // Close the original client socket since we've duplicated it
        close(client_fd);
        
#ifdef MORPHOS
        // On MorphOS, use ksh from the development environment
        execl("Work:/Development/gg/bin/ksh", "ksh", NULL);
#else
        // On other systems, use standard sh
        execl("/bin/sh", "sh", NULL);
#endif
        
        // If execl returns, it failed
        perror("execl");
        _exit(1);  // Use _exit instead of exit in child after vfork
    } else if (pid > 0) {
        // Parent process - monitor the shell process
        close(client_fd);  // Close client socket in parent
        
        // Wait for shell process to finish
        waitpid(pid, &status, 0);
    } else {
        // Fork failed
        perror("fork");
        close(client_fd);
    }
    
    close(client_fd);
}

// Function to handle each client connection in extended mode
void handle_extended_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    printf("Handling client in extended mode\n");
    
    // Main loop for extended protocol
    while (1) {
        bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) break;
        
        buffer[bytes_read] = '\0';
        
        // Check if it's an extended command first
        if (handle_extended_commands(client_fd, buffer)) {
            continue;
        }
        
        // For any other commands, we'll just respond that we're in extended mode
        // but the command isn't recognized
        send(client_fd, "UNKNOWN_COMMAND\n", 16, 0);
    }
    
    close(client_fd);
}

int main(int argc, char *argv[]) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int port;
    int opt;
    pid_t pid;
#ifdef MORPHOS
    char client_ip[INET_ADDRSTRLEN];
#endif
    
    client_len = sizeof(client_addr);
    port = DEFAULT_PORT;
    
    // Parse command line arguments for port
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port number. Using default port %d\n", DEFAULT_PORT);
            port = DEFAULT_PORT;
        }
    }
    
    // Set up signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
    
    // Set socket options to reuse address
    opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        exit(1);
    }
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Add a small delay to ensure port is released
    sleep(1);
    
    // Bind the socket
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        fprintf(stderr, "Failed to bind to port %d. Error: %s\n", port, strerror(errno));
        close(server_fd);
        exit(1);
    }
    
    // Listen for connections
    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        close(server_fd);
        exit(1);
    }
    
    printf("NetShell server listening on port %d (with extended protocol support)...\n", port);
    printf("Waiting for connections (Press Ctrl+C to stop)...\n");
    
    // Accept and handle connections
    while (server_running) {
        // Periodically clean up zombie processes
        waitpid(-1, NULL, WNOHANG);
        
#ifdef MORPHOS
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&client_len);
#else
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
#endif
        
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue; // Interrupted by signal, continue loop
            } else {
                perror("accept");
                continue;
            }
        }
        
#ifdef MORPHOS
        // On MorphOS, use simpler approach for client IP
        printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
#else
        // Get client address information
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("New connection from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
#endif
        
        // Fork to handle the client
#ifdef MORPHOS
        pid = vfork();
#else
        pid = fork();
#endif
        
        if (pid == 0) {
            // Child process - handle client
            close(server_fd);  // Close server socket in child
            
            // Check if client wants extended protocol
            int extended_mode = check_extended_protocol(client_fd);
            
            if (extended_mode) {
                printf("Extended protocol activated for connection %s:%d\n", 
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                handle_extended_client(client_fd);
            } else {
                printf("Basic protocol mode for connection %s:%d\n", 
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                handle_basic_client(client_fd);
            }
            
#ifdef MORPHOS
            _exit(0);  // Use _exit instead of exit in child after vfork
#else
            exit(0);  // Exit child process
#endif
        } else if (pid > 0) {
            // Parent process - close client socket and continue accepting
            close(client_fd);
        } else {
            // Fork failed
            perror("fork");
            close(client_fd);
        }
    }
    
    // Close server socket
    close(server_fd);
    printf("\nServer shutting down...\n");
    
    return 0;
}