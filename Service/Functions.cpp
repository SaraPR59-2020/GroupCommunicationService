#include "Functions.h"

#define WIN32_LEAN_AND_MEAN 

list_socket* init_list() {
	list_socket* socks = (list_socket*)malloc(sizeof(list_socket));
	socks->head = NULL;
	socks->limit = MAX_SOCKETS_IN_GROUP;
	socks->len = 0;

	return socks;
}

bool list_add(list_socket* list, SOCKET sock) {
	listsocket_item* temp_ = list->head;
	if (list->len < list->limit) {
		listsocket_item* new_item = (listsocket_item*)malloc(sizeof(listsocket_item));
		new_item->socket = sock;
		new_item->next = NULL;

		listsocket_item* current = list->head;
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

void list_print(listsocket_item* head, char* group) {
	listsocket_item* tmp = head;
	if (head == NULL)
	{
		printf("There is not any SOCKETS in the '%s'!\n", group);
		return;
	}printf("SOCKETS in '%s' \t", group);
	while (tmp != NULL) {
		if (tmp->next == NULL) {
			printf("%d\t", (int)tmp->socket);
		}
		else {
			printf("%d\t", (int)tmp->socket);
		}
		tmp = tmp->next;
	}
	printf("\n");
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

//
hash_table* init_hash_table() {
	hash_table* ht = (hash_table*)malloc(sizeof(hash_table));

	if (ht == NULL) {
		printf("ERROR: Hash table initialization failed. :(\n");
		return NULL;
	}

	for (int i = 0; i < HASH_TABLE_SIZE; i++) {
		ht->items[i].group_name = (char*)malloc(MAX_GROUP_NAME * sizeof(char));
		if (ht->items[i].group_name == NULL) {
			// Handle memory allocation failure
			/*for (int j = 0; j < i; j++) {
				free(ht->items[j].group_name);
			}*/
			// Cleanup already allocated memory before returning NULL
			//free(ht);
			return NULL;
		}
		ht->items[i].sockets = init_list();
		ht->items[i].added = false;
		ht->items[i].group_queue = (queue*)malloc(sizeof(queue*));
		if (ht->items[i].group_queue == NULL) {
			// Handle memory allocation failure
			/*for (int j = 0; j < i; j++) {
							free(ht->items[j].group_name);
			}*/
			// Cleanup already allocated memory before returning NULL
			//free(ht);
			return NULL;
		}
		ht->items[i].next = NULL;
	}

	InitializeCriticalSection(&ht->cs);
	return ht;
}

bool addgroup_temp(hash_table* ht, hashtable_item* item, char* group_name) {
	strcpy(item->group_name, group_name);
	//item->sockets = init_list();
	item->added = true;
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

	if (item->added == false) {						//if first item in the list is empty - is not added yet, only initialized
		ret = addgroup_temp(ht, item, group_name);
	}
	else {
		printf("WARNING: Collision in hash table.\n");
		while (1) {										//try to find empty space in the list if there was a collision
			if (item->next == NULL) {					//if there is no next item in the list	
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
	if (item != NULL && strcmp(item->group_name, group_name) == 0) {	//group already exists
		LeaveCriticalSection(&ht->cs);
		return true;
	}
	else
	{													//group does not exist on this index
		while (item->next != NULL) {					//try to find group in the list if there was a collision
			if (strcmp(item->group_name, group_name) == 0) {
				LeaveCriticalSection(&ht->cs);
				return true;
			}
			item = item->next;
		}
	
	}
	LeaveCriticalSection(&ht->cs);
	return false;
}

bool hashtable_addsocket(hash_table* ht, char* group_name, SOCKET new_socket) {
	int index = hash(group_name);
	bool ret = false;

	EnterCriticalSection(&ht->cs);
	hashtable_item* item = &(ht->items[index]);
	if (item != NULL || strcmp(item->group_name, group_name) == 0) {
		
		ret = list_add(item->sockets, new_socket);
		LeaveCriticalSection(&ht->cs);
		return ret;
	}
	else
	{
		while (item->next != NULL) {
			if (strcmp(item->group_name, group_name) == 0) {
				
				ret = list_add(item->sockets, new_socket);
				LeaveCriticalSection(&ht->cs);
				return ret;
			}
			item = item->next;
		}

	}

	LeaveCriticalSection(&ht->cs);
	return ret;
}

bool list_remove(list_socket* list, SOCKET sock) {
	listsocket_item* prev = list->head;
	listsocket_item* temp = prev->next;

	if (prev == NULL) {
		printf("WARINING: List is empty.\n");
		return false;
	}
	if (prev->socket == sock)
	{
		list->head = temp;
		free(prev);
		list->len--;
		return true;
	}
	while (temp != NULL) {
		if (temp->socket == sock) {
			prev->next = temp->next;
			free(temp);
			list->len--;
			return true;
		}
		prev = temp;
		temp = temp->next;
	}
	return false;
}

bool hashtable_removesocket(hash_table* ht, char* group_name, SOCKET socket) {
	int index = hash(group_name);
	bool ret = false;

	EnterCriticalSection(&ht->cs);
	hashtable_item* item = &(ht->items[index]);
	if (strcmp(item->group_name, group_name) == 0) {
		
		ret = list_remove(item->sockets, socket);
		LeaveCriticalSection(&ht->cs);
		return ret;
	}
	else
	{
		while (item->next != NULL) {
			if (strcmp(item->group_name, group_name) == 0) {
				
				ret = list_remove(item->sockets, socket);
				LeaveCriticalSection(&ht->cs);
				return ret;
			}
			item = item->next;
		}

	}
	LeaveCriticalSection(&ht->cs);
	return ret;
}

list_socket* hashtable_getsockets(hash_table* ht, char* group_name) {
	int index = hash(group_name);

	EnterCriticalSection(&ht->cs);
	hashtable_item* item = &(ht->items[index]);
	if (strcmp(item->group_name, group_name) == 0) {
		LeaveCriticalSection(&ht->cs);
		return item->sockets;
	}
	else
	{
		while (item->next != NULL) {
			if (strcmp(item->group_name, group_name) == 0) {
				LeaveCriticalSection(&ht->cs);
				return item->sockets;
			}
			item = item->next;
		}

	}
	LeaveCriticalSection(&ht->cs);
	return NULL;
}

queue* getqueue(hash_table* ht, char* group_name) {
	int index = hash(group_name);

	EnterCriticalSection(&ht->cs);
	hashtable_item* item = &(ht->items[index]);
	if (strcmp(item->group_name, group_name) == 0) {
		LeaveCriticalSection(&ht->cs);
		return item->group_queue;
	}
	else
	{
		while (item->next != NULL) {
			if (strcmp(item->group_name, group_name) == 0) {
				LeaveCriticalSection(&ht->cs);
				return item->group_queue;
			}
			item = item->next;
		}

	}
	LeaveCriticalSection(&ht->cs);

	return NULL;
}


bool enqueue(queue* q, char* message) {
	EnterCriticalSection(&(q->cs));
	node* new_node = (node*)malloc(sizeof(node));
	new_node->message = (char*)malloc(sizeof(char) * MAX_MESSAGE_SIZE);
	new_node->next = NULL;
	if (new_node == NULL) {
		LeaveCriticalSection(&(q->cs));
		return false;
	}
	strcpy_s(new_node->message, MAX_MESSAGE_SIZE, message);
	new_node->next = NULL;

	if (q->tail != NULL) {
		q->tail->next = new_node;
	}

	q->tail = new_node;

	if (q->head == NULL) {
		q->head = new_node;
	}
	LeaveCriticalSection(&(q->cs));
	return true;
}
char* dequeue(queue* q) {
	EnterCriticalSection(&(q->cs));
	if (q->head == NULL) {
		LeaveCriticalSection(&(q->cs));
		return (char*)QUEUE_EMPTY;
	}
	node* tmp = q->head;
	char* result = q->head->message;
	q->head = q->head->next;

	if (q->head == NULL) {
		q->tail = NULL;
	}

	LeaveCriticalSection(&(q->cs));
	free(tmp);
	return result;
}