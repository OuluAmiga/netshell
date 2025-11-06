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
#include <sys/time.h>
#include <time.h>

#define DEFAULT_PORT 2324
#define BUFFER_SIZE 4096
#define MAX_PATH 512
#define SESSION_DIR ".config/netshell"
#define DEFAULT_SESSION_FILE ".config/netshell/default"
#define EXTENDED_PROTOCOL_MAGIC "NETSHELL_EXTENDED_V1\n"
#define EXTENDED_ACK "EXTENDED_ACK\n"

// Global flag for extended protocol mode
int extended_mode = 0;

// Session configuration structure
struct SessionConfig {
    char hostname[256];
    int port;
    char username[64];
    char description[256];
    time_t last_used;
    int is_default;
};

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
            return 1;
        }
    }
    
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
                    free(file_buffer);
                    return 1;
                }
            }
        }
    }

    free(file_buffer);
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
            }
        } else if (strncmp(response, "NOT_FOUND", 9) == 0) {
            // File not found on server
        }
    }

    if (file_buffer) free(file_buffer);
    return 0;
}

// Function to execute a command and return output
int execute_command(const char* hostname, int port, const char* command) {
    int sockfd = connect_to_server(hostname, port);
    if (sockfd < 0) {
        return 1;
    }

    // Try to negotiate extended protocol
    extended_mode = negotiate_extended_protocol(sockfd);
    
    if (extended_mode) {
        // We don't need extended features for simple command execution
        // Just send the command and get the output
    }

    // Send the command
    char cmd_buffer[BUFFER_SIZE];
    snprintf(cmd_buffer, sizeof(cmd_buffer), "%s\n", command);
    if (send(sockfd, cmd_buffer, strlen(cmd_buffer), 0) < 0) {
        perror("send");
        close(sockfd);
        return 1;
    }

    // Receive and print output with timeout
    char response[BUFFER_SIZE];
    ssize_t bytes_read;
    fd_set read_fds;
    struct timeval timeout;
    
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        
        // Set timeout to 2 seconds
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        
        int select_result = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (select_result < 0) {
            perror("select");
            break;
        } else if (select_result == 0) {
            // Timeout - no more data, break
            break;
        }
        
        if (FD_ISSET(sockfd, &read_fds)) {
            bytes_read = recv(sockfd, response, sizeof(response) - 1, 0);
            if (bytes_read > 0) {
                response[bytes_read] = '\0';
                printf("%s", response);
                fflush(stdout);
            } else if (bytes_read <= 0) {
                // Connection closed or error
                break;
            }
        }
    }

    close(sockfd);
    return 0;
}

// Function to execute a command from file and return output
int execute_command_from_file(const char* hostname, int port, const char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        return 1;
    }

    // Read the entire file content
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *command = malloc(file_size + 1);
    if (!command) {
        fclose(file);
        perror("malloc");
        return 1;
    }

    size_t bytes_read = fread(command, 1, file_size, file);
    command[bytes_read] = '\0';
    fclose(file);

    // Remove trailing newline if present
    if (bytes_read > 0 && command[bytes_read - 1] == '\n') {
        command[bytes_read - 1] = '\0';
    }

    int result = execute_command(hostname, port, command);
    free(command);
    return result;
}

// Function to create config directory if it doesn't exist
int create_config_dir() {
    char home_dir[1024];
    char config_path[1024];
    
    if (getenv("HOME") == NULL) {
        fprintf(stderr, "HOME environment variable not set\n");
        return 1;
    }
    
    snprintf(home_dir, sizeof(home_dir), "%s", getenv("HOME"));
    snprintf(config_path, sizeof(config_path), "%s/%s", home_dir, SESSION_DIR);
    
    // Create config directory
    if (mkdir(config_path, 0755) != 0 && errno != EEXIST) {
        perror("mkdir config directory");
        return 1;
    }
    
    return 0;
}

// Function to save session config
int save_session_config(const struct SessionConfig *config, const char *session_name) {
    if (create_config_dir() != 0) {
        return 1;
    }
    
    char home_dir[1024];
    char session_path[1024];
    
    snprintf(home_dir, sizeof(home_dir), "%s", getenv("HOME"));
    snprintf(session_path, sizeof(session_path), "%s/%s/%s", home_dir, SESSION_DIR, session_name);
    
    FILE *file = fopen(session_path, "w");
    if (!file) {
        perror("fopen session file");
        return 1;
    }
    
    fprintf(file, "hostname=%s\n", config->hostname);
    fprintf(file, "port=%d\n", config->port);
    fprintf(file, "username=%s\n", config->username);
    fprintf(file, "description=%s\n", config->description);
    fprintf(file, "last_used=%ld\n", config->last_used);
    fprintf(file, "is_default=%d\n", config->is_default);
    
    fclose(file);
    return 0;
}

