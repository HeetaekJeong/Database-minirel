#ifndef __BF_COMPONENT_H
#define __BF_COMPONENT_H

#define BFE_FREELIST_FULL



// LRU_List, Free_List, Hash_Table
typedef struct LRU_List {     /* LRU's head is the most recent & tail the oldest */
  int           size;         /* total number of pages in LRU */
  BFpage        *head;        /* head|most recent page */
  BFpage        *tail;        /* tail|most old page */
}

typedef struct Free_List {    /* Singly? Liked List. added & removed at the head */
  int           size;         /* total number of pages in the list */
  int           max_bfpage;   /* maximum number of free pages */
  BFpage        *head;        /* points to head page of the Free list */
}

typedef struct Hash_Table {
  int           hash_size;
  BFhash_entry  **hash_entries;
}

// BFpage, BFhash_entry, _buffer_request_control (BFreq)
typedef struct BFpage {
  PFpage         fpage;       /* page data from the file                 */
  struct BFpage  *nextpage;   /* next in the linked list of buffer pages */
  struct BFpage  *prevpage;   /* prev in the linked list of buffer pages */
  bool_t         dirty;       /* TRUE if page is dirty                   */
  short          count;       /* pin count associated with the page      */
  int            pageNum;     /* page number of this page                */
  int            fd;          /* PF file descriptor of this page         */
  int            unixfd;      /* Unix file descriptor of this page       */
} BFpage;

typedef struct BFhash_entry {
  struct BFhash_entry *nextentry;     /* next hash table element or NULL */
  struct BFhash_entry *preventry;     /* prev hash table element or NULL */
  int fd;                             /* file descriptor                 */
  int pageNum;                        /* page number                     */
  struct BFpage *bpage;               /* ptr to buffer holding this page */
} BFhash_entry;


typedef struct _buffer_request_control {
    int         fd;                     /* PF file descriptor */
    int         unixfd;                 /* Unix file descriptor */
    int         pagenum;                /* Page number in the file */
    bool_t      dirty;                  /* TRUE if page is dirty */
} BFreq;

// LRU_List Function
void LRU_List_Init(LRU_List *LRU);                    /* initialize LRU as empty */
int L_add_page(LRU_List *LRU, BFpage *bfpage);        /* add new page into LRU */
BFpage* L_find_victim(LRU_List *LRU); // JM_edit - argument를 뭘로 할지?
int L_make_head(LRU_List *LRU, BFpage *bfpage);       // JM_edit - 만들어야 함.
void LRU_delete(LRU_List *LRU);                       // JM_edit

// Free_List funtions 
void Free_List_Init(Free_List *FRL, int max_bfpage);
//void bfpage_clean_val(BFpage *bfpage);
int F_add_free(Free_List *FRL, BFpage *bfpage);
BFpage* F_remove_free(Free_List *FRL);
void Free_List_delete(Free_List *FRL);                // JM_edit

// Hash_Table functions
void Hash_Table_Init(Hash_Table *HT, int hash_size);
int H_get_index(Hash_Table *HT, int fd, int pageNum); // JM_edit - hash function을 어떤 것을 쓸지?
int H_add_page(Hash_Table *HT, BFpage *add_page);
BFhash_entry* H_get_entry(Hash_Table *HT, int fd, int pageNum);
int H_remove_page(Hash_Table *HT, int fd, int pageNum);
void Hash_Table_delete(Hash_Table *HT);               // JM_edit

#endif
