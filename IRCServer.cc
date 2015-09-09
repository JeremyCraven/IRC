
const char * usage =
"                                                               \n"
"IRCServer:                                                     \n"
"                                                               \n"
"Simple server program used to communicate multiple users       \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   IRCServer <port>                                            \n"
"                                                               \n"
"Where 1024 < port < 65536.                                     \n"
"                                                               \n"
"In another window type:                                        \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where talk-server      \n"
"is running. <port> is the port number you used when you run    \n"
"daytime-server.                                                \n"
"                                                               \n";

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "HashTableVoid.h"
#include "IRCServer.h"
#include "LinkedList.h"

int QueueLength = 5;

HashTableVoid hashTable;
HashTableVoid hashRooms;
HashTableVoid hashMessages;
HashTableVoid hashCount;

int IRCServer::open_server_socket(int port) {

	// Set the IP address and port for this server
	struct sockaddr_in serverIPAddress; 
	memset(&serverIPAddress, 0, sizeof(serverIPAddress));
	serverIPAddress.sin_family = AF_INET;
	serverIPAddress.sin_addr.s_addr = INADDR_ANY;
	serverIPAddress.sin_port = htons((u_short) port);
  
	// Allocate a socket
	int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
	if (masterSocket < 0) {
		perror("socket");
		exit( -1 );
	}

	// Set socket options to reuse port. Otherwise we will
	// have to wait about 2 minutes before reusing the sae port number
	int optval = 1; 
	int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
			     (char *) &optval, sizeof( int ));
	
	// Bind the socket to the IP address and port
	int error = bind(masterSocket,
			  (struct sockaddr *)&serverIPAddress,
			  sizeof(serverIPAddress));
	if (error) {
		perror("bind");
		exit( -1 );
	}
	
	// Put socket in listening mode and set the 
	// size of the queue of unprocessed connections
	error = listen(masterSocket, QueueLength);
	if (error) {
		perror("listen");
		exit( -1 );
	}

	return masterSocket;
}

void IRCServer::runServer(int port) {
	int masterSocket = open_server_socket(port);

	initialize();
	
	while ( 1 ) {
		
		// Accept incoming connections
		struct sockaddr_in clientIPAddress;
		int alen = sizeof(clientIPAddress );
		int slaveSocket = accept(masterSocket,
					  (struct sockaddr *)&clientIPAddress,
					  (socklen_t*)&alen);
		
		if (slaveSocket < 0) {
			perror("accept");
			exit( -1 );
		}
		
		// Process request.
		processRequest(slaveSocket);		
	}
}

int main(int argc, char ** argv) {
	// Print usage if not enough arguments
	if ( argc < 2 ) {
		fprintf( stderr, "%s", usage );
		exit( -1 );
	}
	
	// Get the port from the arguments
	int port = atoi( argv[1] );

	IRCServer ircServer;

	// It will never return
	ircServer.runServer(port);
	
}

//
// Commands:
//   Commands are started by the client.
//
//   Request: ADD-USER <USER> <PASSWD>\r\n
//   Answer: OK\r\n or DENIED\r\n
//
//   REQUEST: GET-ALL-USERS <USER> <PASSWD>\r\n
//   Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//
//   REQUEST: CREATE-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LIST-ROOMS <USER> <PASSWD>\r\n
//   Answer: room1\r\n
//           room2\r\n
//           ...
//           \r\n
//
//   Request: ENTER-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LEAVE-ROOM <USER> <PASSWD>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: SEND-MESSAGE <USER> <PASSWD> <MESSAGE> <ROOM>\n
//   Answer: OK\n or DENIED\n
//
//   Request: GET-MESSAGES <USER> <PASSWD> <LAST-MESSAGE-NUM> <ROOM>\r\n
//   Answer: MSGNUM1 USER1 MESSAGE1\r\n
//           MSGNUM2 USER2 MESSAGE2\r\n
//           MSGNUM3 USER2 MESSAGE2\r\n
//           ...\r\n
//           \r\n
//
//    REQUEST: GET-USERS-IN-ROOM <USER> <PASSWD> <ROOM>\r\n
//    Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//