// Function to load session config
int load_session_config(struct SessionConfig *config, const char *session_name) {
    char home_dir[1024];
    char session_path[1024];
    
    snprintf(home_dir, sizeof(home_dir), "%s", getenv("HOME"));
    snprintf(session_path, sizeof(session_path), "%s/%s/%s", home_dir, SESSION_DIR, session_name);
    
    FILE *file = fopen(session_path, "r");
    if (!file) {
        return 1; // Session file doesn't exist
    }
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0; // Remove newline
        
        if (strncmp(line, "hostname=", 9) == 0) {
            strncpy(config->hostname, line + 9, sizeof(config->hostname) - 1);
        } else if (strncmp(line, "port=", 5) == 0) {
            config->port = atoi(line + 5);
        } else if (strncmp(line, "username=", 9) == 0) {
            strncpy(config->username, line + 9, sizeof(config->username) - 1);
        } else if (strncmp(line, "description=", 12) == 0) {
            strncpy(config->description, line + 12, sizeof(config->description) - 1);
        } else if (strncmp(line, "last_used=", 10) == 0) {
            config->last_used = atol(line + 10);
        } else if (strncmp(line, "is_default=", 11) == 0) {
            config->is_default = atoi(line + 11);
        }
    }
    
    fclose(file);
    return 0;
}

// Function to load default session name
int get_default_session_name(char *session_name, size_t name_size) {
    char home_dir[1024];
    char default_path[1024];
    
    snprintf(home_dir, sizeof(home_dir), "%s", getenv("HOME"));
    snprintf(default_path, sizeof(default_path), "%s/%s", home_dir, DEFAULT_SESSION_FILE);
    
    FILE *file = fopen(default_path, "r");
    if (!file) {
        return 1; // No default session set
    }
    
    if (fgets(session_name, name_size, file)) {
        session_name[strcspn(session_name, "\n")] = 0; // Remove newline
        fclose(file);
        return 0; // Success
    }
    
    fclose(file);
    return 1; // Error reading
}

// Function to set default session
int set_default_session(const char *session_name) {
    if (create_config_dir() != 0) {
        return 1;
    }
    
    char home_dir[1024];
    char default_path[1024];
    
    snprintf(home_dir, sizeof(home_dir), "%s", getenv("HOME"));
    snprintf(default_path, sizeof(default_path), "%s/%s", home_dir, DEFAULT_SESSION_FILE);
    
    FILE *file = fopen(default_path, "w");
    if (!file) {
        perror("fopen default session file");
        return 1;
    }
    
    fprintf(file, "%s\n", session_name);
    fclose(file);
    
    // Update the session config to mark it as default
    struct SessionConfig config;
    if (load_session_config(&config, session_name) == 0) {
        config.is_default = 1;
        config.last_used = time(NULL);
        save_session_config(&config, session_name);
    }
    
    return 0;
}

// Function to unset default session
int unset_default_session() {
    char home_dir[1024];
    char default_path[1024];
    
    snprintf(home_dir, sizeof(home_dir), "%s", getenv("HOME"));
    snprintf(default_path, sizeof(default_path), "%s/%s", home_dir, DEFAULT_SESSION_FILE);
    
    // Remove the default session file
    if (remove(default_path) != 0 && errno != ENOENT) {
        perror("remove default session file");
        return 1;
    }
    
    // Also update the session config to clear default flag
    char session_name[256];
    if (get_default_session_name(session_name, sizeof(session_name)) == 0) {
        struct SessionConfig config;
        if (load_session_config(&config, session_name) == 0) {
            config.is_default = 0;
            save_session_config(&config, session_name);
        }
    }
    
    return 0;
}

