#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

const char *server_ip = "127.0.0.1";
const int server_port = 5555;
int process_pop3(int infd, int outfd);
void handle_signal(int sig);

int my_printf(const char *message){
	printf("[P-%d][C-%d]: %s\n", getppid(), getpid(), message);
	return 0;
}

/* Neue Client Verbindung */
int handle_connection(int client_socket) {
	my_printf("Client connected");
	process_pop3(client_socket, client_socket);
	close(client_socket);
	return 0;
}

/* Server wartet auf Verbindungen */
int listen_for_connections(int server_socket){
	socklen_t client_len;
	int client_socket;
	struct sockaddr_in client_address;
	int pid;
	if (listen(server_socket, 5) < 0) {
		perror("Listen");
	}
	
	signal(SIGINT, handle_signal);
	my_printf("Listening for incoming connections...");
	while(1){
		client_len = sizeof(struct sockaddr);
		client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_len);
		if(client_socket < 0){
			perror("error accept");
		} else {
			pid = fork();
			if (pid == -1) {
				perror("error on fork");
			}
			/* Sohn Prozess (Neuer Client) */ 
			if (pid == 0){
				close(server_socket);
				handle_connection(client_socket);
				exit(0);
			/* Vater Prozess (Server) */
			} else {
				close(client_socket);
			}
		}		
	}
	return 0;
}

int start_server(){
	/*struct in_addr *ip_address = malloc(sizeof(struct in_addr));*/
	struct in_addr ip_address;
	struct sockaddr_in socket_address;
	int server_socket;
	int enable = 1; /* für reuse address */
	
	if(inet_aton(server_ip, &ip_address) < 0) {
		perror("Error IP");
	} else {
		my_printf("Server IP: OK");
	}
	
	socket_address.sin_family = AF_INET;
	socket_address.sin_port = htons(server_port);
	socket_address.sin_addr = ip_address;
	
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket");
	}
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		perror("setsockopt(SO_REUSEADDR) failed");
    }
	if (bind(server_socket, (struct sockaddr*)&socket_address, 32) < 0) {
		perror("Bind");
	} else {
		my_printf("Server started...");
	}
	
	listen_for_connections(server_socket);
	
	return 0;
}

int main(int argc, char *argv[]) {
	
	start_server();
	
	
	/*process_pop3(1,0);*/
	return 0;
}
