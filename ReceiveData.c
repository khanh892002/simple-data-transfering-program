#include "supportLibFunc.h"
#include <sys/types.h>
#include <sys/stat.h>

int directoryExists(const char* path) {
	#ifdef _WIN32
		struct _stat info;
		if (_stat(path, &info) != 0) return 0;
		return (info.st_mode & S_IFDIR) != 0;
	#else
		struct stat info;
		if (stat(path, &info) != 0) return 0;
		return (info.st_mode & S_IFDIR) != 0;
	#endif
}

int main(int argc, char *argv[]) {
	if ((argc < 3) || (strcmp(argv[1], "-out") != 0)) {// sai cu phap
		fprintf(stderr,"Su dung cu phap: ReceiveData -out <thuMuc_output>\n");
		system("pause");
		exit(1);
	}
    
	const char *storing_dir = argv[2];

	if(!directoryExists(storing_dir)) {
		fprintf(stderr, "Thu muc %s khong ton tai\n", storing_dir);
		exit(1);
	}

	int sockfd, newsockfd;
	struct sockaddr_in addr;
	// doi tuong sockaddr_in dung de chua thong tin dia chi, bao gom loai dia chi, dia chi (IP), port
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

	//cau hinh dia chi truoc khi gan vao socket cua server
	addr.sin_family = AF_INET; // su dung dang dia chi IPv4
	addr.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY = inet_addr("<IP_addr cua may chu>")
	addr.sin_port = htons(PORT); // chuyen short integer sang dang network byte order

	// bind(int fd, struct sockaddr *local_addr, socklen_t addr_length)
	// ham se gan dia chi duoc chi dinh vao socket
	if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0){
		perror("Loi gan dia chi vao socket");
		#ifdef _WIN32
			closesocket(sockfd);
			WSACleanup();
		#else
			close(sockfd);
		#endif
		exit(1);
	}

	// ham listen thiet lap cho socket listen cac ket noi den, dua vao mot hang doi cho toi khi duoc nhan boi ham accept()
	// ta thiet lap hang doi chi chua toi da 5 ket noi
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
	
	struct sockaddr_in cliAddr;
	socklen_t addrlen = sizeof(cliAddr);	
	//ham accept se luu lai thong tin dia chi cua client
	//tra ve mot socket dung de xu ly ket noi vua nhan, de co the tiep tuc su dung socket ban dau (sockfd)
	//va cung de co the nhan them cac ket noi khac trong khi socket moi (newsockfd) co the duoc dung de giao tiep voi client
	if((newsockfd = accept(sockfd, (struct sockaddr*) &cliAddr, &addrlen)) < 0) {
		perror("Ket noi that bai");
		exit(1);
	}
	printf("Server: nhan duoc ket noi tu %s o port %d\n", inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));

	while(1){
		char check[5] = {0};//De nhan chuoi TEXT/FILE tu client neu client muon gui file
		size_t len_input = recv(newsockfd, check, 4, 0);
		//ham recv lay thong tin tu newsockfd (client),
		//vao buffer <size - 1> byte ky tu
		if (len_input > 0) {
			check[len_input] = '\0';
			if (strcmp(check, "FILE") == 0) {
				char buffer[BUFFER_SIZE] = {0};
				//nhan ten file
				len_input = recv(newsockfd, buffer, BUFFER_SIZE - 1,0);
				if (len_input <= 0)
					perror("Khong nhan duoc ten file");
				else {
					buffer[len_input] = '\0';
					
					char filePath[256];
					snprintf(filePath, sizeof(filePath), "%s/%s", storing_dir, buffer);

					FILE *outfile = fopen(filePath, "wb");
					if (outfile == NULL) {
						perror("Loi mo file");
						send(newsockfd, "FAIL", 4, 0);
					}
					else {
						send(newsockfd, "SCSS", 4, 0);

						size_t fileSize;
						len_input = recv(newsockfd, (char*)&fileSize, sizeof(fileSize), 0);
						if (len_input <= 0)
							perror("Nhan kich thuoc file that bai");
						else {
							size_t total_bytes_received = 0;
							while(total_bytes_received < fileSize && (len_input = recv(newsockfd, buffer, BUFFER_SIZE, 0)) > 0) {
								fwrite(buffer, 1, len_input, outfile);
								total_bytes_received += len_input;
							}
							if(total_bytes_received == fileSize)
								printf("Da nhan file tu %s port %d va luu vao %s\n", inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port), filePath);
							else printf("Chua nhan het file %s tu %s, %d", buffer, inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));
						}
						fclose(outfile);
					}
				}
			} else if (strcmp(check, "TEXT") == 0) {
				char message[BUFFER_SIZE] = {0};
				len_input = recv(newsockfd, message, BUFFER_SIZE - 1, 0);
				if (len_input > 0) {
					message[len_input] = '\0';
					printf("Tin nhan tu %s, port %d: %s\n", inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port), message);
				}
			} else if (strcmp(check, "EXIT") == 0) {
				printf("Nhan duoc thong bao ngat ket noi tu %s, %d\n", inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));
				break;
			}
			else printf("Lenh khong hop le tu %s, %d\n", inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));
		} else {
			perror("Ket noi tu client bi loi hoac da dong ket noi\n");
			break;
		}
	}

	#ifdef _WIN32
		closesocket(newsockfd);
		closesocket(sockfd);
		WSACleanup();
	#else
		close(newsockfd);
		close(sockfd);
	#endif
	
	printf("Dong server ReceiveData\n");
	return 0; 
}