#include <string.h>
#include <stdio.h>

#include "bf_component.h"

#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while(0)


/* =========================
 *  LRU_List !!
 * =========================
 *  */
void LRU_List_Init(LRU_List *LRU) {
  LRU->size = 0;
  LRU->head = NULL;
  LRU->tail = NULL;
}

int L_add_page(LRU_List *LRU, BFpage *bfpage) {
  
  // when it's the first page
  if (LRU->head == NULL && LRU->tail == NULL) {
    LRU->head = bfpage;
    LRU->tail = bfpage;
    bfpage->nextpage = NULL;
    bfpage->prevpage = NULL;
  }
  // add to the head (new page is the most recent)
  else {
    LRU->head->prevpage = bfpage; 
    bfpage->next_page = LRU->head;
    bfpage->prevpage = NULL

    LRU->head = bfpage;
  }

  LRU->size++;
  return BFE_OK;
}

BFpage* L_find_victim(LRU_List *LRU) {

  BFpage* bfpage_ptr; 

  // find the victim page, searching from the tail
  bfpage_ptr = LRU->tail;
  for (int i = 0; i < LRU->size; i++) {
    if (bfpage_ptr->count == 0) break;
    else bfpage_ptr = bfpage_ptr->prevpage;
  }

  // remove the victim from the LRU
  if (bfpage_ptr->count != 0) // no victim
    handle_error("There is no victim in the LRU pool");

  L_detach_page(LRU, bfpage_ptr);
  LRU->size--;

  return bfpage_ptr;
}

int L_make_head(LRU_List *LRU, BFpage *target_page) {

  // check if "bfpage" is in hash, but not in LRU?
  if ((target_page->nextpage == NULL || target_page->prevpage == NULL)
      && (LRU->head != target_page && LRU->tail == target_page))
    handle_error("L_make_head error. in HT, but not in LRU");

  // fix the location of the bfpage to the head
  L_detach_page(LRU, target_page); 

  LRU->head->prevpage = target_page;
  target_page->next_page = LRU->head;
  target_page->prev_page = NULL;
  LRU->head = target_page;

  return BFE_OK;
}

BFpage* L_detach_page(LRU_List *LRU, BFpage *bfpage) {

  if (LRU->head != bfpage && LRU->tail != bfpage) {         // in between 
    bfpage->nextpage->prevpage = bfpage->prevpage;
    bfpage->prevpage->nextpage = bfpage->nextpage;
  }
  else if (LRU->head != bfpage && LRU->tail == bfpage) {    // tail
    bfpage->prevpage->nextpage = NULL;
    LRU->tail = bfpage->prevpage;
  }
  else if (LRU->head == bfpage && LRU->tail != bfpage) {    // first & not only
    bfpage->nextpage->prevpage = NULL; 
    LRU->head = bfpage->nextpage;
  }
  else if (LRU->head == bfpage && LRU->tail == bfpage) {    // first & only
    LRU->head = NULL;
    LRU->tail = NULL;
  }
  return bfpage;
}

void L_show(LRU_List *LRU) {
  BFpage *ptr;
  ptr = LRU->head;

  printf(  "pageNum fd unixfd count dirty");

  for (int i = 0; i < LRU->size; i++) {
    printf("  %d    %d   %d    %d    %d",ptr->pageNum,ptr->fd,ptr->unixfd,ptr->count,ptr->dirty);  
    printf("\n");
    ptr = ptr->nextpage;
  }
  printf("\n");
}

void LRU_delete(LRU_List *LRU) { // JM_edit

}


/* =========================
 *  Free_List !!
 * =========================
 *  */
void Free_List_Init(Free_List *FRL, int max_bfpage) {
 
  BFpage* cur_ptr;
  BFpage* next_ptr;

  /* set size, max_bfpage, *head */
  FRL->size = 1;
  FRL->max_bfpage = max_bfpage;

  if ( (FRL->head = malloc(sizeof(BFpage)) ) == NULL) 
    handle_error("Free_List head malloc failed");
  bfpage_clean_val(FRL->head);


  /* create lists of free pages (only next, no prev) */
  cur_ptr = FRL->head;
  for (int i = 0; i < FRL->max_bfpage; i++) {
    if ( (next_ptr = malloc(sizeof(BFpage)) ) == NULL) 
      handle_error("Free_List free page %d malloc failed",i);
    bfpage_clean_val(next_ptr);

    cur_ptr->nextpage = next_ptr;
    cur_ptr->prevpage = NULL;

    next_ptr = cur_ptr;
    FRL->num_bfpage++;
  }
  next_ptr->nextpage = NULL;
  next_ptr->prevpage = NULL;
}

static void bfpage_clean_val(BFpage *bfpage) {
  strcpy(bfpage->fpage.pagebuf,"");
  bfpage->dirty = 0;
  bfpage->count = 0;
  bfpage->pageNum = 0;
  bfpage->fd = 0;
  bfpage->unixfd = 0;
}

