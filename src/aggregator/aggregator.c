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
#include <string.h> // memcpy
#include <float.h>  // float min/max
#include <libfds/api.h>
#include <libfds/aggregator.h>
#include "hash_table.h"
#include "aggr_local_header.h"

/** Array of sizes of used datatypes.
  * DO NOT REORDER,
  * INDEXES ARE ASSOCIATED WITH enum fds_aggr_types !!
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
     6,  // size of MAC address
     8,  // size of string by default
     8,  // size of timestemp
     4,  // size of IPv4
     16, // size of IPv6
     8  // size of unassigned data
};

typedef struct fds_aggr_s fds_aggr_t;
typedef struct hash_table fds_aggr_hash_table_t;
typedef int (*aggr_function)(void *, void *);

static inline int
aggr_sum_uint8(uint8_t src, uint8_t dst)
{
    return dst + src;
}

/**
 * \brief Function for making sum of values
 *
 * \param[in] src Pointer to sources value field
 * \param[out] dst Poiter to destination value field
 *
 * \return #FDS_OK on success
 */
int
aggr_sum(const struct field *src, struct field *dst)
{
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
        // Here must be error handling
        return FDS_ERR_NOTFOUND;
    }

    return FDS_OK;
}

/**
 * \brief Function for choosing maximum from two values
 *
 * \param[in] src Pointer to sources value field
 * \param[out] dst Poiter to destination value field
 *
 * \return #FDS_OK on success
 */
int
aggr_max(const struct field *src, struct field *dst){
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
        // Here must be error handling
        return FDS_ERR_NOTFOUND;
    }

    return FDS_OK;
}

/**
 * \brief Function for choosing minimum from two values
 *
 * \param[in] src Pointer to sources value field
 * \param[out] dst Poiter to destination value field
 *
 * \return #FDS_OK on success
 */
int
aggr_min(const struct field *src, struct field *dst){
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
        // Here must be error handling
        return FDS_ERR_NOTFOUND;
    }

    return FDS_OK;
}

int 
aggr_or(const struct field *src, struct field *dst){

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
    // static const aggr_function table[] = {&aggr_sum, &aggr_min, &aggr_max, &aggr_or};
    // const enum fds_aggr_function function = field->fnc;
    // return table[function];

    switch(field->type){
    case FDS_AGGR_UNSIGNED_8:
        switch (field->fnc)
        {
        case FDS_AGGR_SUM :            
            return &aggr_sum_uint8;
        case FDS_AGGR_MIN:
            return &aggr_min_uint8;
        case FDS_AGGR_MAX:
            return &aggr_max_uint8;
        case FDS_AGGR_OR:
            return &aggr_or_uint8;
        default:
            break;
        }
        break;
    case FDS_AGGR_UNSIGNED_16:
        break;
    case FDS_AGGR_UNSIGNED_32:
        break;
    case FDS_AGGR_UNSIGNED_64:
        break;
    case FDS_AGGR_SIGNED_8:
        break;
    case FDS_AGGR_SIGNED_16:
        break;
    case FDS_AGGR_SIGNED_32:
        break;
    case FDS_AGGR_SIGNED_64:
        break;
    case FDS_AGGR_DOUBLE:
        break;
    case FDS_AGGR_BOOLEAN: 
        break;
    case FDS_AGGR_MAC_ADDRESS:
        break;
    case FDS_AGGR_STRING:
        break;
    case FDS_AGGR_IP_ADDRESS:
        break;
    case FDS_AGGR_DATE_TIME_NANOSECONDS:
        break;
    default:
        break; 

    }




}

fds_aggr_t *
fds_aggr_create(fds_aggr_t *memory, size_t table_size){
    if (memory == NULL){
        return NULL;
    }

    memory->key_list = NULL;
    memory->key_size = 0;
    memory->key_count = 0;

    memory->val_list = NULL;
    memory->val_size = 0;
    memory->val_count = 0;
    // memory->sort_flags = 0;
    memory->get_fnc  = NULL;
    // Initialization of hash table
    int rc = hash_table_init(memory->table, table_size);
    if (rc != FDS_OK){
        return NULL;
    }
    return memory;
}

