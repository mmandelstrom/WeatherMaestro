/** Simple linked list util
 * Caller is responsible for:
 * - Declaring and nulling the Linked_list before adding item
 * - Calling dispose when finished */

#ifndef __LINKED_LIST_H__
#define __LINKED_LIST_H__

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

#define linked_list_foreach(list, node) \
  for (Linked_Item* node = (list)->head; node != NULL; node = node->next)

int linked_list_add(Linked_List* _Linked_List, Linked_Item** _Linked_Item_Ptr, void* _item);

void linked_list_remove(Linked_List* _Linked_List, Linked_Item* _Linked_Item);

void linked_list_dispose(Linked_List* _Linked_List);

#endif
