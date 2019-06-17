/**
 * \file src/converters/converters.c
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \author Pavel Yadlouski <xyadlo00@stud.fit.vutbr.cz>
 * \brief Conversion of IPFIX Data Record to JSON  (source code)
 * \date May 2019
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2019 CESNET z.s.p.o.
 */

#include <libfds.h>
#include <math.h>
#include <stdlib.h>
#include <inttypes.h>
#include "protocols.h"

/** Base size of the conversion buffer           */
#define BUFFER_BASE   4096
/** IANA enterprise number (forward fields)      */
#define IANA_EN_FWD   0
/** IANA enterprise number (reverse fields)      */
#define IANA_EN_REV   29305
/** IANA identificator of TCP flags              */
#define IANA_ID_FLAGS 6
/** IANA identificator of protocols              */
#define IANA_ID_PROTO 4

struct context {
    /** Data begin                                                                           */
    char *buffer_begin;
    /** The past-the-end element (a character that would follow the last character)          */
    char *buffer_end;
    /** Position of the next write operation                                                 */
    char *write_begin;
    /**  Flag for realocation                                                                */
    bool allow_real;
    /**  Other flags                                                                         */
    uint32_t flags;
} ; /**< Converted JSON record                                                               */

typedef int (*converter_fn)(struct context *,const struct fds_drec_field *);

// Free space in buffer
size_t buffer_remain(const struct context *buffer) {return buffer->buffer_end - buffer->write_begin;}
// Total size of allocated buffer
size_t buffer_alloc(const struct context *buffer) {return buffer->buffer_end - buffer->buffer_begin;}
// Used buffer size
size_t buffer_used(const struct context *buffer) {return buffer->write_begin - buffer->buffer_begin;}

/**
 * \brief Reserve memory of the conversion buffer
 *
 * Requests that the string capacity be adapted to a planned change in size to a length of up
 * to n characters.
 * \param[in] n Minimal size of the buffer
 * \return #FDS_OK on success
 * \return #FDS_ERR_NOMEM in case of memory allocation error
 * \retunr #FDS_ERR_BUFFER in case if flag for reallocation is not set
 */
int
buffer_reserve (struct context *buffer, size_t n)
{
    if (n <= buffer_alloc(buffer)) {
        // Nothing to do
        return FDS_OK;
    }
    if (buffer->allow_real == 0){
        return FDS_ERR_BUFFER;
    }
    size_t used = buffer_used(buffer);

    // Prepare a new buffer and copy the content
    const size_t new_size = ((n / BUFFER_BASE) + 1) * BUFFER_BASE;
    char *new_buffer = (char*)realloc( buffer->buffer_begin, new_size * sizeof(char));
    if (new_buffer == NULL){
        return FDS_ERR_NOMEM;
    }

    buffer->buffer_begin = new_buffer;
    buffer->buffer_end   = new_buffer + new_size;
    buffer->write_begin  = new_buffer + used;

    return FDS_OK;
}

/**
* \brief Append the conversion buffer
* \note
*   If the buffer length is not sufficient enough, it is automatically reallocated to fit
*   the string.
* \param[in] str String to add
* \return #FDS_OK on success
* \return ret_code in case of memory allocation error
*/

int
buffer_append(struct context *buffer,const char *str)
{
    const size_t len = strlen(str) + 1; // "\0"

    int ret_code = buffer_reserve(buffer ,buffer_used(buffer) + len);
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    memcpy(buffer->write_begin, str, len);
    buffer->write_begin += len - 1;
    return FDS_OK;
}

/**
 * \brief Convert an integer to JSON string
 * \param[in] field Field to convert
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG invalid data format
 * \return ret_code in case of memory allocation error
 */
int
to_int(struct context *buffer,const struct fds_drec_field *field)
{
    // Print as signed integer
    int res = fds_int2str_be(field->data, field->size, buffer->write_begin, buffer_remain(buffer));
    if (res > 0) {
        buffer->write_begin += res;
        return FDS_OK;
    }

    if (res != FDS_ERR_BUFFER) {
        return FDS_ERR_ARG;
    }

    // Reallocate and try again
    int ret_code = buffer_reserve(buffer ,buffer_used(buffer) + FDS_CONVERT_STRLEN_INT);
    if (ret_code != FDS_OK) {
        return ret_code;
    }
    return to_int(buffer ,field);
}

