
#define DEF_TABLE_SIZE 65536

/* Node for storing an item in a linked list */
struct node{
    char *key;
    void *value;
    struct node *next;
};

/* Structure for storing a linked list */
struct list {
    struct node *head; /* poiter to first element in the list */
    struct node *tail; /* poiter to last element in the list  */
};

/* Information about hash table*/
struct hash_table{
    size_t size;           /* Size of table          */
    struct list *table; /* Array with linked lists*/
};

/** \brief Function for generating hash
  *
  * \param[in] key Base for hash function
  *
  * \return Index in hash table
  */
int
hash_fnc(union field_id key);

/** \brief Fucntion for filling in key and falue
  * \param[in]
  */
void
insert_key(struct hash_table *table, char *key, int value);

/** \brief Fucntion for allocating for hash table
  *
  * \param[in] table Poiter to table to initialization
  * \param[out] table Poiter allocated memory for table
  */
struct hash_table *
hash_table_init(struct hash_table *table);

/** \brief Function for find element in hash table
  *
  * \param[in] table Poiter to hash table
  * \param[in] id Id of element to be found
  *
  * \return Pointer to element with given ID in success or NULL if ID is not found
  */
char*
find(struct hash_table *table, union field_id id);

/** \brief Function for cleaning up resources
  *
  * \param table Pointer to table to be cleaned
  */
void
clean(struct hash_table *table);
