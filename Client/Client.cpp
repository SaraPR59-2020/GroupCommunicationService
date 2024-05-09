// Client.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include "conio.h"
#include <charconv>
#include <mutex>
using namespace std;

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma warning (disable: 4996)

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 33033
#define BUFFER_SIZE 256
#define MAX_GROUP_NAME 20
#define MAX_MESSAGE_LENGTH 25
#define MAX_NUMBER_GROUPS 10

DWORD WINAPI ThreadRECV(LPVOID lpParam);
void Connect(char* queueName);
void ExitGroup(char* queueName);
void Disconnect();
void SendMessageToPass(char* message);
void GetCurrentListOfGroups();
void printAllGroups();
void deleteFromGroups(char* groupName);
bool contains_hash(const char* str);
bool isInGroup(char* groupName);
bool isInAny();

SOCKET connectSocket;
char groups[MAX_NUMBER_GROUPS][MAX_GROUP_NAME];
int groupCounter = 0;

HANDLE hRecv;
HANDLE hMutex;
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
		printf("\n\tSuccasfuly conected to service! \n");
	}

	iResult = recv(connectSocket, databuffer, BUFFER_SIZE, 0);
	printf("\n===== Service: %s\n", databuffer);

	printf("\t\t\t\tGROUP COMMUNICATION SERVICE\n");
	
	//non-blocking mode
	u_long mode = 1;
	iResult = ioctlsocket(connectSocket, FIONBIO, &mode);
	if (iResult != NO_ERROR) {

		printf("ioctlsocket failed with error: %ld\n", iResult);
		return false;
	}

	//mutex for crirical section
	hMutex = CreateMutex(NULL, FALSE, NULL);

	//thread for news
	hRecv = CreateThread(NULL, NULL, &ThreadRECV, &connectSocket, NULL, NULL);

	bool doWhile = true;

	while (doWhile)
	{
		int optin = 0;

		WaitForSingleObject(hMutex, INFINITE);
		printf("\n\t1. ENTER GROUP\n");
		printf("\t2. SEND MESSAGE TO THE GROUP\n");
		printf("\t3. PRINT ALL GROUPS THAT YOU ARE PART OF\n");
		printf("\t4. DISCONNECT FROM THE GROUP\n");
		printf("\t5. SEE ALL CURRENT AVAILABLE GROUPS\n");
		printf("\t6. EXIT SERVICE\n");
		ReleaseMutex(hMutex);

		char option = _getch();
		char delimiter[] = "#";
		//char groupName[MAX_MESSAGE_LENGTH];
		switch (option - 48) {
			case 1:
				if (groupCounter == MAX_NUMBER_GROUPS)
				{
					printf("You are already part of maximum number of groups, please leave some group first...\n");
					break;
				}

				printf("Enter name of group chat (min three characters): \t");
				char queueName[MAX_GROUP_NAME];
				WaitForSingleObject(hMutex, INFINITE);
				gets_s(queueName, MAX_GROUP_NAME - 1);
				ReleaseMutex(hMutex);

				if (groupCounter > 0)
				{
					if (isInGroup(queueName))
					{
						printf("You are already part of group '%s', please enter another group...\n", queueName);
						break;
					}
					else
					{
						messageLen = strlen(queueName);
						if (messageLen <= 2) {
							printf("Invalid type of input, try agian...\n");
							break;
						}
						else
						{
							Connect(queueName);
							strcpy_s(groups[groupCounter], MAX_GROUP_NAME, queueName);
							groupCounter++;
						}
					}
					break;
				}
				else
				{
					messageLen = strlen(queueName);
					if (messageLen <= 2) {
						printf("Invalid type of input, try agian...\n");
						break;
					}
					else
					{
						strcpy_s(groups[groupCounter], MAX_GROUP_NAME, queueName);
						groupCounter++;
						Connect(queueName);

					}
					break;
				}
				break;
			case 2:
				if (!isInAny()) {
					break;
				}
				char groupName[MAX_GROUP_NAME];
				printf("Enter group in witch you want to send message:\t");
				WaitForSingleObject(hMutex, INFINITE);
				gets_s(groupName, MAX_MESSAGE_LENGTH - 1);
				ReleaseMutex(hMutex);
				if(isInGroup(groupName))
				{ 
					printf("Enter message (do not enter '#' - it will be deleted from message): \t");
					char input[MAX_MESSAGE_LENGTH];
					WaitForSingleObject(hMutex, INFINITE);
					gets_s(input, MAX_MESSAGE_LENGTH - 1);
					ReleaseMutex(hMutex);

					char message[MAX_MESSAGE_LENGTH];
					if (strpbrk(input, "#") != NULL)
					{
						char* before, * after;
						before = strtok(input, delimiter);
						after = strtok(NULL, delimiter);

						strcpy(message, groupName);
						strcat(message, "#");
						if(before != NULL) strcat(message, before);
						if(after != NULL) strcat(message, after);
						strcat(message, "S");
					}
					else
					{
						strcpy(message, groupName);
						strcat(message, "#");
						strcat(message, input);
						strcat(message, "S");
					}
					printf("Sending message to service...\n");
					SendMessageToPass(message);
				}
				else
				{
					printf("You are not part of group '%s', please enter group first...\n", groupName);
				}
				break;
			case 3:
				if (!isInAny()) {
					break;
				}
				printAllGroups();
				break;
			case 4:
				if (!isInAny()) {
					break;
				}
				char groupNametToLeave[MAX_GROUP_NAME];
				printf("Enter group you want to leave: \t");
				WaitForSingleObject(hMutex, INFINITE);
				gets_s(groupNametToLeave, MAX_GROUP_NAME - 1);
				ReleaseMutex(hMutex);
						
				if(isInGroup(groupNametToLeave))
				{
					ExitGroup(groupNametToLeave);
					deleteFromGroups(groupNametToLeave);
				}
				else
				{
					printf("You are not part of group '%s', please enter group first...\n", groupNametToLeave);
				}
				break;
			case 5:
				GetCurrentListOfGroups();
				Sleep(1000);
				break;
			case 6:
				Disconnect();
				doWhile = false;
				shutingDown = true;	
				break;
			default:
				printf("Invalid type of input, try agian...\n");
				break;
		}
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

	WaitForSingleObject(hMutex, INFINITE);	//wait for thread to finish and then close handle
	CloseHandle(hMutex);
	WaitForSingleObject(hRecv, INFINITE);
	CloseHandle(hRecv);
	closesocket(connectSocket);

	if (WSACleanup() != 0)
	{
		printf("WSACleanup faild with error: %d", WSAGetLastError());
		return 1;
	}
	return 0;
}
void printAllGroups() {
	printf("\n\tList of all groups:\n");
	for (int i = 0; i < groupCounter; i++) {
		printf("\t\tGroup %d: %s\n", i + 1, groups[i]);
	}
}
void deleteFromGroups(char* groupName) {
	for (int i = 0; i < groupCounter; i++)
	{
		if (strcmp(groups[i], groupName) == 0)
		{
			for (int j = i; j < groupCounter - 1; j++)
			{
				strcpy(groups[j], groups[j + 1]);
			}
			groupCounter--;
			break;
		}
	}
}

