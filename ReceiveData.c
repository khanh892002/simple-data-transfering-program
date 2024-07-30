#include "supportLibFunc.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#ifdef _WIN32
	#include <windows.h>
	CRITICAL_SECTION mutex;
#else
	#include <pthread.h>
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

const char *storing_dir;
typedef struct {
	int client_sock;
	char client_ip[INET_ADDRSTRLEN];
	int client_port;
} client_info;//De xu ly xuat thong tin dia chi cua client ra man hinh

//Nhung thanh phan xu ly doi pho hinh thuc tan cong request flooding
#define MAX_CONNS_PER_IP 3 //So luong ket noi toi da tu mot IP
#define MIN_WAITING_TIME_OF_AN_IP 45
//Khoang thoi gian toi thieu de mot dia chi IP duoc xem nhu la mot ket noi moi den server
#define MAX_IPS_COUNT 1000//So luong dia chi IP toi da trong mang
typedef struct {
	char ipAddr[INET_ADDRSTRLEN];
	int connectionCount;
	time_t latestConnection;
} connection_info;//De dem so luong ket noi va thoi gian ket noi gan nhat tu mot dia chi
connection_info *connsList[MAX_IPS_COUNT];
int ip_count = 0;//do mang connsList se khong xoa di cac IP da ngat ket noi nen moi dung den
connection_info* find_or_create_new_conn(const char* checkedIPAddr) {
	for (int i = 0; i < ip_count; i++) {
		if (strncmp(connsList[i]->ipAddr, checkedIPAddr, INET6_ADDRSTRLEN) == 0)
			return connsList[i];
	}

	if (ip_count < MAX_IPS_COUNT) {
		connection_info* newIP = (connection_info*) malloc(sizeof(connection_info));
		strncpy(newIP->ipAddr, checkedIPAddr, INET6_ADDRSTRLEN);
		newIP->connectionCount = 1;
		newIP->latestConnection = 0;
		connsList[ip_count++] = newIP;
		return newIP;
	}

	return NULL;//So IP trong danh sach dat toi da
}

int acceptable_conn(const char* ipAddr) {
	time_t current_moment = time(NULL);
	int result = 0;

	#ifdef _WIN32
		EnterCriticalSection(&mutex);
	#else
		pthread_mutex_lock(&mutex);
	#endif

	connection_info* checkedConn = find_or_create_new_conn(ipAddr);
	if (checkedConn == NULL)
		printf("So dia chi IP trong danh sach dat den toi da\nHay khoi dong lai server\n");
	else {
		result = 1;
		if (difftime(current_moment, checkedConn->latestConnection) > MIN_WAITING_TIME_OF_AN_IP) {
			checkedConn->connectionCount = 1;
			checkedConn->latestConnection = current_moment;
		} else
			if (checkedConn->connectionCount < MAX_CONNS_PER_IP) {
				checkedConn->connectionCount++;
				checkedConn->latestConnection = current_moment;
			} else
				result = 0;
	}

	#ifdef _WIN32
		LeaveCriticalSection(&mutex);
	#else
		pthread_mutex_unlock(&mutex);
	#endif

	return result;
}

void handleFILE(int clientSock, char* clientIP, int clientPort) {
	char buffer[BUFFER_SIZE] = {0};
	//nhan ten file
	size_t len_input = recv(clientSock, buffer, BUFFER_SIZE - 1,0);
	if (len_input <= 0)
		perror("Khong nhan duoc ten file");
	else {
		buffer[len_input] = '\0';
		
		char filePath[MAX_PATH_LENGTH];
		int pathLenValid = snprintf(filePath, sizeof(filePath), "%s/%s", storing_dir, buffer);
		if (pathLenValid < 0 || pathLenValid >= sizeof(filePath)) {
			fprintf(stderr, "Duong dan luu file qua dai");
			send(clientSock, "FAIL", 4, 0);
		}
		else {
			FILE *outfile = fopen(filePath, "wb");
			if (outfile == NULL) {
				perror("Loi mo file");
				send(clientSock, "FAIL", 4, 0);
			}
			else {
				send(clientSock, "SCSS", 4, 0);
				size_t fileSize;
				len_input = recv(clientSock, (char*)&fileSize, sizeof(fileSize), 0);
				if (len_input <= 0)
					perror("Nhan kich thuoc file that bai");
				else {
					size_t total_bytes_received = 0;
					while(total_bytes_received < fileSize && (len_input = recv(clientSock, buffer, BUFFER_SIZE, 0)) > 0) {
						fwrite(buffer, 1, len_input, outfile);
						total_bytes_received += len_input;
					}
					if(total_bytes_received == fileSize)
						printf("Da nhan file tu %s o port %d va luu vao %s\n", clientIP, clientPort, filePath);
					else printf("Chua nhan het file %s tu %s o port %d", buffer, clientIP, clientPort);
				}
				fclose(outfile);
			}
		}
	}
}

void handleTEXT(int clientSock, char* clientIP, int clientPort) {
	char message[BUFFER_SIZE] = {0};
	size_t len_input = recv(clientSock, message, BUFFER_SIZE - 1, 0);
	if (len_input > 0) {
		message[len_input] = '\0';
		printf("Tin nhan tu %s port %d: %s\n", clientIP, clientPort, message);
	}
}

