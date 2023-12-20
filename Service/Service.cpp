// Service.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _WINSOCK_DEPRECATED_NO_WARNINGS
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

#define SERVER_PORT 27016
#define BUFFER_SIZE 256


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
	do
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
		printf("Successfully conected to Client1. Client address: %s : %d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
		
		do
		{
			iResult = recv(acceptSocket, databuffer, BUFFER_SIZE, 0);
			if (iResult > 0)	// Check if message is successfully received
			{
				databuffer[iResult] = '\0';

				// Log message text
				printf("Client sent: %s.\n", databuffer);


			}
			else if (iResult == 0)	// Check if shutdown command is received
			{
				// Connection was closed successfully
				printf("Connection with closed.\n");
				shutdown(acceptSocket, SD_BOTH);
				closesocket(acceptSocket);
				break;
			}
			else	// There was an error during recv
			{

				printf("recv failed with error: %d\n", WSAGetLastError());
				shutdown(acceptSocket, SD_BOTH);
				closesocket(acceptSocket);
				break;
			}

			printf("Enter message: ");
			gets_s(databuffer, BUFFER_SIZE);

			iResult = send(acceptSocket, databuffer, (int)strlen(databuffer), 0);
			if (iResult == INVALID_SOCKET)
			{
				printf("Sending test mesage to client1 failed with error: %d\n", WSAGetLastError());
				closesocket(listenSocket);
				break;
			}
		} while (true);
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

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
