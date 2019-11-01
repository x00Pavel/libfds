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


#include <assert.h> // static_assert
#include <libfds/api.h>
#include "aggr_local_header.h"
#include "hash_table.h"
#include "xxhash.h"


int 
hash_table_init(struct hash_table *table, size_t table_size)
{

	assert(table != NULL);
	assert(table_size > 0);

    table->list = (struct node**)calloc(table_size, sizeof(struct node*));
    if (table->list == NULL){
        return FDS_ERR_NOMEM;
    }

	for (int i = 0; i < table_size; i++){
		table->list[i]->next = NULL;
        // Linking to list 
	}
	table->size = table_size;

	return FDS_OK;
}


/** \brief Function finds the given key in the Linked List
 *
 * \param[in] list Poiter to linked list
 * \param[in] key  Key to be found
 *
 * \return In success, index of given key,
 *  return -1 if key is not present
 */
bool
find_key(const struct node *list, const char *key, size_t key_size, struct node *element)
{

	assert(list != NULL);
	assert(key != NULL);

	int index = 0;
	struct node *tmp = list;

	while (tmp != NULL){
		if(strncmp(key, list->data, key_size)){
            element = tmp;
			return true;
		}
		tmp = tmp->next;
	}

	return false;
}

/** \brief Function for generating hash
 *
 * \param[in] key      Base for hash function
 * \param[in] key_size Size of key
 *
 * \warning Be default, seed for hash function is 0
 *
 * \return Index to hash table
 */
unsigned long
hash_fnc(char *key, size_t key_size){return XXH64(key, key_size, 0);}

/** \brief Function for filling in key and value
 *
 * \param[in] table    Pointer to table
 * \param[in] key      Key to be inserted
 * \param[in] key_size Size of key
 * \param[in] value    Value for given key
 *
 * \return #FDS_OK on success
 */
int 
get_element(const fds_aggr_t *memory, struct node **res)
{
	int ret_code = 0;
	// Make index to hash table
    const unsigned long index = hash_fnc(memory->key, memory->key_size);

    // Extracting Linked List at a given index
    struct node *list = (struct node*) memory->table->list[index];
    struct node *element;

    if (list == NULL){
        // This is the first element in list
        size_t node_size = offsetof(struct node, data) + memory->key_size + memory->val_size;
        element = (struct node *) malloc(node_size);
        if (element == NULL) {
            hash_table_clean(memory->table);
            return HASH_ERR;
        }
        memcpy(element->data, memory->key, memory->key_size);
        element->next = NULL;
        *res = element;
        return HASH_NEW;
    }
    // Try to find key in list
    else if(find_key(list, memory->key, memory->key_size, element)){
        *res = element;
        return HASH_FOUND;
    }
    // Try to insert to not empty list 
    else{
        size_t node_size = offsetof(struct node, data) + memory->key_size + memory->val_size;
        element = (struct node *) malloc(node_size);
        if (element == NULL) {
            hash_table_clean(memory->table);
            return HASH_ERR;
        }
        memcpy(element->data, memory->key, memory->key_size);
        element->next = list;
        memory->table->list[index] = element;
        *res = element;
        return HASH_NEW;
    }
/*
	// Insert key
    strncpy(item->key, memory->key, memory->key_size);
    
    // Insert all value fields
    *item->val_fields = (struct field *) malloc (sizeof(struct field) * memory->val_count + memory->val_size); 
    strncpy(*item->val_fields, memory->val_list,
        sizeof(struct field) * memory->val_count + memory->val_size);
    
    *item->next = NULL;

    // Insetting key on index
    if (list == NULL) {
        // Absence of Linked List at a given Index of hash table
        memory->table->list[index].head = item;
        memory->table->list[index].tail = item;
	} else {
        // A Linked List is present at given index of hash table
		int find_index = find_key(list, memory->key);
		if (find_index == FDS_ERR_NOTFOUND){
			 // Key not found in existing linked list
			 // Adding the key at the end of the linked list
             memory->table->list[index].tail->next = item;
             memory->table->list[index].tail = item;
        } else {
			// Key already present in linked list
			// Updating the value of already existing key
			struct node *element = get_element(list, find_index);

			aggr_function fn;
			// Do propriety function with field
			for (int i = 0; i < memory->val_count; i++){
				fn = get_function(&memory->val_list[i]);

			 	ret_code = fn(&memory->val_list[i], element->val_fields[i]);

				if(ret_code != FDS_OK){
					return ret_code;
				}
			}
		}
	}
*/
	return FDS_OK;
}

void
hash_table_clean(struct hash_table *table)
{
	assert(table != NULL);

	for (int i = 0; i < table->size; i++){
        // Take first item in linked list
        struct node *item = table->list[i];
        if(item){
            while(true){
                struct node* tmp = item->next;
                free(item);
                if(tmp != NULL){
                    item = tmp;
                }
                else{
                    break;
                }
            }
            if (table->list[i]){
                free(table->list[i]);
            }
        }
        else{
            continue;
        }        
	}

}
