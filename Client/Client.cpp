// Client.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define SafeCloseHandle(handle) if(handle) CloseHandle(handle);
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "conio.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma warning (disable: 4996)

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 27016
#define BUFFER_SIZE 256
#define MAX_MESSAGE_LENGTH 256

DWORD WINAPI ThreadRECV(LPVOID lpParam);
void Connect(char* queueName);
void Disconnect(char* queueName);
void SendMessageToPass(char* message);

SOCKET connectSocket;
char groupName[MAX_MESSAGE_LENGTH];
HANDLE hRecv;
int messageLen;
bool shutingDown = false;

int main()
{
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
	{
		printf("WSAStartup faild with error: %d", WSAGetLastError());
		return 1;
	}
	char databuffer[BUFFER_SIZE];

	connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connectSocket == INVALID_SOCKET)
	{
		printf("socket faild with error: %d", WSAGetLastError());
		if (WSACleanup() != 0)
		{
			printf("WSACleanup faild with error: %d", WSAGetLastError());
			return 1;
		}
		return 1;
	}

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);
	serverAddress.sin_port = htons(SERVER_PORT);

	int iResult = 0;

	iResult = connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
	if (iResult == SOCKET_ERROR)
	{
		printf("'connect' failed with error: %d", WSAGetLastError());
		closesocket(connectSocket);
		if (WSACleanup() != 0)
		{
			printf("WSACleanup faild with error: %d", WSAGetLastError());
			return 1;
		}
		return 1;
	}
	else
	{
		printf("Succasfuly conected to service! \n\n\n");
	}
	
	//non-blocking mode
	u_long mode = 1;
	iResult = ioctlsocket(connectSocket, FIONBIO, &mode);
	if (iResult != NO_ERROR) {

		printf("ioctlsocket failed with error: %ld\n", iResult);
		return false;
	}

	//thread for news
	hRecv = CreateThread(NULL, NULL, &ThreadRECV, &connectSocket, NULL, NULL);

	bool doWhile = true;

	while(doWhile)
	{
		int option = 0;
		printf("\t\t\t\tGROUP COMMUNICATION SERVICE\n");
		printf("\t1. ENTER GROUP\n");
		printf("\t2. EXIT\n");
		printf("\n");
		option = _getch();

		switch (option - 48) {
		case 1:
			printf("Enter name of group chat (min three characters): \n");
			char queueName[MAX_MESSAGE_LENGTH];
			gets_s(queueName, MAX_MESSAGE_LENGTH - 1);
			strcpy_s(groupName, MAX_MESSAGE_LENGTH, queueName);
			
			//printf(queueName);
			messageLen = strlen(queueName);
			if (messageLen <= 2) {
				printf("Invalid type of input, try agian...\n");
				break;
			}
			else
			{
				Connect(queueName);
				while (doWhile)
				{
					int option = 0;
					printf("\t\t\t\tGROUP COMMUNICATION SERVICE\n");
					printf("\t1. SEND MESSAGE TO THE GROUP\n");
					printf("\t2. EXIT GROUP AND SERVICE\n");
					printf("\n");
					option = _getch();
					char delimiter[] = "#";
					switch (option - 48) {
					case 1:
						printf("Enter message (do not enter '#' - it will be deleted from message): \n");
						char input[MAX_MESSAGE_LENGTH];
						gets_s(input, MAX_MESSAGE_LENGTH - 1);

						
						char message[MAX_MESSAGE_LENGTH];
						if (strpbrk(input, "#") != NULL)
						{
							char* before, * after;
							before = strtok(input, delimiter);
							after = strtok(NULL, delimiter);

							strcpy(message, groupName);
							strcat(message, "#");
							strcat(message, before);
							strcat(message, after);
							strcat(message, "S");
						}
						else
						{
							strcpy(message, groupName);
							strcat(message, "#");
							strcat(message, input);
							strcat(message, "S");
						}
						printf("Sending message '%s' to service...\n", message);
						SendMessageToPass(message);
						break;
					case 2:
						doWhile = false;
						Disconnect(groupName);
						shutingDown = true;
						break;
					default:
						printf("Invalid type of input, try agian...\n");
						break;
					}
				}
			}
			break;
		case 2:
			doWhile = false;
			printf("Shutingdown client...\n");
			shutingDown = true;
			break;
		default:
			printf("Invalid type of input, try agian...\n");
			break;
		}
		printf("\n");
	}

	iResult = shutdown(connectSocket, SD_BOTH);
	if (iResult == SOCKET_ERROR)
	{
		printf("SD failed with error: %d", WSAGetLastError());
		printf("Enter word on selected letter to send: ");
		closesocket(connectSocket);
		if (WSACleanup() != 0)
		{
			printf("WSACleanup faild with error: %d", WSAGetLastError());
			return 1;
		}
	}

	CloseHandle(hRecv);
	closesocket(connectSocket);

	if (WSACleanup() != 0)
	{
		printf("WSACleanup faild with error: %d", WSAGetLastError());
		return 1;
	}
	return 0;
}