/**
 * \brief Convert an unsigned integer to JSON string
 * \param[in] field Field to convert
 * \throw invalid_argument if the field is not a valid field of this type
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG in case of wrong argument
 * \return ret_code in case of memory allocation error
*/
int
to_uint(struct context *buffer, const struct fds_drec_field *field)
{
    // Print as unsigned integer
    int res = fds_uint2str_be(field->data, field->size, buffer->write_begin, buffer_remain(buffer));
    if (res > 0) {
        buffer->write_begin += res;
        return FDS_OK;
    }

    if (res != FDS_ERR_BUFFER) {
        return FDS_ERR_ARG;
    }

    // Reallocate and try again
    int ret_code = buffer_reserve(buffer ,buffer_used(buffer) + FDS_CONVERT_STRLEN_INT);
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    return to_uint(buffer, field);
}

/**
 * \brief Convert an octet array to JSON string
 *
 * \note
 *   Because the JSON doesn't directly support the octet array, the result string is wrapped in
 *   double quotes.
 * \param[in] field Field to convert
 * \throw invalid_argument if the field is not a valid field of this type
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG iinvalid data format
 * \return ret_code in case of memory allocation error
 */
int
to_octet(struct context *buffer, const struct fds_drec_field *field)
{
    if (field->size <= 8) {
        // Print as unsigned integer
        to_uint(buffer, field);
        return FDS_OK;
    }

    const size_t mem_req = (2 * field->size) + 5U; // "0x" + 2 chars per byte + 2x "\"" + "\0"

    int ret_code = buffer_reserve(buffer ,buffer_used(buffer) + mem_req);
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    buffer->write_begin[0] = '"';
    buffer->write_begin[1] = '0';
    buffer->write_begin[2] = 'x';
    buffer->write_begin += 3;
    int res = fds_octet_array2str(field->data, field->size, buffer->write_begin, buffer_remain(buffer));
    if (res >= 0) {
        buffer->write_begin += res;
        *(buffer->write_begin++) = '"';
        return FDS_OK;
    }

    // Restore position and throw an exception
    return FDS_ERR_ARG;
}

/**
 * \brief Convert a float to JSON string
 *
 * \note
 *   If the value represent infinite or NaN, instead of number a corresponding string
 *   is stored.
 * \param[in] field Field to convert
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG if failed to convert a float number
 * \return ret_code in case of memory allocation error
 */
int
to_float(struct context *buffer, const struct fds_drec_field *field)
{
    // We cannot use default function because "nan" and "infinity" values
    double value;
    if (fds_get_float_be(field->data, field->size, &value) != FDS_OK) {
        return FDS_ERR_ARG;
    }

    if (isfinite(value)) {
        // Normal value
        const char *fmt = (field->size == sizeof(float))
            ? ("%." FDS_CONVERT_STRX(FLT_DIG) "g")  // float precision
            : ("%." FDS_CONVERT_STRX(DBL_DIG) "g"); // double precision
        int str_real_len = snprintf(buffer->write_begin, buffer_remain(buffer), fmt, value);
        if (str_real_len < 0) {
            return FDS_ERR_ARG;
        }

        if ((size_t) str_real_len >= buffer_remain(buffer)) {
            // Reallocate the buffer and try again
            int ret_code = buffer_reserve( buffer,2 * buffer_alloc(buffer)); // Just double it
            if (ret_code != FDS_OK) {
                return ret_code;
            }
            return to_float(buffer,field);
        }

        buffer->write_begin += str_real_len;
        return FDS_OK;
    }

    // +/-Nan or +/-infinite
    const char *str;
    // Size 8 (double)
    if (isinf(value) && value >= 0) {
        str = "\"inf\"";
    } else if (isinf(value) && value < 0) {
        str = "\"-inf\"";
    } else if (isnan(value) && value >= 0) {
        str = "\"nan\"";
    } else if (isnan(value) && value < 0) {
        str = "\"-nan\"";
    } else {
        str = "null";
    }

    size_t size = strlen(str) + 1; // + '\0'

    int ret_code = buffer_reserve(buffer, buffer_used(buffer) + size);
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    strcpy(buffer->write_begin, str);
    buffer->write_begin += size;
    return FDS_OK;
}

