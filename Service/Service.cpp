// Service.cpp : This file contains the 'main' function. Program execution begins and ends there.
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

DWORD WINAPI ClientHandle(LPVOID params);


#define SERVER_PORT 27016
#define BUFFER_SIZE 256

typedef struct pom {
	SOCKET my_socket;
	int idx;
} pom;


int main()
{
	WSADATA wsadata;

	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
	{
		printf("WSAStartup failed with error : %d", WSAGetLastError());
		return 1;
	}

	char databuffer[BUFFER_SIZE] ;

	sockaddr_in socketAddress;
	memset((char*)&socketAddress, 0, sizeof(socketAddress));

	socketAddress.sin_family = AF_INET;
	socketAddress.sin_addr.s_addr = INADDR_ANY;
	socketAddress.sin_port = htons(SERVER_PORT);

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
	{
		printf("Listen socket faild with error\n");
		if (WSACleanup() != 0)
		{
			printf("WSACleanup faild with error: %d\n", WSAGetLastError());
			return 1;
		}
		return 1;
	}

	int iResult = bind(listenSocket, (struct sockaddr*)&socketAddress, sizeof(socketAddress));
	if (iResult == SOCKET_ERROR)
	{
		printf("Binding faild with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		if (WSACleanup() != 0)
		{
			printf("WSACleanup faild with error: %d\n", WSAGetLastError());
			return 1;
		}
		return 1;
	}
	bool bOptVal = true;
	int bOptLen = sizeof(bool);
	iResult = setsockopt(listenSocket, SOL_SOCKET, SO_CONDITIONAL_ACCEPT, (char*)&bOptVal, bOptLen);
	if (iResult == SOCKET_ERROR) {
		printf("setsockopt for SO_CONDITIONAL_ACCEPT failed with error: %u\n", WSAGetLastError());
	}

	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("Listening faild with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		if (WSACleanup() != 0)
		{
			printf("WSACleanup faild with error: %d\n", WSAGetLastError());
			return 1;
		}
		return 1;
	}

	SOCKET acceptSocket = INVALID_SOCKET;

	printf("Server socket is set to listening mode. Waiting for new connection.\n");

	int clientNum = 0;

	FD_SET set;
	timeval timeVal;
	timeVal.tv_sec = 1;
	timeVal.tv_usec = 0;

	do
	{
		FD_ZERO(&set);
		FD_SET(listenSocket, &set);

		int selectResult = select(0, &set, NULL, NULL, &timeVal);

		if (selectResult == SOCKET_ERROR)
		{
			printf("Select failed with error: %d\n", WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}
		else if (selectResult == 0) // timeout expired
		{

			continue;
		}
		else if (FD_ISSET(listenSocket, &set))
		{
			sockaddr_in clientAddr;
			int clientAddrSize = sizeof(clientAddr);

			acceptSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
			if (acceptSocket == INVALID_SOCKET)
			{
				printf("Accept failed with error: %d\n", WSAGetLastError());
				closesocket(acceptSocket);
				if (WSACleanup() != 0)
				{
					printf("WSACleanup faild with error: %d\n", WSAGetLastError());
					return 1;
				}
				return 1;
			}
			printf("Successfully conected to Client. Client address: %s : %d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
			pom params = { 
				acceptSocket,
				clientNum 
			};
			//printf(databuffer);
			HANDLE thread = CreateThread(NULL, NULL, &ClientHandle, &params, NULL, NULL);
			clientNum++;
		}
	} while (true);

	iResult = shutdown(acceptSocket, SD_BOTH);
	if (iResult == SOCKET_ERROR)
	{
		printf("'shutdown' faild with error: %d\n", WSAGetLastError());
		closesocket(acceptSocket);
		if (WSACleanup() != 0)
		{
			printf("WSACleanup faild with error: %d\n", WSAGetLastError());
			return 1;
		}
		return 1;
	}

	printf("press any key o exit: ");
	_getch();

	closesocket(acceptSocket);
	closesocket(listenSocket);
	if (WSACleanup() != 0)
	{
		printf("WSACleanup faild with error: %d\n", WSAGetLastError());
		return 1;
	}
	return 0;
}

//thread for conection: accepting client into group, entering client in hash table and putting message into queue
DWORD WINAPI ClientHandle(LPVOID params)
{
	pom p = *(pom*)params;
	SOCKET acceptedSocket = p.my_socket;
	int ind = p.idx;

	FD_SET set;
	timeval timeVal;
	timeVal.tv_sec = 0;
	timeVal.tv_usec = 0;
	bool odjavljen = false;
	int iResult;

	do {
		FD_ZERO(&set);
		FD_SET(acceptedSocket, &set);

		char dataBuffer[BUFFER_SIZE];
		if (odjavljen)
			break;
		iResult = select(0, &set, NULL, NULL, &timeVal);
		if (iResult == SOCKET_ERROR)
		{
			printf("select failed: %ld\n", WSAGetLastError());
			closesocket(acceptedSocket);
			odjavljen = true;
		}
		else if (iResult == 0)
		{
			Sleep(1000);
			continue;
		}
		else
		{
			int iResult = recv(acceptedSocket, dataBuffer, BUFFER_SIZE, 0);

			if (iResult > 0)	
			{
				printf("Choosen group: \t");
				printf(dataBuffer);
			}
			else if (iResult == 0)
			{
				printf("Connection with client closed.\n");
				closesocket(acceptedSocket);
			}
			else
			{
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(acceptedSocket);
			}
		}
	} while (true);
	printf("The client has been closed\n");
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
