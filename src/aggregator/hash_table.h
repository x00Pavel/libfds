/**
 * \file src/aggregator/hash_table.h
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \author Pavel Yadlouski <xyadlo00@vutbr.cz>
 * \brief Functions for operating with hash table (header file)
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

/** Node for storing an item in a linked list */
struct node{
    char *key;                         /*< Key of node  */
    union fds_aggr_field_value *value; /*< Value of key */
    struct node *next;                 /*< Next node    */
};

/** Structure for storing a linked list */
struct list {
    struct node *head; /*< poiter to first element in the list */
    struct node *tail; /*< poiter to last element in the list  */
};

/** Information about hash table */
struct hash_table{
    size_t size;       /*< Count of free fields    */
    struct list *list; /*< Array with linked lists */
};

/** \brief Function for generating hash
  *
  * \param[in] key      Base for hash function
  * \param[in] key_szie Size of key
  *
  * \warning Be default, seed for hash function is 0
  *
  * \return Index to hash table
  */
unsigned long
hash_fnc(char *key, size_t key_size);

/** \brief Fucntion for filling in key and value
  *
  * \param[in] table    Pointer to table
  * \param[in] key      Key to be iserted
  * \param[in] key_size Size of key
  * \param[in] value    Value for given key
  * 
  * \return #FDS_OK on success
  */
FDS_API
insert_key(struct hash_table *table, char *key, size_t key_size, union fds_aggr_field_value *value);

/** \brief Fucntion for allocating for hash table
  *
  * \param[in]  table      Poiter to table to initialization
  * \param[in]  table_size Requared size of table
  * \param[out] table      Poiter to allocated memory for table
  */
FDS_API
hash_table_init(struct hash_table *table, size_t table_size);

/** \brief Function finds the given key in the Linked List
  *
  * \param[in] list Poiter to linked list
  * \param[in] key  Key to be found
  *
  * \return In success, index of given key, 
  *  return -1 if key is not present
  */
FDS_API
find_key(struct node *list, char* key);

/** \brief Function for getting node from linked list
  *
  * \param[in] list  Poiter to linked list
  * \param[in] index Index to be found
  *
  * \return Pointer to node on given index in success or NULL if node is not found
  */
struct node *
get_element(struct node *list, int index);


/** \brief Function for cleaning up resources
  *
  * \param[in] table Pointer to table to be cleaned
  */
void
hash_table_clean(struct hash_table *table);
