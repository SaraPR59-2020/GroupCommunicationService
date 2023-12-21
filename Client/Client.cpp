// Client.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include "../Common/Common.cpp" 

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

char groupName[MAX_MESSAGE_LENGTH];
int messageLen;


int main()
{
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
	{
		printf("WSAStartup faild with error: %d", WSAGetLastError());
		return 1;
	}
	char databuffer[BUFFER_SIZE];

	SOCKET connectSocket;
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
		printf("Succasfuly conected to service! \n");
	}
	
	//non-blocking mode
	u_long mode = 1;
	iResult = ioctlsocket(connectSocket, FIONBIO, &mode);
	if (iResult != NO_ERROR) {

		printf("ioctlsocket failed with error: %ld\n", iResult);
		return false;
	}

	do
	{
		int option = 0;
		printf("\t\t\t\tGROUP COMUNICATION SERVIS DATA TO SEND\n");
		printf("\t1. ENTER GROUP\n");
		printf("\t2. EXIT\n");
		printf("\n\n");
		option = _getch(); // ascii vrednost karaktera

		switch (option - 48) {
		case 1:
			printf("Enter name of group chat: \n");
			char queueName[MAX_MESSAGE_LENGTH];
			gets_s(queueName, MAX_MESSAGE_LENGTH - 1);
			//printf(queueName);
			strcpy_s(groupName, MAX_MESSAGE_LENGTH, queueName);

			messageLen = strlen(queueName);
			if (messageLen == 0) {
				printf("Invalid type of input, try agian...\n");
				break;
			}

			Connect(connectSocket, queueName);
			break;

		case 2:
			Disconnect(connectSocket, queueName);
			break;
		default:
			printf("Invalid type of input, try agian...\n");
			break;
		}
		printf("\n\n");

	} while (true);

	iResult = shutdown(connectSocket, SD_BOTH);
	if (iResult == SOCKET_ERROR)
	{
		printf("SD faailed with error: %d", WSAGetLastError());
		printf("Enter word on selected letter to send: ");
		closesocket(connectSocket);
		if (WSACleanup() != 0)
		{
			printf("WSACleanup faild with error: %d", WSAGetLastError());
			return 1;
		}
	}

	printf("press any key o exit: ");
	_getch();

	closesocket(connectSocket);
	if (WSACleanup() != 0)
	{
		printf("WSACleanup faild with error: %d", WSAGetLastError());
		return 1;
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