/**
 * \brief Convert a boolean to JSON string
 *
 * \note If the stored boolean value is invalid, an exception is thrown!
 * \param[in] field Field to convert
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG if failed to convert a boolean to string
 * \return ret_code in case of memory allocation error
 */
int
to_bool(struct context *buffer, const struct fds_drec_field *field)
{
    if (field->size != 1) {
        return FDS_ERR_ARG;
    }

    int res = fds_bool2str(field->data, buffer->write_begin, buffer_remain(buffer));
    if (res > 0) {
        buffer->write_begin += res;
        return FDS_OK ;
    }

    if (res != FDS_ERR_BUFFER) {
        return FDS_ERR_ARG;
    }

    // Reallocate and try again
    int ret_code = buffer_reserve(buffer, buffer_used(buffer) + FDS_CONVERT_STRLEN_FALSE); // false is longer
    if (ret_code !=FDS_OK){
        return ret_code;
    }
    return to_bool(buffer, field);
}


/**
 * \brief Convert a datetime to JSON string
 *
 * Based on the configuration, the output string is formatted or represent UNIX timestamp
 * (in milliseconds). Formatted string is based on ISO 8601 and use only millisecond precision
 * because JSON parsers typically doesn't support anything else.
 * \param[in] field Field to convert
 * \return FDS_OK on success
 * \return FDS_ERR_ARG if failed to convert a timestamp to string
 * \return ret_code in case of memory allocation error
 */
int
to_datetime(struct context *buffer, const struct fds_drec_field *field)
{
    const enum fds_iemgr_element_type type = field->info->def->data_type;


    if ((buffer->flags & FDS_CD2J_TS_FORMAT_MSEC) != 0) {
        // Convert to formatted string
        enum fds_convert_time_fmt fmt = FDS_CONVERT_TF_MSEC_UTC; // Only supported by JSON parser

        int ret_code = buffer_reserve(buffer,buffer_used(buffer) + FDS_CONVERT_STRLEN_DATE + 2); // 2x '\"'
        if (ret_code != FDS_OK){
            return ret_code;
        }

        *(buffer->write_begin++) = '"';
        int res = fds_datetime2str_be(field->data, field->size, type, buffer->write_begin,
            buffer_remain(buffer), fmt);
        if (res > 0) {
            // Success
            buffer->write_begin += res;
            *(buffer->write_begin++) = '"';
            return FDS_OK;
        }

        return FDS_ERR_ARG;
    }

    // Convert to UNIX timestamp (in milliseconds)
    uint64_t time;
    if (fds_get_datetime_lp_be(field->data, field->size, type, &time) != FDS_OK) {
        return FDS_ERR_ARG;
    }

    time = htobe64(time); // Convert to network byte order and use fast libfds converter
    int res = fds_uint2str_be(&time, sizeof(time), buffer->write_begin, buffer_remain(buffer));
    if (res > 0) {
        buffer->write_begin += res;
        return FDS_OK;
    }

    if (res != FDS_ERR_BUFFER) {
        return FDS_ERR_ARG;
    }

    int ret_code = buffer_reserve(buffer,buffer_used(buffer) + FDS_CONVERT_STRLEN_INT);
    if (ret_code != FDS_OK){
        return ret_code;
    }

    return to_datetime(buffer, field);
}


/**
 * \brief Convert a MAC address to JSON string
 *
 * \note
 *   Because the JSON doesn't directly support the MAC address, the result string is wrapped in
 *   double quotes.
 * \param[in] field Field to convert
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG if failed to convert a MAC address to string
 * \return ret_code in case of memory allocation error
 */
int
to_mac(struct context *buffer,const struct fds_drec_field *field)
{
    int ret_code = buffer_reserve(buffer,buffer_used(buffer) + FDS_CONVERT_STRLEN_MAC + 2); // MAC address + 2x '\"'
    if (ret_code != FDS_OK){
        return ret_code;
    }

    *(buffer->write_begin++) = '"';
    int res = fds_mac2str(field->data, field->size, buffer->write_begin, buffer_remain(buffer));
    if (res > 0) {
        buffer->write_begin += res;
        *(buffer->write_begin++) = '"';
        return FDS_OK;
    }

    return FDS_ERR_ARG;
}

