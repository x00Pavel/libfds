#include <libfds.h>

#include "hash_table.h"
#include "xxhash.h"

FDS_API
hash_table_init(struct hash_table *table, size_t table_size){
	
	assert(table != NULL);

    table.list = (struct list *)calloc(table_size, sizeof(struct list));
    if (table.list == NULL){
        return FDS_ERR_NOMEM;
    }

	for (int i = 0; i < table_size; i++){
		table.list[i].head = NULL;
		table.list[i].tail = NULL;
	}
	table.size = table_size;


	return FDS_OK; 
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

FDS_API
find_key(struct node *list, char *key){

	int index = 0;
	struct node *tmp = list;

	while (tmp != NULL){
		if(tmp->key = key){
			return index;
		}
		tmp = tmp->next;
		index++;
	}

	return FDS_ERR_NOTFOUND;
}

unsigned long
hash_fnc(char *key, size_t key_size){
	return XXH64(key, key_size, 0)
}

FDS_API
insert_key(struct hash_table *table, char *key, size_t key_size, void *value) {

    if (table == NULL || key == NULL){
        return FDS_ERR_ARG;
    }

	// Make index to hash table
    unsigned long index = hash_fnc(key, key_size);

    // Extracting Linked List at a given index 
    struct node *list = (struct node*) table->list[index].head;

    // Creating an item to insert in the hash table 
	struct node *item = (struct node*) malloc(sizeof(struct node));
	if (item == NULL){
		hash_table_clean(table);
		return FDS_ERR_NOMEM;
	}
	item->key = key;
	item->value = value;
	item->next = NULL;

	// Iserting key on index
    if (list == NULL){
		// Absence of Linked List at a given Index of hash table 
 		table->list[index].head = item;
		table->list[index].tail = item;
	} else {
        // A Linked List is present at given index of hash table 
		int find_index = find_key(list, key);
		if (find_index == FDS_ERR_NOTFOUND){ 
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
}

void
hash_table_clean(struct hash_table *table){

	for (int i = 0; i < table->size; i++){

        struct list *tmp_list = table->list[i];
		struct node *tmp_node = tmp_list[tmp_index].head;

		while(tmp_node != NULL){
			struct node *tmp_next = tmp_node.next;
			free(tmp_node);
			tmp_node = tmp_next;
		}

        free(table->list[i]);
    }

}