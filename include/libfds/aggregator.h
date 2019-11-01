/**
 * \file /include/libfds/aggregator.h
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \author Pavel Yadlouski <xyadlo00@stud.fit.vutbr.cz>
 * \brief Aggregation module for IPFIX collector (header file)
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

/** Enum of datatypes */
enum fds_aggr_types {
     FDS_AGGR_UNSIGNED_8 = 0,
     FDS_AGGR_UNSIGNED_16 = 1,
     FDS_AGGR_UNSIGNED_32 = 2,
     FDS_AGGR_UNSIGNED_64 = 3,
     FDS_AGGR_SIGNED_8 = 4,
     FDS_AGGR_SIGNED_16 = 5,
     FDS_AGGR_SIGNED_32 = 6,
     FDS_AGGR_SIGNED_64 = 7,
     FDS_AGGR_DOUBLE = 8,
     FDS_AGGR_BOOLEAN = 9,
     FDS_AGGR_MAC_ADDRESS = 10,
     FDS_AGGR_STRING = 11,
     FDS_AGGR_IP_ADDRESS = 12, // For both IPv4 and IPv6
     FDS_AGGR_DATE_TIME_NANOSECONDS = 13,
     FDS_AGGR_UNASSIGNED = 255
};


/** Union represents ID of element. ID can be integer or pointer (and other)
  * ptr_id MUST be specified or can be NULL
  * If ptr_id is not NULL, int_id will be ignored.
  */
union fds_aggr_field_id{
    uint64_t int_id; /*< Integer value of ID */
    void *ptr_id;    /*< Pointer to ID       */
};

typedef struct fds_aggr_memory memory_s;

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
    /*  Type for both IPv4 and IPv6. 
        In case of IPv4, it mapped to IPv6 address. That hybrid address
        consists of 80 "0" bits, followed by 16 "1" bits ("FFFF" in hexadecimal), followed by the
        original 32-bit IPv4 address (too technical for most of us).

        Here is an example of how that would work:
        IPv4 address:	169.291.13.133
        Maps to IPv6 address:	0000:0000:0000:0000:FFFF:A9DB:0D85
        Simplified:	::FFFF:A9DB:0D85
        The prefix of the mapped address places it in the range of mapped IPv4 addresses.
        Because of that, the IPv4 portion is often left in the more familiar dotted-decimal format:
        ::FFFF:169.219.13.133 */
    uint8_t  ip[16]; 
    uint8_t  mac[6];
    uint64_t timestamp; // In nanoseconds
};

/** Avaliable functions of fields */
enum fds_aggr_function{
    FDS_AGGR_SUM,
    FDS_AGGR_MIN,
    FDS_AGGR_MAX,
    FDS_AGGR_OR,
    FDS_AGGR_KEY
};

/** \brief Pointer to function for processing data record
  *
  * Function is specified by user.
  *
  * \param Pointer to data record
  * \param ID of field to be found
  * \param Union for writing down values (then this values will be set keys or values)
  *
  * \return #FDS_OK on success
  */
typedef int (*fds_aggr_cb_get_value)(void *, union fds_aggr_field_id, union fds_aggr_field_value *);

/** Structure for description input fields */
struct fds_aggr_input_field {
    union fds_aggr_field_id id;          /*< ID of field                            */
    enum fds_aggr_types type;   /*< Datatype of value                      */
    enum fds_aggr_function fnc; /*< Function of field (KEY, SUM, MIN, etc) */

    // In future can be extendet
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
  * \return #FDS_ERR_NOMEM only if allocation in hash_table_init fault
  */
fds_aggr_t *
fds_aggr_create(fds_aggr_t *memory, size_t table_size);

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
fds_aggr_setup(fds_aggr_t *memory, 
               const struct fds_aggr_input_field *input_fields, 
               size_t input_size,
               fds_aggr_cb_get_value *fnc, 
               char *key);
    
/** \brief Function for cleaning all resources
 *
 */
FDS_API int 
fds_aggr_destroy(fds_aggr_t *memory);

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
fds_aggr_add_item(memory_s *memory, const void *record);

/** \brief Function for initialization cursor for hash table.
 *
 * \param[in] cursor Pointer to cursor
 * \param[in] rec    Pointer to hash table
 *
 * Set #cursor on the first node in hash table
 *
 * \return FDS_OK on success
 */
FDS_API int 
fds_aggr_cursor_init(struct list * cursor, const struct hash_table *table);
// fds_aggr_cursor_init(memory_s *memory);

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
