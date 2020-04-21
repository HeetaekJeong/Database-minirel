#include <"stdio.h">


#include "bf.h"
#include "bf_component.h"

static LRU_List *LRU;
static Free_List *FRL;
static Hash_Table *HT;

static writeback(BFpage* victim) { // JM_edit
  if (victim->dirty == FALSE)
    handle_error("Tried to writeback, when the victim is not dirty");
  else {
    if (pwrite(victim->unixfd, victim->fpage.pagebuf, PAGE_SIZE, ((victim->pagenum))*PAGE_SIZE) != PAGE_SIZE) {
      return BFE_INCOMPLETEWRITE;
    }
  }
}

void bf_handle_error(int error) {
  switch(error) {
    case BFE_FREELIST_FULL: printf(""); exit(EXIT_FAILURE);
    case 
    case 
    case 
    case 
    case
  }
}

void BF_Init(void) {
  LRU_List_Init(LRU);
  Free_List_Init(FRL, BF_MAX_BUFS);
  Hash_Table_Init(HT, BF_HASH_TBL_SIZE);
}

int BF_GetBuf(BFreq bq, PFpage **fpage) {
  int result_val;
  BFhash_entry* hash_entry_ptr;
  BFpage* bfpage_ptr;


  // check parameters // JM_edit

  // check HT to find if it's in Buffer (LRU). if resident, return.
  hash_entry_ptr = H_get_entry(HT, bq.fd, bq.pageNum);
  if (hash_entry_ptr != NULL) {
    hash_entry_ptr->count++;
    res = L_make_head(LRU, hash_entry_ptr->bpage);
    return res;
  }
   
  // if not resident in LRU, get new page from freelist.
  bfpage_ptr = F_remove_free(FRL);

  // if free list is empty, find a victim in LRU (remove from LRU & HT) & write to disk if dirty.
  if (bfpage_ptr == NULL) {
    bfpage_ptr = L_find_victim(LRU);
    if (bfpage_ptr->dirty == 1) {
      writeback(bfpage_ptr);
    }
    H_remove_page(HT, bfpage_ptr->fd, bfpage_ptr->pageNum);
  }

  // add new BFpage to LRU & HT
  bfpage_ptr->dirty = FALSE;
  bfpage_ptr->count = 1;
  bfpage_ptr->pageNum = bq.pageNum;
  bfpage_ptr->fd = bq.fd;
  bfpage_ptr->unixfd = bq.unixfd;

  if (L_add_page(LRU, bfpage_ptr) != BFE_OK || H_add_page(HT, bfpage_ptr) != BFE_OK)
    return BFE_PAGENOTINBUF;

  // read the file asked (using the unixfd)
  if (pread(bfpage_ptr.unixfd, pfpage_ptr->fpage.pagebuf, PAGE_SIZE, (bq.pagenum)*PAGE_SIZE) == -1)
    return BFE_INCOMPLETEREAD;

  *fpage = &(bfpage_ptr->fpage);
  return BFE_OK;
}

int BF_AllocBuf(BFreq bq, PFpage **fpage);
int BF_UnpinBuf(BFreq bq);
int BF_TouchBuf(BFreq bq);
int BF_FlushBuf(int fd);
void BF_ShowBuf(void);
void BF_PrintError(const char *s);

