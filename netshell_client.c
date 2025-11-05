#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <termios.h>
#include <poll.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DEFAULT_PORT 2324
#define BUFFER_SIZE 4096
#define MAX_PATH 512
#define EXTENDED_PROTOCOL_MAGIC "NETSHELL_EXTENDED_V1\n"
#define EXTENDED_ACK "EXTENDED_ACK\n"

// Global flag for extended protocol mode
int extended_mode = 0;

// Function to establish connection
int connect_to_server(const char* hostname, int port) {
    struct sockaddr_in server_addr;
    struct hostent *server;
    int sockfd;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    // Get host information
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "No such host: %s\n", hostname);
        close(sockfd);
        return -1;
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr, server->h_addr_list[0], server->h_length);

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

// Function to enable raw mode for terminal (for ncurses compatibility)
void enable_raw_mode(struct termios *orig_termios) {
    struct termios raw = *orig_termios;
    
    // Disable canonical mode and echo
    raw.c_lflag &= ~(ECHO | ICANON);
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0) {
        perror("tcsetattr");
    }
}

// Function to disable raw mode and restore terminal
void disable_raw_mode(struct termios *orig_termios) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, orig_termios) < 0) {
        perror("tcsetattr");
    }
}

// Check if the server supports extended protocol
int negotiate_extended_protocol(int sockfd) {
    // Send the magic string to activate extended protocol
    if (send(sockfd, EXTENDED_PROTOCOL_MAGIC, strlen(EXTENDED_PROTOCOL_MAGIC), 0) < 0) {
        perror("send magic string");
        return 0;
    }

    char response[BUFFER_SIZE];
    ssize_t bytes_read = recv(sockfd, response, sizeof(response) - 1, 0);
    if (bytes_read > 0) {
        response[bytes_read] = '\0';
        if (strncmp(response, EXTENDED_ACK, strlen(EXTENDED_ACK)) == 0) {
            printf("Extended protocol activated\n");
            return 1;
        }
    }
    
    printf("Extended protocol not supported by server, using basic mode\n");
    return 0;
}

