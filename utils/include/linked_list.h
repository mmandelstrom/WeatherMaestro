#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

typedef struct Linked_Item
{
  struct Linked_Item* prev;
  struct Linked_Item* next;

  void* item; /* Item could be anything */
  
} Linked_Item;

typedef struct
{
  Linked_Item* head;
  Linked_Item* tail;

} Linked_List;


/** Adds item to a given Linked_List struct
 * Caller is only responsible for declarations */
int linked_list_add(Linked_List* _Linked_List, Linked_Item** _Linked_Item_Ptr, void* _item);

/** Removes Linked_Item from Linked_List */
void linked_list_remove(Linked_List* _Linked_List, Linked_Item* _Linked_Item);

/** Disposes of the entire Linked_List and all it's items
 * It does NOT dispose of the item pointer */
void linked_list_dispose(Linked_List* _Linked_List);

#endif
