#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE    512

// 소켓 함수 오류 출력 후 종료
void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// 사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while(left > 0){
		received = recv(s, ptr, left, flags);
		if(received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if(received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

int main(int argc, char *argv[])
{
	int retval;
	int size=3;
	int indexOfDomain = 0;
	char *domainInfo[20];

	// 윈속 초기화
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("connect()");

	// 데이터 통신에 사용할 변수
	char buf[BUFSIZE+1];
	int len;

	// 서버와 데이터 통신
	while(1){
		// 데이터 입력
		printf("\n[보낼 데이터] ");
		if(fgets(buf, BUFSIZE+1, stdin) == NULL)
			break;
		
		// Client 종료하기
		if(!strcmp(buf,"exit\n"))
			exit(1);

		// '\n' 문자 제거
		len = strlen(buf);
		if(buf[len-1] == '\n')
			buf[len-1] = '\0';
		if(strlen(buf) == 0)
			continue;
		

		HOSTENT *ptr = gethostbyname(buf);
		if(ptr == NULL){
			err_display("gethostbyname()");
			continue;
		}

		indexOfDomain = 0;
		domainInfo[indexOfDomain] = (char *) malloc ( sizeof(char*) * strlen(ptr->h_name));
		domainInfo[indexOfDomain++] = ptr->h_name;
		
		
		int aliases_index =0;
		char **aliases_temp;
	    char **ptr2 = ptr->h_aliases;
		
		aliases_temp = (char**)malloc(sizeof(char*)*15);
		for(int i=0; i<15; i++)
			aliases_temp[i] = (char*)malloc(sizeof(char)*100);
	    while(*ptr2){
			strcpy(aliases_temp[aliases_index],*ptr2); 
			domainInfo[indexOfDomain] = (char *) malloc (sizeof(char*) * strlen(*ptr2));
			domainInfo[indexOfDomain++] = aliases_temp[aliases_index];
		    ++ptr2;
	    }

		char **ptr3 = ptr->h_addr_list;
		int ip_index =0;
		char **ip_temp;
		ip_temp = (char**)malloc(sizeof(char*)*15);
		for(int i=0; i<15; i++)
			ip_temp[i] = (char*)malloc(sizeof(char)*100);
		IN_ADDR addr;
	    while(*ptr3){
			memcpy(&addr, *ptr3, ptr->h_length);
			strcpy(ip_temp[ip_index],inet_ntoa(addr)); 
			domainInfo[indexOfDomain] = (char *) malloc ( sizeof(char*) * strlen(*ptr3));
			domainInfo[indexOfDomain++] = ip_temp[ip_index++];
		    ++ptr3;
	    }

		for(int i=0; i< indexOfDomain; i++){
			
			// 데이터 입력(시뮬레이션)
			len = strlen(domainInfo[i]);
			strncpy(buf, domainInfo[i], len);

			// 데이터 보내기(고정 길이)
			retval = send(sock, (char *)&len, sizeof(int), 0);
			if(retval == SOCKET_ERROR){
				err_display("send()");
				break;
			}

			// 데이터 보내기(가변 길이)
			retval = send(sock, buf, len, 0);
			if(retval == SOCKET_ERROR){
				err_display("send()");
				break;
			}
			printf("[TCP 클라이언트] %d+%d바이트를 "
				"보냈습니다.\n", sizeof(int), retval);

			// 데이터 받기
			retval = recvn(sock, buf, retval, 0);
			if(retval == SOCKET_ERROR){
				err_display("recv()");
				break;
			}
			else if(retval == 0)
				break;

			// 받은 데이터 출력
			buf[retval] = '\0';
			//printf("[TCP 클라이언트] %d바이트를 받았습니다.\n", retval);
			printf("[받은 데이터] %s\n", buf);
			
		}
	}

	// closesocket()
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}