#ifdef _WIN32
DWORD WINAPI handleClient(LPVOID arg) {
	client_info *cliInfo = (client_info *)arg;
	int clientSock = cliInfo->client_sock;
	char *clientIP = cliInfo->client_ip;
	int clientPort = cliInfo->client_port;
	free(arg);

	char check[5] = {0};//De nhan chuoi TEXT/FILE/EXIT tu client neu client muon gui file
	while(1) {
		size_t len_input = recv(clientSock, check, 4, 0);
		//ham recv lay thong tin tu newsockfd (client),
		//vao buffer <size - 1> byte ky tu
		if (len_input > 0) {
			check[len_input] = '\0';
			if (strncmp(check, "FILE", 4) == 0) handleFILE(clientSock, clientIP, clientPort);
			else if (strncmp(check, "TEXT", 4) == 0) handleTEXT(clientSock, clientIP, clientPort);
			else if (strncmp(check, "EXIT", 4) == 0) {
				printf("Nhan duoc thong bao ngat ket noi tu %s o port %d\n", clientIP, clientPort);
				break;
			}
			else printf("Lenh khong hop le tu %s o port %d\n", clientIP, clientPort);
		} else {
			perror("Ket noi tu client bi loi hoac da dong ket noi\n");
			break;
		}
	}
	closesocket(clientSock);

	connection_info *conn = find_or_create_new_conn(clientIP);
	conn->connectionCount--;

	return 0;
}
#else
void* handleClient(void* arg) {
	client_info *cliInfo = (client_info *)arg;
	int clientSock = cliInfo->client_sock;
	char *clientIP = cliInfo->client_ip;
	int clientPort = cliInfo->client_port;
	free(arg);

	char check[5] = {0};
	while(1) {
		size_t len_input = recv(clientSock, check, 4, 0);
		if (len_input > 0) {
			check[len_input] = '\0';
			if (strncmp(check, "FILE", 4) == 0) handleFILE(clientSock, clientIP, clientPort);
			else if (strncmp(check, "TEXT", 4) == 0) handleTEXT(clientSock, clientIP, clientPort);
			else if (strncmp(check, "EXIT", 4) == 0) {
				printf("Nhan duoc thong bao ngat ket noi tu %s o port %d\n", clientIP, clientPort);
				break;
			}
			else printf("Lenh khong hop le tu %s o port %d\n", clientIP, clientPort);
		} else {
			perror("Ket noi tu client bi loi hoac da dong ket noi\n");
			break;
		}
	}
	close(clientSock);
	
	connection_info *conn = find_or_create_new_conn(clientIP);
	conn->connectionCount--;
	
	return 0;
}
#endif

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
	if ((argc < 3) || (strncmp(argv[1], "-out", 4) != 0)) {// sai cu phap
		fprintf(stderr,"Su dung cu phap: ReceiveData -out <thuMuc_output>\n");
		exit(1);
	}
    
	storing_dir = argv[2];
	if (strlen(storing_dir) >= MAX_PATH_LENGTH) {
		fprintf(stderr, "Duong dan thu muc luu tru qua dai\n");
		exit(1);
	}
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
		InitializeCriticalSection(&mutex);
	#else
		pthread_mutex_init(&mutex, NULL);
	#endif

	//Tao socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd < 0) {
		perror("Loi tao socket");
		exit(1);
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&waitingTime, sizeof(waitingTime)) < 0)
		perror("Loi thiet lap thoi gian cho cua server");
	if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&waitingTime, sizeof(waitingTime)) < 0)
		perror("Loi thiet lap thoi gian gui cua server");

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

	while(1) {
		struct sockaddr_in cliAddr;
		socklen_t addrlen = sizeof(cliAddr);	
		//ham accept se luu lai thong tin dia chi cua client
		//tra ve mot socket dung de xu ly ket noi vua nhan, de co the tiep tuc su dung socket ban dau (sockfd)
		//va cung de co the nhan them cac ket noi khac trong khi socket moi (newsockfd) co the duoc dung de giao tiep voi client
		if((newsockfd = accept(sockfd, (struct sockaddr*) &cliAddr, &addrlen)) < 0) {
			perror("Ket noi that bai");
			continue;
		}
		printf("Server: nhan duoc ket noi tu %s o port %d\n", inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));

		if (setsockopt(newsockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&waitingTime, sizeof(waitingTime)) < 0)
			fprintf(stderr, "Loi thiet lap thoi gian cho cua client o port %d\n", ntohs(cliAddr.sin_port));
		if (setsockopt(newsockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&waitingTime, sizeof(waitingTime)) < 0)
			fprintf(stderr, "Loi thiet lap thoi gian gui cua client o port %d\n", ntohs(cliAddr.sin_port));
		// Kiem tra lan cuoi ket noi cua ip

		client_info *clientInfo = malloc(sizeof(client_info));
		if (!clientInfo) {
			perror("Khong tao duoc bien luu tru thong tin client");
			#ifdef _WIN32
				closesocket(newsockfd);
			#else
				close(newsockfd);
			#endif
			continue;
		}

		clientInfo->client_sock = newsockfd;
		strncpy(clientInfo->client_ip, inet_ntoa(cliAddr.sin_addr), INET6_ADDRSTRLEN);
		clientInfo->client_port = ntohs(cliAddr.sin_port);

		if (!acceptable_conn(clientInfo->client_ip)) {
			printf("Luong ket noi tu %s vuot qua gioi han\n", clientInfo->client_ip);
			#ifdef _WIN32
				closesocket(newsockfd);
			#else
				close(newsockfd);
			#endif
			continue;
		}

		#ifdef _WIN32
			CreateThread(NULL, 0, handleClient, clientInfo, 0, NULL);
		#else
			pthread_t thread_id;
			pthread_create(&thread_id, NULL, handleClient, clientInfo);
			pthread_detach(thread_id);//tach thread de tu dong giai phong tai nguyen sau khi ket thu thread
		#endif
	}

	#ifdef _WIN32
		closesocket(sockfd);
		DeleteCriticalSection(&mutex);
		WSACleanup();
	#else
		pthread_mutex_destroy(&mutex);
		close(sockfd);
	#endif
	
	printf("Dong server ReceiveData\n");
	return 0; 
}