// Function to list available sessions
void list_sessions() {
    char home_dir[1024];
    char session_dir[1024];
    
    snprintf(home_dir, sizeof(home_dir), "%s", getenv("HOME"));
    snprintf(session_dir, sizeof(session_dir), "%s/%s", home_dir, SESSION_DIR);
    
    // Check for default session
    char default_session[256];
    int has_default = (get_default_session_name(default_session, sizeof(default_session)) == 0);
    
    printf("Available sessions:\n");
    
    // Use system command to list session files
    char command[1024];
    snprintf(command, sizeof(command), "ls -1 %s 2>/dev/null | grep -v 'default$'", session_dir);
    
    FILE *fp = popen(command, "r");
    if (fp) {
        char session_name[256];
        int found_sessions = 0;
        while (fgets(session_name, sizeof(session_name), fp)) {
            session_name[strcspn(session_name, "\n")] = 0;
            
            // Load the session to check if it's default
            struct SessionConfig config;
            int is_default = 0;
            if (load_session_config(&config, session_name) == 0) {
                is_default = config.is_default;
            }
            
            if (is_default || (has_default && strcmp(session_name, default_session) == 0)) {
                printf("  %s (default)\n", session_name);
            } else {
                printf("  %s\n", session_name);
            }
            found_sessions = 1;
        }
        pclose(fp);
        
        if (!found_sessions) {
            printf("  No sessions found.\n");
        }
    } else {
        printf("  No sessions found.\n");
    }
    
    if (has_default) {
        printf("\nDefault session: %s\n", default_session);
    } else {
        printf("\nNo default session set.\n");
    }
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
                        if (send_file_to_server(sockfd, arg1, arg2)) {
                            printf("File sent successfully\n");
                        } else {
                            printf("Failed to send file\n");
                        }
                    }
                } else if (extended_mode && strcmp(cmd, "get_file") == 0) {
                    if (!arg1 || !arg2) {
                        printf("Usage: get_file <remote_path> <local_path>\n");
                    } else {
                        if (receive_file_from_server(sockfd, arg1, arg2)) {
                            printf("File received successfully\n");
                        } else {
                            printf("Failed to receive file\n");
                        }
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
        fprintf(stderr, "Usage: %s [options] [hostname] [port]\n", argv[0]);
        fprintf(stderr, "       %s [options] -e <command> [hostname] [port]\n", argv[0]);
        fprintf(stderr, "       %s [options] -E <file> [hostname] [port]  (eval from file)\n", argv[0]);
        fprintf(stderr, "       %s <session_name> (use saved session)\n", argv[0]);
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "  -e, --eval <command>     Execute command and exit\n");
        fprintf(stderr, "  -E, --eval-file <file>   Execute command from file and exit\n");
        fprintf(stderr, "  -s, --session <name>     Use saved session\n");
        fprintf(stderr, "  -l, --list               List saved sessions\n");
        fprintf(stderr, "  -S, --save <name>        Save current connection as session\n");
        fprintf(stderr, "  -d, --default <name>     Set default session\n");
        fprintf(stderr, "  -D, --unset-default      Unset default session\n");
        fprintf(stderr, "  -a, --address <addr>     Specify address (for save mode)\n");
        fprintf(stderr, "  -p, --port <port>        Specify port (for save mode)\n");
        fprintf(stderr, "  -u, --username <user>    Specify username (for save mode)\n");
        fprintf(stderr, "  -desc, --description <desc>  Specify description (for save mode)\n");
        fprintf(stderr, "Examples:\n");
        fprintf(stderr, "  %s -e \"ls -la\" 192.168.1.136 2324\n", argv[0]);
        fprintf(stderr, "  %s -E script.sh 192.168.1.136 2324\n", argv[0]);
        fprintf(stderr, "  %s myserver\n", argv[0]);
        fprintf(stderr, "  %s -S myserver -a 192.168.1.136 -p 2324 -desc \"My server\"\n", argv[0]);
        fprintf(stderr, "  %s -d myserver  (set default)\n", argv[0]);
        fprintf(stderr, "  %s -e \"ls -la\"  (use default session if set)\n", argv[0]);
        return 1;
    }

    int port = DEFAULT_PORT;
    const char *hostname = NULL;
    const char *session_name = NULL;
    const char *command = NULL;
    const char *command_file = NULL;
    int eval_mode = 0;
    int eval_file_mode = 0;
    int save_session = 0;
    int list_sessions_flag = 0;
    int set_default = 0;
    int unset_default = 0;
    char temp_session_name[256] = {0};
    char temp_hostname[256] = {0};
    char temp_username[64] = "unknown";
    char temp_description[256] = "No description";
    int temp_port = DEFAULT_PORT;
    int has_temp_config = 0;
    int arg_idx = 1;

    // Parse command line options
    while (arg_idx < argc) {
        if (strcmp(argv[arg_idx], "-e") == 0 || strcmp(argv[arg_idx], "--eval") == 0) {
            if (arg_idx + 1 >= argc) {
                fprintf(stderr, "Error: -e/--eval requires a command argument\n");
                return 1;
            }
            eval_mode = 1;
            command = argv[arg_idx + 1];
            arg_idx += 2;
        } else if (strcmp(argv[arg_idx], "-E") == 0 || strcmp(argv[arg_idx], "--eval-file") == 0) {
            if (arg_idx + 1 >= argc) {
                fprintf(stderr, "Error: -E/--eval-file requires a file argument\n");
                return 1;
            }
            eval_file_mode = 1;
            command_file = argv[arg_idx + 1];
            arg_idx += 2;
        } else if (strcmp(argv[arg_idx], "-s") == 0 || strcmp(argv[arg_idx], "--session") == 0) {
            if (arg_idx + 1 >= argc) {
                fprintf(stderr, "Error: -s/--session requires a session name\n");
                return 1;
            }
            session_name = argv[arg_idx + 1];
            arg_idx += 2;
        } else if (strcmp(argv[arg_idx], "-l") == 0 || strcmp(argv[arg_idx], "--list") == 0) {
            list_sessions_flag = 1;
            arg_idx++;
        } else if (strcmp(argv[arg_idx], "-S") == 0 || strcmp(argv[arg_idx], "--save") == 0) {
            if (arg_idx + 1 >= argc) {
                fprintf(stderr, "Error: -S/--save requires a session name\n");
                return 1;
            }
            save_session = 1;
            strncpy(temp_session_name, argv[arg_idx + 1], sizeof(temp_session_name) - 1);
            arg_idx += 2;
        } else if (strcmp(argv[arg_idx], "-d") == 0 || strcmp(argv[arg_idx], "--default") == 0) {
            if (arg_idx + 1 >= argc) {
                fprintf(stderr, "Error: -d/--default requires a session name\n");
                return 1;
            }
            set_default = 1;
            session_name = argv[arg_idx + 1];
            arg_idx += 2;
        } else if (strcmp(argv[arg_idx], "-D") == 0 || strcmp(argv[arg_idx], "--unset-default") == 0) {
            unset_default = 1;
            arg_idx++;
        } else if (strcmp(argv[arg_idx], "-a") == 0 || strcmp(argv[arg_idx], "--address") == 0) {
            if (arg_idx + 1 >= argc) {
                fprintf(stderr, "Error: -a/--address requires an address\n");
                return 1;
            }
            strncpy(temp_hostname, argv[arg_idx + 1], sizeof(temp_hostname) - 1);
            has_temp_config = 1;
            arg_idx += 2;
        } else if (strcmp(argv[arg_idx], "-p") == 0 || strcmp(argv[arg_idx], "--port") == 0) {
            if (arg_idx + 1 >= argc) {
                fprintf(stderr, "Error: -p/--port requires a port number\n");
                return 1;
            }
            temp_port = atoi(argv[arg_idx + 1]);
            if (temp_port <= 0 || temp_port > 65535) {
                fprintf(stderr, "Invalid port: %s\n", argv[arg_idx + 1]);
                return 1;
            }
            has_temp_config = 1;
            arg_idx += 2;
        } else if (strcmp(argv[arg_idx], "-u") == 0 || strcmp(argv[arg_idx], "--username") == 0) {
            if (arg_idx + 1 >= argc) {
                fprintf(stderr, "Error: -u/--username requires a username\n");
                return 1;
            }
            strncpy(temp_username, argv[arg_idx + 1], sizeof(temp_username) - 1);
            arg_idx += 2;
        } else if (strcmp(argv[arg_idx], "-desc") == 0 || strcmp(argv[arg_idx], "--description") == 0) {
            if (arg_idx + 1 >= argc) {
                fprintf(stderr, "Error: --description requires a description\n");
                return 1;
            }
            strncpy(temp_description, argv[arg_idx + 1], sizeof(temp_description) - 1);
            arg_idx += 2;
        } else if (argv[arg_idx][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[arg_idx]);
            return 1;
        } else {
            // This must be hostname, session name, or port
            if (!hostname && !session_name && !save_session) {
                // Check if this looks like a session name (was given to -S) or just an address
                // If save_session is 1, this should be treated as hostname for saving
                if (save_session && strlen(temp_session_name) > 0) {
                    strncpy(temp_hostname, argv[arg_idx], sizeof(temp_hostname) - 1);
                    has_temp_config = 1;
                } else {
                    // Could be session name or hostname - check if it's a valid IP/hostname format
                    hostname = argv[arg_idx];
                }
            } else if (hostname && port == DEFAULT_PORT) {
                // Likely a port number
                int parsed_port = atoi(argv[arg_idx]);
                if (parsed_port > 0 && parsed_port <= 65535) {
                    port = parsed_port;
                } else {
                    fprintf(stderr, "Invalid port: %s\n", argv[arg_idx]);
                    return 1;
                }
            }
            arg_idx++;
        }
    }

    // Handle operations that don't require connection
    if (list_sessions_flag) {
        list_sessions();
        return 0;
    }
    
    if (set_default) {
        if (set_default_session(session_name) == 0) {
            printf("Default session set to '%s'.\n", session_name);
        } else {
            fprintf(stderr, "Failed to set default session to '%s'\n", session_name);
            return 1;
        }
        return 0;
    }
    
    if (unset_default) {
        if (unset_default_session() == 0) {
            printf("Default session unset.\n");
        } else {
            fprintf(stderr, "Failed to unset default session\n");
            return 1;
        }
        return 0;
    }

    // If save session was requested, save the current configuration
    if (save_session) {
        if (strlen(temp_session_name) == 0) {
            fprintf(stderr, "Error: Session name required for save operation\n");
            return 1;
        }
        
        struct SessionConfig config;
        if (strlen(temp_hostname) > 0) {
            strncpy(config.hostname, temp_hostname, sizeof(config.hostname) - 1);
        } else {
            fprintf(stderr, "Error: Hostname required for save operation\n");
            return 1;
        }
        
        config.port = temp_port;
        strncpy(config.username, temp_username, sizeof(config.username) - 1);
        strncpy(config.description, temp_description, sizeof(config.description) - 1);
        config.last_used = time(NULL);
        config.is_default = 0; // Don't automatically make it default
        
        if (save_session_config(&config, temp_session_name) == 0) {
            printf("Session '%s' saved.\n", temp_session_name);
        } else {
            fprintf(stderr, "Failed to save session '%s'\n", temp_session_name);
        }
        return 0;
    }

    // Check for default session if no hostname or session specified
    if (!hostname && !session_name) {
        char default_session[256];
        if (get_default_session_name(default_session, sizeof(default_session)) == 0) {
            session_name = default_session;
            //printf("Using default session: %s\n", default_session); // Comment out to reduce output
        }
    }

    // If using a session, load the configuration
    if (session_name) {
        struct SessionConfig config;
        if (load_session_config(&config, session_name) == 0) {
            hostname = config.hostname;
            port = config.port;
            
            // Update last used time
            config.last_used = time(NULL);
            save_session_config(&config, session_name);
        } else {
            fprintf(stderr, "Session '%s' not found\n", session_name);
            printf("Available sessions:\n");
            list_sessions();
            return 1;
        }
    }

    // Handle eval modes
    if (eval_mode && command) {
        if (!hostname) {
            char default_session[256];
            if (get_default_session_name(default_session, sizeof(default_session)) == 0) {
                struct SessionConfig config;
                if (load_session_config(&config, default_session) == 0) {
                    hostname = config.hostname;
                    port = config.port;
                } else {
                    fprintf(stderr, "Default session '%s' not found\n", default_session);
                    return 1;
                }
            } else {
                fprintf(stderr, "No hostname or default session specified for eval mode\n");
                return 1;
            }
        }
        return execute_command(hostname, port, command);
    }

    if (eval_file_mode && command_file) {
        if (!hostname) {
            char default_session[256];
            if (get_default_session_name(default_session, sizeof(default_session)) == 0) {
                struct SessionConfig config;
                if (load_session_config(&config, default_session) == 0) {
                    hostname = config.hostname;
                    port = config.port;
                } else {
                    fprintf(stderr, "Default session '%s' not found\n", default_session);
                    return 1;
                }
            } else {
                fprintf(stderr, "No hostname or default session specified for eval-file mode\n");
                return 1;
            }
        }
        return execute_command_from_file(hostname, port, command_file);
    }

    // If hostname is NULL at this point, we have an error
    if (!hostname) {
        fprintf(stderr, "No hostname, session, or default session specified\n");
        return 1;
    }

    // Connect to server and enter interactive mode
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