/**
 * \file include/libfds/aggregator.h
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \author Pavel Yadlouski <xyadlo00@stud.fit.vutbr.cz>
 * \brief Aggregation modul for IPFIX collector
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

 #include <stddef.h>    // size_t
 #include <stdint.h>    // uintXX_t, intXX_t
 #include <stdbool.h>   // bool
 #include <time.h>      // struct timespec
 #include <string.h>    // memcpy
 #include <float.h>     // float min/max
 #include <assert.h>    // static_assert
 #include <arpa/inet.h> // inet_ntop

 #include "libfds.h"

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
     FDS_AGGR_DATE_TIME_NANOSECONDS = 13,
     FDS_AGGR_IPV4_ADDRESS = 14,
     FDS_AGGR_IPV6_ADDRESS = 15,
     FDS_AGGR_UNASSIGNED = 255
 };


/** Union represents ID of element. ID can be integer or pointer (and other) 
  * ptr_id MUST be specified or can be NULL 
  * If ptr_id is not NULL, int_id will be ingnored.
  */
union field_id{
    uint32_t int_id;
    void *ptr_id;
};

/** Cursor for data records */
struct fds_aggr_cursor{
    union field_id id; /*< ID of field         */
    uint8_t *value;    /*< Value of this field */
}/;

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
    uint8_t mac[6];
    uint64_t timestamp; 
};

/** Avaliable functions of fields */
enum fds_aggr_function{
    FDS_KEY_FIELD,
    FDS_SUM,
    FDS_MIN,
    FDS_MAX
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
typedef int (*fds_aggr_get_element)(void *, int, union fds_aggr_field_value *);

/** Structure for description input fields */
struct input_field{
    union field_id id;          /*< ID of field                            */
    enum fds_aggr_types type;   /*< Datatype of value                      */
    enum fds_aggr_function fnc; /*< Function of field (KEY, SUM, MIN, etc) */

    // In future can be extendet
};

/** Informational structure with basic info about field */
struct field_info{
    union field_id id;          /* ID of field       */
    enum fds_aggr_function fnc; /* Function of field */
};

/** Structure for storing processed data about fields */
struct fds_aggr_memory{
    union field_id *key_id_list;   /*< Array of all key fields   */
    size_t key_size;               /*< Size of key               */
    struct field_info *val_list;   /*< Array of all value fields */
    size_t val_size;               /*< Size of all values fields */
    uint32_t sort_flags;           /*< Sorting flags             */
    fds_aggr_get_element *get_fnc; /*< Pointer to GET function (specified by user) */
};

/** \brief Function for initialization memory to use
  *
  * Function takes as parameter structure \p fds_aggr_memory, that will be initialized.
  * By default, all values are 0 or NULL
  *
  * Function do following steps:
  * 1. check pointer on structure
  * 2. allocate memory for this structure
  * 3. set default values
  *
  * \param memory Pointer for structure to initialize
  *
  * \return FDS_OK On success
 */
int
fds_aggr_init(struct fds_aggr_memory *memory);

/** \brief Function for processing input data
  *
  * Function do following steps:
  * 1. Aggregate input array of structures with information about key fields
  * 2. Allocate memory for key (later key will be add with GET function)
  * 3. Create hashtabel
  *
  * \param[in] fields Array of structures with information about fields
  * \param[in] memory Poiter to memory to be initialized
  * \param[in] size   Count of structures in array
  * \param[in] fnc Pointer for GET Function
  */
int
fds_aggr_setup(const struct input_field input[], struct fds_aggr_memory *memory, size_t size
               /*, int (*fds_aggr_get_element)(void*, int, struct fds_aggr_field*)*/);

/** \brief Function for cleaning all resources
  *
  */
void
fds_agr_cleanup();

/** \brief Build output record based on template from setup function
  *
  * Function do following steps:
  * 1. Find the field by ID
  * 2. Get values from this fields
  * 3. Do corresponding operation
  * 4. Write in output record
  *
  * \param[in] rec_list Pointer to list of all data records
  * \param[in] memory   Pointer to structure with info about fields
  */
void
fds_aggr_add_rec(void *rec_list, const struct fds_aggr_memory *memory);

/** \bried Function for initialization cursor for all record. Set cursor on first record
  *
  * \param[in] cursor Pointer for aggrigation structure
  * \param[in] rec    Pointer for all datarecords
  * \param[in] flags  Flags for iteration throught records
  *
  * \return FDS_OK on success
  */
FDS_API int
fds_aggr_cursor_init(struct fds_aggr_cursor *cursor, void *rec, uint32_t flags);

/** \brief Function for iteration through all datarecords
  *
  * \return FDS_OK on success
  */
FDS_API int
fds_aggr_cursor_next();


/*
        TODO
    1) зачем курсор, если есть функция get_element?
    2) какой датовый тип писать в union fds_aggr_field_value для MAC адресов, временных печатей, IP адрессов
       и т.д
    3) если имеем union с int и void*, то размер этого union будет равен размеру int или не обязатьльно?
    4) как узнать какой ID использовать (int или void*)? может специфицировать что void* по умолчанию будет NULL?
    5) как после заполнения ID в лист, потом проходить по этому листу?

*/
