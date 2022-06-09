#include <stdio.h>
#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable : 4996)

#define PORT "27015"

using namespace std;


void sendFile(const char* fileName, SOCKET clientSocket);
void receiveFile(const char* fileType, SOCKET clientSocket);