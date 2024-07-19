#include "supportLibFunc.h"

int main(int argc, char *argv[]) {
	if (argc < 2) {// sai cu phap
		fprintf(stderr,"Su dung cu phap: ReceiveData -out <thuMuc_output>\n");
		exit(1);
	}
    
	const char *output_dir = argv[2];

	int sockfd, newsockfd;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	char buffer[BUFFER_SIZE];
	bzero(&buffer, BUFFER_SIZE);
	
	#ifdef _WIN32
		WSADATA wsaData;
		if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
			perror("WSAStartup failed");
			exit(1);
		}
	#endif

	//Tao socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd < 0) {
		perror("Loi tao socket");
		exit(1);
	}

	// clear address structure
	bzero((char *) &addr, sizeof(addr));

	// setup the host_addr structure for use in bind call
	addr.sin_family = AF_INET; // su dung dang dia chi IPv4
	addr.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY = inet_addr("<IP_addr cua may chu>")
	addr.sin_port = htons(PORT); // chuyen short integer sang dang network byte order

	// bind(int fd, struct sockaddr *local_addr, socklen_t addr_length)
	// bind() passes file descriptor, the address structure, and the length of the address structure
	// This bind() call will bind  the socket to the current IP address on port, portno
	if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0){
		perror("Loi ket noi socket voi address");
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
		newsockfd = accept(sockfd, (struct sockaddr*)&addr, &addrlen);//Khong anh huong den addr da bind voi sockfd truoc do
		if (newsockfd < 0){
			perror("Loi nhan ket noi tu client");
			#ifdef _WIN32
				closesocket(sockfd);
				WSACleanup();
			#else
				close(sockfd);
			#endif
			exit(1);
		}
		
		//printf("Server: nhan duoc ket noi tu %s o port %d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
		
		int len_input = recv(newsockfd, buffer, BUFFER_SIZE, 0);
		if (len_input < 0){
			perror("Loi nhan du lieu tu socket client");
			#ifdef _WIN32
				closesocket(newsockfd);
				WSACleanup();
			#else
				close(newsockfd);
			#endif
			continue;
		}

		if(strncmp(buffer, "FILE", 4) == 0) {
			char filename[256];
			snprintf(filename, sizeof(filename), "%s/received_file", output_dir);
			FILE *outfile = fopen(filename, "wb");
			if (outfile == NULL){
				perror("Loi mo file");
				#ifdef _WIN32
					closesocket(newsockfd);
				#else
					close(newsockfd);
				#endif
				continue;
			}
			
			while((len_input = read(newsockfd, buffer, BUFFER_SIZE)) > 0) {
				fwrite(buffer, 1, len_input, outfile);
			}
			fclose(outfile);
			printf("File received and saved to %s\n", filename);
		} else {
			printf("Client message: %s\n", buffer);
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