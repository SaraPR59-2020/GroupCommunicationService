#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <WinSock2.h>

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "conio.h"


#define QUEUE_EMPTY "QUEUE IS EMPTY"
#define MAX_MESSAGE_SIZE 256


#define MAX_GROUP_NAME 256
#define HASH_TABLE_SIZE 10
#define MAX_SOCKETS_IN_GROUP 10

#pragma region list_structs
typedef struct list_socket_item {
	SOCKET socket;
	struct list_socket_item* next;
} list_socket_item;
//zna se lista konkretna sa nekim properijima kao 
typedef struct list_socket {
	list_socket_item* head;
	int len;
	int limit;
} list_socket;
#pragma endregion list_structs

#pragma region queue_structs
typedef struct node {
	char* message;
	struct node* next;
} node;

typedef struct queue {
	CRITICAL_SECTION cs;
	node* head;
	node* tail;
} queue;
#pragma endregion queue_structs

#pragma region hash_structs
typedef struct hashtable_item {
	char* group_name;
	list_socket* sockets;
	queue* group_queue;
	struct hashtable_item* next;
} hashtable_item;
//nasa tabela....
typedef struct {
	CRITICAL_SECTION cs;
	hashtable_item* items;
} hash_table;
#pragma endregion hash_structs

list_socket* init_list();
bool list_add(list_socket* list, SOCKET sock);

void init_queue(queue* q);

unsigned int hash(char* group_name);
hash_table* init_hash_table();
bool hashtable_addgroup(hash_table* ht, char* group_name);
bool hashtable_addsocket(hash_table* ht, char* group_name, SOCKET new_socket);
bool hashtable_findgroup(hash_table* ht, char* group_name);