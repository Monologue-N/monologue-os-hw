#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "my402list.h"

/* Returns the number of elements in the list. */
int My402ListLength(My402List *list)
{
    return list->num_members;
}

/* Returns TRUE if the list is empty. Returns FALSE otherwise. */
int  My402ListEmpty(My402List *list)
{
    return list->num_members <= 0 ? TRUE : FALSE;
}

/* If list is empty, just add obj to the list. Otherwise, add obj after Last(). This function returns TRUE if the operation is performed successfully and returns FALSE otherwise. */
int  My402ListAppend(My402List *list, void *obj)
{
    My402ListElem *new = (My402ListElem *)malloc(sizeof(My402ListElem));
    new->obj = obj;
    if (!new) {
        return FALSE;   
    } else { 
        if (My402ListEmpty(list)) {   
            new->next = new->prev = &(list->anchor);
            list->anchor.next = list->anchor.prev = new;
        } else {    
            new->next = &(list->anchor);
            new->prev = list->anchor.prev;
            list->anchor.prev->next = new;
            list->anchor.prev = new;
        }
        list->num_members++;
        return TRUE;    
    }
}

/* If list is empty, just add obj to the list. Otherwise, add obj before First(). This function returns TRUE if the operation is performed successfully and returns FALSE otherwise. */
int  My402ListPrepend(My402List *list, void *obj)
{
    My402ListElem *new = (My402ListElem *)malloc(sizeof(My402ListElem));
    new->obj = obj;
    if (!new) {
        return FALSE;   
    } else {
        if (My402ListEmpty(list)) {   
            new->next = new->prev = &(list->anchor);
            list->anchor.next = list->anchor.prev = new;
        } else {    
            new->next = list->anchor.next;
            new->prev = &(list->anchor);
            list->anchor.next->prev = new;
            list->anchor.next = new;
        }
        list->num_members++;
        return TRUE;    
    }
}

/* Unlink and delete elem from the list. Please do not delete the object pointed to by elem and do not check if elem is on the list. */
void My402ListUnlink(My402List *list, My402ListElem *elem)
{
    if (!My402ListEmpty(list)) {
        elem->prev->next = elem->next;
        elem->next->prev = elem->prev;
        list->num_members--;
        free(elem);
    }
}

/* Unlink and delete all elements from the list and make the list empty. Please do not delete the objects pointed to by the list elements. */
void My402ListUnlinkAll(My402List *list)
{
    if (!My402ListEmpty(list)) {
            My402ListElem *elem = NULL;
            for (elem = My402ListFirst(list); elem != NULL; elem = My402ListNext(list, elem)) {
                list->num_members--;
                free(elem); 
            }
    }
}

/* Insert obj between elem and elem->prev. If elem is NULL, then this is the same as Prepend(). This function returns TRUE if the operation is performed successfully and returns FALSE otherwise. Please do not check if elem is on the list. */
int  My402ListInsertBefore(My402List *list, void *obj, My402ListElem *elem)
{
    if (elem == NULL) {
        My402ListPrepend(list, obj);
        return TRUE;
    } else {
        My402ListElem *new = (My402ListElem *)malloc(sizeof(My402ListElem));
        if (!new) {
            return FALSE;
        } else {
            new->prev = elem->prev;
            new->next = elem;
            elem->prev->next = new;
            elem->prev = new;
            new->obj = obj;
            list->num_members++;
            return TRUE;    
        }
    }
}

/* Insert obj between elem and elem->next. If elem is NULL, then this is the same as Append(). This function returns TRUE if the operation is performed successfully and returns FALSE otherwise. Please do not check if elem is on the list. */
int  My402ListInsertAfter(My402List *list, void *obj, My402ListElem *elem)
{
    if (elem == NULL) {
        My402ListAppend(list, obj);
        return TRUE;
    } else {
        My402ListElem *new = (My402ListElem *)malloc(sizeof(My402ListElem));
        if (!new) {
            return FALSE;
        } else {
            new->next = elem->next;
            new->prev = elem;
            elem->next->prev = new;
            elem->next = new;
            new->obj = obj;
            list->num_members++;
            return TRUE;    
        }
    }
}

/* Returns the first list element or NULL if the list is empty. */
My402ListElem *My402ListFirst(My402List *list)
{
    if (My402ListEmpty(list)) {
        return NULL;    
    } else {
        return list->anchor.next;
    }
}

/* Returns the last list element or NULL if the list is empty. */
My402ListElem *My402ListLast(My402List *list)
{
    if (My402ListEmpty(list)) {
        return NULL;   
    } else {
        return list->anchor.prev;
    }
}

/* Returns elem->next or NULL if elem is the last item on the list. Please do not check if elem is on the list. */
My402ListElem *My402ListNext(My402List *list, My402ListElem *elem)
{
    if (elem == list->anchor.prev) {
        return NULL;    
    } else {
        return elem->next;
    }
}

/* Returns elem->prev or NULL if elem is the first item on the list. Please do not check if elem is on the list. */
My402ListElem *My402ListPrev(My402List *list, My402ListElem *elem)
{
    if (elem == list->anchor.next) {
        return NULL;    // return NULL if elem is the first item
    } else {
        return elem->prev;
    }
}

/* Returns the list element elem such that elem->obj == obj. Returns NULL if no such element can be found. */
My402ListElem *My402ListFind(My402List *list, void *obj)
{
    My402ListElem *elem = NULL;
    for (elem = My402ListFirst(list); elem != NULL; elem = My402ListNext(list, elem)) {
        if (elem->obj == obj) {
            return elem;
        }
    }
    return NULL;
}

/* Initialize the list into an empty list. Returns TRUE if all is well and returns FALSE if there is an error initializing the list. */
int My402ListInit(My402List *list)
{
    if (!list) return FALSE;    
    list->anchor.next = &(list->anchor);
    list->anchor.prev = &(list->anchor);
    list->anchor.obj = NULL;
    list->num_members = 0;
    return TRUE;    
}

/* Allow the list to be traversed */
void My402ListTraverse(My402List *list)
{
    My402ListElem *elem = NULL;
    for (elem = My402ListFirst(list); elem != NULL; elem = My402ListNext(list, elem)) {
        printf("%d ", *((int*)(elem->obj)));
    }
    printf("\n");
}
