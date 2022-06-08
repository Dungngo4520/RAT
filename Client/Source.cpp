#include "Header.h"

#define PORT "27015"

int main(int argc, char* argv[]) {
	WSADATA wsaData;
	SOCKET connectSocket = INVALID_SOCKET;
	addrinfo *result = NULL;
	addrinfo *ptr = NULL;
	addrinfo hints;

	const char *sendBuf = "test";
	char recvBuf[BUFSIZ];
	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("Error WSAStartup %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo(NULL, PORT, &hints, &result);
	if (iResult != 0) {
		printf("Error getaddrinfo. %d\n", iResult);
		WSACleanup();
		return 2;
	}

	ptr = result;
	while (ptr) {
		connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (connectSocket == INVALID_SOCKET) {
			printf("Error socket. %d\n", WSAGetLastError());
			WSACleanup();
			return 3;
		}

		iResult = connect(connectSocket, ptr->ai_addr, ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(connectSocket);
			connectSocket = INVALID_SOCKET;
			ptr = ptr->ai_next;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (connectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 4;
	}

	printf("Connected to server with port %s\n", PORT);

	iResult = send(connectSocket, sendBuf, strlen(sendBuf), 0);
	if (iResult == SOCKET_ERROR) {
		printf("Error send. %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

	printf("Bytes Sent: %ld\n", iResult);

	// shutdown the connection since no more data will be sent
	iResult = shutdown(connectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

	do {

		iResult = recv(connectSocket, recvBuf, BUFSIZ, 0);
		if (iResult > 0)
			printf("Bytes received: %d\n", iResult);
		else if (iResult == 0)
			printf("Connection closed\n");
		else
			printf("Error recv. %d\n", WSAGetLastError());

	} while (iResult > 0);

	// cleanup
	closesocket(connectSocket);
	WSACleanup();

	getchar();
	return 0;
}