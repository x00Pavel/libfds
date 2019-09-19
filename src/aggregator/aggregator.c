/**
 * \file src/aggregator/aggregator.c
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \author Pavel Yadlouski <xyadlo00@vutbr.cz>
 * \brief Aggregation functions for IPFIX data records (source code)
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

#include <assert.h>
#include <libfds.h>

/** Aray of sizes of used datatypes.
  * DO NOT REODER,
  * INDEXIS ARE ASSOCITED WITH enum fds_aggr_types !!
  */
size_t size_of[] = {
     4, // ??????  size of octet array
     sizeof(uint8_t),
     sizeof(uint16_t),
     sizeof(uint32_t),
     sizeof(uint64_t),
     sizeof(int8_t),
     sizeof(int16_t),
     sizeof(int32_t),
     sizeof(int64_t),
     sizeof(double),
     sizeof(bool),
     6,  // size of MAC adress
     8,  // size of string by default
     8,  // size of timestemp
     4,  // size of IPv4
     16, // size of IPv6
     255 = 8  // size of unassigned data
};


FDS_API int
fds_aggr_init(struct fds_aggr_memory *memory, size_t table_size){

    if (memory == NULL){
        return FDS_ERR_ARG;
    }

    memory->key_list = NULL;
    memory->key_size = 0;
    memory->key_count = 0;

    memory->val_list = NULL;
    memory->val_size = 0;
    memory->val_count = 0;
    // memory->sort_flags = 0;
    memory->get_fnc  = NULL;
    // Initializaton of hash table
    int rc = hash_table_init(memory->table, table_size);
    if (rc != FDS_OK){
        return rc;
    }
    return FDS_OK;
}

FDS_API int
fds_aggr_setup( const struct input_field *input_fields,
                size_t input_size,
                struct fds_aggr_memory *memory,
                fds_aggr_get_element *fnc,
                char *key){

    assert(input_fields != NULL);
    assert(fnc != NULL);
    assert(memory != NULL);
    assert(input_size > 0);

    int key_count = 0;
    int val_count = 0;

    memory->get_fnc = fnc;

    for (int i = 0; i < input_size; i++){
        if (input[i].fnc == FDS_KEY_FIELD){
            // Count total key size
            memory->key_size += size_of[input[i].type];

            // Count new array size
            const size_t array_size = (key_count + 1) * sizeof(union field_id);

            memory->key_list = realloc(memory->key_list, array_size);
            if(memory->key_list == NULL){
                return FDS_ERR_NOMEM;
            }

            memory->key_list[key_count].id = input[i].id;
            memory->val_list[val_count].type = input[i].type;
            memory->key_list[key_count].size = size_of[input[i].type];
            memory->key_list[key_count].fnc = input[i].fnc;

            key_count++;
        }
        else {
            // Count total values size
            memory->val_size += size_of[input[i].type];

            // Count new array size
            const size_t array_size = (val_count + 1) * sizeof(struct field);

            memory->val_list = realloc(memory->val_list, array_size);
            if(memory->key_list == NULL){
                return FDS_ERR_NOMEM;
            }

            memory->val_list[val_count].id = input[i].id;
            memory->val_list[val_count].type = input[i].type;
            memory->val_list[val_count].size = size_of[input[i].type];
            memory->val_list[val_count].fnc = input[i].fnc;
            val_count++;
        }
    }

    memory->key_count = key_count;
    memory->val_count = val_count;

    memory->key = (char *) malloc(memory->key_size);
    if(memory->key == NULL){
        return FDS_ERR_NOMEM;
    }

    return FDS_OK;
}

FDS_API int
fds_aggr_add_item(void *record, const struct fds_aggr_memory *memory){
    union fds_aggr_field_value *value;
    fds_aggr_get_element *get_element = memory->get_fnc;

    int ret_code;
    int offset = 0;

    // Can i do this in another way?

    // Make key
    for (int i = 0; i < memory->key_count; i++){
        ret_code = get_element(record, memory->key_list[i].id, value);

        if (ret_code != FDS_OK){
            return ret_code;
        }

        memory->key[offset] = (char *)value;
        offset += memory->key_list[i].size;
    }

    // Getting value fields
    for (int i = 0; i < memory->val_count; i++){
        ret_code = get_element(record, memory->val_list[i].id, value);

        if (ret_code != FDS_OK){
            return ret_code;
        }

        memory->val_list[offset].value = value;
        offset += memory->key_list[i].size;
    }

    // Insert key to hash table
    ret_code = insert_key(memory);

    if(ret_code != FDS_OK){
        return ret_code;
    }

    return FDS_OK;
}

FDS_API int
fds_aggr_cursor_next(const struct list *table){

    assert(table != NULL);

    const struct list *tmp = table->next;

    if (tmp == NULL){
        return FDS_EOC;
    }

    tabel = tmp;
    
    return FDS_OK;
}

void
fds_aggr_cleanup(struct fds_aggr_memory *memory){

    assert(memory != NULL);
    assert(memory->key_list != NULL);
    assert(memory->val_list != NULL);
    assert(memory->key != NULL);

    free(memory->key_list);
    free(memory->val_list);
    free(memory->key);

}
