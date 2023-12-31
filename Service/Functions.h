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
	typedef struct listsocket_item {
		SOCKET socket;
		struct listsocket_item* next;
	} listsocket_item;
	typedef struct list_socket {
		listsocket_item* head;
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
	typedef struct {
		CRITICAL_SECTION cs;
		hashtable_item* items;
	} hash_table;
#pragma endregion hash_structs

	list_socket* init_list();	//functions for conection action and communication thread
	bool list_add(list_socket* list, SOCKET sock);	//functions for conection action and communication thread
	void list_print(listsocket_item* head, char* group);	//functions for conection action and communication thread

	bool list_remove(list_socket* list, SOCKET sock);	//for deconnection

	//functions for conection action and communication thread
	void init_queue(queue* q);

	unsigned int hash(char* group_name);
	hash_table* init_hash_table();
	bool hashtable_addgroup(hash_table* ht, char* group_name);
	bool hashtable_addsocket(hash_table* ht, char* group_name, SOCKET new_socket);
	bool hashtable_findgroup(hash_table* ht, char* group_name);

	//for deconnection
	bool hashtable_removesocket(hash_table* ht, char* group_name, SOCKET socket);
	list_socket* hashtable_getsockets(hash_table* ht, char* group_name);

	//adding message to queue
	bool enqueue(queue* q, char* message);
	queue* getqueue(hash_table* ht, char* group_name);
	char* dequeue(queue* q);