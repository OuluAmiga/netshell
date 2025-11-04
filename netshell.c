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

#ifdef MORPHOS
#include <sys/time.h>
#include <sys/ioctl.h>
// Define missing constants for MorphOS
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif
#endif
#include <sys/time.h>
#include <sys/stat.h>

#define DEFAULT_PORT 2324
#define BACKLOG 10
#define BUFFER_SIZE 1024

volatile sig_atomic_t server_running = 1;

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    server_running = 0;
    // Reinstall the signal handler (needed for some systems)
    signal(sig, signal_handler);
}

// Define socklen_t if not available on MorphOS
// Define socklen_t if not available on MorphOS
#ifdef MORPHOS
#ifndef socklen_t
typedef unsigned int socklen_t;
#endif
#endif

// Function to handle each client connection
void handle_client(int client_fd) {
    pid_t pid;
    int status;
    
    // Simply redirect client I/O to shell I/O using dup2
    // This is a simpler approach that avoids complex pipe handling
    
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
    
    printf("NetShell server listening on port %d...\n", port);
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
            handle_client(client_fd);
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