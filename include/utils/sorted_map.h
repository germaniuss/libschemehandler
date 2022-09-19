#ifndef _UTILS_SORTED_MAP_H
#define _UTILS_SORTED_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

typedef struct sorted_map sorted_map;
    typedef struct sorted_map_iterator sorted_map_iterator;

    /***************************************************************************
    * Allocates a new, empty sorted_map with given comparator function.               *
    ***************************************************************************/ 
    sorted_map* sorted_map_alloc (int (*comparator)(void*, void*));

    /***************************************************************************
    * If the sorted_map does not contain the key, inserts it in the sorted_map and associates*
    * the value with it, returning NULL. Otherwise, the value is updated and   *
    * the old value is returned.                                               *  
    ***************************************************************************/ 
    void* sorted_map_put (sorted_map* my_sorted_map, void* key, void* value);

    /***************************************************************************
    * Returns true if the key is sorted_mapped in the sorted_map.                            *
    ***************************************************************************/
    bool sorted_map_contains_key (sorted_map* my_sorted_map, void* key);

    /***************************************************************************
    * Returns the value associated with the key, or NULL if the key is not     *
    * sorted_mapped in the sorted_map.                                                       *
    ***************************************************************************/
    void* sorted_map_get (sorted_map* my_sorted_map, void* key);

    /***************************************************************************
    * If the key is sorted_mapped in the sorted_map, removes the sorted_mapping and returns the value*
    * of that sorted_mapping. If the sorted_map did not contain the sorted_mapping, returns NULL.   *
    ***************************************************************************/ 
    void* sorted_map_remove (sorted_map* my_sorted_map, void* key);

    /***************************************************************************
    * Removes all the contents of the sorted_map. Deallocates the sorted_map structures. The *
    * client user is responsible for deallocating the actual contents.         *  
    ***************************************************************************/ 
    void sorted_map_clear (sorted_map* my_sorted_map);

    /***************************************************************************
    * Returns the size of the sorted_map, or namely, the amount of key/value sorted_mappings *
    * in the sorted_map.                                                              *
    ***************************************************************************/ 
    size_t sorted_map_size (sorted_map* my_sorted_map);

    /***************************************************************************
    * Checks that the sorted_map maintains the AVL-tree property.                     *
    ***************************************************************************/  
    bool sorted_map_is_healthy (sorted_map* my_sorted_map);

    /***************************************************************************
    * Deallocates the entire sorted_map. Only the sorted_map and its nodes are deallocated.  *
    * The user is responsible for deallocating the actual data stored in the   * 
    * sorted_map.                                                                     *
    ***************************************************************************/ 
    void sorted_map_free (sorted_map* my_sorted_map);

    /***************************************************************************
    * Returns the iterator over the sorted_map. The entries are iterated in order.    *
    ***************************************************************************/  
    sorted_map_iterator* sorted_map_iterator_alloc (sorted_map* my_sorted_map);

    /***************************************************************************
    * Returns the number of keys not yet iterated over.                        *
    ***************************************************************************/ 
    size_t sorted_map_iterator_has_next (sorted_map_iterator* iterator);

    /***************************************************************************
    * Loads the next sorted_mapping in the iteration order.                           *
    ***************************************************************************/  
    bool sorted_map_iterator_next (sorted_map_iterator* iterator, 
                            void** key_pointer, 
                            void** value_pointer);

    /***************************************************************************
    * Returns a true if the sorted_map was modified during the iteration.             *
    ***************************************************************************/  
    bool sorted_map_iterator_is_disturbed (sorted_map_iterator* iterator);

    /***************************************************************************
    * Deallocates the sorted_map iterator.                                            *
    ***************************************************************************/  
    void sorted_map_iterator_free (sorted_map_iterator* iterator);

#ifdef __cplusplus
}
#endif

#if defined(_UTILS_IMPL) || defined(_UTILS_SORTED_MAP_IMPL)

#include <stdlib.h>

typedef struct sorted_map_entry {
    void*             key;
    void*             value;
    struct sorted_map_entry* left;
    struct sorted_map_entry* right;
    struct sorted_map_entry* parent;
    int               height;
} sorted_map_entry;

struct sorted_map {
    sorted_map_entry* root;
    int      (*comparator)(void*, void*);
    size_t     size;
    size_t     mod_count;
};

struct sorted_map_iterator {
    sorted_map*       owner_sorted_map;
    sorted_map_entry* next;
    size_t     iterated_count;
    size_t     expected_mod_count;
};
    