// Function to send a file to the server
int send_file_to_server(int sockfd, const char* local_path, const char* remote_path) {
    struct stat file_stat;
    FILE *file;
    char command[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char *file_buffer;
    ssize_t bytes_read;

    // Get file size
    if (stat(local_path, &file_stat) != 0) {
        perror("stat");
        return 0;
    }

    // Open file for reading
    file = fopen(local_path, "rb");
    if (!file) {
        perror("fopen");
        return 0;
    }

    // Allocate buffer for file content
    file_buffer = malloc(file_stat.st_size);
    if (!file_buffer) {
        perror("malloc");
        fclose(file);
        return 0;
    }

    // Read file content
    if (fread(file_buffer, 1, file_stat.st_size, file) != (size_t)file_stat.st_size) {
        perror("fread");
        free(file_buffer);
        fclose(file);
        return 0;
    }

    fclose(file);

    // Send command to server
    snprintf(command, sizeof(command), "SEND_FILE %s %ld\n", remote_path, (long)file_stat.st_size);
    if (send(sockfd, command, strlen(command), 0) < 0) {
        perror("send command");
        free(file_buffer);
        return 0;
    }

    // Wait for server response
    bytes_read = recv(sockfd, response, sizeof(response) - 1, 0);
    if (bytes_read > 0) {
        response[bytes_read] = '\0';
        if (strncmp(response, "READY", 5) == 0) {
            // Send file content
            if (send(sockfd, file_buffer, file_stat.st_size, 0) < 0) {
                perror("send file");
                free(file_buffer);
                return 0;
            }

            // Wait for final response
            bytes_read = recv(sockfd, response, sizeof(response) - 1, 0);
            if (bytes_read > 0) {
                response[bytes_read] = '\0';
                if (strncmp(response, "OK", 2) == 0) {
                    printf("File %s sent successfully\n", local_path);
                    free(file_buffer);
                    return 1;
                }
            }
        }
    }

    free(file_buffer);
    printf("Failed to send file: %s\n", response);
    return 0;
}

// Function to receive a file from the server
int receive_file_from_server(int sockfd, const char* remote_path, const char* local_path) {
    char command[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char *file_buffer = NULL;
    FILE *file;
    long file_size = 0;
    ssize_t bytes_read;

    // Send command to server
    snprintf(command, sizeof(command), "GET_FILE %s\n", remote_path);
    if (send(sockfd, command, strlen(command), 0) < 0) {
        perror("send command");
        return 0;
    }

    // Wait for server response with file size
    bytes_read = recv(sockfd, response, sizeof(response) - 1, 0);
    if (bytes_read > 0) {
        response[bytes_read] = '\0';
        
        if (strncmp(response, "SIZE", 4) == 0) {
            // Parse file size
            sscanf(response, "SIZE %ld", &file_size);
            
            // Allocate buffer for file content
            file_buffer = malloc(file_size);
            if (!file_buffer) {
                perror("malloc");
                return 0;
            }

            // Read file content
            ssize_t total_read = 0;
            while (total_read < file_size) {
                bytes_read = recv(sockfd, file_buffer + total_read, 
                                file_size - total_read, 0);
                if (bytes_read <= 0) break;
                total_read += bytes_read;
            }

            if (total_read == file_size) {
                // Write file to local system
                file = fopen(local_path, "wb");
                if (file) {
                    if (fwrite(file_buffer, 1, file_size, file) == (size_t)file_size) {
                        printf("File %s received successfully\n", local_path);
                        fclose(file);
                        free(file_buffer);
                        return 1;
                    } else {
                        perror("fwrite");
                    }
                    fclose(file);
                } else {
                    perror("fopen");
                }
            } else {
                printf("Error receiving file: incomplete data\n");
            }
        } else if (strncmp(response, "NOT_FOUND", 9) == 0) {
            printf("File not found on server: %s\n", remote_path);
        } else {
            printf("Server error: %s\n", response);
        }
    }

    if (file_buffer) free(file_buffer);
    return 0;
}

// Interactive mode for command line operations
void interactive_mode(int sockfd) {
    struct termios orig_termios;
    char input_buffer[BUFFER_SIZE];
    char command_buffer[BUFFER_SIZE];
    char *cmd, *arg1, *arg2;
    ssize_t bytes_read;
    struct pollfd pfd[2]; // 0: stdin, 1: socket

    // Save original terminal settings
    if (tcgetattr(STDIN_FILENO, &orig_termios) < 0) {
        perror("tcgetattr");
        return;
    }

    printf("\nNetShell Client Connected\n");
    printf("Type 'help' for commands, 'exit' to quit\n");
    printf("> ");

    // Setup poll structures
    pfd[0].fd = STDIN_FILENO;
    pfd[0].events = POLLIN;
    pfd[1].fd = sockfd;
    pfd[1].events = POLLIN;

    while (1) {
        // Poll for input from both stdin and socket
        int ret = poll(pfd, 2, 1000);  // 1 second timeout

        if (ret < 0) {
            perror("poll");
            break;
        }

        if (pfd[0].revents & POLLIN) {
            // Input from stdin
            if (fgets(input_buffer, sizeof(input_buffer), stdin)) {
                // Remove newline
                input_buffer[strcspn(input_buffer, "\n")] = 0;
                
                // Parse command
                if (strlen(input_buffer) == 0) {
                    printf("> ");
                    continue;
                }
                
                // Copy to command buffer for parsing
                strcpy(command_buffer, input_buffer);
                
                // Parse the command
                char *saveptr;
                cmd = strtok_r(command_buffer, " ", &saveptr);
                if (!cmd) {
                    printf("Invalid command\n> ");
                    continue;
                }
                
                arg1 = strtok_r(NULL, " ", &saveptr);
                arg2 = strtok_r(NULL, " ", &saveptr);
                
                if (strcmp(cmd, "help") == 0) {
                    printf("Available commands:\n");
                    if (extended_mode) {
                        printf("  send_file <local_path> <remote_path> - Send a file to server\n");
                        printf("  get_file <remote_path> <local_path> - Download a file from server\n");
                    }
                    printf("  ncurses <command> - Run command with ncurses support\n");
                    printf("  exit - Exit the client\n");
                    printf("  Other commands are sent directly to the remote shell\n");
                } else if (strcmp(cmd, "exit") == 0) {
                    printf("Disconnecting...\n");
                    break;
                } else if (strcmp(cmd, "ncurses") == 0) {
                    if (!arg1) {
                        printf("Usage: ncurses <command>\n> ");
                        continue;
                    }
                    // Send the ncurses command to the server
                    snprintf(input_buffer, sizeof(input_buffer), "%s\n", arg1);
                    if (send(sockfd, input_buffer, strlen(input_buffer), 0) < 0) {
                        perror("send");
                        break;
                    }
                    
                    // Enable raw mode for ncurses application
                    enable_raw_mode(&orig_termios);
                    
                    // Now forward data between stdin/stdout and socket
                    fd_set read_fds;
                    int max_fd;
                    while (1) {
                        FD_ZERO(&read_fds);
                        FD_SET(STDIN_FILENO, &read_fds);
                        FD_SET(sockfd, &read_fds);
                        
                        max_fd = (STDIN_FILENO > sockfd) ? STDIN_FILENO : sockfd;
                        max_fd++;
                        
                        if (select(max_fd, &read_fds, NULL, NULL, NULL) < 0) {
                            perror("select");
                            break;
                        }
                        
                        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
                            char ch;
                            ssize_t result = read(STDIN_FILENO, &ch, 1);
                            if (result > 0) {
                                if (send(sockfd, &ch, 1, 0) < 0) {
                                    break;
                                }
                            } else if (result <= 0) {
                                break; // stdin closed or error
                            }
                        }
                        
                        if (FD_ISSET(sockfd, &read_fds)) {
                            char ch;
                            ssize_t bytes = recv(sockfd, &ch, 1, 0);
                            if (bytes <= 0) {
                                // Connection closed
                                break;
                            }
                            if (write(STDOUT_FILENO, &ch, 1) < 0) {
                                break;
                            }
                        }
                    }
                    
                    // Restore terminal
                    disable_raw_mode(&orig_termios);
                    printf("\n> ");
                    continue;
                } else if (extended_mode && strcmp(cmd, "send_file") == 0) {
                    if (!arg1 || !arg2) {
                        printf("Usage: send_file <local_path> <remote_path>\n");
                    } else {
                        send_file_to_server(sockfd, arg1, arg2);
                    }
                } else if (extended_mode && strcmp(cmd, "get_file") == 0) {
                    if (!arg1 || !arg2) {
                        printf("Usage: get_file <remote_path> <local_path>\n");
                    } else {
                        receive_file_from_server(sockfd, arg1, arg2);
                    }
                } else {
                    // Regular command - send to server
                    snprintf(input_buffer, sizeof(input_buffer), "%s\n", command_buffer);
                    if (send(sockfd, input_buffer, strlen(input_buffer), 0) < 0) {
                        perror("send");
                        break;
                    }
                }
            }
            printf("> ");
        }

        if (pfd[1].revents & POLLIN) {
            // Input from socket (server response)
            bytes_read = recv(sockfd, input_buffer, sizeof(input_buffer) - 1, 0);
            if (bytes_read <= 0) {
                if (bytes_read < 0) {
                    perror("recv");
                }
                printf("\nConnection closed by server\n");
                break;
            }
            input_buffer[bytes_read] = '\0';
            printf("%s", input_buffer);
            fflush(stdout);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <hostname> [port]\n", argv[0]);
        fprintf(stderr, "       %s <hostname> send_file <local_path> <remote_path>\n", argv[0]);
        fprintf(stderr, "       %s <hostname> get_file <remote_path> <local_path>\n", argv[0]);
        return 1;
    }

    const char *hostname = argv[1];
    int port = DEFAULT_PORT;
    
    if (argc >= 3) {
        // Check if it's a special command (send_file, get_file, etc.)
        if (strcmp(argv[2], "send_file") == 0) {
            if (argc != 5) {
                fprintf(stderr, "Usage: %s <hostname> send_file <local_path> <remote_path>\n", argv[0]);
                return 1;
            }
            
            int sockfd = connect_to_server(hostname, port);
            if (sockfd < 0) {
                return 1;
            }
            
            // Try to negotiate extended protocol
            extended_mode = negotiate_extended_protocol(sockfd);
            
            if (!extended_mode) {
                printf("Extended protocol required for file operations\n");
                close(sockfd);
                return 1;
            }
            
            send_file_to_server(sockfd, argv[3], argv[4]);
            close(sockfd);
            return 0;
        } else if (strcmp(argv[2], "get_file") == 0) {
            if (argc != 5) {
                fprintf(stderr, "Usage: %s <hostname> get_file <remote_path> <local_path>\n", argv[0]);
                return 1;
            }
            
            int sockfd = connect_to_server(hostname, port);
            if (sockfd < 0) {
                return 1;
            }
            
            // Try to negotiate extended protocol
            extended_mode = negotiate_extended_protocol(sockfd);
            
            if (!extended_mode) {
                printf("Extended protocol required for file operations\n");
                close(sockfd);
                return 1;
            }
            
            receive_file_from_server(sockfd, argv[3], argv[4]);
            close(sockfd);
            return 0;
        } else {
            // Port specified as second argument
            port = atoi(argv[2]);
            if (port <= 0 || port > 65535) {
                fprintf(stderr, "Invalid port: %s\n", argv[2]);
                return 1;
            }
        }
    }

    int sockfd = connect_to_server(hostname, port);
    if (sockfd < 0) {
        return 1;
    }

    // Try to negotiate extended protocol
    extended_mode = negotiate_extended_protocol(sockfd);

    // Enter interactive mode
    interactive_mode(sockfd);

    close(sockfd);
    return 0;
}