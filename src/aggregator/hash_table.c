/**
 * \file src/aggregator/hash_table.c
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \author Pavel Yadlouski <xyadlo00@vutbr.cz>
 * \brief Functions for operating with hash table (source code)
 * \date July 2019
 */

/*
 * Copyright (C) 2016-2019 CESNET, z.s.p.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is``, and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */


#include <libfds.h>
#include <assert.h>

#include "hash_table.h"
#include "xxhash.h"

typedef int (*aggr_function)(struct context *, const struct field *);

/** \brief Find a function for value field
  * \param[in] field Field to process
  * 
  * \return Conversion function
  */
aggr_function 
get_function(const struct field *field)
{
    // Conversion table, based on types defined by enum fds_iemgr_element_type
    static const aggr_function table[] = {
        &aggr_sum,
		&aggr_max,
		&aggr_min,
		&aggr_or
    };

    const size_t table_size = sizeof(table) / sizeof(table[0]);
    const enum fds_aggr_function function = field->fnc;

    return table[function];
}

FDS_API
hash_table_init(struct hash_table *table, size_t table_size)
{
	
	assert(table != NULL);
	assert(table_size > 0);

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
get_element(struct node *list, int index)
{
	
	assert(list != NULL);

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

	assert(list != NULL);
	assert(key != NULL);

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
hash_fnc(char *key, size_t key_size){return XXH64(key, key_size, 0)}


FDS_API
insert_key(const struct fds_aggr_memory *memory)
{
	int ret_code = 0;
	// Make index to hash table
    const unsigned long index = hash_fnc(memory->key, memory->key_size);

    // Extracting Linked List at a given index 
    const struct node *list = (struct node*) memory->table->list[index].head;

    // Creating an item to insert in the hash table 
	const struct node *item = (struct node*) malloc(sizeof(struct node));
	if (item == NULL){
		// hash_table_clean(table);
		return FDS_ERR_NOMEM;
	}

	// Insert key
	item->key = memore->key;
	
	// Isert all value fields
	for (int i = 0; i < memory->val_count; i++){
		item->value[i] = memory->val_list[i].value;
	}
	
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

			aggr_function fn;
			// Do propriet function with field
			for (int i = 0; i < memory->val_count; i++){
				fn = get_function(memory->val_list[i]);
				ret_code = fn(memory->val_lsit[i].value, element->value);
				if(ret_code != FDS_Ok){
					return ret_code;
				}
			}
		}
	}

	return FDS_OK;
}

void
hash_table_clean(struct hash_table *table)
{

	assert(table != NULL);

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