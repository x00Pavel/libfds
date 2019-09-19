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

#include "hash_table.h"
#include "xxhash.h"

typedef int (*aggr_function)(const struct field *);

/**
 * \brief Function for making sum of values
 *
 * \param[in] src Pointer to sources value field
 * \param[out] dst Poiter to destenation value field
 *
 * \return #FDS_OK on success
*/
int
aggr_sum(const struct field *src, struct field *dst)
{
	assert(src != NULL);
	assert(dst != NULL);

    switch (src->type) {
    case FDS_AGGR_UNSIGNED_8:
        dst->value->uint8 += src->value->uint8;
        break;
    case FDS_AGGR_UNSIGNED_16:
        dst->value->uint16 += src->value->uint16;
        break;
    case FDS_AGGR_UNSIGNED_32:
        dst->value->uint32 += src->value->uint32;
        break;
    case FDS_AGGR_UNSIGNED_64:
        dst->value->uint64 += src->value->uint64;
        break;
    case FDS_AGGR_SIGNED_8:
        dst->value->int8 += src->value->int8;
        break;
    case FDS_AGGR_SIGNED_16:
        dst->value->int16 += src->value->int16;
        break;
    case FDS_AGGR_SIGNED_32:
        dst->value->int32 += src->value->int32;
        break;
    case FDS_AGGR_SIGNED_64:
        dst->value->int64 += src->value->int64;
        break;
    case FDS_AGGR_DOUBLE:
        dst->value->dbl += src->value->dbl;
        break;
    default:
        // Hete must be error handling
        return FDS_ERR_NOTFOUND;
    }

    return FDS_OK;
}

/**
 * \brief Function for choosing maximum from two values
 *
 * \param[in] src Pointer to sources value field
 * \param[out] dst Poiter to destenation value field
 *
 * \return #FDS_OK on success
 */
int
aggr_max(const struct field *src, struct field *dst)
{
    assert(src != NULL);
    assert(dst != NULL);

    switch (src->type) {
    case FDS_AGGR_UNSIGNED_8:
        if (src->value->uint8 > dst->value->uint8)
			dst->value->uint8 = src->value->uint8;
        break;
    case FDS_AGGR_UNSIGNED_16:
        if (src->value->uint8 > dst->value->uint8)
            dst->value->uint16 = src->value->uint16;
        break;
    case FDS_AGGR_UNSIGNED_32:
        if (src->value->uint32 > dst->value->uint32)
            dst->value->uint32 = src->value->uint32;
        break;
    case FDS_AGGR_UNSIGNED_64:
        if (src->value->uint64 > dst->value->uint64)
            dst->value->uint64 = src->value->uint64;
        break;
    case FDS_AGGR_SIGNED_8:
        if (src->value->int8 > dst->value->int8)
			dst->value->int8 = src->value->int8;
        break;
    case FDS_AGGR_SIGNED_16:
        if (src->value->int16 > dst->value->int16)
			dst->value->int16 = src->value->int16;
        break;
    case FDS_AGGR_SIGNED_32:
        if (src->value->int32 > dst->value->int32)
            dst->value->int32 = src->value->int32;
        break;
    case FDS_AGGR_SIGNED_64:
        if (src->value->int64 > dst->value->int64)
            dst->value->int64 = src->value->int64;
        break;
    case FDS_AGGR_DOUBLE:
        if (src->value->dbl > dst->value->dbl)
            dst->value->dbl = src->value->dbl;
        break;
    case FDS_AGGR_DATE_TIME_NANOSECONDS:
        if (src->value->timestamp > dst->value->timestamp)
            dst->value->timestamp = src->value->timestamp;
        break;
    default:
        // Hete must be error handling
        return FDS_ERR_NOTFOUND;
    }

    return FDS_OK;
}

/**
 * \brief Function for choosing minimum from two values
 *
 * \param[in] src Pointer to sources value field
 * \param[out] dst Poiter to destenation value field
 *
 * \return #FDS_OK on success
 */
int
aggr_min(const struct field *src, struct field *dst)
{
    assert(src != NULL);
    assert(dst != NULL);

    switch (src->type) {
    case FDS_AGGR_UNSIGNED_8:
        if (src->value->uint8 < dst->value->uint8)
			dst->value->uint8 = src->value->uint8;
        break;
    case FDS_AGGR_UNSIGNED_16:
        if (src->value->uint8 < dst->value->uint8)
            dst->value->uint16 = src->value->uint16;
        break;
    case FDS_AGGR_UNSIGNED_32:
        if (src->value->uint32 < dst->value->uint32)
            dst->value->uint32 = src->value->uint32;
        break;
    case FDS_AGGR_UNSIGNED_64:
        if (src->value->uint64 < dst->value->uint64)
            dst->value->uint64 = src->value->uint64;
        break;
    case FDS_AGGR_SIGNED_8:
        if (src->value->int8 < dst->value->int8)
			dst->value->int8 = src->value->int8;
        break;
    case FDS_AGGR_SIGNED_16:
        if (src->value->int16 < dst->value->int16)
			dst->value->int16 = src->value->int16;
        break;
    case FDS_AGGR_SIGNED_32:
        if (src->value->int32 < dst->value->int32)
            dst->value->int32 = src->value->int32;
        break;
    case FDS_AGGR_SIGNED_64:
        if (src->value->int64 < dst->value->int64)
            dst->value->int64 = src->value->int64;
        break;
    case FDS_AGGR_DOUBLE:
        if (src->value->dbl < dst->value->dbl)
            dst->value->dbl = src->value->dbl;
        break;
    case FDS_AGGR_DATE_TIME_NANOSECONDS:
        if (src->value->timestamp < dst->value->timestamp)
            dst->value->timestamp = src->value->timestamp;
        break;
    default:
        // Hete must be error handling
        return FDS_ERR_NOTFOUND;
    }


    return FDS_OK;
}

/**
 * \brief Find a function for value field
 *
 * \param[in] field Field to process
 *
 *  \return Conversion function
 */
aggr_function
get_function(const struct field *field)
{
	// Aggregation functions
    static const aggr_function table[] = {
        &aggr_sum,
		&aggr_min,
		&aggr_max
    };

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
        ((i + 1) != table_size) ? table.list[i].next = table.list[i + 1] : NULL;
	}
	table.size = table_size;


	return FDS_OK;
}

struct node *
get_element(const struct node *list, int index)
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

FDS_API int
find_key(const struct node *list, const char *key)
{

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

FDS_API int
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
		hash_table_clean(table);
		return FDS_ERR_NOMEM;
	}

	// Insert key
	item->key = memore->key;

	// Isert all value fields
	item->val_fields = memory->val_list;

	item->next = NULL;

	// Iserting key on index
    if (list == NULL){
		// Absence of Linked List at a given Index of hash table
 		table->list[index].head = item;
		table->list[index].tail = item;
	} else {
        // A Linked List is present at given index of hash table
		int find_index = find_key(list, memory->key);
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

			 	ret_code = fn(memory->val_lsit[i], element->val_fields[i]);

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
