#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>
#define MAX_NAME_LENGTH 20
#define MAX_MESSAGE_LENGTH 2000
#define MAX_PORT_LENGTH 6 // Max length of port number (including '\0')
#define BACKLOG 5 // Maximum length of the queue of pending connections


char name[MAX_NAME_LENGTH];
int PORT;

void *receive_thread(void *arg);
void sending(int PORT_server);
void receiving(int server_fd);

int main(int argc, char const *argv[]) {
    printf("Enter name:");
    fgets(name, MAX_NAME_LENGTH, stdin); // Limit input to 19 characters to avoid buffer overflow
    name[strcspn(name, "\n")] = 0; // Remove newline character

    printf("Enter your port number:");
    scanf("%d", &PORT);

    int server_fd;
    struct sockaddr_in address;

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    pthread_t tid;
    if (pthread_create(&tid, NULL, &receive_thread, (void *)&server_fd) != 0) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
    }
    int PORT_server;
    printf("Enter the port to send message:");
    scanf("%d", &PORT_server);
    getchar(); 
    while (1) {
        sending(PORT_server);
    }
    close(server_fd);
    return 0;
}

void *receive_thread(void *arg) {
    int server_fd = *((int *)arg);
    receiving(server_fd);
    return NULL;
}

void sending(int PORT_server) {
    char buffer[MAX_MESSAGE_LENGTH] = {0};
    // int PORT_server;

    // printf("Enter the port to send message:");
    // scanf("%d", &PORT_server);
    // getchar(); // Clear the newline character from the input buffer

    int sock = 0;
    struct sockaddr_in serv_addr;
    char hello[1024] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(PORT_server);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        close(sock); // Close socket on connection failure
        return;
    }
    
    char dummy;
    scanf("%[^\n]%*c", hello);
    // printf("Message: %s\n", hello);
    sprintf(buffer, "%s[PORT:%d] says: %s", name, PORT, hello);
    send(sock, buffer, strlen(buffer), 0);
    // printf("Message sent\n");
    // fgets(buffer, MAX_MESSAGE_LENGTH, stdin); // Use fgets to handle spaces in input

    // // Remove leading whitespace and newline characters
    // char *pos = buffer;
    // while (*pos == ' ' || *pos == '\n')
    //     pos++;

    // // Format the message with sender's name
    // snprintf(buffer, sizeof(buffer), "%s[PORT:%d] says: %s", name, PORT, pos);

    // int sent_bytes = send(sock, buffer, strlen(buffer), 0);
    // if (sent_bytes < 0) {
    //     perror("Error sending message");
    // } else {
    //     printf("\nMessage sent\n");
    // }
    close(sock);
}


void receiving(int server_fd) {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    fd_set current_sockets, ready_sockets;

    FD_ZERO(&current_sockets);
    FD_SET(server_fd, &current_sockets);

    int k = 0;
    while (1)
    {
        k++;
        ready_sockets = current_sockets;

        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0) {
            perror("select() error");
            exit(EXIT_FAILURE);
        }

        for (int i =0; i < FD_SETSIZE; i++){
            if (FD_ISSET(i, &ready_sockets)){
                
                if (i == server_fd) {
                    int client_socket;
                    if ((client_socket = accept(server_fd, (struct sockaddr *)&address,
                                                (socklen_t *)&addrlen)) < 0) {
                        perror("accept");
                        continue; // Continue to next iteration to avoid closing an uninitialized socket
                    }

                    FD_SET(client_socket, &current_sockets);
                } else {
                    char buffer[MAX_MESSAGE_LENGTH] = {0};
                    int valread = recv(i, buffer, sizeof(buffer), 0);
                    if (valread <= 0) {
                        if (valread < 0) {
                            perror("Error receiving message");
                        }
                        close(i);
                        FD_CLR(i, &current_sockets);
                    } else {
                        printf("\n%s\n", buffer);
                    }
                }
            }

            if (k == (FD_SETSIZE*2)){
                break;
            }
        }
    }
    
    // while (1) {
    //     ready_sockets = current_sockets;

    //     if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0) {
    //         perror("Error in select");
    //         exit(EXIT_FAILURE);
    //     }

    //     for (int i = 0; i < FD_SETSIZE; i++) {
    //         if (FD_ISSET(i, &ready_sockets)) {
    //             if (i == server_fd) {
    //                 int client_socket;
    //                 if ((client_socket = accept(server_fd, (struct sockaddr *)&address,
    //                     (socklen_t *)&addrlen)) < 0) {
    //                     perror("accept");
    //                 continue; // Continue to next iteration to avoid closing an uninitialized socket
    //                     }
    //                     FD_SET(client_socket, &current_sockets);
    //             } else {
    //                 char buffer[MAX_MESSAGE_LENGTH] = {0};
    //                 int valread = recv(i, buffer, sizeof(buffer), 0);
    //                 if (valread <= 0) {
    //                     if (valread < 0) {
    //                         perror("Error receiving message");
    //                     }
    //                     close(i);
    //                     FD_CLR(i, &current_sockets);
    //                 } else {
    //                     printf("\n%s\n", buffer);
    //                 }
    //             }
    //         }
    //     }
    // }
}