/**
 * \brief Convert an IPv4/IPv6 address to JSON string
 *
 * \note
 *   Because the JSON doesn't directly support IP addresses, the result string is wrapped in double
 *   quotes.
 * \param[in] field Field to convert
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG if failed to convert an IP address to string
 * \return ret_code in case of memory allocation error
 */
int
to_ip(struct context *buffer, const struct fds_drec_field *field)
{
    // Make sure that we have enough memory
    int ret_code = buffer_reserve(buffer, buffer_used(buffer) + FDS_CONVERT_STRLEN_IP + 2); // IPv4/IPv6 address + 2x '\"'
    if (ret_code != FDS_OK){
        return ret_code;
    }
    *(buffer->write_begin++) = '"';
    int res = fds_ip2str(field->data, field->size, buffer->write_begin, buffer_remain(buffer));
    if (res > 0) {
        buffer->write_begin += res;
        *(buffer->write_begin++) = '"';
        return FDS_OK;
    }

    return FDS_ERR_ARG;
}

/**
 * \def ESCAPE_CHAR
 * \brief Auxiliary function for escaping characters
 * \param[in] ch Character
 */
#define ESCAPE_CHAR(ch) { \
    out_buffer[idx_output++] = '\\'; \
    out_buffer[idx_output++] = (ch); \
}

/**
 * \brief Convert IPFIX string to JSON string
 *
 * Non-ASCII characters are automatically converted to the special escaped sequence i.e. '\uXXXX'.
 * Quote and backslash are always escaped and while space (and control) characters are converted
 * based on active configuration.
 * \param[in] field Field to convert
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG if failed to convert an IP address to string
 * \return ret_code in case of memory allocation error
 */
int
to_string(struct context *buffer, const struct fds_drec_field *field)
{
    /* Make sure that we have enough memory for the worst possible case (escaping everything)
     * This case contains only non-printable characters that will be replaced with string
     * "\uXXXX" (6 characters) each.
     */
    const size_t max_size = (6 * field->size) + 3U; // '\uXXXX' + 2x "\"" + 1x '\0'
    int ret_code = buffer_reserve(buffer,buffer_used(buffer) + max_size);
    if (ret_code != FDS_OK){
        return ret_code;
    }

    const uint8_t *in_buffer =(const uint8_t *)(field->data);
    char *out_buffer = buffer->write_begin;
    uint32_t idx_output = 0;



    // Beginning of the string
    out_buffer[idx_output++] = '"';

    for (uint32_t i = 0; i < field->size; ++i) {
        // All characters from the extended part of ASCII must be escaped
        if (in_buffer[i] > 0x7F) {
            snprintf(&out_buffer[idx_output], 7, "\\u00%02x", in_buffer[i]);
            idx_output += 6;
            continue;
        }

        /*
         * Based on RFC 4627 (Section: 2.5. Strings):
         * Control characters (i.e. 0x00 - 0x1F), '"' and  '\' must be escaped
         * using "\"", "\\" or "\uXXXX" where "XXXX" is a hexa value.
         */
        if (in_buffer[i] > 0x1F && in_buffer[i] != '"' && in_buffer[i] != '\\') {
            // Copy to the output buffer
            out_buffer[idx_output++] = in_buffer[i];
            continue;
        }

        // Copy as escaped character
        switch(in_buffer[i]) {
        case '\\': // Reverse solidus
        ESCAPE_CHAR('\\');
            continue;
        case '\"': // Quotation
        ESCAPE_CHAR('\"');
            continue;
        default:
            break;
        }

        if ((buffer->flags & FDS_CD2J_NON_PRINTABLE) != 0) {
            // Skip white space characters
            continue;
        }

        switch(in_buffer[i]) {
        case '\t': // Tabulator
        ESCAPE_CHAR('t');
            break;
        case '\n': // New line
        ESCAPE_CHAR('n');
            break;
        case '\b': // Backspace
        ESCAPE_CHAR('b');
            break;
        case '\f': // Form feed
        ESCAPE_CHAR('f');
            break;
        case '\r': // Return
        ESCAPE_CHAR('r');
            break;
        default: // "\uXXXX"
            snprintf(&out_buffer[idx_output], 7, "\\u00%02x", in_buffer[i]);
            idx_output += 6;
            break;
        }
    }

    // End of the string
    out_buffer[idx_output++] = '"';
    buffer->write_begin += idx_output;
    return FDS_OK;
}

