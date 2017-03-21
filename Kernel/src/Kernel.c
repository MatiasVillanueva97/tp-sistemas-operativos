/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "sockets.h"


#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

int main(void)
{
	int socket, new_fd,numbytes;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	char buf[100];

	socket=crearSocketYBindeo(PORT);

	escuchar(socket);

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(socket, (struct sockaddr *)&their_addr, &sin_size);

		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		 inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s); // para poder imprimir la ip del server
		 printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(socket); // child doesn't need the listener
			if (send(new_fd, "Hello, paton!", 13, 0) == -1){
				perror("send");
			}
			if ((numbytes = recv(new_fd, buf, 100-1, 0)) == -1) {
				    perror("recv");
				    exit(1);
			}
			buf[numbytes]= '\0';
			printf("Kernel: received '%s'\n",buf);
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}