// TODO: This is an example of a library function
void Connect(char* queueName) {
	FD_SET set;
	timeval timeVal;
	FD_ZERO(&set);
	FD_SET(connectSocket, &set);
	timeVal.tv_sec = 0;
	timeVal.tv_usec = 0;

	char message[MAX_MESSAGE_LENGTH];
	strcpy_s(message, queueName);
	strcat(message, "C");

	int iResult = select(0, NULL, &set, NULL, &timeVal);
	if (iResult == SOCKET_ERROR)
	{
		printf("Error in select: %ld\n", WSAGetLastError());
	}
	else if (iResult == 0)
	{
		Sleep(1000);
	}
	else if (iResult > 0)
	{
		iResult = send(connectSocket, message, strlen(message) + 1, 0);
	}

	if (iResult == SOCKET_ERROR)
	{
		printf("Send failed with error: %ld\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
	}
}
void Disconnect(char* queueName) {
	FD_SET set;
	timeval timeVal;
	FD_ZERO(&set);
	FD_SET(connectSocket, &set);
	timeVal.tv_sec = 0;
	timeVal.tv_usec = 0;

	char message[MAX_MESSAGE_LENGTH];
	strcpy_s(message, queueName);
	strcat(message, "D");

	int iResult = select(0, NULL, &set, NULL, &timeVal);
	if (iResult == SOCKET_ERROR)
	{
		printf("Error in select: %ld\n", WSAGetLastError());
	}
	else if (iResult == 0)
	{
		Sleep(1000);
	}
	else if (iResult > 0)
	{
		iResult = send(connectSocket, message, strlen(message) + 1, 0);
	}

	if (iResult == SOCKET_ERROR)
	{
		printf("Send failed with error: %ld\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
	}
}

void SendMessageToPass(char* message) {
	FD_SET set;
	timeval timeVal;
	FD_ZERO(&set);
	FD_SET(connectSocket, &set);
	timeVal.tv_sec = 0;
	timeVal.tv_usec = 0;

	int iResult = select(0, NULL, &set, NULL, &timeVal);
	if (iResult == SOCKET_ERROR)
	{
		printf("Error in select: %ld\n", WSAGetLastError());
	}
	else if (iResult == 0)
	{
		Sleep(1000);
	}
	else if (iResult > 0)
	{
		printf("%s\n", message);
		iResult = send(connectSocket, message, strlen(message) + 1, 0);
	}

	if (iResult == SOCKET_ERROR)
	{
		printf("Send failed with error: %ld\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
	}
}

//thread for communication: accepts news from service 
DWORD WINAPI ThreadRECV(LPVOID lpParam) {

	int iResult;
	SOCKET connectSocketRECV = *(SOCKET*)lpParam;
	char dataBuffer[BUFFER_SIZE];

	FD_SET set;
	timeval timeVal;
	timeVal.tv_sec = 0;
	timeVal.tv_usec = 0;

	bool serverOut = false;

	while (shutingDown != true) {

		FD_ZERO(&set);
		FD_SET(connectSocketRECV, &set);

		iResult = select(0, &set, NULL, NULL, &timeVal);

		if (iResult == SOCKET_ERROR)
		{
			serverOut = true;

			printf("select failed: %ld\n", WSAGetLastError());
			printf("Service was probably closed\n");
			SafeCloseHandle(hRecv);
			return 0;
		}

		if (iResult == 0)
		{
			Sleep(1000);
			continue;
		}
		if (iResult > 0) {

			FD_SET set;
			timeval timeVal;
			timeVal.tv_sec = 0;
			timeVal.tv_usec = 0;
			FD_ZERO(&set);
			FD_SET(connectSocketRECV, &set);

			iResult = select(0, &set, NULL, NULL, &timeVal);
			if (iResult == SOCKET_ERROR)
			{
				printf("select failed in ReceiveFunction: %ld\n", WSAGetLastError());
			}
			else if (iResult == 0)
			{
				Sleep(1000);
				continue;
			}
			else if (iResult > 0)
			{
				iResult = recv(connectSocketRECV, dataBuffer, BUFFER_SIZE, 0);

				if (iResult > 0) {
					printf("News from service: %s\n", dataBuffer);
				}
				else {
					printf("recv failed with error: %d\n", WSAGetLastError());
					closesocket(connectSocketRECV);
				}
			}
		}
	}
	return 0;
}
// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
