#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE    512

// ���� �Լ� ���� ��� �� ����
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

// ���� �Լ� ���� ���
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

// ����� ���� ������ ���� �Լ�
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

// Server���� ������ ������ ������ ���� �Լ�
bool receiveData(int retval, SOCKET sock, int len, char buf[])
{

	// ������ �ޱ�(���� ����)
	retval = recvn(sock, (char *)&len, sizeof(int), 0);
	if(retval == SOCKET_ERROR){
		err_display("recv()");
		return false;
	}
	else if(retval == 0)
		return false;

	// ������ �ޱ�(���� ����)
	retval = recvn(sock, buf, len, 0);
	if(retval == SOCKET_ERROR){
		err_display("recv()");
		return false;
	}
	else if(retval == 0)
		return false;

	// ���� ������ ���
	buf[retval] = '\0';
	//printf("[TCP Ŭ���̾�Ʈ] %d����Ʈ�� �޾ҽ��ϴ�.\n", retval);

	// Server���� Data�� ���� �������� return false;
	if(!strcmp(buf,"Transfer_Complete"))
		return false;
	

	printf("[���� ������] %s\n", buf);

	return true;
}
int main(int argc, char *argv[])
{
	int retval;

	// ���� �ʱ�ȭ
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

	// ������ ��ſ� ����� ����
	char buf[BUFSIZE + 1];
	int len;

	// ������ ������ ���
	while (1) {
		// ������ �Է�
		printf("\n[���� ������] ");
		if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
			break;

		// '\n' ���� ����
		len = strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0)
			continue;

		// ������ ���� �߸��Ǿ��� �� ���� ��Ÿ����
		// "exit" ������ ����
		HOSTENT *ptr = gethostbyname(buf);
		if (ptr == NULL && strcmp(buf,"exit")==1) {
			err_display("gethostbyname()");
			continue;
		}

		// ������ �Է�(�ùķ��̼�)
		len = strlen(buf);

		// Domain Name ������ ������(���� ����)
		retval = send(sock, (char *)&len, sizeof(int), 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			break;
		}

		// ������ ������(���� ����)
		retval = send(sock, buf, len, 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			break;
		}

		// server���� exit�� ������ Client �� Program �����ϱ�
		if(!strcmp(buf,"exit")){

			// closesocket()
			closesocket(sock);
			// ���� ����
			WSACleanup();
			return 0;
		}
		printf("[TCP Ŭ���̾�Ʈ] %d+%d����Ʈ�� "
			"���½��ϴ�.\n", sizeof(int), retval);


		while(1){
			// Server���� Data�� ���������� �ޱ�
			// ���� error�� ���ų� Data�� ���� �������� break�� ���� ���� Loop�� �����
			if(receiveData(retval, sock, len, buf) == 0)
				break;
		}
	}

	return 0;
}