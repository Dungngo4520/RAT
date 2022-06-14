#include <stdio.h>
#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <time.h>
#include <regex>

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable : 4996)

#define PORT "27015"

using namespace std;


void sendFile(const char* fileName, SOCKET clientSocket);
void receiveFile(int fileType, SOCKET clientSocket);

BOOL sendSignal(BOOL signal, SOCKET connectSocket);
BOOL receiveSignal(SOCKET connectSocket);

BOOL sendSignal(BOOL signal, SOCKET connectSocket) {
	if (send(connectSocket, (char*)&signal, sizeof(signal), 0) != sizeof(BOOL)) {
		printf("[ERROR] send signal failed. %d\n", WSAGetLastError());
		return FALSE;
	}
	return TRUE;
}

BOOL receiveSignal(SOCKET connectSocket) {
	BOOL signal = FALSE;
	if (recv(connectSocket, (char*)&signal, sizeof(signal), 0) != sizeof(signal)) {
		printf("[ERROR] recv. %d\n", WSAGetLastError());
	}
	return signal;
}