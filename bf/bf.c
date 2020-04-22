#include <string.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "minirel.h"
#include "bf.h"
#include "bf_component.h"

#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while(0)

extern int BFerrno;

/* declare LRU, FRL, HT here */
static LRU_List *LRU;
static Free_List *FRL;
static Hash_Table *HT;

/* writeback function. only used in BF layer, bf.c */
static writeback(BFpage* victim) {
  if (victim->dirty == FALSE)
    handle_error("Tried to writeback, when the victim is not dirty");
  else {
    if (pwrite(victim->unixfd, victim->fpage.pagebuf, PAGE_SIZE, ((victim->pagenum+1))*PAGE_SIZE) != PAGE_SIZE) {
      { BFerrno = BFE_INCOMPLETEWRITE; return BFE_INCOMPLETEWRITE; }
    }
  }
}

/* ================================================================ */
 
void BF_Init(void) {
  LRU = malloc(sizeof(LRU_List));
  FRL = malloc(sizeof(Free_List));
  HT = malloc(sizeof(Hash_Table));

  LRU_List_Init(LRU);
  Free_List_Init(FRL, BF_MAX_BUFS);
  Hash_Table_Init(HT, BF_HASH_TBL_SIZE);
}

int BF_GetBuf(BFreq bq, PFpage **fpage) {
  /* printf("BF_GetBuf\n");  JM_edit */
  /*Show_Hash(HT); */

  int res;
  BFhash_entry* get_entry;
  BFpage* bfpage_ptr;

  /* check parameters JM_edit */

  /* check HT to find if it's in Buffer (LRU). if resident, return. */
  get_entry = H_get_entry(HT, bq.fd, bq.pagenum);
  if (get_entry != NULL) {
    get_entry->bpage->count++;
    res = L_make_head(LRU, get_entry->bpage);
    if (res != 0) { BFerrno = res; return res; }
  }
   
  /* if not resident in LRU, get new page from freelist. */
  bfpage_ptr = F_remove_free(FRL);

  /* if free list is empty, find a victim in LRU (remove from LRU & HT) & write to disk if dirty. */
  if (bfpage_ptr == NULL) {
    bfpage_ptr = L_find_victim(LRU);
    if (bfpage_ptr->dirty == 1) {
      writeback(bfpage_ptr);
    }
    H_remove_page(HT, bfpage_ptr->fd, bfpage_ptr->pagenum);
  }

  /* add new BFpage to LRU & HT */
  bfpage_ptr->dirty = FALSE;
  bfpage_ptr->count = 1;
  bfpage_ptr->pagenum = bq.pagenum;
  bfpage_ptr->fd = bq.fd;
  bfpage_ptr->unixfd = bq.unixfd;

  if (L_add_page(LRU, bfpage_ptr) != BFE_OK || H_add_page(HT, bfpage_ptr) != BFE_OK)
    { BFerrno = BFE_PAGENOTINBUF; return BFE_PAGENOTINBUF; }

  /* read the file asked (using the unixfd) */
  if (pread(bfpage_ptr->unixfd, bfpage_ptr->fpage.pagebuf, PAGE_SIZE, (bq.pagenum+1)*PAGE_SIZE) == -1)
    { BFerrno = BFE_INCOMPLETEREAD; return BFE_INCOMPLETEREAD; }

  *fpage = &(bfpage_ptr->fpage);
  return BFE_OK;
}

