// Common.cpp : Defines the functions for the static library.
//

#include <stdlib.h>
#include <stdio.h>

#include "pch.h"
#include "framework.h"
#include <WinSock2.h>
#include "Common.h"

#define MAX_MESSAGE_LENGTH 256

// TODO: This is an example of a library function
void Connect(SOCKET connectSocket, char* queueName) {
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
		iResult = send(connectSocket, queueName, (int)sizeof(queueName), 0);
	}

	if (iResult == SOCKET_ERROR)
	{
		printf("Send failed with error: %ld\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
	}
}
void Disconnect(SOCKET connectSocket, char* queueName) {}
void SendMessage(SOCKET connectSocket, void* message, int messageSize) {}
