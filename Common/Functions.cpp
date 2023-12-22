#include "framework.h"
#include "pch.h"
#include "functions.h"

list_socket* init_list() {
	list_socket* socks = (list_socket*)malloc(sizeof(list_socket));
	socks->head = NULL;
	socks->limit = MAX_SOCKETS_IN_GROUP;
	socks->len = 0;

	return socks;
}

bool list_add(list_socket* list, SOCKET sock) {
	list_socket_item* temp_ = list->head;
	if (list->len < list->limit) {
		list_socket_item* new_item = (list_socket_item*)malloc(sizeof(list_socket_item));
		new_item->socket = sock;
		new_item->next = NULL;

		list_socket_item* current = list->head;
		if (current == NULL) {
			current = new_item;
			list->head = current;
			list->len++;
			return true;
		}
		else {
			while (current->next != NULL) {
				current = current->next;
			}
			current->next = new_item;
			list->len++;
		}
		list->head = temp_;
	}
	else {
		printf("WARINING: List is full.\n");
		return false;
	}
	return true;
}

void init_queue(queue* q) {
	q->head = NULL;
	q->tail = NULL;

	InitializeCriticalSection(&(q->cs));
}

unsigned int hash(char* group_name) {
	unsigned int hash = 5381;
	int c;
	while (c = *group_name++) {
		hash = ((hash << 5) + hash) + c;
	}
	return hash % HASH_TABLE_SIZE;
}

hash_table* init_hash_table() {
	hash_table* ht = (hash_table*)malloc(sizeof(hash_table));
	ht->items = (hashtable_item*)malloc(sizeof(hashtable_item) * HASH_TABLE_SIZE);


	for (int i = 0; i < HASH_TABLE_SIZE; i++) {
		ht->items[i].sockets = NULL;
		ht->items[i].group_name = (char*)malloc(MAX_GROUP_NAME);
		ht->items[i].group_queue = (queue*)malloc(sizeof(queue*));
		ht->items[i].next = NULL;
	}

	InitializeCriticalSection(&ht->cs);

	return ht;
}

bool addgroup_temp(hash_table* ht, hashtable_item* item, char* group_name) {
	if (strcpy_s(item->group_name, MAX_GROUP_NAME, group_name) != 0) {
		printf("ERROR: Adding new group to hash table failed. :(\n");
		return false;
	}
	item->sockets = init_list();
	if (item->sockets == NULL) {
		printf("ERROR: Adding new list of sockets to the group in hash table failed. :(\n");
		return false;
	}
	init_queue(item->group_queue);

	return true;
}
bool hashtable_addgroup(hash_table* ht, char* group_name) {
	int index = hash(group_name);
	bool ret = false;
	EnterCriticalSection(&ht->cs);
	hashtable_item* item = &(ht->items[index]);

	if (item->sockets == NULL) {
		ret = addgroup_temp(ht, item, group_name);
	}
	else {
		printf("WARNING: Collision in hash table.\n");
		while (1) {
			if (item->next == NULL) {
				item->next = (hashtable_item*)malloc(sizeof(hashtable_item));
				item->next->next = NULL;
				item->next->sockets = NULL;
				item->next->group_name = (char*)malloc(MAX_GROUP_NAME);
				item->next->group_queue = (queue*)malloc(sizeof(queue*));

				ret = addgroup_temp(ht, item->next, group_name);
				break;
			}
			else {
				item = item->next;
			}
		}
	}
	LeaveCriticalSection(&ht->cs);
	return ret;
}
bool hashtable_findgroup(hash_table* ht, char* group_name) {
	int index = hash(group_name);

	EnterCriticalSection(&ht->cs);
	hashtable_item* item = &(ht->items[index]);
	if (item->sockets == NULL) {
		LeaveCriticalSection(&ht->cs);
		return false;
	}
	LeaveCriticalSection(&ht->cs);
	return true;
}

bool hashtable_addsocket(hash_table* ht, char* group_name, SOCKET new_socket) {
	int index = hash(group_name);

	EnterCriticalSection(&ht->cs);
	hashtable_item* item = &(ht->items[index]);
	if (item->sockets == NULL) {
		printf("ERROR: Group doesn't exist in hash table.\n");
		LeaveCriticalSection(&ht->cs);
		return false;
	}

	bool ret = list_add(item->sockets, new_socket);

	LeaveCriticalSection(&ht->cs);
	return ret;
}
