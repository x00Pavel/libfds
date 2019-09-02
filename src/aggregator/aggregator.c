#include "aggregator.h"

/** Aray of sizes of used datatypes.
  * DO NOT REODER,
  * INDEXIS ARE ASSOCITED WITH enum fds_aggr_types !!
  */
size_t size_of[] = {
     4, // ??????  size of  octet array
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


FDS_API
fds_aggr_init(struct fds_aggr_memory *memory){

    if (memory == NULL){
        return FDS_ERR_ARG;
    }

    memory->key_id_list = NULL;
    memory->key_size = 0;
    memory->val_id_list = NULL;
    memory->val_size = 0;
    memory->sort_flags = 0;

    return FDS_OK;
}


int
fds_aggr_setup(const struct input_field *input_fields, struct fds_aggr_memory *memory, size_t size,
                int (*fds_aggr_get_element)(void*, int, struct fds_aggr_field*)){

    int key_count = 0;
    int val_count = 0;


   for (int i = 0; i < size; i++){
        if (input[i].fnc == FDS_KEY_FIELD){
            // Count total key size
            memory->key_size += size_of[input[i].type];
            // Count new array size
            const size_t array_size = (key_count + 1) * sizeof(union field_id); 
            memory->key_id_list = realloc(memory->key_id_list, array_size);
            if(memory->key_id_list == NULL){
                return FDS_ERR_NOMEM;
            }
            memory->key_id_list[key_count] = input[i].id;
            key_count++;
        }
        else {
            // Count total values size
            memory->val_size += size_of[input[i].type];
            // Count new array size
            const size_t array_size = (val_count + 1) * sizeof(struct field_info);
            memory->val_list = realloc(memory->val_list, array_size);
            if(memory->key_list == NULL){
                return FDS_ERR_NOMEM;
            }
            memory->val_list[val_count].id = input[i].id;
            memory->val_list[val_count].fnc = input[i].fnc;
            val_count++;
        }
    }


}
