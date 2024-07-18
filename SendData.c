#include "supportLibFunc.h"
#include <netdb.h>

int main(int argc, char *argv[]) {
	if (argc < 3) {
	   fprintf(stderr,"Su dung cu phap: SendData <destination address> [SendText <text> | SendFile <path> <buffer_size>]\n");
	   exit(1);
	}

	const char *destination_ip = argv[1];

	if (strcmp(argv[2], "SendText") == 0) {
		const char *mess = argv[3];
		struct sockaddr_in serv_addr;
		char buffer[BUFFER_SIZE];
		bzero(buffer,BUFFER_SIZE);
		
		int sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock < 0)
			perror("Loi tao socket");
		else {
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_port = htons(PORT);

			if(inet_pton(AF_INET, destination_ip, &serv_addr.sin_addr) <= 0) {
				perror("Dia chi IP khong hop le");
				close(sock);
			}
			else {
				if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
					perror("Ket noi that bai");
					close(sock);
				}
				else {
					send(sock, mess, strlen(text), 0);
					close(sock);
				}
			}
		}
	} else if (strcmp(argv[2], "SendFile") == 0) {
		const char *filePath = argv[3];
		size_t buffer_size = strtoul(argv[4], NULL, 0);
		struct sockaddr_in serv_addr;
		char *buffer = malloc(buffer_size);
		if(buffer == NULL) {
			perror("Loi bo nho");
		}
		else {
			int sock = socket(AF_INET, SOCK_STREAM, 0);
			if(sock < 0) {
				perror("Loi tao socket");
				free(buffer);
			}
			else {
				serv_addr.sin_family = AF_INET;
				serv_addr.sin_port = htons(PORT);
				if(inet_pton(AF_INET, destination_ip, &serv_addr.sin_addr) <= 0) {
					perror("Dia chi IP khong hop le");
					close(sock);
					free(buffer);
				}
				else {
					if (connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
						perror("Ket noi that bai");
					}
					else {
						FILE *file = fopen(filePath, "rb");
						if (file == NULL) {
							perror("Mo file that bai");
						}
						else {
							send(sock, "FILE", 4, 0);
							size_t bytes_read;
							while ((bytes_read = fread(buffer, 1, buffer_size, file)) > 0) {
								send(sock, buffer, bytes_read, 0);
							}

							fclose(file);
						}
					}
					close(sock);
					free(buffer);
				}
			}
		}
	} else {
		fprintf(stderr, "");
		exit(1);
	}

	int sockfd, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;// là một con trỏ đối tượng chứa thông tin hostname

	char buffer[256];
	
    
	server = gethostbyname(argv[1]); //hàm trả về con trỏ hostent chứa thông tin hostname
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}
	
    bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(PORT);
	
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR connecting");
	printf("Please enter the message: ");

	bzero(buffer,256);
	fgets(buffer,255,stdin);
	n = write(sockfd, buffer, strlen(buffer));
	if (n < 0) error("ERROR writing to socket");

	bzero(buffer,256);
	n = read(sockfd, buffer, 255);
	if (n < 0) error("ERROR reading from socket");
	printf("%s\n", buffer);
	close(sockfd);
	return 0;
}