bool isInAny() {
	if (groupCounter == 0) {
		printf("You are not part of any group, please enter group first...\n");
		return false;
	}
	return true;
}

bool isInGroup(char* groupName) {
	bool isGroup = false;
	for (int i = 0; i < groupCounter; i++)
	{
		if (strcmp(groups[i], groupName) == 0)
		{
			return true;
		}
	}
	return isGroup;
}
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
void ExitGroup(char* queueName) {
	FD_SET set;
	timeval timeVal;
	FD_ZERO(&set);
	FD_SET(connectSocket, &set);
	timeVal.tv_sec = 0;
	timeVal.tv_usec = 0;

	char message[MAX_MESSAGE_LENGTH];
	strcpy_s(message, queueName);
	strcat(message, "E");

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
void Disconnect() {
	FD_SET set;
	timeval timeVal;
	FD_ZERO(&set);
	FD_SET(connectSocket, &set);
	timeVal.tv_sec = 0;
	timeVal.tv_usec = 0;

	char message[MAX_MESSAGE_LENGTH];

	strcpy(message, groups[0]);
	for (int i = 1; i < groupCounter; i++) {
		strcat(message, ",");
		strcat(message,groups[i]);
		
	}
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
void GetCurrentListOfGroups() {
	FD_SET set;
	timeval timeVal;
	FD_ZERO(&set);
	FD_SET(connectSocket, &set);
	timeVal.tv_sec = 0;
	timeVal.tv_usec = 0;

	char message[MAX_MESSAGE_LENGTH];
	strcpy_s(message, "L");

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

		if (iResult == SOCKET_ERROR) {
			serverOut = true;
			printf("select failed: %ld\n", WSAGetLastError());
			printf("Service was probably closed\n");
			CloseHandle(hRecv);
			return 0;
		}
		else if (iResult == 0) {
			//Sleep(1000);
			continue;
		}
		else if (iResult > 0) {
			WaitForSingleObject(hMutex, INFINITE);
			iResult = recv(connectSocketRECV, dataBuffer, BUFFER_SIZE, 0);
			if (iResult > 0) {
				if (contains_hash(dataBuffer))
				{
					char* before, * after;
					before = strtok(dataBuffer, "#");
					after = strtok(NULL, "#");
					printf("===== News from service: New message in group '%s' : %s\n", before, after);
				}
				else {
					printf("===== News from service: %s\n", dataBuffer);
				}
			}
			else {
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(connectSocketRECV);
			}
			ReleaseMutex(hMutex);
		}
	}
	return 0;
}

bool contains_hash(const char* str) {
	while (*str != '\0') {
		if (*str == '#') {
			return true;
		}
		str++; // Move to the next character
	}
	return false;
}