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
#include <cstdlib>
#include <stdio.h>
#include "conio.h"
#include "Functions.h"
#include <charconv>
#include <string>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma warning (disable: 6011)
#pragma warning (disable: 6387)
#pragma warning (disable: 4996)

#define SERVER_PORT 33033
#define BUFFER_SIZE 256
#define MAX_CLIENTS 100
#define MAX_MESSAGE_LENGTH 256
#define MAX_GROUP_NAME 20
char groups[MAX_NUMBER_GROUPS][MAX_GROUP_NAME];


bool shutDownService = false;
int clientNum = 0;

void deleteFromGroups(char* groupName);	//future function for deleting group from the list of groups

DWORD WINAPI ClientHandle(LPVOID params);
HANDLE thread[MAX_CLIENTS];

DWORD WINAPI SendMessageFromQueue(LPVOID lpParam);
HANDLE threadSendMess[MAX_NUMBER_GROUPS];
int groupNum = 0;

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

	FD_SET set;
	timeval timeVal;
	timeVal.tv_sec = 1;
	timeVal.tv_usec = 0;


	while (!shutDownService)
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

			char messageToSend[MAX_MESSAGE_LENGTH];
			strcpy(messageToSend, "Wellcome to service.\n");
			iResult = send(acceptSocket, messageToSend, strlen(messageToSend) + 1, 0);

			pom params = { 
				acceptSocket,
				clientNum,
				clientAddr
			};
			thread[clientNum] = CreateThread(NULL, NULL, &ClientHandle, &params, NULL, NULL);
			clientNum++;
		}
		
		if(shutDownService)
			break;
	};

	WaitForMultipleObjects(groupNum, threadSendMess, TRUE, INFINITE);
	for (int group = 0; group < groupNum; group++)
		CloseHandle(threadSendMess[group]);

	iResult = shutdown(acceptSocket, SD_BOTH);
		if (iResult == SOCKET_ERROR)
		{
			printf("'shutdown' of acceptSocket faild with error: %d\n", WSAGetLastError());
			closesocket(acceptSocket);
			if (WSACleanup() != 0)
			{
				printf("WSACleanup faild with error: %d\n", WSAGetLastError());
				return 1;
			}
			return 1;
		}
	closesocket(acceptSocket);

	iResult = shutdown(listenSocket, SD_BOTH);
	if (iResult == SOCKET_ERROR)
	{
		printf("'shutdown' of listenSocket faild with error: %d\n", WSAGetLastError());
		closesocket(acceptSocket);
		if (WSACleanup() != 0)
		{
			printf("WSACleanup faild with error: %d\n", WSAGetLastError());
			return 1;
		}
		return 1;
	}
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
				printf("\nMessage from client '%s : %d' : %s \n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), dataBuffer);
				char delimiter[] = "#";
				char option = dataBuffer[strlen(dataBuffer) - 1];
				for (int i = strlen(dataBuffer) - 1; i < strlen(dataBuffer); ++i)
					dataBuffer[i] = dataBuffer[i + 1];

				if (option == 'C')
				{
					char messageToSend[MAX_MESSAGE_LENGTH];

					printf("\nChoosen group of client '%s : %d' :\t%s\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), dataBuffer);
					if (!hashtable_findgroup(ht, (dataBuffer))) {
						printf("Initialization and creation of new group '%s'...\n", dataBuffer);
						if(hashtable_addgroup(ht, (dataBuffer)))
						{
							printf("Group '%s' successfuly created!\n", dataBuffer);
						}
						else
						{
							printf("Group '%s' creation failed!\n", dataBuffer);
						}
						if (hashtable_addsocket(ht, (dataBuffer), acceptedSocket))
						{
							printf("Client '%s : %d' successfuly added to the group '%s'!\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), dataBuffer);
						}
						else
						{
							printf("Client '%s : %d' failed to add to the group '%s'!\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), dataBuffer);
						}
						threadSendMess[groupNum] = CreateThread(NULL, NULL, &SendMessageFromQueue, dataBuffer, NULL, NULL);
						strcpy_s(groups[groupNum], MAX_GROUP_NAME, dataBuffer);
						groupNum++;
					}
					else {
						printf("Adding to an existing group group '%s'...\n", dataBuffer);
						hashtable_addsocket(ht, (dataBuffer), acceptedSocket);
						
					}
					list_socket* lista = hashtable_getsockets(ht, (dataBuffer));
					list_print(lista->head, (dataBuffer));
					
					strcpy(messageToSend, "Successfuly joined to the group '");
					strcat(messageToSend, dataBuffer);	
					strcat(messageToSend, "'!\n");

					printf("Sending message to client '%s : %d' : %s\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), messageToSend);

					iResult = send(acceptedSocket, messageToSend, strlen(messageToSend) + 1, 0);

					if (iResult == SOCKET_ERROR)
					{
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(acceptedSocket);
						return 1;
					}
				}
				else if (option == 'D')
				{
					printf("\nClient '%s : %d' wants to disconnect.\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

					char *groupBelong = strtok(dataBuffer, ",");

					while (groupBelong != NULL) {
						if (hashtable_removesocket(ht, groupBelong, acceptedSocket)) {
								printf("Client '%s : %d' successfuly disconnect from the group '%s'!\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), groupBelong);
								list_socket* lista = hashtable_getsockets(ht, (groupBelong));
								list_print(lista->head, (groupBelong));
						}
						groupBelong = strtok(NULL, ",");
					}
					
					disconnected = true;
					closesocket(acceptedSocket);
					CloseHandle(thread[client]);
					clientNum--;
				}
				else if (option == 'S')
				{
					char* message,* group;
					group = strtok(dataBuffer, delimiter);
					message = strtok(NULL, delimiter);
					printf("Client wants to send message: %s\t to the group: %s\n", message, group);
					enqueue(getqueue(ht, (group)), (message));
				}
				else if(option == 'L')
				{
					if (groupNum > 0)
					{
						
						if (groupNum == 0)
						{
							char messageToSend[MAX_MESSAGE_LENGTH];
							strcpy(messageToSend, "No groups available at the moment.\n");
							iResult = send(acceptedSocket, messageToSend, strlen(messageToSend) + 1, 0);
							free(messageToSend);
						}
						else {
							char messageToSend[MAX_MESSAGE_LENGTH];
							strcpy(messageToSend, "List of all groups:\t");
							for (int i = 0; i < groupNum; i++) {
								strcat(messageToSend,  groups[i]);
								strcat(messageToSend, "\t");
							}
							strcat(messageToSend, "\n");

							iResult = send(acceptedSocket, messageToSend, strlen(messageToSend) + 1, 0);
						}
					}
					else
					{
						char* messageToSend = (char*)malloc(MAX_MESSAGE_SIZE);
						strcpy(messageToSend, "No groups available at the moment.\n");
						iResult = send(acceptedSocket, messageToSend, strlen(messageToSend) + 1, 0);
						free(messageToSend);
					}
				}
				else if (option == 'E')
				{
					printf("\nClient '%s : %d' wants to disconnect from the group: '%s'\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), dataBuffer);

					if (hashtable_removesocket(ht, dataBuffer, acceptedSocket)) {
						printf("Client '%s : %d' successfuly disconnect from the group '%s'!\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), dataBuffer);
						list_socket* lista = hashtable_getsockets(ht, (dataBuffer));
						list_print(lista->head, (dataBuffer));

						char* messageToSend = (char*)malloc(MAX_MESSAGE_SIZE);
						strcpy(messageToSend, "You have been successfully excluded from the desired group.\n");
						iResult = send(acceptedSocket, messageToSend, strlen(messageToSend) + 1, 0);
						free(messageToSend);
					}
					else {
						char* messageToSend = (char*)malloc(MAX_MESSAGE_SIZE);
						strcpy(messageToSend, "You are not a member of the desired group.\n");
						iResult = send(acceptedSocket, messageToSend, strlen(messageToSend) + 1, 0);
						free(messageToSend);
					}
				}
				else
				{
					printf("Somehow invalid message came from client...\n");
				}
				printf("\n");
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
	if (clientNum == 0)
	{
		printf("No more clients connected. Do you want to shutdown?\n");
		if (_getch() == 'y' || _getch() == 'Y')
		{
			printf("Shutting down the service...\n");
			shutDownService = true;
		}
		else if (_getch() == 'n' || _getch() == 'N')
		{
			printf("Service is still running...\n");
			shutDownService = false;
		}
	}
	return 0;
}

void deleteFromGroups(char* groupName) {
	for (int i = 0; i < groupNum; i++)
	{
		if (strcmp(groups[i], groupName) == 0)
		{
			for (int j = i; j < groupNum - 1; j++)
			{
				strcpy(groups[j], groups[j + 1]);
			}
			groupNum--;
			break;
		}
	}
}

//thread for group: checking if there is messages in the queue and sending it 
DWORD WINAPI SendMessageFromQueue(LPVOID lpParam) {
	char* group = (char*)lpParam;
	queue* groupQueue = getqueue(ht, group);
	list_socket* list = hashtable_getsockets(ht, group);
	listsocket_item* socketsInList;

	if (groupQueue == NULL || list == NULL) {
		printf("Error: Group or list not found for group '%s'\n", group);
		return 1;
	}

	while (1) {
		EnterCriticalSection(&(groupQueue->cs));
		if (groupQueue->head != NULL) {
			char* pom = dequeue(groupQueue);
			LeaveCriticalSection(&(groupQueue->cs));

			socketsInList = list->head;
			while (socketsInList != NULL) {
				char message[MAX_MESSAGE_LENGTH - 1];
				snprintf(message, sizeof(message), "%s#%s", group, pom);

				int iResult = send(socketsInList->socket, message, strlen(message) + 1, 0);
				if (iResult == SOCKET_ERROR) {
					printf("send failed with error: %d\n", WSAGetLastError());
					closesocket(socketsInList->socket);
					list_remove(list, socketsInList->socket);
				}
				socketsInList = socketsInList->next;
			}
			free(pom);
		}
		else {
			LeaveCriticalSection(&(groupQueue->cs));
			//Sleep(5000); 
		}
	}
	return 0;
}