int BF_AllocBuf(BFreq bq, PFpage **fpage) {
  int res;
  BFhash_entry* new_entry;
  BFpage* bfpage_ptr;

  /* check parameters JM_edit */
  
  /* check HT to find if it's in Buffer (LRU). if resident, return error */
  new_entry = H_get_entry(HT, bq.fd, bq.pagenum);
  if (new_entry != NULL)
    { BFerrno = BFE_PAGEINBUF; return BFE_PAGEINBUF; }

  /* get new page from the Free_List.  */
  bfpage_ptr = F_remove_free(FRL); 
  
  /* if empty, find victim and remove (from LRU & HT) & write to disk if dirty. */
  if (bfpage_ptr == NULL) {
    bfpage_ptr = L_find_victim(LRU);
    if (bfpage_ptr->dirty == 1) {
      writeback(bfpage_ptr);
    }
    H_remove_page(HT, bfpage_ptr->fd, bfpage_ptr->pagenum);
  }

  /* add new BFpage to LRU & HT */
  bfpage_ptr->dirty = FALSE;
  bfpage_ptr->count = 1;
  bfpage_ptr->pagenum = bq.pagenum;
  bfpage_ptr->fd = bq.fd;
  bfpage_ptr->unixfd = bq.unixfd;
  bfpage_ptr->prevpage = NULL;
  bfpage_ptr->nextpage = NULL;

  if (L_add_page(LRU, bfpage_ptr) != BFE_OK) {
    BFerrno = BFE_PAGENOTINBUF; return BFE_PAGENOTINBUF;
  }
  if (H_add_page(HT, bfpage_ptr) != BFE_OK) {
    BFerrno = BFE_PAGENOTINBUF; return BFE_PAGENOTINBUF;
  }

  *fpage = &(bfpage_ptr->fpage);
  return BFE_OK;
}

int BF_UnpinBuf(BFreq bq) {
  /* printf("BF_Unpin\n");
  Show_Hash(HT);   JM_edit */

  BFhash_entry* unpin_entry;

  /* check parameters. */ /* JM_edit  */

  /* find the page in HT/LRU. */
  unpin_entry = H_get_entry(HT, bq.fd, bq.pagenum); 

  /* if not resident || count == 0, error */
  if (unpin_entry == NULL)
    { BFerrno = BFE_PAGENOTINBUF; return BFE_PAGENOTINBUF; }
  else if (unpin_entry->bpage->count < 0)
    { BFerrno = BFE_PAGEUNPINNED; return BFE_PAGEUNPINNED; }

  /* decrease the pin. */
  unpin_entry->bpage->count--;
  return BFE_OK;
}

int BF_TouchBuf(BFreq bq) {
  int res; 
  BFhash_entry* touch_entry;
  /* check parameters. */ /* JM_edit */
      
  /* find the page in HT/LRU. */
  touch_entry = H_get_entry(HT, bq.fd, bq.pagenum);

  /* if not resident || count == 0, error */
  if (touch_entry == NULL) 
    { BFerrno = BFE_PAGENOTINBUF; return BFE_PAGENOTINBUF; }
  else if (touch_entry->bpage->count < 0) 
    { BFerrno = BFE_PAGEUNPINNED;return BFE_PAGEUNPINNED; }

  /* make it dirty & make it head & count++ */
  touch_entry->bpage->dirty = TRUE;
  if (res = L_make_head(LRU, touch_entry->bpage) != BFE_OK) 
    { BFerrno = res; return res; }

  return BFE_OK;
}

int BF_FlushBuf(int fd) {

  BFpage* bfpage_ptr;
  BFpage* bfpage_iter;

  int res;
  int i;
  /* check parameters. */ /* JM_edit */

  /* if LRU is empty, return */
  bfpage_ptr = LRU->tail;
  bfpage_iter = LRU->tail;
  if (bfpage_ptr == NULL) return BFE_OK;

  /* search from tail to head, checking fd */
  for (i = 0; i < LRU->size; i++) {
    /*printf("FLUSH int i: %d\n",i); */
    /* BF_ShowBuf();  JM_edit */

    bfpage_iter = bfpage_iter->prevpage;

    if (bfpage_ptr->fd == fd) {
      L_detach_page(LRU, bfpage_ptr);                                     /* remove from LRU*/ 
      LRU->size--;
      if (bfpage_ptr->dirty == TRUE) writeback(bfpage_ptr);               /* writeback if dirty */
      if ((res = H_remove_page(HT, bfpage_ptr->fd, bfpage_ptr->pagenum))  /* remove from HT */
          != BFE_OK) { BFerrno = res; return res; }
      if ((res = F_add_free(FRL, bfpage_ptr))                             /* add to FRL */
          != BFE_OK) { BFerrno = res; return res; }
    }
    bfpage_ptr = bfpage_iter;
  }

  return BFE_OK;
}

void BF_ShowBuf(void) {
  L_show(LRU); 
  /*Show_Hash(HT);*/
}

void BF_PrintError(const char *s) {
  /* perror("%s, %d",s,BFerrno); */
  exit(-1);
}


