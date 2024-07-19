#include "supportLibFunc.h"

int main(int argc, char *argv[]) {
	if (argc < 3) {//sai cu phap
	   fprintf(stderr,"Su dung cu phap: SendData <destination address> [SendText <text> | SendFile <path> <buffer_size>]\n");
	   exit(1);
	}

	const char *destination_ip = argv[1];

	if (strcmp(argv[2], "SendText") == 0) {
		const char *mess = argv[3];
		struct sockaddr_in serv_addr;
		char buffer[BUFFER_SIZE];
		bzero(buffer,BUFFER_SIZE);
		
		#ifdef _WIN32
			WSADATA wsaData;
			if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
				perror("WSAStartup failed");
				exit(1);
			}
		#endif

		int sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock < 0)
			perror("Loi tao socket");
		else {
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_port = htons(PORT);

			if(inet_pton(AF_INET, destination_ip, &serv_addr.sin_addr) <= 0)
				perror("Dia chi IP khong hop le");
			else {
				if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
					perror("Ket noi that bai");
				else
					send(sock, mess, strlen(mess), 0);
			}
			#ifdef _WIN32
				closesocket(sock);
				WSACleanup();
			#else
				close(sock);
			#endif
		}
	} else if (strcmp(argv[2], "SendFile") == 0) {
		const char *filePath = argv[3];
		size_t buffer_size = strtoul(argv[4], NULL, 0);
		struct sockaddr_in serv_addr;
		char *buffer = malloc(buffer_size);
		if(buffer == NULL)
			perror("Loi bo nho");
		else {
			#ifdef _WIN32
				WSADATA wsaData;
				if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
					perror("WSAStartup failed");
					free(buffer);
					exit(1);
				}
			#endif
			int sock = socket(AF_INET, SOCK_STREAM, 0);
			if(sock < 0)
				perror("Loi tao socket");
			else {
				serv_addr.sin_family = AF_INET;
				serv_addr.sin_port = htons(PORT);
				if(inet_pton(AF_INET, destination_ip, &serv_addr.sin_addr) <= 0)
					perror("Dia chi IP khong hop le");
				else {
					if (connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
						perror("Ket noi that bai");
					else {
						FILE *file = fopen(filePath, "rb");
						if (file == NULL)
							perror("Mo file that bai");
						else {
							send(sock, "FILE", 4, 0);
							size_t bytes_read;
							while ((bytes_read = fread(buffer, 1, buffer_size, file)) > 0) {
								send(sock, buffer, bytes_read, 0);
							}
							fclose(file);
						}
					}
				}
				#ifdef _WIN32
					closesocket(sock);
					WSACleanup();
				#else
					close(sock);
				#endif
			}
			free(buffer);
		}
	} else {
		fprintf(stderr, "Lenh khong hop le");
		exit(1);
	}

	return 0;
}