/*******************************************************************************
* Creates a new sorted_map entry and initializes its fields.                          *
*******************************************************************************/  
static sorted_map_entry* sorted_map_entry_t_alloc(void* key, void* value) 
{
    sorted_map_entry* entry = malloc(sizeof(*entry));

    if (!entry) 
    {
        return NULL;
    }
    
    entry->key    = key;
    entry->value  = value;
    entry->left   = NULL;
    entry->right  = NULL;
    entry->parent = NULL;
    entry->height = 0;

    return entry;
}

/*******************************************************************************
* Returns the height of an entry. The height of a non-existent entry is        *
* assumed to be -1.                                                            *
*******************************************************************************/
static int height(sorted_map_entry* node) 
{
    return node ? node->height : -1;
}

/*******************************************************************************
* Returns the maximum of the two input integers.                               *
*******************************************************************************/
static int max(int a, int b) 
{
    return a > b ? a : b;
}

/*******************************************************************************
* Performs a left rotation and returns the new root of a (sub)tree.            *
*******************************************************************************/
static sorted_map_entry* left_rotate(sorted_map_entry* node_1)
{
    sorted_map_entry* node_2 = node_1->right;
    
    node_2->parent    = node_1->parent;
    node_1->parent    = node_2;
    node_1->right     = node_2->left;
    node_2->left      = node_1;

    if (node_1->right) 
    {
        node_1->right->parent = node_1;
    }
    
    node_1->height = max(height(node_1->left), height(node_1->right)) + 1;
    node_2->height = max(height(node_2->left), height(node_2->right)) + 1;
    
    return node_2;
}

/*******************************************************************************
* Performs a right rotation and returns the new root of a (sub)tree.           *
*******************************************************************************/
static sorted_map_entry* right_rotate(sorted_map_entry* node_1)
{
    sorted_map_entry* node_2 = node_1->left;
    
    node_2->parent   = node_1->parent;
    node_1->parent   = node_2;
    node_1->left     = node_2->right;
    node_2->right    = node_1;

    if (node_1->left) 
    {
        node_1->left->parent = node_1;
    }
    
    node_1->height = max(height(node_1->left), height(node_1->right)) + 1;
    node_2->height = max(height(node_2->left), height(node_2->right)) + 1;
    
    return node_2;
}

/*******************************************************************************
* Performs a right rotation following by a left rotation and returns the root  *
* of the new (sub)tree.                                                        * 
*******************************************************************************/
static sorted_map_entry* right_left_rotate(sorted_map_entry* node_1) 
{
    sorted_map_entry* node_2 = node_1->right;
    
    node_1->right = right_rotate(node_2);
    
    return left_rotate(node_1);
}

/*******************************************************************************
* Performs a left rotation following by a right rotation and returns the root  *
* of the new (sub)tree.                                                        * 
*******************************************************************************/
static sorted_map_entry* left_right_rotate(sorted_map_entry* node_1)
{
    sorted_map_entry* node_2 = node_1->left;
    
    node_1->left = left_rotate(node_2);
    
    return right_rotate(node_1);
}

/*******************************************************************************
* Fixes the tree in order to balance it. Basically, we start from 'p_entry'    *
* go up the chain towards parents. If a parent is disbalanced, a set of        *
* rotations are applied. If 'insertion_mode' is on, it means that previous     *  
* modification was insertion of an entry. In such a case we need to perform    *
* only one rotation. If 'insertion_mode' is off, the last operation was        *
* removal and we need to go up until the root node.                            *
*******************************************************************************/  
static void fix_after_modification(sorted_map* p_sorted_map, 
                                   sorted_map_entry* entry,
                                   bool insertion_mode)
{
    sorted_map_entry* parent = entry->parent;
    sorted_map_entry* grand_parent;
    sorted_map_entry* sub_tree;

    while (parent) 
    {
        if (height(parent->left) == height(parent->right) + 2)
        {
            grand_parent = parent->parent;

            if (height(parent->left->left) >= height(parent->left->right)) 
            {
                sub_tree = right_rotate(parent);
            }
            else
            {
                sub_tree = left_right_rotate(parent);
            }
                
            if (!grand_parent) 
            {
                p_sorted_map->root = sub_tree;
            }
            else if (grand_parent->left == parent) 
            {
                grand_parent->left = sub_tree;
            }
            else
            {
                grand_parent->right = sub_tree;
            }
             
            if (grand_parent)
            {
                grand_parent->height = 
                        max(height(grand_parent->left),
                                   height(grand_parent->right)) + 1;
            }   

            /* Fixing after insertion requires only one rotation. */
            if (insertion_mode) 
            {
                return;
            }
        }
        else if (height(parent->right) == height(parent->left) + 2) 
        {
            grand_parent = parent->parent;

            if (height(parent->right->right) >= height(parent->right->left)) 
            {
                sub_tree = left_rotate(parent);
            }
            else
            {
                sub_tree = right_left_rotate(parent);
            }
                
            if (!grand_parent)
            {
                p_sorted_map->root = sub_tree;
            }
            else if (grand_parent->left == parent)
            {
                grand_parent->left = sub_tree;
            }
            else
            {
                grand_parent->right = sub_tree;
            }
             
            if (grand_parent)
            {   
                grand_parent->height = 
                        max(height(grand_parent->left),
                            height(grand_parent->right)) + 1;
            }

            /* Fixing after insertion requires only one rotation. */
            if (insertion_mode) 
            {
                return;
            }
        }

        parent->height = max(height(parent->left),
                             height(parent->right)) + 1;
        parent = parent->parent;
    }
}

