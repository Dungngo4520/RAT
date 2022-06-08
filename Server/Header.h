#include <stdio.h>
#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define PORT "27015"

using namespace std;


void sendFile(const char* filePath, SOCKET clientSocket);
void receiveFile(const char* fileName, SOCKET clientSocket);