void IRCServer::processRequest(int fd) {
	// Buffer used to store the comand received from the client
	const int MaxCommandLine = 1024;
	char commandLine[ MaxCommandLine + 1 ];
	int commandLineLength = 0;
	int n;
	
	// Currently character read
	unsigned char prevChar = 0;
	unsigned char newChar = 0;
	
	//
	// The client should send COMMAND-LINE\n
	// Read the name of the client character by character until a
	// \n is found.
	//

	// Read character by character until a \n is found or the command string is full.
	while (commandLineLength < MaxCommandLine && read( fd, &newChar, 1) > 0) {
		if (newChar == '\n' && prevChar == '\r') {
			break;
		}
		
		commandLine[commandLineLength] = newChar;
		commandLineLength++;

		prevChar = newChar;
	}
	
	// Add null character at the end of the string
	// Eliminate last \r
	commandLineLength--;
        commandLine[commandLineLength] = 0;

	printf("RECEIVED: %s\n", commandLine);

	const char * delim = " ";
	char * token = strtok(commandLine, delim);

	const char * command = token;
	token = strtok(NULL, delim);

	const char * user = token;
	token  = strtok(NULL, delim);

	const char * password = token;
	token = strtok(NULL, "\r");

	const char * args = token;

	printf("command=%s\n", command);
	printf("user=%s\n", user);
	printf("password=%s\n", password );
	printf("args=%s\n", args);

	if (!strcmp(command, "ADD-USER")) {
		addUser(fd, user, password, args);
	}
	else if (!strcmp(command, "ENTER-ROOM")) {
		enterRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "LEAVE-ROOM")) {
		leaveRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "SEND-MESSAGE")) {
		sendMessage(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-MESSAGES")) {
		getMessages(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-USERS-IN-ROOM")) {
		getUsersInRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-ALL-USERS")) {
		getAllUsers(fd, user, password, args);
	}
	else if (!strcmp(command, "CREATE-ROOM")) {
		createRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "LIST-ROOMS")) {
		listRooms(fd, user, password, args);
	}
	else if (!strcmp(command, "CHECK-PASSWORD")) {
		checkPassword(fd, user, password);
	}
	else {
		const char * msg =  "UNKNOWN COMMAND\r\n";
		write(fd, msg, strlen(msg));
	}

	close(fd);	
}

void IRCServer::initialize() {
	// Open Password File

	passwords = fopen(PASSWORD_FILE, "a+");

	// Initialize users in room

	char user[100];
	char password[100];
	char c;
	int i = 0;
	int j = 0;
	int userpass = 0;

	while ((c = fgetc(passwords)) != EOF) {
	    if (c == 32 && (userpass % 2 == 0)) {
		userpass = 1;
	    }
	    else if (c == 32 && (userpass % 2 == 1)) {
		userpass = 0;
		user[i] = 0;
		password[j] = 0;
		hashTable.insertItem((const char *)user, (void *)strdup(password));  
		i = 0;
		j = 0;
	    }
 	    else if (userpass % 2 == 0 && c != 32) {
		user[i++] = c;	
	    }
	    else if (userpass % 2 == 1 && c != 32) {
		password[j++] = c;
	    }
	}
}

bool IRCServer::checkPassword(int fd, const char * user, const char * password) {
	if (user == NULL) {
	    write(fd, "NO USERNAME ENTERED\r\n", strlen("NO USERNAME ENTERED\r\n"));
	    return false;
	}
	if (password == NULL) {
	    write(fd, "NO PASSWORD ENTERED\r\n", strlen("NO PASSWORD ENTERED\r\n"));
	    return false;	    
	}
	
	void * valuev;

	if (hashTable.find(user, &valuev) == true) {
	    const char * value = (const char *)valuev;
	    
	    if (!strcmp(value, password)) {
		return true;
	    }
	    else {
		write(fd, "ERROR (Wrong password)\r\n", strlen("ERROR (Wrong password)\r\n"));
		return false;
	    }
	}
	else {
	    write(fd, "ERROR (Wrong password)\r\n", strlen("ERROR (Wrong password)\r\n"));
	}

	return false;
}