/*******************************************************************************
* Performs the actual insertion of an entry.                                   *
*******************************************************************************/
static void insert(sorted_map* my_sorted_map, void* key, void* value) 
{
    sorted_map_entry* new_entry = sorted_map_entry_t_alloc(key, value);
    sorted_map_entry* x;
    sorted_map_entry* parent;

    if (!new_entry)
    {
        return;
    }
    
    if (!my_sorted_map->root)
    {
        my_sorted_map->root = new_entry;
        my_sorted_map->size++;
        my_sorted_map->mod_count++;
        
        return;
    }

    x = my_sorted_map->root;
    parent = NULL;

    while (x) 
    {
        parent = x;

        if (my_sorted_map->comparator(new_entry->key, x->key) < 0)
        {
            x = x->left;
        }
        else
        {
            x = x->right;
        }
    }

    new_entry->parent = parent;

    if (my_sorted_map->comparator(new_entry->key, parent->key) < 0) 
    {
        parent->left = new_entry;
    }
    else
    {
        parent->right = new_entry;
    }

    /** TRUE means we choose the insertion mode for fixing the tree. */
    fix_after_modification(my_sorted_map, new_entry, true);
    my_sorted_map->size++;
    my_sorted_map->mod_count++;
}

/*******************************************************************************
* Returns the minimum entry of a subtree rooted at 'p_entry'.                  *
*******************************************************************************/  
static sorted_map_entry* min_entry(sorted_map_entry* entry)
{
    while (entry->left) 
    {
        entry = entry->left;
    }
    
    return entry;
}

/*******************************************************************************
* Returns the successor entry as specified by the order implied by the         *
* comparator.                                                                  *
*******************************************************************************/
static sorted_map_entry* get_successor_entry(sorted_map_entry* entry)
{
    sorted_map_entry* parent;

    if (entry->right) 
    {
        return min_entry(entry->right);
    }
    
    parent = entry->parent;

    while (parent && parent->right == entry)
    {
        entry = parent;
        parent = parent->parent;
    }

    return parent;
}

/*******************************************************************************
* This routine is responsible for removing entries from the tree.              *
*******************************************************************************/  
static sorted_map_entry* delete_entry(sorted_map* my_sorted_map, sorted_map_entry* entry)
{
    sorted_map_entry* parent;
    sorted_map_entry* child;
    sorted_map_entry* successor;

    void* tmp_key;
    void* tmp_value;

    if (!entry->left && !entry->right)
    {
        /** The node to delete has no children. */
        parent = entry->parent;

        if (!parent) 
        {
            my_sorted_map->root = NULL;
            my_sorted_map->size--;
            my_sorted_map->mod_count++;
            
            return entry;
        }

        if (entry == parent->left) 
        {
            parent->left = NULL;
        }
        else
        {
            parent->right = NULL;
        }

        my_sorted_map->size--;
        my_sorted_map->mod_count++;
        
        return entry;
    }

    if (!entry->left || !entry->right)
    {
        /** The node has exactly one child. */
        if (entry->left)
        {
            child = entry->left;
        }
        else
        {
            child = entry->right;
        }

        parent = entry->parent;
        child->parent = parent;

        if (!parent) 
        {
            my_sorted_map->root = child;
            my_sorted_map->size--;
            my_sorted_map->mod_count++;
            
            return entry;
        }

        if (entry == parent->left)
        {
            parent->left = child;
        }
        else 
        {
            parent->right = child;
        }

        my_sorted_map->size--;
        my_sorted_map->mod_count++;
        
        return entry;
    }

    /** The node to remove has both children. */
    tmp_key        = entry->key;
    tmp_value      = entry->value;
    successor      = min_entry(entry->right);
    entry->key     = successor->key;
    entry->value   = successor->value;
    child          = successor->right;
    parent         = successor->parent;

    if (parent->left == successor)
    {
        parent->left = child;
    }   
    else
    {
        parent->right = child;
    }
 
    if (child)
    {
        child->parent = parent;
    }
        
    my_sorted_map->size--;
    my_sorted_map->mod_count++;
    successor->key   = tmp_key;
    successor->value = tmp_value;
    
    return successor;
}

