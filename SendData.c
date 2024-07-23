#include "supportLibFunc.h"

int connect2Serv(const char* serv_ip) {
	struct sockaddr_in serv_addr;
	#ifdef _WIN32
		WSADATA wsaData;
		if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
			perror("WSAStartup failed");
			return -1;
		}
	#endif

	//Tao socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Loi tao socket");
		return -1;
	}

	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&waitingTime, sizeof(waitingTime)) < 0)
		perror("Loi thiet lap thoi gian cho cua ket noi");
	if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&waitingTime, sizeof(waitingTime)) < 0)
		perror("Loi thiet lap thoi gian gui cua ket noi");

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
		return -1;
	}

	return sock;
}

void sendFile(int sock, const char* filePath, size_t buffer_size) {
	if (strlen(filePath) >= MAX_PATH_LENGTH) {
		fprintf(stderr, "Duong dan den file qua dai\n");
		return;
	}

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
	if (fileName == NULL) fileName = strrchr(filePath, '\\');
	if (fileName == NULL) fileName = filePath;
	else fileName++; //Bo qua ky tu \ hoac /

	if (strlen(fileName) >= BUFFER_SIZE)
		fprintf(stderr, "Ten file qua dai\n");
	else {
		send(sock, fileName, strlen(fileName) + 1, 0); // gui ten file

		char check[5] = {0};
		size_t bytes_read = recv(sock, check, 4, 0);
		if (strcmp(check, "SCSS") == 0) {//server co the luu duoc file
			//gui kich thuoc file
			fseek(fin, 0, SEEK_END);
			bytes_read = ftell(fin);
			fseek(fin, 0, SEEK_SET);
			//kieu du lieu size_t dam bao luu duoc kich thuoc file len den hang GB
			send(sock, (char*)&bytes_read, sizeof(bytes_read), 0);

			//gui noi dung file
			while((bytes_read = fread(buffer, 1, buffer_size, fin)) > 0) {
				send(sock, buffer, bytes_read, 0);
			}
			printf("Gui file %s thanh cong\n", fileName);
		}
		else fprintf(stderr, "Server khong tao duoc file\n");
	}
	fclose(fin);
	free(buffer);
}

int checkBufferSize(const char *buffer_str) {
	char *theRest;
	long value = strtol(buffer_str, &theRest, 10);
	//ham strtol tra ve ket qua la so nguyen long dau tien trong chuoi buffer_str
	//con tro theRest se tro den phan chuoi con lai phia sau so nguyen duoc lay ra
	//tham so cuoi cung la base can lay, 10 la thap phan
	return (*theRest == '\0' && value > 0);
	//neu buffer_str chi co so nguyen thi theRest tro den gia tri '\0'
} 

int main(int argc, char *argv[]) {
	if (argc < 2) {//sai cu phap
	   fprintf(stderr,"Su dung cu phap: SendData <destination address>\n");
	   exit(1);
	}

	const char *destination_ip = argv[1];
	int sock = connect2Serv(destination_ip);
	if (sock < 0) return 1;

	char command[BUFFER_SIZE];
	while(1) {
		printf("Nhap lenh theo cu phap (SendText <message> | SendFile <file_path> <buffer_size> | exit): \n");
		fgets(command, BUFFER_SIZE, stdin);
		//fgets se chi truyen vao command BUFFER_SIZE - 1 ky tu
		//va ky tu cuoi cung se tu dong duoc gan '\0'
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
			if (filePath != NULL && buffer_size_str != NULL && checkBufferSize(buffer_size_str)) {
				size_t buffer_size = (size_t)atoi(buffer_size_str);
				sendFile(sock, filePath, buffer_size);
			} else
				fprintf(stderr, "Can nhap dung cu phap: SendFile <file_path> <buffer_size nguyen duong>\n");
		}
		else if (strcmp(cmd, "exit") == 0) {
			send(sock, "EXIT", 4, 0);
			break;
		}
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