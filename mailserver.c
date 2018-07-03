#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define PMAX 100

const char *server_ip = "127.0.0.1";
const int pop3_port = 5555;
const int smtp_port = 5556;
int pcurrent = 0;

int process_pop3(int infd, int outfd);
void *process_smtp(void *args);
void handle_signal(int sig);

int my_printf(const char *message){
	printf("[P-%d][C-%d]: %s\n", getppid(), getpid(), message);
	return 0;
}

/* Neue Client Verbindung */
int handle_connection(int client_socket) {
	my_printf("Client connected on POP3");
	process_pop3(client_socket, client_socket);
	close(client_socket);
	return 0;
}

/* Neue SMTP Connection */
int handle_smtp_connection(int client_socket) {
	my_printf("Client connected on SMTP");
	pthread_t tid[PMAX];
	pthread_create(&tid[pcurrent], NULL, process_smtp, &client_socket);
	pcurrent++;
	/*pthread_detach(tid);*/
	return 0;
}

/* Server wartet auf Verbindungen */
int listen_for_connections(int pop3_socket, int smtp_socket){
		
	/* Client */
	socklen_t client_len;
	int client_socket = 0;
	struct sockaddr_in client_address;
	int pid = 0;
	
	/* Select */
	fd_set readfds;
	fd_set writefds;
	
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
		
	FD_SET(pop3_socket, &readfds);
	FD_SET(smtp_socket, &readfds);
	/*FD_SET(client_socket, &writefds);*/

	if (listen(pop3_socket, 5) < 0) {
		perror("Listen POP3");
	}
	
	if (listen(smtp_socket, 5) < 0) {
		perror("Listen SMTP");
	}
	
	signal(SIGINT, handle_signal);
	my_printf("Listening for incoming connections...");
	
	while(1){
		
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		
		FD_SET(pop3_socket, &readfds);
		FD_SET(smtp_socket, &readfds);
		

		
		if(select(5, &readfds, &writefds, NULL, NULL) < 0) {
			perror("Error select"); exit(1);
		}
		
		/* POP3 */
		if (FD_ISSET(pop3_socket, &readfds)) {
			my_printf("Connection on POP3-Server");
			client_len = sizeof(struct sockaddr);
			client_socket = accept(pop3_socket, (struct sockaddr*)&client_address, &client_len);
			
			if(client_socket < 0){
				perror("error accept"); exit(1);
			}
			 
			if ((pid = fork()) == -1) {
				perror("error on fork");
			}
		
			/* Sohn Prozess (Neuer Client) */ 
			if (pid == 0){
				close(pop3_socket);
				handle_connection(client_socket);
				exit(0);
			/* Vater Prozess (Server) */
			} else {
				close(client_socket);
			}

		}
		
		/* SMTP */
		if (FD_ISSET(smtp_socket, &readfds)) {
			my_printf("Connection on SMTP-Server");
			client_len = sizeof(struct sockaddr);
			client_socket = accept(smtp_socket, (struct sockaddr*)&client_address, &client_len);		
			if(client_socket < 0){
				perror("error accept"); exit(1);
			}
			handle_smtp_connection(client_socket);
		}			
	}
	return 0;
}

int setup_socket(const int port) {
	
	struct sockaddr_in socket_address;
	struct in_addr ip_address;
	
	int server_socket = 0;
	int enable = 1; /* für reuse address */
	
	/* Setup IP Address for server */
	if(inet_aton(server_ip, &ip_address) < 0) {
		perror("Error IP");
	}
	
	socket_address.sin_family = AF_INET;
	socket_address.sin_port = htons(port);
	socket_address.sin_addr = ip_address;
	
	/* Setup Socket */
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket");
	}
	
	/* Enable Reuse */
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		perror("setsockopt(SO_REUSEADDR) failed");
    }
    
    /* Bind */
    if (bind(server_socket, (struct sockaddr*)&socket_address, 32) < 0) {
		perror("Bind");
	}
	return server_socket;
}

int start_server(){
	int pop3_socket = 0, smtp_socket = 0;

	pop3_socket = setup_socket(pop3_port);
	my_printf("Server started for POP3 ... ");
	smtp_socket = setup_socket(smtp_port);
	my_printf("Server started for SMTP ... ");
	
	listen_for_connections(pop3_socket, smtp_socket);
	
	return 0;
}

int main(int argc, char *argv[]) {
	
	start_server();
	
	
	/*process_pop3(1,0);*/
	return 0;
}