int
fds_aggr_setup(fds_aggr_t *memory, 
               const struct fds_aggr_input_field *input_fields, 
               size_t input_size,
               fds_aggr_cb_get_value *fnc, 
               char *key){
    assert(input_fields != NULL);
    assert(fnc != NULL);
    assert(memory != NULL);
    assert(input_size > 0);

    if ((input_fields == NULL) || (input_size == 0)){
        return FDS_ERR_ARG;
    }
    

    int key_count = 0;
    int val_count = 0;

    memory->get_fnc = fnc;

    for (int i = 0; i < input_size; i++){
        if (input_fields[i].fnc == FDS_AGGR_KEY){
            // Count total key size
            memory->key_size += size_of[input_fields[i].type];

            // Count new array size
            const size_t array_size = (key_count + 1) * sizeof(union fds_aggr_field_id);

            memory->key_list = realloc(memory->key_list, array_size);
            if(memory->key_list == NULL){
                return FDS_ERR_NOMEM;
            }

            memory->key_list[key_count].id = input_fields[i].id;
            memory->val_list[key_count].type = input_fields[i].type;
            memory->key_list[key_count].size = size_of[input_fields[i].type];
            memory->key_list[key_count].fnc = input_fields[i].fnc;

            key_count++;
        }
        else {
            // dodat vyber funkci pres switch jako v libnf 
            // Count total values size
            memory->val_size += size_of[input_fields[i].type];

            // Count new array size
            const size_t array_size = (val_count + 1) * sizeof(field_s);

            memory->val_list = realloc(memory->val_list, array_size);
            if(memory->key_list == NULL){
                return FDS_ERR_NOMEM;
            }

            memory->val_list[val_count].id = input_fields[i].id;
            memory->val_list[val_count].type = input_fields[i].type;
            memory->val_list[val_count].size = size_of[input_fields[i].type];
            memory->val_list[val_count].fnc = input_fields[i].fnc;
            val_count++;
        }
    }

    // If user did not set any key or value field
    if((key_count == 0) || (val_count == 0)){
        return FDS_ERR_ARG;
    }

    memory->key_count = key_count;
    memory->val_count = val_count;

    memory->key = (char *) malloc(memory->key_size);
    if(memory->key == NULL){
        return FDS_ERR_NOMEM;
    }

    return FDS_OK;
}

int
fds_aggr_add_record(fds_aggr_t *memory, const void *record){
    // union fds_aggr_field_value *value;
    fds_aggr_cb_get_value get_value = memory->get_fnc;

    int ret_code;
    int key_offset = 0;
    int val_offset = 0;

    // Make key
    for (int i = 0; i < memory->key_count; i++){
        ret_code = get_value(record, memory->key_list[i].id, memory->key_list[i].value);
        if (ret_code != FDS_OK){
            return ret_code;
        }

        memcpy(memory->key[key_offset], (char *) memory->key_list[i].value, memory->key_list[i].size);
        key_offset += memory->key_list[i].size;
    }

    // Getting value fields
    for (int i = 0; i < memory->val_count; i++){
        ret_code = get_value(record, memory->val_list[i].id, memory->val_list[i].value);

        if (ret_code != FDS_OK){
            return ret_code;
        }

        memory->val[val_offset] = memory->val_list[i].value;
        memcpy(memory->val[val_offset], memory->val_list[i].value, memory->val_list[i].size);
        val_offset += memory->val_list[i].size;
    }

    // Insert key to hash table
    struct node *item;
    ret_code = get_element(memory, &item);

    // Insert values to node in hash table
    if(ret_code == HASH_NEW){
        // It is first node with this key 
        memcpy(item->data[memory->key_size], memory->val, memory->val_size);
    }
    else if (ret_code == HASH_FOUND){
        // Key already present in hash table, so actualize it value
        char *val_list = item->data[memory->key_size];
        
        // Offset of new value 
        size_t offset = 0;
        for (int i = 0; i < memory->val_count; i++){
            // Get function of relevant value
            aggr_function fnc = get_function(memory->val_list[i].fnc);

            // Copy value to actualize it
            int32_t *val = (int32_t *) malloc(sizeof(memory->val_list[i].size));
            memcpy(val, val_list[offset], memory->val_list[i].size);
            
            // Actualize value
            fnc(memory->val_list[i].value, val);

            // Insert actualized value on it place
            memcpy(val_list[offset], val, memory->val_list[i].size);

            // Actualize offset 
            offset += memory->val_list[i].size;
        }        
    }
    else{
        // There is an error
        return FDS_ERR_ARG;
    }

    return FDS_OK;
}
/*
int
fds_aggr_cursor_init(fds_aggr_cursor_t *cursor, const fds_aggr_hash_table_t *table){

    assert(cursor != NULL);

    cursor = (fds_aggr_cursor_t *) malloc(sizeof(fds_aggr_cursor_s));
    if (cursor == NULL){
        return FDS_ERR_NOMEM;
    }


}

int
fds_aggr_cursor_next(fds_aggr_cursor_t *cursor)
{
    assert(cursor != NULL);

    const fds_aggr_cursor_t *tmp = cursor->next;

    if (tmp == NULL){
        return FDS_EOC;
    }

    cursor = tmp;
    
    return FDS_OK;
}
*/
int
fds_aggr_destroy(fds_aggr_t *memory){

    assert(memory != NULL);
    assert(memory->key_list != NULL);
    assert(memory->val_list != NULL);
    assert(memory->key != NULL);

    free(memory->key_list);
    free(memory->val_list);
    free(memory->key);

}