int F_add_free(Free_List *FRL, BFpage *bfpage) {
  // if free list is full, report error
  if (FRL->size >= FRL->maxsize) 
    handle_error("Free list is full!");

  // clean new free page & link to the head
  bfpage_clean_val(bfpage);
  bfpage->nextpage = FRL->head;
  bfpage->prevpage = NULL;

  // relocate head
  FRL->head = bfpage;

  return BFE_OK;
}

BFpage* F_remove_free(Free_List *FRL) {
  BFpage *return_page;

  // if free list is empty, return NULL
  if (FRL->size == 0) return NULL;
  else {
    return_page = FRL->head;

    FRL->head = FRL->head->nextpage;
    FRL->size--;

    return return_page;
  }
}

void Free_List_delete(Free_List *FRL) { // JM_edit
}


/* ========================= 
 *  Hash_Table !!
 * =========================
 *  */


void Hash_Table_Init(Hash_Table *HT, int hash_size) {
  HT->hash_size = hash_size;
  HT->hash_entries = malloc(sizeof(BFhash_entry*)*HT->hash_size);
  for (int i = 0; i < HT->hash_size; i++) {
    HT->hash_entires[i] = NULL;
  }
}

int H_get_index(Hash_Table *HT, int fd, int pageNum) {
  return (123 * (fd + 13) * (pagenum + 17) + 87) % 31 % ht->size; // JM_edit
}

int H_add_page(Hash_Table *HT, BFpage *add_page) {
  int hash_index;
  BFhash_entry *new_entry;
  BFhash_entry *hash_entry_ptr;

  hash_index = H_get_index(HT, add_page->fd, add_page->pageNum);

  if (H_get_entry(HT, add_page->fd, add_page->pageNum) != NULL) {
    return BFerrno = BFE_HASHPAGEEXIST;
  }
  else {

    new_entry = malloc(sizeof(BFhash_entry)); 
    new_entry->fd = add_page->fd;
    new_entry->pageNum = add_page->pageNum;
    new_entry->bpage = add_page;

    if (HT->hash_entries[hash_index] == NULL) {   // was empty
      new_entry->nextentry = NULL;
      new_entry->preventry = NULL;
      HT->hash_entries[hash_index] = new_entry;
    }
    else {
      hash_entry_ptr = HT->hash_entries[hash_index];
      while(1) {
        if (hash_entry_ptr->nextentry == NULL) {
          hash_entry_ptr->nextentry = new_entry;
          new_entry->preventry = hash_entry_ptr->nextentry;
          return BFE_OK;
        }
        else {
          hash_entry_ptr = hash_entry_ptr->nextentry;
        }
      }
    }
  }
}

BFhash_entry* H_get_entry(Hash_Table *HT, int fd, int pageNum) {
  int hash_index;
  BFhash_entry *hash_entry_ptr;

  hash_index = H_get_index(HT, fd, pageNum);
  hash_entry_ptr = HT->hash_entries[hash_index]

  if (hash_entry_ptr == NULL) {
    return NULL;
  }
  else {
    while(1) {
      if (hash_entry_ptr->fd == fd && hash_entry_ptr->pageNum == pageNum) {
        return hash_entry_ptr;
      }
      else {
        if (hash_entry_ptr->nextentry == NULL) return NULL;
        else hash_entry_ptr = hash_entry_ptr->nextentry;
      }
    }
  }

  return  NULL;
}

int H_remove_page(Hash_Table *HT, int fd, int pageNum) {

  BFhash_entry *victim_ptr;
  int hash_index;
   
  // find the hash_entry corresponding to the requested fd + pageNum. 
  hash_index = H_get_index(HT, remove_page->fd, remove_page->pageNum);
  victim_ptr = H_get_entry(HT, fd, pageNum);

  // hash_entry is empty
  if (victim_ptr == NULL) return BFerrno = BFE_HASHNOFOUND;

  // find and remove the hash_entry
  else if (victim_ptr->nextentry != NULL && victim_ptr->preventry != NULL) { // in between
    victim_ptr->nextentry->preventry = victim_ptr->preventry;
    victim_ptr->preventry->nextentry = victim_ptr->nextentry;
  }
  else if (victim_ptr->nextentry == NULL && victim_ptr->preventry != NULL) { // last
    victim_ptr->preventry->nextentry = NULL;
  }
  else if (victim_ptr->nextentry != NULL && victim_ptr->preventry == NULL) { // first & not only
    HT->hash_entries[hash_index] = victim_ptr->nextentry;
    victim_ptr->nextentry->preventry = NULL;
  }
  else if (victim_ptr->nextentry == NULL && victim_ptr->preventry == NULL) { // first & only
    HT->hash_entries[hash_index] = NULL;
  }

  free(victim_ptr);
  return BFE_OK;
}

void Hash_Table_delete(Hash_Table *HT) { // JM_edit

}