void IRCServer::addUser(int fd, const char * user, const char * password, const char * args) {
	if (user == NULL) {
	    write(fd, "NO USERNAME ENTERED\r\n", strlen("NO USERNAME ENTERED\r\n"));\
	    return;
	}
	if (password == NULL) {
	    write(fd, "NO PASSWORD ENTERED\r\n", strlen("NO PASSWORD ENTERED\r\n"));
	    return;
	}

	if (!hashTable.find(user, &placeHolder)) {
	    hashTable.insertItem(user, (void *)strdup(password));
	    fprintf(passwords, "%s %s ", user, password);
	    fflush(passwords);
	    write(fd, "OK\r\n", strlen("OK\r\n"));
	}
	else {
	    write(fd, "USER ALREADY EXISTS\r\n", strlen("USER ALREADY EXISTS\r\n"));
	}

	return;		
}

void IRCServer::enterRoom(int fd, const char * user, const char * password, const char * args) {
	if (args == NULL) {
	    write(fd, "ROOM DOES NOT EXIST\r\n", strlen("ROOM DOES NOT EXIST\r\n"));
	    return;
	}

	if (checkPassword(fd, user, password)) {
	    void * list;

	    if (hashRooms.find(args, &list)) {
		if (!llist_exists((LinkedList *)list, (char *)user)) {
		    llist_add((LinkedList *)list, (char *)user);
		}
		write(fd, "OK\r\n", strlen("OK\r\n"));		
	    }
	    else {
		write(fd, "ERROR (No room)\r\n", strlen("ERROR (No room)\r\n"));
	    } 	 	  	  
	}
}

void IRCServer::leaveRoom(int fd, const char * user, const char * password, const char * args) {
	if (args == NULL) {
	    write(fd, "NO ROOM ENTERED\r\n", strlen("NO ROOM ENTERED\r\n"));
	    return;
	}

	if (checkPassword(fd, user, password)) {
	    void * list;

	    if (hashRooms.find(args, &list)) {
		if (llist_exists((LinkedList *)list, (char *)user)) {
		    llist_remove((LinkedList *)list, (char *)user);
		    write(fd, "OK\r\n", strlen("OK\r\n"));
		}
		else {
		    write(fd, "ERROR (No user in room)\r\n", strlen("ERROR (No user in room)\r\n"));
		}
	    }
	    else {
		write(fd, "ROOM DOES NOT EXIST\r\n", strlen("ROOM DOES NOT EXIST\r\n"));
	    }
	}
}

void IRCServer::sendMessage(int fd, const char * user, const char * password, const char * args) {
	if (checkPassword(fd, user, password)) {
	    char * token = strtok((char *)args, " ");
	    const char * room = token;
	    token = strtok(NULL, "\r\n");
	    const char * message = token;

	    void * listUsers;
	    hashRooms.find(args, &listUsers);

	    if (!llist_exists((LinkedList *)listUsers, (char *)user)) {
		write(fd, "ERROR (user not in room)\r\n", strlen("ERROR (user not in room)\r\n"));
		return;
	    }

	    if (room == NULL) {
		write(fd, "NO ROOM ENTERED\r\n", strlen("NO ROOM ENTERED\r\n"));
		return;
	    }
	    if (message== NULL) {
		write(fd, "NO MESSAGE ENTERED\r\n", strlen("NO MESSAGE ENTERED\r\n"));
		return;
	    }

	    void * list;

	    if (hashMessages.find(room, &list)) {
		void * count;

		hashCount.find(room, &count);
		(*((int *)count))++;

		char * str = (char *)malloc(10000 * sizeof(char));
		sprintf(str, "%d %s %s\r\n", (*((int *)count)-1), user, message); 
		llist_insert_last((LinkedList *)list, str);
		
		if (llist_number_elements((LinkedList *)list) == 101) {
		    llist_remove_first((LinkedList *)list);
		}
		write(fd, "OK\r\n", strlen("OK\r\n"));
		free(str);
	    }
	    else {
		write(fd, "ROOM DOES NOT EXIST\r\n", strlen("ROOM DOES NOT EXIST\r\n"));
	    } 	    
	}
}