#undef ESCAPE_CHAR

/**
 * \brief Convert TCP flags to JSON string
 *
 * \note The result string is wrapped in double quotes.
 * \param[in] field Field to convert
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG if failed to convert TCP flags to string
 * \return ret_code in case of memory allocation error
 */
int
to_flags(struct context *buffer, const struct fds_drec_field *field)
{
    if (field->size != 1 && field->size != 2) {
        return FDS_ERR_ARG;
    }

    uint8_t flags;
    if (field->size == 1) {
        flags = *field->data;
    } else {
        flags = ntohs(*(uint16_t*)(field->data));
    }

    const size_t size = 8; // 2x '"' + 6x flags
    int ret_code = buffer_reserve(buffer,buffer_used(buffer) + size + 1); // '\0'
    if (ret_code != FDS_OK){
        return ret_code;
    }

    char *buff = buffer->write_begin;
    buff[0] = '"';
    buff[1] = (flags & 0x20) ? 'U' : '.';
    buff[2] = (flags & 0x10) ? 'A' : '.';
    buff[3] = (flags & 0x08) ? 'P' : '.';
    buff[4] = (flags & 0x04) ? 'R' : '.';
    buff[5] = (flags & 0x02) ? 'S' : '.';
    buff[6] = (flags & 0x01) ? 'F' : '.';
    buff[7] = '"';
    buff[8] = '\0';

    buffer->write_begin += size;
    return FDS_OK;
}

/**
 * \brief Convert a protocol to JSON string
 *
 * \note The result string is wrapped in double quotes.
 * \param[in] field Field to convert
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG if failed to convert a protocol to string
 * \return ret_code in case of memory allocation error
 */
int
to_proto(struct context *buffer, const struct fds_drec_field *field)
{
    if (field->size != 1) {
        return FDS_ERR_ARG;
    }

    const char *proto_str = protocols[*field->data];
    const size_t proto_len = strlen(proto_str);
    int ret_code = buffer_reserve(buffer,buffer_used(buffer) + proto_len + 3); // 2x '"' + '\0'
    if (ret_code != FDS_OK){
        return ret_code;
    }

    *(buffer->write_begin++) = '"';
    memcpy(buffer->write_begin, proto_str, proto_len);
    buffer->write_begin += proto_len;
    *(buffer->write_begin++) = '"';
    return FDS_OK;
}

/**
 * \brief Find a conversion function for an IPFIX field
 * \param[in] field An IPFIX field to convert
 * \return Conversion function
 */
converter_fn
get_converter(const struct fds_drec_field *field)
{
    // Conversion table, based on types defined by enum fds_iemgr_element_type
    static const converter_fn table[] = {
        &to_octet,    // FDS_ET_OCTET_ARRAY
        &to_uint,     // FDS_ET_UNSIGNED_8
        &to_uint,     // FDS_ET_UNSIGNED_16
        &to_uint,     // FDS_ET_UNSIGNED_32
        &to_uint,     // FDS_ET_UNSIGNED_64
        &to_int,      // FDS_ET_SIGNED_8
        &to_int,      // FDS_ET_SIGNED_16
        &to_int,      // FDS_ET_SIGNED_32
        &to_int,      // FDS_ET_SIGNED_64
        &to_float,    // FDS_ET_FLOAT_32
        &to_float,    // FDS_ET_FLOAT_64
        &to_bool,     // FDS_ET_BOOLEAN
        &to_mac,      // FDS_ET_MAC_ADDRESS
        &to_string,   // FDS_ET_STRING
        &to_datetime, // FDS_ET_DATE_TIME_SECONDS
        &to_datetime, // FDS_ET_DATE_TIME_MILLISECONDS
        &to_datetime, // FDS_ET_DATE_TIME_MICROSECONDS
        &to_datetime, // FDS_ET_DATE_TIME_NANOSECONDS
        &to_ip,       // FDS_ET_IPV4_ADDRESS
        &to_ip        // FDS_ET_IPV6_ADDRESS
        // Other types are not supported yet
    };

    const size_t table_size = sizeof(table) / sizeof(table[0]);
    const enum fds_iemgr_element_type type = (field->info->def != NULL)
        ? (field->info->def->data_type) : FDS_ET_OCTET_ARRAY;

    if (type >= table_size) {
        return &to_octet;
    } else {
        return table[type];
    }
}