/*******************************************************************************
* Searches for an entry with key 'key'. Returns NULL if there is no such.      *
*******************************************************************************/  
static sorted_map_entry* find_entry(sorted_map* my_sorted_map, void* key)
{
    sorted_map_entry* p_entry = my_sorted_map->root;

    while (p_entry && my_sorted_map->comparator(key, p_entry->key) != 0)
    {
        if (my_sorted_map->comparator(key, p_entry->key) < 0)
        {
            p_entry = p_entry->left;
        }
        else
        {
            p_entry = p_entry->right;
        }
    }

    return p_entry;
}

sorted_map* sorted_map_alloc(int (*comparator)(void*, void*)) 
{
    sorted_map* my_sorted_map;

    if (!comparator) 
    {
        return NULL;
    }
    
    my_sorted_map = malloc(sizeof(*my_sorted_map));

    if (!my_sorted_map) 
    {
        return NULL;
    }
    
    my_sorted_map->root = NULL;
    my_sorted_map->comparator = comparator;
    my_sorted_map->size = 0;
    my_sorted_map->mod_count = 0;

    return my_sorted_map;
}

void* sorted_map_put(sorted_map* my_sorted_map, void* key, void* value)
{
    sorted_map_entry* target;
    void* old_value;

    if (!my_sorted_map) 
    {
        return NULL;
    }
    
    target = find_entry(my_sorted_map, key);

    if (target)
    {
        old_value = target->value;
        target->value = value;
        return old_value; 
    } 

    insert(my_sorted_map, key, value);
    return NULL;
}

bool sorted_map_contains_key (sorted_map* my_sorted_map, void* key)
{
    if (!my_sorted_map) 
    {
        return false;
    }

    return find_entry(my_sorted_map, key);
}

void* sorted_map_get(sorted_map* my_sorted_map, void* key)
{
    sorted_map_entry* entry;

    if (!my_sorted_map) 
    {
        return NULL;
    }
    
    entry = find_entry(my_sorted_map, key);
    
    return entry ? entry->value : NULL;
}

void* sorted_map_remove(sorted_map* my_sorted_map, void* key)
{
    void* value;
    sorted_map_entry* entry;

    if (!my_sorted_map) 
    {
        return NULL;
    }
    
    entry = find_entry(my_sorted_map, key);

    if (!entry) 
    {
        return NULL;
    }
    
    value = entry->value;
    entry = delete_entry(my_sorted_map, entry);
    fix_after_modification(my_sorted_map, entry, false);
    free(entry);
    
    return value;
}

/*******************************************************************************
* This routine implements the actual checking of tree balance.                 *
*******************************************************************************/  
static bool check_balance_factors_impl(sorted_map_entry* entry)
{
    if (!entry) 
    {
        return true;
    }
    
    if (abs(height(entry->left) - height(entry->right)) > 1)        
    {
        return false;
    }
    
    if (!check_balance_factors_impl(entry->left)) 
    {
        return false;
    }
    
    if (!check_balance_factors_impl(entry->right)) 
    {
        return false;
    }
    
    return true;
}

/*******************************************************************************
* Checks that every node in the sorted_map is balanced.                               *
*******************************************************************************/  
static int check_balance_factors(sorted_map* my_sorted_map) 
{
    return check_balance_factors_impl(my_sorted_map->root);
}

