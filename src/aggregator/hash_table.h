

/** Node for storing an item in a linked list */
struct node{
    char *key;
    void *value;
    struct node *next;
};

<<<<<<< HEAD
/** Structure for storing a linked list */
struct list {
    struct node *head; /*< poiter to first element in the list */
    struct node *tail; /*< poiter to last element in the list  */
};

/** Information about hash table */
struct hash_table{
    size_t size;       /*< Count of free fields    */
    struct list *list; /*< Array with linked lists */
=======
/* Structure for storing a linked list */
struct list {
    struct node *head; /* poiter to first element in the list */
    struct node *tail; /* poiter to last element in the list  */
};

/* Information about hash table*/
struct hash_table{
    size_t size;           /* Size of table          */
    struct list *table; /* Array with linked lists*/
>>>>>>> 4f2cf3d2c00636c24ee0a264ed07d7775bf91c78
};

/** \brief Function for generating hash
  *
  * \param[in] key Base for hash function
  *
  * \return Index to hash table
  */
int
hash_fnc(union field_id key);

/** \brief Fucntion for filling in key and falue
  * \param[in]
  */
void
insert_key(struct hash_table *table, char *key, int value);
<<<<<<< HEAD

/** \brief Fucntion for allocating for hash table
  *
  * \param[in] table Poiter to table to initialization
  * \param[out] table Poiter allocated memory for table
  */
void
hash_table_init(struct hash_table *table);

/** \brief Function finds the given key in the Linked List
  *
  * \param[in] list Poiter to linked list
  * \param[in] key Key to be found
  * 
  * \return In success, index of given key  
  */ 
int
find_key(struct node *list, char* key);

/** \brief Function for getting node from linked list
  *
  * \param[in] list Poiter to linked list
  * \param[in] index Index to be found
  *
  * \return Pointer to node on given index in success or NULL if node is not found
  */
struct node * 
get_element(struct node *list, int index);

=======

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
>>>>>>> 4f2cf3d2c00636c24ee0a264ed07d7775bf91c78

/** \brief Function for cleaning up resources
  *
  * \param table Pointer to table to be cleaned
  */
void
clean(struct hash_table *table);
