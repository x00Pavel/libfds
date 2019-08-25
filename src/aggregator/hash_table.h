
#define DEF_TABLE_SIZE 65536

/* Node for storing an item in a linked list */
struct node{
    int key;
    int value;
    struct node *next;
};

/* Structure for storing a linked list at each index of hash table  */
struct list {
    struct node *head; /* poiter to first element in list */
    struct node *tail; /* poiter to last element in list  */
};

/** \brief Function for generating hash
  *
  * \param[in] key Base for hash function
  *
  * \return Index in hash table
  */
int
hash_fnc(int key);

/** \brief Fucntion for filling in key and falue
  * \param[in]
  */
void
insert(int key, int value);

/** \brief Fucntion for allocating for hash table 
  *
  */
void hash_table_init();
