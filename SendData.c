#include "supportLibFunc.h"

int connect2Serv(const char* serv_ip) {
	struct sockaddr_in serv_addr;
	#ifdef _WIN32
		WSADATA wsaData;
		if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
			perror("WSAStartup failed");
			system("pause");
			return -1;
		}
	#endif

	//Tao socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Loi tao socket");
		system("pause");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	
	//Xy ly dia chi ip
	if (inet_pton(AF_INET, serv_ip, &serv_addr.sin_addr) <= 0) {
		perror("Dia chi ip server khong hop le");
		#ifdef _WIN32
			closesocket(sock);
			WSACleanup();
		#else
			close(sock);
		#endif
		system("pause");
		return -1;
	}

	//Ket noi den server
	if (connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Ket noi den server that bai");
		#ifdef _WIN32
			closesocket(sock);
			WSACleanup();
		#else
			close(sock);
		#endif
		system("pause");
		return -1;
	}

	return sock;
}

void sendFile(int sock, const char* filePath, size_t buffer_size) {
	char *buffer = malloc(buffer_size);
	if (buffer == NULL) {
		perror("Loi cap vung nho");
		return;
	}

	FILE *fin = fopen(filePath, "rb");
	if (fin == NULL) {
		fprintf(stderr, "Khong mo duoc file %s\n", filePath);
		free(buffer);
		return;
	}

	send(sock, "FILE", 4, 0);//gui vao bien check tren server
	
	//ham strrchr(const char* str, int c)
	//tim kiem su xuat hien cuoi cung cua ky tu c (unsigned char) trong str
	const char *fileName = strrchr(filePath,'/');
	if (fileName == NULL)
		fileName = strrchr(filePath, '\\');
	if (fileName == NULL)
		fileName = filePath;
	else fileName++; //Bo qua ky tu \ hoac /
	send(sock, fileName, strlen(fileName) + 1, 0); // gui ten file

	char check[5] = {0};
	size_t bytes_read = recv(sock, check, 4, 0);
	if (strcmp(check, "SCSS") == 0) {//server co the luu duoc file
		//gui kich thuoc file
		fseek(fin, 0, SEEK_END);
		bytes_read = ftell(fin);
		fseek(fin, 0, SEEK_SET);
		send(sock, (char*)&bytes_read, sizeof(bytes_read), 0);
		
		//gui noi dung file
		while((bytes_read = fread(buffer, 1, buffer_size, fin)) > 0) {
			send(sock, buffer, bytes_read, 0);
		}
		printf("Gui file %s thanh cong\n", fileName);
	}
	else fprintf(stderr, "Server khong tao duoc file\n");

	fclose(fin);
	free(buffer);
}

int main(int argc, char *argv[]) {
	if (argc < 2) {//sai cu phap
	   fprintf(stderr,"Su dung cu phap: SendData <destination address>\n");
	   system("pause");
	   exit(1);
	}

	const char *destination_ip = argv[1];
	int sock = connect2Serv(destination_ip);
	if (sock < 0) return 1;

	char command[BUFFER_SIZE];
	while(1) {
		printf("Nhap lenh theo cu phap (SendText <message> | SendFile <file_path> <buffer_size>): \n");
		fgets(command, BUFFER_SIZE, stdin);
		size_t len = strlen(command);
		if (len > 0 && command[len - 1] == '\n')
			command[len - 1] = '\0';
		
		char *cmd = strtok(command, " ");
		if(cmd == NULL) continue;

		if (strcmp(cmd, "SendText") == 0) {
			char *text = strtok(NULL, "");
			if (text != NULL) {
				send(sock, "TEXT", 4, 0);// gui vao bien check tren server
				send(sock, text, strlen(text), 0);
			}
			else
				fprintf(stderr, "Can nhap noi dung de truyen tin nhan\n");
		}
		else if (strcmp(cmd, "SendFile") == 0) {
			char *filePath = strtok(NULL, " ");
			char *buffer_size_str = strtok(NULL, " ");
			if (filePath != NULL && buffer_size_str != NULL) {
				size_t buffer_size = strtoul(buffer_size_str, NULL, 0);
				sendFile(sock, filePath, buffer_size);
			} else
				fprintf(stderr, "Can nhap thong tin duong dan va buffer size\n");
		}
		else if (strcmp(cmd, "exit") == 0) break;
		else fprintf(stderr, "Lenh khong hop le");
	}

	#ifdef _WIN32
		closesocket(sock);
		WSACleanup();
	#else
		close(sock);
	#endif

	return 0;
}