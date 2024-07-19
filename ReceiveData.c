#include "supportLibFunc.h"

int main(int argc, char *argv[]) {
	if ((argc < 3) || (strcmp(argv[1], "-out") != 0)) {// sai cu phap
		fprintf(stderr,"Su dung cu phap: ReceiveData -out <thuMuc_output>\n");
		system("pause");
		exit(1);
	}
    
	const char *storing_dir = argv[2];
	int sockfd, newsockfd;
	struct sockaddr_in addr;

	#ifdef _WIN32
		WSADATA wsaData;
		if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
			perror("WSAStartup failed");
			system("pause");
			exit(1);
		}
	#endif

	//Tao socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd < 0) {
		perror("Loi tao socket");
		system("pause");
		exit(1);
	}

	// setup the host_addr structure for use in bind call
	addr.sin_family = AF_INET; // su dung dang dia chi IPv4
	addr.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY = inet_addr("<IP_addr cua may chu>")
	addr.sin_port = htons(PORT); // chuyen short integer sang dang network byte order

	// bind(int fd, struct sockaddr *local_addr, socklen_t addr_length)
	// bind() passes file descriptor, the address structure, and the length of the address structure
	// This bind() call will bind  the socket to the current IP address on port, portno
	if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0){
		perror("Loi ket noi socket voi address");
		system("pause");
		#ifdef _WIN32
			closesocket(sockfd);
			WSACleanup();
		#else
			close(sockfd);
		#endif
		exit(1);
	}

	// This listen() call tells the socket to listen to the incoming connections.
	// The listen() function places all incoming connection into a backlog queue until accept() call accepts the connection.
	// Here, we set the maximum size for the backlog queue to 5.
	if ((listen(sockfd, 5)) != 0) {
		printf("Loi socket server listening\n");
		system("pause");
		#ifdef _WIN32
			closesocket(sockfd);
			WSACleanup();
		#else
			close(sockfd);
		#endif
		exit(1);
	}
	else
		printf("Server listening\n");
	
	while(1){
		//system("cls");
	// This accept() function will write the connecting client's address info 
	// into the the address structure and the size of that structure is clilen.
	// The accept() returns a new socket file descriptor for the accepted connection.
	// So, the original socket file descriptor can continue to be used 
	// for accepting new connections while the new socker file descriptor is used for
	// communicating with the connected client.
		struct sockaddr_in cliAddr;
		socklen_t addrlen = sizeof(cliAddr);
		if ((newsockfd = accept(sockfd, (struct sockaddr*)&cliAddr, &addrlen)) < 0) {
			perror("Loi nhan ket noi tu client");
			system("pause");
			continue;
		}
		
		printf("Server: nhan duoc ket noi tu %s o port %d\n", inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));
		
		//De nhan chuoi TEXT/FILE tu client neu client muon gui file
		char check[5] = {0};
		int len_input = recv(newsockfd, check, 4, 0);
		//ham recv lay thong tin tu newsockfd (client),
		//vao buffer <size - 1> byte ky tu
		if (len_input < 0) {
			perror("Loi nhan du lieu tu socket client");
			#ifdef _WIN32
				closesocket(newsockfd);
			#else
				close(newsockfd);
			#endif
			continue;
		}

		check[len_input] = '\0';
		if (strcmp(check, "FILE") == 0) {
			char buffer[BUFFER_SIZE] = {0};
			//nhan ten file
			len_input = recv(newsockfd, buffer, BUFFER_SIZE - 1,0);
			if (len_input <= 0) {
				perror("Khong nhan duoc ten file");
			}
			else {
				buffer[len_input] = '\0';
				char filePath[256];
				snprintf(filePath, sizeof(filePath), "%s%s", storing_dir, buffer);
				
				FILE *outfile = fopen(filePath, "wb");
				if (outfile == NULL)
					perror("Loi mo file");
				else {
					while((len_input = recv(newsockfd, buffer, BUFFER_SIZE, 0)) > 0) {
						fwrite(buffer, 1, len_input, outfile);
					}
					fclose(outfile);
					printf("File received and saved to %s\n", filePath);
				}
			}
		} else {
			char message[BUFFER_SIZE] = {0};
			len_input = recv(newsockfd, message, BUFFER_SIZE - 1, 0);
			if (len_input > 0) {
				message[len_input] = '\0';
				printf("Tin nhan tu client: %s\n", message);
			}
		}

		#ifdef _WIN32
			closesocket(newsockfd);
		#else
			close(newsockfd);
		#endif
	}

	#ifdef _WIN32
		closesocket(sockfd);
		WSACleanup();
	#else
		close(sockfd);
	#endif
	
	return 0; 
}