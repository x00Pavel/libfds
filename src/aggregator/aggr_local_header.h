/**
 * \file src/aggregator/aggr_local_header.h
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \author Pavel Yadlouski <xyadlo00@vutbr.cz>
 * \brief Local header file for developers
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

#include <stddef.h> // size_t
#include <libfds/aggregator.h>


/** Informational structure with basic info about field */
struct field {
    union fds_aggr_field_id id;        /*< ID of field       */
    union fds_aggr_field_value *value; /*< Value of field    */
    enum fds_aggr_types type;          /*< Datatype of value */
    size_t size;                       /*< Size of field     */
    enum fds_aggr_function fnc;        /*< Function of field */
};

typedef struct field field_s;

/** Structure for storing processed data about fields */
struct fds_aggr_s {
    field_s *key_list;           /*< Array of all key fields   */
    size_t key_count;            /*< Count of keu fields       */
    size_t key_size;             /*< Size of key               */
    char *key;                   /*< Pointer to allocated key  */
    field_s *val_list;           /*< Array of all value fields */
    size_t val_count;            /*< Count of value fields     */
    size_t val_size;             /*< Size of all values fields */
    char *val;                   /*< All values                */
    uint32_t sort_flags;         /*< Sorting flags             */
    fds_aggr_cb_get_value *get_fnc; /*< Pointer to GET function (specified by user) */
    struct hash_table *table;    /*< Poiter to hash table      */
};