void IRCServer::getMessages(int fd, const char * user, const char * password, const char * args) {
	if (checkPassword(fd, user, password)) {
	    char * token = strtok((char *)args, " ");
	    const char * number = token;
	    token = strtok(NULL, "\r");
	    const char * room = token;
	
	    if (args == NULL) {
		write(fd, "NO ROOM ENTERED\r\n", strlen("NO ROOM ENTERED\r\n"));
		return;
	    }

	    if (number == NULL) {
		write(fd, "NO MESSAGE NUMBER ENTERED\r\n", strlen("NO MESSAGE NUMBER ENTERED\r\n"));
		return;
	    }

	    if (room == NULL) {
		write(fd, "NO ROOM ENTERED\r\n", strlen("NO ROOM ENTERED\r\n"));
		return;
	    }

	    if (!hashMessages.find(room, &placeHolder)) {
		write(fd, "ROOM DOES NOT EXIST\r\n", strlen("ROOM DOES NOT EXIST\r\n"));
		return;
	    }
	    
	    void * list;

	    hashMessages.find(room, &list);
	    llist_printToUserMessages((LinkedList *)list, fd, atoi(number));
	}
}

void IRCServer::getUsersInRoom(int fd, const char * user, const char * password, const char * args) {
	if (args == NULL) {
	    write(fd, "NO ROOM ENTERED\r\n", strlen("NO ROOM ENTERED\r\n"));
	    return;
	}

	if (!hashRooms.find(args, &placeHolder)) {
	    write(fd, "ROOM DOES NOT EXIST\r\n", strlen("ROOM DOES NOT EXIST\r\n"));
	    return;
	} 

	if (checkPassword(fd, user, password)) {
	    void * list;

	    hashRooms.find(args, &list);

	    LinkedList temp = *(LinkedList *)list;

	    llist_sort(&temp, 1);
	    llist_printToUser((LinkedList *)list, fd);
	    write(fd, "\r\n", strlen("\r\n"));
	}
}

void IRCServer::createRoom(int fd, const char * user, const char * password, const char * args) {
	if (args == NULL) {
	    write(fd, "INVALID ROOM NAME\r\n", strlen("INVALID ROOM NAME\r\n"));
	    return;
	}

	if (checkPassword(fd, user, password)) {
	    if (!hashRooms.find(args, &placeHolder)) {
		LinkedList room;
		llist_init(&room);
		void * temp = malloc(sizeof(void *));
		memcpy(temp, (void *)&room, sizeof(void *));
		hashRooms.insertItem(args, temp);

		LinkedList msg;
		llist_init(&msg);
		void * temp2 = malloc(sizeof(void *));
		memcpy(temp2, (void *)&room, sizeof(void *));
		hashMessages.insertItem(args, temp2);

		int i = 0;
		void * temp3 = malloc(sizeof(void *));
		memcpy(temp3, (void *)&i, sizeof(void *));
		hashCount.insertItem(args, temp3); 

		write(fd, "OK\r\n", strlen("OK\r\n"));
	    }
	    else {
		write(fd, "ROOM ALREADY EXISTS\r\n", strlen("ROOM ALREADY EXISTS\r\n"));
	    }
	}
}

void IRCServer::listRooms(int fd, const char * user, const char * password, const char * args) {
	if (checkPassword(fd, user, password)) {
	    HashTableVoidIterator iterator(&hashRooms);
	
	    write(fd, "LIST OF ROOMS\r\n", strlen("LIST OF ROOMS\r\n"));

	    const char * room;
	    void * list;

	    while (iterator.next(room, list)) {
		write(fd, room, strlen(room));
		write(fd, "\r\n", strlen("\r\n"));
	    }	
	}
}

void IRCServer::getAllUsers(int fd, const char * user, const char * password, const  char * args) {
	if (checkPassword(fd, user, password)) {
	    HashTableVoidIterator iterator(&hashTable);

	    const char * username;
	    LinkedList temp;
	    llist_init(&temp);	

	    while (iterator.next(username, placeHolder)) {
		llist_add(&temp, (char *)username);
	    }

	    llist_sort(&temp, 1);
	    llist_printToUser(&temp, fd);
	    write(fd, "\r\n", strlen("\r\n"));	    
	}
}