/**
 * \brief Append the buffer with a name of an Information Element
 *
 * If the definition of a field is unknown, numeric identification is added.
 * \param[in] field Field identification to add
 * \param[in] buffer Buffer
 * \return #FDS_OK on success
 * \return ret_code in case of memory allocation error
 */
int
add_field_name(struct context *buffer, const struct fds_drec_field *field)
{
    const struct fds_iemgr_elem *def = field->info->def;

    if (def == NULL) {
        int scope_size = 32;
        char raw_name[scope_size];

        // Unknown field - max length is "\"en" + 10x <en> + ":id" + 5x <id> + '\"\0'
        snprintf(raw_name, scope_size, "\"en%" PRIu32 ":id%" PRIu16 "\":", field->info->en,
            field->info->id);

        int ret_code = buffer_append(buffer,raw_name);
        if (ret_code != FDS_OK){
            return ret_code;
        }
        return FDS_OK;
    }

    const size_t scope_size = strlen(def->scope->name);
    const size_t elem_size = strlen(def->name);

    size_t size = scope_size + elem_size + 5; // 2x '"' + 2x ':' + '\0'
    int ret_code = buffer_reserve(buffer, buffer_used(buffer) + size);
    if (ret_code != FDS_OK){
        return ret_code;
    }
    *(buffer->write_begin++) = '"';
    memcpy(buffer->write_begin, def->scope->name, scope_size);
    buffer->write_begin += scope_size;

    *(buffer->write_begin++) = ':';
    memcpy(buffer->write_begin, def->name, elem_size);
    buffer->write_begin += elem_size;
    *(buffer->write_begin++) = '"';
    *(buffer->write_begin++) = ':';

    return FDS_OK;
}
/**
 * \breaf Function for scoping fields with same ID
 *
 * \param[in] rec
 * \param[in] buffer
 * \param[in] fn Convert for field
 * \param[in] en Enterprise Number of field
 * \param[in] id ID of field
 *
 * \return #FDS_OK on success
 * \return ret_code in case of memory allocation error
 *
*/
int
multi_fields (const struct fds_drec *rec, struct context *buffer,
    converter_fn fn, uint32_t en, uint16_t id)
{
    // inicialization of iterator
    uint16_t iter_flag = (buffer->flags & FDS_CD2J_IGNORE_UNKNOWN) ? FDS_DREC_UNKNOWN_SKIP : 0;
    struct fds_drec_iter iter_mul_f;
    fds_drec_iter_init(&iter_mul_f, (struct fds_drec *) rec, iter_flag);

    // multi fields must be like "enXX:idYY":[value, value...]
    int ret_code;
    // append "["
    ret_code = buffer_append(buffer,"[");
    if (ret_code != FDS_OK){
        return ret_code;
    }

    // looking for multi fields
    while (fds_drec_iter_next(&iter_mul_f) != FDS_EOC) {
        const struct fds_tfield *def = iter_mul_f.field.info;
        char *writer_pos = buffer->write_begin;

        // check for simular ID and enterprise number
        if (def->id != id || def->en != en){
            continue;
        }
        ret_code = fn(buffer, &iter_mul_f.field);

        switch (ret_code) {
            // Recover from a conversion error
            case FDS_ERR_ARG:
                buffer->write_begin = writer_pos;
                ret_code = buffer_append(buffer, "null");
                if (ret_code != FDS_OK){
                    return ret_code;
                }
            case FDS_OK:
                break;
            default:
                // Other erros -> completly out
                return ret_code;
            }

            // if it last field, then go out from loop
            if (def->flags & FDS_TFIELD_LAST_IE){
                break;
            }

            // otherwise add "," and continue
            ret_code = buffer_append(buffer, ",");
            if (ret_code != FDS_OK){
                return ret_code;
            }
            continue;

    }

    // add "]" in the end if trehe are no more fields with same ID or EN
    ret_code = buffer_append(buffer, "]" );
    if (ret_code != FDS_OK){
        return ret_code;
    }

    return FDS_OK;
}

