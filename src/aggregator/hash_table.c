#include "hash_table.h"
#include "libfds.h"

<<<<<<< HEAD
struct hash_table *
hash_table_init(struct hash_table *table, size_t table_size)
{
    table.list = (struct list *)calloc(table_size, sizeof(struct list));
    if (table.list == NULL){
        return NULL;
    }

	for (int i = 0; i < table_size; i++){
		table.list[i].head = NULL;
		table.list[i].tail = NULL;
	}

	table.size = table_size; //

    return table;
}

struct node * 
get_element(struct node *list, int index){
	
	int i = 0;
	struct node *tmp = list;

	while (i != index){
		tmp = tmp->next;
		i++;
	}
	return tmp;
}
=======
struct list *
hash_table_init(struct list *table) {

    table = (struct list *)calloc(DEF_TABLE_SIZE, sizeof(struct list));
    if (table == NULL){
        return NULL;
    }

    return table;
}

>>>>>>> 4f2cf3d2c00636c24ee0a264ed07d7775bf91c78

FDS_API
insert_key(struct hash_table *table, char *key, void *value) {

    if (table == NULL || key == NULL){
        return FDS_ERR_ARG;
    }

<<<<<<< HEAD
	// Make index to hash table
    int index = hash(key);

    // Extracting Linked List at a given index 
    struct node *list = (struct node*) table->list[index].head;

    // Creating an item to insert in the hash table 
	struct node *item = (struct node*) malloc(sizeof(struct node));
	if (item == NULL){
		return FDS_ERR_NOMEM;
	}
=======
    int index = hash(key);

    /* Extracting Linked List at a given index */
    struct node *list = (struct node*) table->list[index].head;

    /* Creating an item to insert in the Hash Table */
	struct node *item = (struct node*) malloc(sizeof(struct node));
>>>>>>> 4f2cf3d2c00636c24ee0a264ed07d7775bf91c78
	item->key = key;
	item->value = value;
	item->next = NULL;

<<<<<<< HEAD
	// Iserting key on index
    if (list == NULL){
		// Absence of Linked List at a given Index of hash table 
 		table->list[index].head = item;
		table->list[index].tail = item;
	} else {
        // A Linked List is present at given index of hash table 
		int find_index = find_key(list, key);
		if (find_index == -1){ 
			 // Key not found in existing linked list
			 // Adding the key at the end of the linked list
			table->list[index].tail->next = item;
			table->list[index].tail = item;
    	} else {
			// Key already present in linked list
			// Updating the value of already existing key
			struct node *element = get_element(list, find_index);
			element->value = value;
		}
	}

	return FDS_OK;
=======
    if (list == NULL){
		/* Absence of Linked List at a given Index of Hash Table */
 		table->list[index].head = item;
		table->list[index].tail = item;
	}
    else {
        /* A Linked List is present at given index of Hash Table */

		int find_index = find(list, key);
		if (find_index == -1)
                {
			/*
			 *Key not found in existing linked list
			 *Adding the key at the end of the linked list
			*/

			array[index].tail->next = item;
			array[index].tail = item;
			size++;
    }


}

FDS_API 
find(struct node *list, char* key){
	int retval = 0;
	struct node *temp = list;
	while (temp != NULL) {
		if (temp->key == key){
			return retval;
		}

  		temp = temp->next;
		retval++;
	}
	return -1;

>>>>>>> 4f2cf3d2c00636c24ee0a264ed07d7775bf91c78
}