/*******************************************************************************
* This routine implements the actual height verification algorithm. It uses a  *
* sentinel value of -2 for denoting the fact that a current subtree contains   *
* at least one unbalanced node.                                                *  
*******************************************************************************/  
static int check_heights_impl(sorted_map_entry* entry)
{
    int height_left;
    int height_right;
    int height_both;

    /**********************************************************
    * The base case: the height of a non-existent leaf is -1. *
    **********************************************************/ 
    if (!entry)
    {
        return -1;
    }
    
    height_left = check_heights_impl(entry->left) + 1;

    if (height_left == -2)
    {
        return -2;
    }
    
    height_right = check_heights_impl(entry->right) + 1;

    if (height_right == -2)
    {
        return -2;
    }
    
    if ((height_both = max(height_left, 
                           height_right)) != entry->height)
    {
        return -2;
    }
    
    return height_both;
}

/*******************************************************************************
* This routine checks that the height field of each sorted_map entry (node) is        *
* correct.                                                                     *
*******************************************************************************/  
static int check_heights(sorted_map* my_sorted_map)
{
    return check_heights_impl(my_sorted_map->root) != -2;
}

bool sorted_map_is_healthy(sorted_map* my_sorted_map) 
{
    if (!my_sorted_map) 
    {
        return false;
    }
    
    if (!check_heights(my_sorted_map)) 
    {
        return false;
    }
    
    return check_balance_factors(my_sorted_map);
}

/*******************************************************************************
* Implements the actual deallocation of the tree entries by traversing the     *
* tree in post-order.                                                          * 
*******************************************************************************/  
static void sorted_map_free_impl(sorted_map_entry* entry)
{
    if (!entry)
    {
        return;
    }
    
    sorted_map_free_impl(entry->left);
    sorted_map_free_impl(entry->right);
    free(entry);
}

void sorted_map_free(sorted_map* my_sorted_map) 
{
    if (!my_sorted_map || !my_sorted_map->root)      
    {
        return;
    }
    
    sorted_map_free_impl(my_sorted_map->root);
    free(my_sorted_map);
}

void sorted_map_clear(sorted_map* my_sorted_map) 
{
    if (!my_sorted_map || !my_sorted_map->root) 
    {
        return;
    }
    
    sorted_map_free_impl(my_sorted_map->root);
    my_sorted_map->mod_count += my_sorted_map->size;
    my_sorted_map->root = NULL;
    my_sorted_map->size = 0;
}

size_t sorted_map_size(sorted_map* my_sorted_map) 
{
    return my_sorted_map ? my_sorted_map->size : 0;
}

sorted_map_iterator* sorted_map_iterator_alloc(sorted_map* my_sorted_map)
{
    sorted_map_iterator* iterator;
    
    if (!my_sorted_map) 
    {
        return NULL;
    }
    
    iterator = malloc(sizeof(*iterator));
    iterator->expected_mod_count = my_sorted_map->mod_count;
    iterator->iterated_count = 0;
    iterator->owner_sorted_map = my_sorted_map;
    iterator->next = my_sorted_map->root ? min_entry(my_sorted_map->root) : NULL;
    
    return iterator;
}

size_t sorted_map_iterator_has_next(sorted_map_iterator* iterator) 
{
    if (!iterator)
    {
        return 0;
    }
    
    /** If the sorted_map was modified, stop iteration. */
    if (sorted_map_iterator_is_disturbed(iterator)) 
    {
        return 0;
    }
    
    return iterator->owner_sorted_map->size - iterator->iterated_count;
}

bool sorted_map_iterator_next(sorted_map_iterator* iterator, 
                         void** key_pointer, 
                         void** value_pointer)
{
    if (!iterator)    
    {
        return false;
    }
    
    if (!iterator->next)     
    {
        return false;
    }
    
    if (sorted_map_iterator_is_disturbed(iterator))
    {
        return false;
    }
    
    *key_pointer   = iterator->next->key;
    *value_pointer = iterator->next->value;
    iterator->iterated_count++;
    iterator->next = get_successor_entry(iterator->next);
    
    return true;
}

bool sorted_map_iterator_is_disturbed(sorted_map_iterator* iterator) 
{
    if (!iterator) 
    {
        return false;
    }
    
    return iterator->expected_mod_count != iterator->owner_sorted_map->mod_count;
}

void sorted_map_iterator_free(sorted_map_iterator* iterator) 
{
    if (!iterator) 
    {
        return;
    }
    
    iterator->owner_sorted_map = NULL;
    iterator->next = NULL;
    free(iterator);
}

#endif
#endif