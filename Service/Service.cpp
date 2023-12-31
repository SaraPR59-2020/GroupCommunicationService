// Service.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <string.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "conio.h"
#include "Functions.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define SERVER_PORT 27016
#define BUFFER_SIZE 256
#define MAX_CLIENTS 100
#define MAX_MESSAGE_LENGTH 256

DWORD WINAPI ClientHandle(LPVOID params);
HANDLE thread[MAX_CLIENTS];

DWORD WINAPI SendMessageFromQueue(LPVOID lpParam);
HANDLE threadSendMess;

hash_table* ht = NULL;

typedef struct pom {
	SOCKET my_socket;
	int idx;
	sockaddr_in addrs;
} pom;


int main()
{
	WSADATA wsadata;

	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
	{
		printf("WSAStartup failed with error : %d", WSAGetLastError());
		return 1;
	}

	ht = init_hash_table();

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

	printf("Server socket is set to listening mode. Waiting for new connection.\n\n\n");

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
			printf("\t\tSuccessfully conected to Client. Client address: %s : %d\n\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
			pom params = { 
				acceptSocket,
				clientNum,
				clientAddr
			};

			thread[clientNum] = CreateThread(NULL, NULL, &ClientHandle, &params, NULL, NULL);
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

	//printf("press any key o exit: ");
	//_getch();

	for (int client = 0; client < clientNum; client++)
		CloseHandle(thread[client]);

	closesocket(acceptSocket);
	closesocket(listenSocket);
	if (WSACleanup() != 0)
	{
		printf("WSACleanup faild with error: %d\n", WSAGetLastError());
		return 1;
	}
	return 0;
}

//thread for connection: accepting client into the group, entering client into the hash table and putting message into queue
DWORD WINAPI ClientHandle(LPVOID params)
{
	pom p = *(pom*)params;
	SOCKET acceptedSocket = p.my_socket;
	sockaddr_in clientAddr = p.addrs;
	int client = p.idx;

	FD_SET set;
	timeval timeVal;
	timeVal.tv_sec = 0;
	timeVal.tv_usec = 0;
	bool disconnected = false;
	int iResult;

	do {
		FD_ZERO(&set);
		FD_SET(acceptedSocket, &set);

		char dataBuffer[BUFFER_SIZE];
		if (disconnected)
			break;
		iResult = select(0, &set, NULL, NULL, &timeVal);
		if (iResult == SOCKET_ERROR)
		{
			printf("select failed: %ld\n", WSAGetLastError());
			closesocket(acceptedSocket);
			disconnected = true;
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
				printf("Message from client: %s \n", dataBuffer);
				char delimiter[] = "#";

				if (dataBuffer[strlen(dataBuffer) - 1] == 'C')
				{
					for (int i = strlen(dataBuffer) - 1; i < strlen(dataBuffer); ++i)
						dataBuffer[i] = dataBuffer[i + 1];

					printf("Choosen group:\t%s\n", dataBuffer);
					if (!hashtable_findgroup(ht, (dataBuffer))) {
						printf("Initialization and creation of new group...\n");
						hashtable_addgroup(ht, (dataBuffer));
						hashtable_addsocket(ht, (dataBuffer), acceptedSocket);
					}
					else {
						printf("Adding to an existing group...\n");
						hashtable_addsocket(ht, (dataBuffer), acceptedSocket);
						
					}
					printf("Client '%s : %d' successfuly added to the group '%s'!\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), dataBuffer);
					list_socket* lista = hashtable_getsockets(ht, (dataBuffer));
					list_print(lista->head, (dataBuffer));

					char messageToSend[BUFFER_SIZE];
					strcpy_s(messageToSend, "Successfuly joined to the group!\n");

					iResult = send(acceptedSocket, messageToSend, MAX_MESSAGE_SIZE, 0);

					if (iResult == SOCKET_ERROR)
					{
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(acceptedSocket);
						return 1;
					}
				}
				else if (dataBuffer[strlen(dataBuffer) - 1] == 'D')
				{
					for (int i = strlen(dataBuffer) - 1; i < strlen(dataBuffer); ++i)
						dataBuffer[i] = dataBuffer[i + 1];

					printf("Client wants to disconnect from the group: %s\n", dataBuffer);
					if (hashtable_removesocket(ht, dataBuffer, acceptedSocket)) {
						printf("Client '%s : %d' successfuly disconnect from the group '%s'!\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), dataBuffer);
						list_socket* lista = hashtable_getsockets(ht, (dataBuffer));
						list_print(lista->head, (dataBuffer));
					}
					disconnected = true;
					closesocket(acceptedSocket);
					CloseHandle(thread[client]);
				}
				else if (dataBuffer[strlen(dataBuffer) - 1] == 'S')
				{
					for (int i = strlen(dataBuffer) - 1; i < strlen(dataBuffer); ++i)
						dataBuffer[i] = dataBuffer[i + 1];

					char* group = strtok(dataBuffer, delimiter);
					char* message = strtok(NULL, delimiter);
					printf("Client wants to send message: %s\t to the group: %s\n", message, group);

					threadSendMess = CreateThread(NULL, NULL, &SendMessageFromQueue, group, NULL, NULL);

					enqueue(getqueue(ht, (group)), (message));
				}
				else
				{
					printf("Somehow invalid message came from client...\n");
				}
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
		printf("\n\n");
	} while (true);
	printf("The client has been closed\n");
	return 0;
}

DWORD WINAPI SendMessageFromQueue(LPVOID lpParam) {
	char* group = (char*)lpParam;
	char* pom = (char*)malloc(sizeof(char) * MAX_MESSAGE_LENGTH);
	queue* groupQueue = getqueue(ht, group);
	list_socket* list = hashtable_getsockets(ht, group);
	int len = list->len;
	listsocket_item* socketsInList = list->head;

	list_print(list->head, group);

	do {
		if (groupQueue->head != NULL) {
			pom = dequeue(groupQueue);
			while (socketsInList != NULL) {
				if (socketsInList->next == NULL) {
					int iResult = send(socketsInList->socket, pom, MAX_MESSAGE_SIZE, 0);

					if (iResult == SOCKET_ERROR)
					{
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(socketsInList->socket);
						return 1;
					}
				}
				else {
					int iResult = send(socketsInList->socket, pom, MAX_MESSAGE_SIZE, 0);

					if (iResult == SOCKET_ERROR)
					{
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(socketsInList->socket);
						return 1;
					}
				}
				socketsInList = socketsInList->next;

			}
			socketsInList = list->head;
		}
		else {
			Sleep(5000);
			continue;
		}
	} while (1);
	free(pom);
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
