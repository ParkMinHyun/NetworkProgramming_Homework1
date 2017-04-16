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
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
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
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
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

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

// Server에서 보내진 데이터 수신을 위한 함수
bool receiveData(int retval, SOCKET sock, int len, char buf[])
{

	// 데이터 받기(고정 길이)
	retval = recvn(sock, (char *)&len, sizeof(int), 0);
	if(retval == SOCKET_ERROR){
		err_display("recv()");
		return false;
	}
	else if(retval == 0)
		return false;

	// 데이터 받기(가변 길이)
	retval = recvn(sock, buf, len, 0);
	if(retval == SOCKET_ERROR){
		err_display("recv()");
		return false;
	}
	else if(retval == 0)
		return false;

	// 받은 데이터 출력
	buf[retval] = '\0';
	//printf("[TCP 클라이언트] %d바이트를 받았습니다.\n", retval);

	// Server에서 Data가 전부 보내지면 return false;
	if(!strcmp(buf,"Transfer_Complete"))
		return false;
	

	printf("[받은 데이터] %s\n", buf);

	return true;
}
int main(int argc, char *argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");

	// 데이터 통신에 사용할 변수
	char buf[BUFSIZE + 1];
	int len;

	// 서버와 데이터 통신
	while (1) {
		// 데이터 입력
		printf("\n[보낼 데이터] ");
		if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
			break;

		// '\n' 문자 제거
		len = strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0)
			continue;

		// 도메인 네임 잘못되었을 때 에러 나타나기
		// "exit" 문구는 예외
		HOSTENT *ptr = gethostbyname(buf);
		if (ptr == NULL && strcmp(buf,"exit")==1) {
			err_display("gethostbyname()");
			continue;
		}

		// 데이터 입력(시뮬레이션)
		len = strlen(buf);

		// Domain Name 데이터 보내기(고정 길이)
		retval = send(sock, (char *)&len, sizeof(int), 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			break;
		}

		// 데이터 보내기(가변 길이)
		retval = send(sock, buf, len, 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			break;
		}

		// server에서 exit를 받으면 Client 및 Program 종료하기
		if(!strcmp(buf,"exit")){

			// closesocket()
			closesocket(sock);
			// 윈속 종료
			WSACleanup();
			return 0;
		}
		printf("[TCP 클라이언트] %d+%d바이트를 "
			"보냈습니다.\n", sizeof(int), retval);


		while(1){
			// Server에서 Data를 지속적으로 받기
			// 만약 error가 나거나 Data가 전부 보내지면 break를 통해 무한 Loop를 벗어나기
			if(receiveData(retval, sock, len, buf) == 0)
				break;
		}
	}

	return 0;
}