int
fds_drec2json(const struct fds_drec *rec, uint32_t flags, char **str,
    size_t *str_size)
{
    // Control pointer to buffer
    bool null_buffer = false;
    if (*str == NULL) {
        null_buffer = true;
        *str = malloc(BUFFER_BASE);
        if (*str == NULL) {
            return FDS_ERR_NOMEM;
        }

        *str_size = BUFFER_BASE;
        flags |= FDS_CD2J_ALLOW_REALLOC;

    }

    struct context record;

    record.buffer_begin = *str;
    record.buffer_end = *str + *str_size;
    record.write_begin = record.buffer_begin;
    record.allow_real = ((flags & FDS_CD2J_ALLOW_REALLOC) != 0);
    record.flags = flags;

    converter_fn fn;
    unsigned int added = 0;
    int ret_code = buffer_append(&record,"{\"@type\":\"ipfix.entry\",");
    if (ret_code != FDS_OK){
        goto error;
    }
    // Try to convert each field in the record
    uint16_t iter_flag = (record.flags & FDS_CD2J_IGNORE_UNKNOWN) ? FDS_DREC_UNKNOWN_SKIP : 0;

    struct fds_drec_iter iter;
    fds_drec_iter_init(&iter, (struct fds_drec *) rec, iter_flag);

    while (fds_drec_iter_next(&iter) != FDS_EOC) {
        // if flag of multi fields is set,
        // then this field will be skiped and processed later
        const fds_template_flag_t field_flags = iter.field.info->flags;
        if ((field_flags & FDS_TFIELD_MULTI_IE) != 0 && (field_flags & FDS_TFIELD_LAST_IE) == 0){
            continue;
        }

        // Separate fields
        if (added != 0) {
            // Add comma
            ret_code = buffer_append(&record,",");
            if (ret_code != FDS_OK){
                goto error;
            }
        }

        // Add field name "<pen>:<field_name>"
        ret_code = add_field_name(&record, &iter.field);
        if (ret_code != FDS_OK){
            goto error;
        }

        // Find a converter
        const struct fds_tfield *def = iter.field.info;
        if ((record.flags & FDS_CD2J_FORMAT_TCPFLAGS) && def->id == IANA_ID_FLAGS
                && (def->en == IANA_EN_FWD || def->en == IANA_EN_REV)) {
            // Convert to formatted flags
            fn = &to_flags;
        } else if ((record.flags & FDS_CD2J_FORMAT_PROTO) && def->id == IANA_ID_PROTO
                && (def->en == IANA_EN_FWD || def->en == IANA_EN_REV)) {
            // Convert to formatted protocol type
            fn = &to_proto;
        } else {
            // Convert to field based on internal type
            fn = get_converter(&iter.field);
        }

        // If nesesary, call function for write multi fields
        if ((field_flags & FDS_TFIELD_MULTI_IE) != 0 && (field_flags & FDS_TFIELD_LAST_IE) != 0){
           ret_code = multi_fields(rec, &record, fn, def->en, def->id);
           if (ret_code != FDS_OK){
               goto error;
           }
           continue;
        }

        // Convert the field
        char *writer_pos = record.write_begin;
        ret_code = fn(&record, &iter.field);


        switch (ret_code) {
        // Recover from a conversion error
        case FDS_ERR_ARG:
            record.write_begin = writer_pos;
            ret_code = buffer_append(&record, "null");
            if (ret_code != FDS_OK){
                goto error;
            }
        case FDS_OK:
            added++;
            continue;
        default:
            // Other erros -> completly out
            goto error;
        }
    }

    //update value of \p str_size
    *str = record.buffer_begin;
    *str_size = buffer_alloc(&record);

    buffer_append(&record,"}\n"); // This also adds '\0'
    return buffer_used(&record);

error:;
    if (null_buffer){
        free(str);
    }
    return ret_code;
}
