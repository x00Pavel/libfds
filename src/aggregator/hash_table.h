#include <libfds.h>

/** Node for storing an item in a linked list */
struct node{
    char *key;         /*< Key of node  */
    void *value;       /*< Value of key */
    struct node *next; /*< Next node    */
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
  * \param[in] table Pointer to table
  * \param[in] key   Key to be iserted
  * \param[in] value Value for given key
  */
FDS_API 
insert_key(struct hash_table *table, char *key, int value);

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
