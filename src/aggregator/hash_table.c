#include "hash_table.h"
#include "libfds.h"

struct list *
hash_table_init(struct list *table) {

    table = (struct list *)calloc(DEF_TABLE_SIZE, sizeof(struct list));
    if (table == NULL){
        return NULL;
    }

    return table;
}


FDS_API
insert_key(struct hash_table *table, char *key, void *value) {

    if (table == NULL || key == NULL){
        return FDS_ERR_ARG;
    }

    int index = hash(key);

    /* Extracting Linked List at a given index */
    struct node *list = (struct node*) table->list[index].head;

    /* Creating an item to insert in the Hash Table */
	struct node *item = (struct node*) malloc(sizeof(struct node));
	item->key = key;
	item->value = value;
	item->next = NULL;

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

}