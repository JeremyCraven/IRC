#include "Client.h"

int open_client_socket(char * host, int port) {
    struct sockaddr_in socketAddress;
    memset((char *)&socketAddress, 0, sizeof(socketAddress));
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_port = htons((u_short)port);
    
    struct hostent * ptrh = gethostbyname(host);

    if (ptrh == NULL) {
	return -1;
	perror("gethostbyname");
	exit(1);
    }

    memcpy(&socketAddress.sin_addr, ptrh->h_addr, ptrh->h_length);

    struct protoent * ptrp = getprotobyname("tcp");

    if (ptrp == NULL) {
	perror("getprotobyname");
	exit(1);
    }

    int sock = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);

    if (sock < 0) {
	perror("socket");
	exit(1);
    }

    if (connect(sock, (struct sockaddr *)&socketAddress, sizeof(socketAddress)) < 0) {
	return -1;
	perror("connect");
	exit(1);
    }

    return sock;

}

int sendCommand(char * host, int port, char * command, char * response) {
    int sock = open_client_socket(host, port);

    if (sock < 0) {
	return 0;
    }

    write(sock, command, strlen(command));
    write(sock, "\r\n", 2);
    
    int n = 0;
    int len = 0;

    while ((n = read(sock, response + len, MAX_RESPONSE - len)) > 0) {
	len += n;
    }

    response[len] = 0;

    close(sock);

    return 1;
}
