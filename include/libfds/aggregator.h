/**
 * \file include/libfds/aggregator.h
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \author Pavel Yadlouski <xyadlo00@stud.fit.vutbr.cz>
 * \brief Aggregation modul for IPFIX collector (header file)
 * \date July 2019
 */
/**
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
 #ifdef __cplusplus
    extern "C"{
#endif

#ifndef LIBFDS_AGGREGATOR_H
#define LIBFDS_AGGREGATOR_H

#include <stddef.h>    // size_t
#include <stdint.h>    // uintXX_t, intXX_t
#include <stdbool.h>   // bool
#include <string.h>    // memcpy
#include <float.h>     // float min/max
#include <libfds/api.h>

#include "../../src/aggregator/hash_table.h"

/** Enum of datatypes */
enum fds_aggr_types {
     FDS_AGGR_OCTET_ARRAY = 0,
     FDS_AGGR_UNSIGNED_8 = 1,
     FDS_AGGR_UNSIGNED_16 = 2,
     FDS_AGGR_UNSIGNED_32 = 3,
     FDS_AGGR_UNSIGNED_64 = 4,
     FDS_AGGR_SIGNED_8 = 5,
     FDS_AGGR_SIGNED_16 = 6,
     FDS_AGGR_SIGNED_32 = 7,
     FDS_AGGR_SIGNED_64 = 8,
     FDS_AGGR_DOUBLE = 9,
     FDS_AGGR_BOOLEAN = 10,
     FDS_AGGR_MAC_ADDRESS = 11,
     FDS_AGGR_STRING = 12,
     FDS_AGGR_IPV4_ADDRESS = 13,
     FDS_AGGR_IPV6_ADDRESS = 14,
     FDS_AGGR_DATE_TIME_NANOSECONDS = 15,
     FDS_AGGR_UNASSIGNED = 255
};


/** Union represents ID of element. ID can be integer or pointer (and other)
  * ptr_id MUST be specified or can be NULL
  * If ptr_id is not NULL, int_id will be ingnored.
  */
union field_id{
    uint32_t int_id; /*< Integer value of ID */
    void *ptr_id;    /*< Pointer to ID       */
};

/** Union for writing down value of field */
union fds_aggr_field_value{
    uint8_t  uint8;
    uint16_t uint16;
    uint32_t uint32;
    uint64_t uint64;
    int8_t   int8;
    int16_t  int16;
    int32_t  int32;
    int64_t  int64;
    double   dbl;
    bool     boolean;
    uint8_t  ip[16];
    uint8_t  mac[6];
    uint64_t timestamp;
};

/** Avaliable functions of fields */
enum fds_aggr_function{
    FDS_SUM,
    FDS_MIN,
    FDS_MAX,
    FDS_KEY_FIELD
};

/** \brief Pointer to function for processing data record
  *
  * Function is specified by user.
  *
  * \param Pointer to data record
  * \param ID of field to be found
  * \param Union for writing down values (then this values will be set keys or values)
  *
  * \return ID of field
  */
typedef int (*fds_aggr_get_element)(void *, union field_id, union fds_aggr_field_value *);

/** Structure for description input fields */
struct input_field {
    union field_id id;          /*< ID of field                            */
    enum fds_aggr_types type;   /*< Datatype of value                      */
    enum fds_aggr_function fnc; /*< Function of field (KEY, SUM, MIN, etc) */

    // In future can be extendet
};

/** Informational structure with basic info about field */
struct field{
    union field_id id;                 /*< ID of field       */
    union fds_aggr_field_value *value; /*< Value of field    */
    enum fds_aggr_types type;          /*< Datatype of value */
    size_t size;                       /*< Size of field     */
    enum fds_aggr_function fnc;        /*< Function of field */
};

/** Structure for storing processed data about fields */
struct fds_aggr_memory{
    struct field *key_list;        /*< Array of all key fields   */
    size_t key_count;              /*< Count of keu fields       */
    size_t key_size;               /*< Size of key               */
    char *key;                     /*< Pointer to allcated key   */
    struct field *val_list;        /*< Array of all value fields */
    size_t val_count;              /*< Count of value fields     */
    size_t val_size;               /*< Size of all values fields */
    uint32_t sort_flags;           /*< Sorting flags             */
    fds_aggr_get_element *get_fnc; /*< Pointer to GET function (specified by user) */
    struct hash_table *table;      /*< Poiter to hash table      */
};

/** \brief Function for initialization memory to use
  *
  * Function takes as parameter structure \p fds_aggr_memory, that will be initialized.
  * By default, all values are 0 or NULL
  *
  * Function do following steps:
  * 1. Check pointer on structure
  * 2. Allocate memory for this structure
  * 3. Set default values
  * 4. Initialize hash table
  *
  * \param[in] memory Pointer for structure to initialize
  * \param[in] table_size Required size of hash table
  *
  * \return #FDS_OK On success
  * \return #FDS_ERR_NOMEM onli if allocation in hash_table_init fault
  */
FDS_API int
fds_aggr_init(struct fds_aggr_memory *memory);

/** \brief Function for processing input data
 *
 * Function do following steps:
 * 1. Aggregate input array of structures with information about key fields
 * 2. Allocate memory for key (later key will be add with GET function)
 *
 * \param[in] input      Array of structures with information about fields
 * \param[in] input_size Count of structures in array
 * \param[in] memory     Poiter to memory to be initialized
 * \param[in] fnc        Pointer for GET Function
 *
 * \return #FDS_OK on success
 * \return #FDS_ERR_NOMEM in case of allocation error
 */
FDS_API int
fds_aggr_setup( const struct input_field *input_fields,
                size_t input_size,
                struct fds_aggr_memory *memory,
                size_t table_size,
                const fds_aggr_get_element *fnc);

/** \brief Function for cleaning all resources
  *
  */
void
fds_aggr_cleanup(struct fds_aggr_memory *memory);

/** \brief Add item to hash table
  *
  * Function do following steps:
  * 1. Find the field by ID
  * 2. Get values from this fields
  * 3. Do corresponding operation or aggregate as key field
  * 4. Write down to hash table
  *
  * \param[in] record Pointer to data records
  * \param[in] memory Pointer to structure with info about fields
  *
  * \return #FDS_OK on success
  * \return #FDS_ERR_NOTFOUND if some fields not found during get_element function
  */
FDS_API int
fds_aggr_add_item(void *record, const struct fds_aggr_memory *memory);

/** \bried Function for initialization cursor for hasht table.
  *
  * \param[in] cursor Pointer to cursor
  * \param[in] rec    Pointer to hash table
  *
  * Set #cursor on the first node in hash table
  *
  * \return FDS_OK on succes
  */
FDS_API int
fds_aggr_cursor_init(struct list *cursor, const struct hash_table *table);

/** \brief Function for iteration through hash table
  *
  * \return FDS_OK on success
  */
FDS_API int
fds_aggr_cursor_next(const struct list *table);

#ifdef __cplusplus
    }
#endif

#endif /* LIBFDS_AGGREGATOR_H */

/*
    TODO
*/
