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

#include <libfds/api.h>
#include <libfds/aggregator.h>

typedef struct fds_aggr_s fds_aggr_t;

/**
 * \brief Enumeration of return codes for hash table
*/
enum{
  HASH_NEW,   /*< NEw element was created */
  HASH_FOUND, /*< Key was found           */
  HASH_ERR    /*< Any error               */
};

/** Node for storing an item in a linked list */
struct node{
    struct node *next; /*< Next node   */
    uint8_t data[1];   /*< key:value*/
};

/** Information about hash table */
struct hash_table{
    size_t size;        /*< Count of free fields    */
    struct node **list; /*< Array with linked lists */
};

/** \brief Function for allocating for hash table
  *
  * \param[in]  table      Poiter to table to initialization
  * \param[in]  table_size Required size of table
  * \param[out] table      Poiter to allocated memory for table
  */
int 
hash_table_init(struct hash_table *table, size_t table_size);

/** \brief Return pointer to element. 
  * If element is not in the hash table, then will be created new element and return it
  *
  * \param[in] list  Poiter to linked list
  * \param[out] res Pointer to element to fill in    
  *
  * \return #HASH_NEW if element was not in the list
  * \return #HASH_FOUND if element with key was found 
  * \return #HASH_ERR in case of any error 
  */

int 
get_element(const fds_aggr_t *memory, struct node **res);

/** \brief Function for cleaning up resources
 *
 * \param[in] table Pointer to table to be cleaned
 */
void
hash_table_clean(struct hash_table *table);
