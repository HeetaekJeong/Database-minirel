#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include "minirel.h"
#include "pf.h"
#include "am.h"
#include "hf.h"
/* JM_edit
#include "../pf/pfUtils.h"
#include "../hf/hfUtils.h"
#include "../am/amUtils.h"
*/


typedef struct AM_Header{ /* AM B+tree index table's header (different from node's header */
	int   indexNo;           
	char  attrType;         
	int   attrLength;      
	int   height_tree;    
	int   nb_leaf;        /* number of leaf */
  int   racine_page;    /* */
  int   num_pages;      /* number of page/node */
} AM_Header;

/* ================================== *
 * node's header: bool_t / if_leaf    *
 *                int    / num_keys   *
 *                int    / previous   *
 *                int    / next       *
 *                RECID  / recId      *
 * ================================== */

typedef struct AM_itab_entry {
  bool_t    valid;     
  int       fd;     
  char      iname[255+4];      
  int       fanout;             
  int       fanout_leaf;    
  AM_Header header;  
  bool_t    dirty;   
} AM_itab_entry;

typedef struct AM_scantab_entry {
  bool_t  valid;           
  int     AMfd;           
  int     current_page;  
  int     current_key;  
  int     current_num_keys;
  int     op;           
  char    value[255]; 
} AM_scantab_entry;

/* */
typedef struct icouple {
  int pagenum;
  int key;
} icouple;

typedef struct fcouple {
  int pagenum;
  float key;
} fcouple;

typedef struct ccouple {
  int pagenum;
  char* key; 
} ccouple;

typedef struct icoupleLeaf {
  RECID recid;
     int key;
} icoupleLeaf;

typedef struct fcoupleLeaf {
  RECID recid;
   float  key;
} fcoupleLeaf;

typedef struct ccoupleLeaf {
  RECID recid;
   char* key;
} ccoupleLeaf;

typedef struct inode {
  bool_t is_leaf;   
  int num_keys;   
  int last_pt;   
  icouple* couple;
} inode;

typedef struct fnode {
  bool_t is_leaf;   
  int num_keys;   
  int last_pt;   
  fcouple* couple;   
} fnode;

typedef struct cnode {
  bool_t is_leaf;            
  int num_keys;   
  int last_pt;   
  ccouple* couple;
} cnode;

typedef struct ileaf {
  bool_t is_leaf;   
  int num_keys;   
  int previous;  
  int next; 
  icoupleLeaf* couple;    
} ileaf;

typedef struct fleaf {
  bool_t is_leaf;   
  int num_keys;   
  int previous;  
  int next;
  fcoupleLeaf* couple;     
} fleaf;

typedef struct cleaf {
  bool_t is_leaf;         
  int num_keys; 
  int previous;
  int next; 
  ccoupleLeaf* couple;     
} cleaf;




/* ================================================================= */

/* some global object / variables */
static AM_itab_entry *AM_itab;
static AM_iscan_entry *AM_iscantab;
static size_t AM_itab_length;
static size_t AM_iscantab_length;

/* some sniffet functions declaration */
static void set_current_iscantab_value(int &a, int &b, int &c, const AM_iscantab_entry &D);
static void set_recid(RECID &recid, int AM_ERROR_CODE);

static bool_t int_comparison(int a, int b, int op);
static bool_t float_comparison(float a, float b, int op);
static bool_t char_comparison(char* a, char* b, int op, int len);

static void insert_keyval() /* JM_edit */
static void set_node_header() /* JM_edit */


/* some B+tree related functions declaration */
Btree_FindLeaf(   )






/* ================================================================= */
void AM_Init(void) {
  AM_itab = malloc(sizeof(AM_itab_entry) * AM_ITAB_SIZE);
  AM_iscantab = malloc(sizeof(AM_iscantab_iter) * MAXSCANS);
  AM_itab_length = 0;
  HF_Init();
}


int AM_CreateIndex(const char *fileName, int indexNo, char attrType, int attrLength, bool_t isUnique) {
  int error, PF_fd, pagenum;
  AM_itab_entry* AM_itab_iter; 
	/* AM_itab_entry* AM_itab_iter; */
  char* header_buf;
	/* char* header_buf; */

  /* Check if AM_itab is full */
  if (AM_itab_length >= AM_ITAB_SIZE) return AME_FULLTABLE;
 
  /* set the iterator to the wanted entry at the AM_itab, and set AM_itab_length accordingly */
  AM_itab_iter = AM_itab + AM_itab_length;
  AM_itab_length++;

  /* create the treefile_name & Create a new file with PF_CreateFile & Open the file */
  sprintf(AM_itab_iter->treefile_name, "%s.%d", fileName, indexNo);
  if ( (error = PF_CreateFile(AM_itab_iter->treefile_name)) != PFE_OK) PF_ErrorHandler(error);
  if ( (PF_fd = PF_OpenFile(AM_itab_iter->treefile_name)) < 0) return AME_PF;

	/* set the metadata of the AM_itab_entry correctly  */
  AM_itab_iter->PF_fd = PF_fd;
  AM_itab_iter->valid = TRUE;
  AM_itab_iter->dirty = TRUE;
  AM_itab_iter->header.racine_page = -1;
	/* set the correct meta data for the header of the tree file */
  AM_itab_iter->header.indexNo = indexNo;
  AM_itab_iter->header.attrType = attrType;
  AM_itab_iter->header.attrLength = attrLength;
  AM_itab_iter->header.height_tree = 0;
  AM_itab_iter->header.nb_leaf = 0;
  AM_itab_iter->header.num_pages=2;
	/* the fanout/fanout_leaf is dependent on the attr_type */
	/* for the internal node, there are 3 int data (is_leaf, pagenum of parent, number of key) */
  AM_itab_iter->fanout = ( (PF_PAGE_SIZE ) - 2*sizeof(int) - sizeof(bool_t)) / (sizeof(int) + AM_itab_iter->header.attrLength) + 1;
  AM_itab_iter->fanout_leaf = ( (PF_PAGE_SIZE - 3*sizeof(int) -sizeof(bool_t) ) / (header.attrLength + sizeof(RECID)) ) + 1;

	/* allocate the header, using the PF_AllocPage */
  if ( (error = PF_AllocPage(AM_itab_iter->PF_fd, &pagenum, &header_buf)) != PFE_OK) PF_ErrorHandler(error);
  if (pagenum != 1) return AME_PF;

	/* transfer the AM_itab_entry's header meta data to the actual header page / header_buf */
  memcpy((char*) (header_buf), (int*) &AM_itab_iter->header.indexNo, sizeof(int));
  memcpy((char*) (header_buf + sizeof(int)), (int*) &AM_itab_iter->header.attrType, sizeof(char));
  memcpy((char*) (header_buf + sizeof(int) + sizeof(char)), (int*) &AM_itab_iter->header.attrLength, sizeof(int));
  memcpy((char*) (header_buf + 2*sizeof(int) + sizeof(char)), (int*) &AM_itab_iter->header.height_tree, sizeof(int));
  memcpy((char*) (header_buf + 3*sizeof(int) + sizeof(char)), (int*) &AM_itab_iter->header.nb_leaf, sizeof(int));
  memcpy((char*) (header_buf + 4*sizeof(int) + sizeof(char)), (int*) &AM_itab_iter->header.racine_page, sizeof(int));
  memcpy((char*) (header_buf + 5*sizeof(int) + sizeof(char)), (int*) &AM_itab_iter->header.num_pages, sizeof(int));


	/* set the table invalid & remove it. it will be valid once it gets opened */
  AM_itab_iter->valid = FALSE;
  if ( (error = PF_UnpinPage(AM_itab_iter->PF_fd, pagenum, 1)) != PFE_OK) PF_ErrorHandler(error);
	if ( (error = PF_CloseFile(AM_itab_iter->PF_fd)) != PFE_OK) PF_ErrorHandler(error);
  AM_itab_length--;

	/* done properly */
  printf("Index : %s created\n", AM_itab_iter->treefile_name);
	/* free(AM_itab_iter);*/ /* JM_edit */
  /* free(header_buf);*/ /* JM_edit */
  return AME_OK;
}


int AM_DestroyIndex(const char *fileName, int indexNo) {
  int error;
  char* destroy_treefile_name;

	/* set the treefile_name, accordingly to the input argument */
  destroy_treefile_name = malloc(sizeof(fileName) + sizeof(int));
  sprintf(destroy_treefile_name, "%s.%i", fileName, indexNo);

	/* destory the file using the PF_Destroy_File */
  if ( (error = PF_DestroyFile(destroy_treefile_name)) != PFE_OK) PF_ErrorHandler(error);

	/* properly destroyed the tree file */
  printf("Index : %s has been destroyed\n", destroy_treefile_name);
  free(destroy_treefile_name);
  return AME_OK;
}


int AM_OpenIndex(const char *fileName, int indexNo) {
  int error, PF_fd, AM_fd;
  AM_itab_entry *AM_itab_iter;
  /* AM_itab_entry *AM_itab_iter; */
  char *header_buf;
  /* char *header_buf; */
  char* open_treefile_name;

  /* check if AM_itab is full */
  if( AM_itab_length >= AM_ITAB_SIZE) return AME_FULLTABLE;

  /* set the open_treefile_name accroding to the input argument */
  open_treefile_name = malloc(sizeof(fileName) + sizeof(int));
  sprintf(open_treefile_name, "%s.%i", fileName, indexNo);
	/* open the file using the PF_OpenFile */
  if ( (PF_fd = PF_OpenFile(open_treefile_name)) < 0) PF_ErrorHandler(PF_fd);
	/* read the header page using the PF_GetThisPage */
  if ( (error = PF_GetThisPage(PF_fd, 1, &header_buf)) != PFE_OK) PF_ErrorHandler(error);

  /* set the metadata for the AM_itab_entry, that the opened treefile will be placed */
  AM_itab_iter = AM_itab + AM_itab_length;

  AM_itab_iter->PF_fd = PF_fd;
  AM_itab_iter->valid = TRUE;
  AM_itab_iter->dirty = FALSE;
	/* set the AM_itab_entry's meta data. since the variables have variable lengths, use the memcpy */
  memcpy(AM_itab_iter->treefile_name, open_treefile_name, sizeof(open_treefile_name));
  memcpy((int*) &AM_itab_iter->header.indexNo,(char*) (header_buf), sizeof(int));
  memcpy((int*) &AM_itab_iter->header.attrType,(char*) (header_buf + sizeof(int)),  sizeof(char));
  memcpy((int*) &AM_itab_iter->header.attrLength, (char*) (header_buf + sizeof(int) + sizeof(char)),sizeof(int));
  memcpy((int*) &AM_itab_iter->header.height_tree,(char*) (header_buf + 2*sizeof(int) + sizeof(char)),  sizeof(int));
  memcpy((int*) &AM_itab_iter->header.nb_leaf,(char*) (header_buf + 3*sizeof(int) + sizeof(char)), sizeof(int));
  memcpy((int*) &AM_itab_iter->header.racine_page, (char*) (header_buf + 4*sizeof(int) + sizeof(char)), sizeof(int));

  memcpy((int*) &AM_itab_iter->header.num_pages, (char*) (header_buf + 5*sizeof(int) + sizeof(char)), sizeof(int));
	/* set the fanout / fanout_leaf according to the type of the attrType  */
  AM_itab_iter->fanout = ( (PF_PAGE_SIZE ) - 2*sizeof(int) - sizeof(bool_t)) / (sizeof(int) + AM_itab_iter->header.attrLength) + 1;
  AM_itab_iter->fanout_leaf = ( (PF_PAGE_SIZE - 3*sizeof(int) -sizeof(bool_t) ) / (AM_itab_iter->header.attrLength + sizeof(RECID)) ) + 1; /* number of recid into a leaf page */

  /* increment the size of the table & set the return position*/
  AM_itab_length ++;
  AM_fd = AM_itab_length -1;
  /* unpin the page, using the PF_UnpinPage */
  if ( (error = PF_UnpinPage(AM_itab_iter->PF_fd, 1, 1)) != PFE_OK) PF_ErrorHandler(error);

	/* properly opened the treefile */
  free(open_treefile_name);
  /* free(header_buf); JM_edit */
  /* free(AM_itab_iter); JM_edit*/
  return AM_fd;
}


int AM_CloseIndex(int AM_fd) {
  AM_itab_entry* AM_itab_iter;
  int error;
  char* header_buf;

  /* check if the table is full & AM_fd, then set the iterator to the correct position */
  if (AM_fd < 0 || AM_fd >= AM_itab_length) return AME_FD;
  AM_itab_iter = AM_itab + AM_fd ;
  if (AM_itab_iter->valid!=TRUE) return AME_INDEXNOTOPEN;

	/* check the header of the treefile & if dirty, write back using the PF functions */
  if (AM_itab_iter->dirty == TRUE) { 
    if ( (error = PF_GetThisPage( AM_itab_iter->PF_fd, 1, &header_buf)) != PFE_OK)
			PF_ErrorHandler(error);

		/* modify the PF page's header properly, using the memcpy. memcpy is used, 
     * because the length is variable, according to the attrType */
    memcpy((char*) (header_buf), (int*) &AM_itab_iter->header.indexNo, sizeof(int));
    memcpy((char*) (header_buf + sizeof(int)), (int*) &AM_itab_iter->header.attrType, sizeof(char));
    memcpy((char*) (header_buf + sizeof(int) + sizeof(char)), (int*) &AM_itab_iter->header.attrLength, sizeof(int));
    memcpy((char*) (header_buf + 2*sizeof(int) + sizeof(char)), (int*) &AM_itab_iter->header.height_tree, sizeof(int));
    memcpy((char*) (header_buf + 3*sizeof(int) + sizeof(char)), (int*) &AM_itab_iter->header.nb_leaf, sizeof(int));
    memcpy((char*) (header_buf + 4*sizeof(int) + sizeof(char)), (int*) &AM_itab_iter->header.racine_page, sizeof(int));
    memcpy((char*) (header_buf + 5*sizeof(int) + sizeof(char)), (int*) &AM_itab_iter->header.num_pages, sizeof(int));

    /* Unpin the page */ 
    if ( (error = PF_UnpinPage(AM_itab_iter->fd, 1, 1) != PFE_OK)) PF_ErrorHandler(error);
  }

  /* if it's dirty or not, close the treefile, using the PF_CLoseFile  */
  if ( (error=PF_CloseFile(AM_itab_iter->fd)) != PFE_OK) PF_ErrorHandler(error);

  /* check if there was no scan in progress */
  for (int i=0; i < AM_iscantab_length; i++) {
      if ( (AM_iscantab+i)->AM_fd == AM_fd) return AME_SCANOPEN;
  }

  /* only the invalid treefiles that are at the end of the invalid pages in the AM_itab
   * can be deleted. Else, they stay in the table, invalidified*/
  AM_itab_iter->valid = FALSE; /* JM_edit */
  if (AM_fd == (AM_itab_length-1)) { /* if the currently closed treefile is the last in the table*/
    AM_itab_length--;
    if (AM_itab_length > 0){
      AM_itab_length--;
      while ( (AM_itab_length > 0) && ((AM_itab + AM_itab_length - 1)->valid == FALSE) ) {
        AM_itab_length--;
      }
    }
  }

  /* free(AM_itab_iter); JM_edit */
  /* free(header_buf); JM_edit */
  return AME_OK;
}


int AM_DeleteEntry(int AM_fd, char *value, RECID recId) {
  int error;
  int return_val
  int target_leaf, num_keys, del_index, target_leaf_num, 
  int keyval_size;
  int offset;
  AM_itab_entry* AM_itab_iter;
  char* page_buf;
  int* visited_path_node;
  RECID target_recid;
  bool_t found;
  found = FALSE;

  /* check the AM_fd & check if the AM_itab is full */
  if (AM_fd < 0 || AM_fd >= AM_itab_length) return AME_FD;
  AM_itab_iter = AM itab + AM_fd;
  if (AM_itab_iter->valid != TRUE) return AME_INDEXNOTOPEN;

  /* set the visited_path_node, according to the height of the treefile 
   * set the target_leaf, using the AM_FindLeaf function*/
  visited_path_node = malloc( AM_itab_iter->header.height_tree * sizeof(int));
  target_leaf = AM_FindLeaf(AM_fd, value, visited_path_node);
  if (target_leaf < 0) return AME_KEYNOTFOUND;

  /* set the target_leaf_num, to use the PF_GetThisPage && read the page to page_buf*/
  target_leaf_num = visited_path_node[(AM_itab_iter->header.height_tree)-1];
  if ( (error = PF_GetThisPage(AM_itab_iter->fd, target_leaf_num, &page_buf)) != PFE_OK) PF_ErrorHandler(error);

  /* find the target keyval entry */
  keyval_size = sizeof(RECID) + AM_itab_iter->header.attrLength;
  /* read the num_keys from the target_leaf */
  memcpy((int*) &num_keys, (char*) (page_buf + sizeof(bool_t)), sizeof(int));
  /* set the initial offset of the keyval position */
  offset = sizeof(bool_t) + 3 * sizeof(int)+target_leaf*keyval_size;

  /* read the whole leaf & find the target keval entry */
  for(del_index = target_leaf; del_index < num_keys; del_index++){
    /* read the keyval record */
    memcpy((int*) &target_recid.pagenum, (char *) (page_buf + offset), sizeof(int));
    memcpy((int*) &target_recid.recnum, (char *) (page_buf + offset+sizeof(int)), sizeof(int));

    /* check if the read record is the target */
    if ( (target_recid.pagenum == recId.pagenum) && (target_recid.recnum == recId.recnum) ) {
      found = TRUE;
      break;
    }
    offset += keyval_size; /* set the offset to the next keyval record */
  }

  /* 1. record found */
  if (found) {
    return_val = AME_OK;

    /* remove the target record & append the following records */
    memmove((char*)(page_buf + offset), (char*)(page_buf + offset + keyval_size), keyval_size*(num_keys - del_index));
    num_keys --;
    /* find the following scan with the same AM_fd & modify the current_key value */
    for (int i = 0; i < AM_iscantab_length; i++) {
      if( (AM_iscantab+i)->AM_fd == AM_fd) {
        (AM_iscantab+i)->current_key--;
      }
    }
    /* modify the target_leaf's number of keys */
    memcpy((char*) (page_buf + sizeof(bool_t)), (int*)&num_keys, sizeof(int));
  }
  /* 2. record not found */
  else {
    return AME_RECNOTFOUND;
  }

  /* Unpin the target_leaf page */
  if ( (error = PF_UnpinPage(AM_itab_iter->fd, target_leaf_num, 1)) != PFE_OK) PF_ErrorHandler(error);

  /* properly deleted record entry */
  free(visited_path_node);
  return return_val;
}


int AM_OpenIndexScan(int AM_fd, int op, char *value) {
  AM_itab_entry* AM_itab_iter;
  AM_iscantab_iter* AM_iscantab_iter;
  int key, error, min_val;
  int* visited_path_node;
  min_val = -2147483648; 

	/* check the paramter & check if the AM_itab / AM_iscantab is full */
  if (AM_fd < 0 || AM_fd >= AM_itab_length) return AME_FD;
  if (op < 1 || op > 6) return AME_INVALIDOP;
  if (AM_iscantab_length >= AM_ITAB_SIZE) return AME_SCANTABLEFULL;

	/* set the iterators for the AM_itab & AM_iscantab */
  AM_itab_iter = AM_itab + AM_fd;
  if (AM_itab_iter->valid != TRUE) return AME_INDEXNOTOPEN;
  AM_iscantab_iter = AM_iscantab + AM_iscantab_length;

  /* set the new AM_iscantab_iter's meta data */
  memcpy((char*) AM_iscantab_iter->value, (char*) value, AM_itab_iter->header.attrLength);
  AM_iscantab_iter->op = op;
  visited_path_node = malloc(AM_itab_iter->header.height_tree * sizeof(int));
 
  /* if NE_OP, use the min_val to get the left most leaf */
  if (op == NE_OP) key = AM_FindLeaf(AM_fd, (char*) &min_val, visited_path_node); 
  else key = AM_FindLeaf(AM_fd, value, visited_path_node);

  /* set the new target AM_iscantab_iter properly */
  AM_iscantab_iter->current_page = visited_path_node[(AM_itab_iter->header.height_tree)-1];
  AM_iscantab_iter->current_key = key;
  AM_iscantab_iter->current_num_keys = 0; /* this is set in findNextEntry */
  AM_iscantab_iter->AM_fd = AM_fd;
  AM_iscantab_iter->valid = TRUE;

  free(visited_path_node);
  return AM_iscantab_length++;
}


static bool_t int_comparison(int a, int b, int op) {
  switch (op) {
    case EQ_OP: return a == b ? TRUE : FALSE;
    case LT_OP: return a < b ? TRUE : FALSE;
    case GT_OP: return a > b ? TRUE : FALSE;
    case LE_OP: return a <= b ? TRUE : FALSE;
    case GE_OP: return a >= b ? TRUE : FALSE;
    case NE_OP: return a != b ? TRUE : FALSE;
    default: return FALSE;
  }
}

static bool_t float_comparison(float a, float b, int op) {
  switch (op) {
    case EQ_OP: return a == b ? TRUE : FALSE;
    case LT_OP: return a < b ? TRUE : FALSE;
    case GT_OP: return a > b ? TRUE : FALSE;
    case LE_OP: return a <= b ? TRUE : FALSE;
    case GE_OP: return a >= b ? TRUE : FALSE;
    case NE_OP: return a != b ? TRUE : FALSE;
    default: return FALSE;
  }
}

static bool_t char_comparison(char* a, char* b, int op, int len) {
  switch (op) {
    case EQ_OP: return strncmp(a, b, len) == 0 ? TRUE : FALSE;
    case LT_OP: return strncmp(a, b, len) < 0 ? TRUE : FALSE;
    case GT_OP: return strncmp(a, b, len) > 0 ? TRUE : FALSE;
    case LE_OP: return strncmp(a, b, len) <= 0 ? TRUE : FALSE;
    case GE_OP: return strncmp(a, b, len) >= 0 ? TRUE : FALSE;
    case NE_OP: return strncmp(a, b, len) != 0 ? TRUE : FALSE;
    default: return FALSE;
  }
}

static void set_current_iscantab_value(int &current_key, int &current_page, int &current_num_keys,
    const AM_iscantab_entry &AM_iscantab_iter) {
  current_key = AM_iscantab_iter->current_key;
  current_value = AM_iscantab_iter->current_value;
  current_num_keys = AM_iscantab_iter->current_num_keys;
}

static void set_recid(RECID &recid, int AM_ERROR_CODE) {
  recid.pagenum = AM_ERROR_CODE;
  recid.recnum = AM_ERROR_CODE;
  AMerrno = AM_ERROR_CODE;
}

RECID AM_FindNextEntry(int scanDesc) {
  RECID recid;
  AM_itab_ele* AM_itab_iter;
  AM_iscantab_entry* AM_iscantab_iter;
  char* page_buf;
  int error;
  int current_key, current_page, current_num_keys;
  int direction, unpinning_page;
  float float_a, float_2;
  int int_a, int_b;
  bool_t found;

  /* check the paramter & check if the AM_iscantab is full */
  if ( (scanDesc < 0) ||  (scanDesc >= AM_iscantab_length && AM_iscantab_length !=0) ) {
    set_recid(recid, AME_INVALIDSCANDESC); return recid; }
  /* set the iterators for AM_iscantab & AM_itab */
  AM_iscantab_iter = AM_iscantab + scanDesc;
  AM_itab_iter = AM_itab + AM_iscantab_iter->AM_fd;
  /* if AM_itab_entry is not valid, return */
  if (AM_itab_iter->valid == TRUE) { set_recid(recid, AME_INDEXNOTOPEN); return recid; }
  /* if AM_iscantab_entry is not valid, return */
  if (AM_iscantab_iter->valid == FALSE) { set_recid(recid, AME_SCANNOTOPEN); return recid; }

  /* read current_page / node */
  if ( (error = PF_GetThisPage(AM_itab_iter->fd, AM_iscantab_iter->current_page, &page_buf)) != PFE_OK) PF_ErrorHandler(error);
  /* set the current_num_keys, by reading the page */
  memcpy((int*) &(AM_iscantab_iter->current_num_keys), (int*) (page_buf + OffsetLeafNumKeys), sizeof(int));
  /* iterate left if less-operation & vice versa */
  direction = (AM_iscantab_iter->op == LT_OP || AM_iscantab_iter->op == LE_OP) ? -1 : 1;


  /* set the necessary */
  found = FALSE;
  set_current_iscantab_value(current_key, current_page, current_num_keys, AM_iscantab_iter);
  /* while next page exists (2 cases for the both direction case) */
  while ( ((current_page >= 2) && (current_key >= 0))
      && ((current_page <= AM_itab_iter->header.num_pages) && (current_key < current_num_keys)) ) {

    /* current_page / node scan */
    while (found == FALSE && current_key >= 0 && current_key < current_num_keys) {
      switch (AM_itab_iter->header.attrType) {

      /* accordingly to the attrType, search for the target value */
      case 'i':
        int_a=*((int*) AM_iscantab_iter->value);
        memcpy( (int*) &int_b,
            (char*) (page_buf+OffsetLeafCouple+current_key*(2*sizeof(int)+AM_itab_iter->header.attrLength)+2*sizeof(int)),
            AM_itab_iter->header.attrLength );
        if (int_comparison( int_b, int_a, AM_iscantab_iter->op) == TRUE) found = TRUE;
        AM_iscantab_iter->current_key += direction;
        set_current_iscantab_value(current_key, current_page, current_num_keys, AM_iscantab_iter);
        break;

      case 'f':
        float_a=*((float*) AM_iscantab_iter->value);
        memcpy((float*) &float_2,
            (char*) (page_buf+OffsetLeafCouple+current_key*(2*sizeof(int)+AM_itab_iter->header.attrLength)+2*sizeof(int)),
            AM_itab_iter->header.attrLength);
        if (float_comparison( float_2, float_a, AM_iscantab_iter->op) == TRUE) found = TRUE;
        AM_iscantab_iter->current_key += direction;
        set_current_iscantab_value(current_key, current_page, current_num_keys, AM_iscantab_iter);
        break;

      case 'c':
        /* JM_edit */
        if (char_comparison( (char*) (page_buf+OffsetLeafCouple+current_key*(2*sizeof(int)+AM_itab_iter->header.attrLength)+2*sizeof(int)), AM_iscantab_iter->value, AM_iscantab_iter->op, AM_itab_iter->header.attrLength) == TRUE) found = TRUE;

        AM_iscantab_iter->current_key += direction;
        set_current_iscantab_value(current_key, current_page, current_num_keys, AM_iscantab_iter);
        break;
      }
    }

    /* 1. Not Found in the Node */
    if (found == FALSE) {

      /* since duplicate key situation is not regarded in this project, if the record with the search key
       * does not exist in the node/page, it does not exist */
      if (AM_iscantab_iter->op == EQ_OP ) {
        if ( (error = PF_UnpinPage(AM_itab_iter->fd, current_page, 0)) != PFE_OK) PF_ErrorHandler(error);
        break;
      }

      unpinning_page = AM_iscantab_iter->current_page;
			/* set the scan's current_page / node, according to the direction of the scan */
      if (direction < 0) {
        memcpy((int*) &(AM_iscantab_iter->current_page), (int*) (page_buf + sizeof(bool_t) + sizeof(int)), sizeof(int));
      } else {
        memcpy((int*) &(AM_iscantab_iter->current_page), (int*)(page_buf + sizeof(bool_t) + sizeof(int)*2), sizeof(int));
      }
      set_current_iscantab_value(current_key, current_page, current_num_keys, AM_iscantab_iter);

      if ( (error = PF_UnpinPage(AM_itab_iter->fd, unpinning_page, 0)) != PFE_OK) PF_ErrorHandler(error);
      if ( (error = PF_GetThisPage(AM_itab_iter->fd, current_page, &page_buf)) != PFE_OK) PF_ErrorHandler(error);

      if (direction < 0) {
        memcpy((int*) &(AM_iscantab_iter->current_key), (int*) page_buf + sizeof(bool_t), sizeof(int));
        set_current_iscantab_value(current_key, current_page, current_num_keys, AM_iscantab_iter);
      } else {
        AM_iscantab_iter->current_key = 0;
        set_current_iscantab_value(current_key, current_page, current_num_keys, AM_iscantab_iter);
      }
    }

    /* 2. Found in the Node */
    else {
			/* Unpin the page & set the recid to return */
      if ( (error = PF_UnpinPage(AM_itab_iter->fd, current_page, 0)) != PFE_OK) PF_ErrorHandler(error);
      memcpy((int*) &recid.pagenum,(char*) (page_buf+OffsetLeafCouple+(current_key-direction)*(2*sizeof(int)+AM_itab_iter->header.attrLength)),  sizeof(int));
      memcpy((int*) &recid.recnum,(char*) (page_buf+OffsetLeafCouple+(current_key-direction)*(2*sizeof(int)+AM_itab_iter->header.attrLength)+sizeof(int)),  sizeof(int));
      return recid;
    }

  } /* End of the while loop */

	/* Unpin the page (here is when the search record is not found)  */
  if (current_page != AM_iscantab_iter->current_page) {
    printf("AM_FindNextEntry: current_page != AM_iscantab_iter->current_page\n");
  }
  if ( (error = PF_UnpinPage(AM_itab_iter->fd, AM_iscantab_iter->current_page, 0)) != PFE_OK) PF_ErrorHandler(error);
	set_recid(recid, AME_EOF);
  return recid;
}


int AM_InsertEntry(int AM_fd, char *value, RECID recId) {
  int error, mem_off, i;
  AM_itab_entry* AM_itab_iter;

  int cur_leafnode, new_leafnode; /* points to the leafnode */
  int split_pos, insert_pos;      /* points to the i'th position of "keyval" inside the node */ 
  int root_node;               /* used in CASE 1., to indicate pagenum of new rootnode */
  int keyval_size;                /* "keyval"'s size, changes according to the attrType */
  bool_t is_leaf; int num_keys, next, previous; /* node's header metadata */
  int* visited_path_node;         /* array of node visited : [root, level1, ... ,leaf ] */
  int len_visited_path_node;

  int nodeNum; 
  int last_pt;
  int left_node, right_node;
  int parent;

  char attrType;

  char* page_buf;     /* holds the page */
  char* temp_buf;     /* used when splitting the node */
  char* new_leaf_buf; /* */
  char *new_node_buf; /* */

  /* holds value, part of "keyval", to copy up to the parent (when splitting) */
  int ivalue; float fvalue; char* cvalue;
 
  /* check parameters & AM_itab */
  if (AM_fd < 0) return AME_FD;
  if (AM_fd >= AM_itab_length) return AME_FD;
  /* set iterator */
  AM_itab_iter = AM_itab + AM_fd;
  /* if the B+tree is not in AM_itab, return */
  if (AM_itab_iter->valid != TRUE) return AME_INDEXNOTOPEN;

  /*==================================================
   * CASE 1. No Existing Root. Initial Insertion     *
   * CASE 2. Leaf Exists & Leaf Not Full             *
   * CASE 3. Leaf Exists & Leaf Full & Split Needed  *
   *==================================================*/

  /*===== CASE 1. No Existing Root. Initial Insertion =====*/
  if (AM_itab_iter->header.racine_page == -1) {

    /* make new tree & write the node/page. Write to root_node, page_buf*/
    if ( (error = PF_AllocPage(AM_itab_iter->fd, &root_node, &page_buf)) != PFE_OK) PF_ErrorHandler(error);

    /* set the values & correctly create the "leaf node" */
    is_leaf = TRUE;
    num_keys = 1;
    previous = FIRST_LEAF;
    next = LAST_LEAF;

    /* JM_edit -> revise ways to initialize/set the object */
    memcpy((char*)(page_buf),(bool_t *) &is_leaf, sizeof(bool_t)); 
    memcpy((char*)(page_buf + sizeof(bool_t)),(int *) &num_keys, sizeof(int));
    memcpy((char*)(page_buf + sizeof(bool_t) + sizeof(int) ),(int *) &previous, sizeof(int)); 
    memcpy((char*)(page_buf + sizeof(bool_t) + 2*sizeof(int)),(int *) &next, sizeof(int));
    /* Insert "keyval" */
    memcpy((char*)(page_buf + sizeof(bool_t) + 3*sizeof(int)), (RECID*) &recId, sizeof(RECID));
    switch (AM_itab_iter->header.attrType) {
      case 'c':
        memcpy((char*)(page_buf+sizeof(bool_t)+3*sizeof(int)+sizeof(RECID)), (char*)value, AM_itab_iter->header.attrLength);
        break;
      case 'i':
        memcpy((char*)(page_buf+sizeof(bool_t)+3*sizeof(int)+sizeof(RECID)), (int*)value, AM_itab_iter->header.attrLength);
        break;
      case 'f':
        memcpy((char*)(page_buf+sizeof(bool_t)+3*sizeof(int)+sizeof(RECID)), (float*)value, AM_itab_iter->header.attrLength);
        break;
      default:
        return AME_INVALIDATTRTYPE;
        break;
    }

    /* set AM_itab_iter's metadata (B+tree's header metadata) */
    AM_itab_iter->header.racine_page = 2;
    AM_itab_iter->header.height_tree = 1;
    AM_itab_iter->header.nb_leaf = 1;
    AM_itab_iter->header.num_pages++;
    AM_itab_iter->dirty = TRUE;

    /* Unpin the page */
    if ( (error = PF_UnpinPage(AM_itab_iter->fd, root_node, 1)) != PFE_OK) PF_ErrorHandler(error);
    /* case1: Insertion properly done, Root node created */
    return AME_OK;
  }


  /*===== CASE 2, CASE 3, (Leaf Exists) =====*/
  /* set visited_path_node & metadata & read the leaf page */
  visited_path_node = malloc(AM_itab_iter->header.height_tree * sizeof(int));
  /* Find the insert key position, in node/page: <0: error, else: key position */
  if ( (insert_pos = AM_FindLeaf(AM_fd, value, visited_path_node)) < 0) return AME_KEYNOTFOUND;
  len_visited_path_node = AM_itab_iter->header.height_tree;
  cur_leafnode = visited_path_node[(AM_itab_iter->header.height_tree)-1];

  /* read the leaf page to "page_buf", with cur_leafnode */
  if ( (error = PF_GetThisPage(AM_itab_iter->fd, cur_leafnode, &page_buf)) != PFE_OK) PF_ErrorHandler(error);

  /* Check the number of keys in the leaf node */
  memcpy((int*) &num_keys, (char*)(page_buf + sizeof(bool_t)), sizeof(int));
  /* set the keyval_size (couple_size) accordingly to the RECID & attrType */
  keyval_size = sizeof(RECID) + AM_itab_iter->header.attrLength;


  /*===== CASE 2. Leaf Exists & Leaf Not Full =====*/
  if ( num_keys < AM_itab_iter->fanout_leaf - 1) {

    /*       "if_leaf"  "num_keys|previous|next"  "keyval_size*insert_pos" */
    mem_off = sizeof(bool_t) + 3*sizeof(int)     + keyval_size*insert_pos;
    /* make some space/room for the new "keyval" entry */
    memmove((char*) (page_buf + mem_off + keyval_size), (char*) (page_buf + mem_off), keyval_size*(num_keys - insert_pos));

    /* Place the new "keyval" entry in the leaf node */
    /* set recId */
    memcpy((char *) (page_buf + mem_off), (RECID*) &recId, sizeof(RECID));
    mem_off += sizeof(RECID);
    /* set value, accordingly to the attrType */
    switch (AM_itab_iter->header.attrType){
      case 'c':
        memcpy((char*)(page_buf + mem_off), (char*) value, AM_itab_iter->header.attrLength);
        break;
      case 'i':
        memcpy((char*)(page_buf + mem_off), (int*) value, AM_itab_iter->header.attrLength);
        break;
      case 'f':
        memcpy((char*)(page_buf + mem_off), (float*) value, AM_itab_iter->header.attrLength);
        break;
      default:
        return AME_INVALIDATTRTYPE;
        break;
    }
    /* setting num_keys right */
    num_keys++;
    memcpy((char*)(page_buf + sizeof(bool_t)), (int*) &num_keys, sizeof(int));

    /* Unpin the leaf page that we just modified */
    if ( (error = PF_UnpinPage(AM_itab_iter->fd, cur_leafnode, 1)) != PFE_OK) PF_ErrorHandler(error);
      
    free(visited_path_node);
    return AME_OK;
  }

  /*===== CASE 3. Leaf Exists & Leaf Full & Split Needed =====*/
  /* JM_edit
   * Phase1: Find the "split_pos", among the "keyval"s  
   * Phase2: Write&Update old & new leafnode
   *
   * Phase3: Loop into Parent Node
   *                                                    */

  else {
    /* temp_buf to hold the entire node's "keyval"s */
    temp_buf = malloc(keyval_size*(num_keys + 1));
    /* mem_off set to the "keyval" starting point */
    mem_off = sizeof(bool_t) + 3*sizeof(int);

    /*=== Phase1. Find the "split_pos", among the "keyval"s ===*/
    /* Copy the "Before" "keyval"s in the target leaf node, to the temp_buf */
    memcpy((char*) (temp_buf) , (char*) (page_buf + mem_off), keyval_size*insert_pos);
    mem_off += keyval_size*insert_pos;
    /* place the Inserting "keyval" into the temp_buf */
    memcpy((char *) (temp_buf + keyval_size * insert_pos ), (RECID*) &recId, sizeof(RECID));
    switch (AM_itab_iter->header.attrType){
      case 'c':
        cvalue = malloc(AM_itab_iter->header.attrLength);
        memcpy((char*)(temp_buf + keyval_size * insert_pos + sizeof(RECID)), (char*) value, AM_itab_iter->header.attrLength);
        break;
      case 'i':
        memcpy((char*)(temp_buf + keyval_size * insert_pos + sizeof(RECID)), (int*) value, AM_itab_iter->header.attrLength);
        break;
      case 'f':
        memcpy((char*)(temp_buf + keyval_size * insert_pos + sizeof(RECID)), (float*) value, AM_itab_iter->header.attrLength);
        break;
      default:
        return AME_INVALIDATTRTYPE;
        break;
    }
    mem_off += keyval_size;
    /* Copy the "After" "keyval"s in the target leaf node, to the temp_buf */
    memcpy((char*) (temp_buf + keyval_size*(insert_pos+1)), (char*) (page_buf + mem_off), keyval_size*(num_keys - insert_pos));

    /* Find the split "keyval" position */
    split_pos = AM_LeafSplitPos(temp_buf, num_keys, AM_itab_iter->header.attrLength, AM_itab_iter->header.attrType);

    /* leaf node full of duplicate keys */
    if (split_pos==0) return AME_DUPLICATEKEY; 


    /*=== Phase2. Write&Update old & new leafnode ===*/
    /* new page for new leaf node */
    if ( (error = PF_AllocPage(AM_itab_iter->fd, &new_leafnode, &new_leaf_buf)) != PFD_OK) PF_ErrorHandler(error);

    /* metadata for the new leaf node's header*/
    /* (bool_t)is_leaf|(int)num_keys|(int)previous|(int)next */
    is_leaf = 1;
    num_keys = (AM_itab_iter->fanout_leaf - 1) - split_pos;
    previous = cur_leafnode;
    memcpy((int *) &next, (char*) (page_buf + sizeof(bool_t) + 2*sizeof(int)), sizeof(int));

    /* Write to the new leaf node's header */
    memcpy((char*)(new_leaf_buf),                                 (bool_t*)&is_leaf,sizeof(bool_t));
    memcpy((char*)(new_leaf_buf + sizeof(bool_t)),                (int*)&num_keys,sizeof(int));
    memcpy((char*)(new_leaf_buf + sizeof(bool_t) + sizeof(int)),  (int*)&previous,sizeof(int));
    memcpy((char*)(new_leaf_buf + sizeof(bool_t) + sizeof(int)*2),(int*)&next,sizeof(int));
    /* copy the "After" "keyval"s */
    memcpy((char*)(new_leaf_buf + sizeof(bool_t) + sizeof(int)*3),(char*)(temp_buf+keyval_size*split_pos),keyval_size*num_keys);

    /* update old_leaf */
    num_keys = split_pos;
    next = new_leafnode;

    memcpy((char*)(page_buf + sizeof(bool_t)),                 (int*)&num_keys, sizeof(int));
    memcpy((char*)(page_buf + sizeof(bool_t) + sizeof(int)*2), (int*)&next, sizeof(int));
    /* copy the "Before" "keyval"s */
    memcpy((char*)(page_buf + sizeof(bool_t) + sizeof(int)*3), (char *)(temp_buf), keyval_size*split_pos);

    /* Copy split_pos's "keyval", to insert into the Parent's Node */
    mem_off = sizeof(bool_t) + 3*sizeof(int) + sizeof(RECID);
    switch (AM_itab_iter->header.attrType){
      case 'c':
        memcpy((char*) cvalue, (char*)(new_leaf_buf + mem_off), AM_itab_iter->header.attrLength);
        printf("Value to insert in upper node : %s\n", cvalue);
        break;
      case 'i':
        memcpy((int*) &ivalue, (char*)(new_leaf_buf + mem_off ), AM_itab_iter->header.attrLength);
        printf("Value to insert in upper node : %d\n", ivalue);
        break;
      case 'f':
        memcpy((float*) &fvalue, (char*)(new_leaf_buf + mem_off ), AM_itab_iter->header.attrLength);
        printf("Value to insert in upper node : %f\n", fvalue);
        break;
      default:
        return AME_INVALIDATTRTYPE;
        break;
    }

    free(temp_buf);

    /* Unpin both cur_leafnode & new_leafnode, after splitting */
    if ( (error = PF_UnpinPage(AM_itab_iter->fd,new_leafnode,1)) != PFE_OK) PF_ErrorHandler(error);
    if ( (error = PF_UnpinPage(AM_itab_iter->fd,cur_leafnode,1)) != PFE_OK) PF_ErrorHandler(error);
     
    /* Update B+tree's header */
    AM_itab_iter->header.nb_leaf++;
    AM_itab_iter->header.num_pages++;
    AM_itab_iter->dirty = TRUE;



    /*=== Phase3: Loop into Parent Node ===*/
    cur_leafnode = visited_path_node[(AM_itab_iter->header.height_tree)-1];
    parent = 0;

    left_node = cur_leafnode;
    right_node = new_leafnode;

    for(i = len_visited_path_node; i>0; i--){

      parent = visited_path_node[i - 1];
      printf("parent page : %d\n", parent);

      /* CASE : PARENT IS ROOT */
      if(parent == len_visited_path_node){
        printf("LOL\n");
      }

      if(parent == AM_itab_iter->header.racine_page){
        printf("create internal node in the root\n");

        error = PF_AllocPage(AM_itab_iter->fd, &nodeNum, &page_buf);
        if (error!=PFE_OK)
          PF_ErrorHandler(error);

        is_leaf = 0;
        num_keys = 1;
        last_pt = right_node;

        mem_off = 0;
        memcpy((char*)(page_buf + mem_off), (int*)&is_leaf, sizeof(bool_t));
        mem_off += sizeof(bool_t);
        memcpy((char*)(page_buf + mem_off), (int*)&num_keys, sizeof(int));
        mem_off += sizeof(int);
        memcpy((char*)(page_buf + mem_off), (int*)&last_pt, sizeof(int));
        mem_off += sizeof(int);

        memcpy((char*)(page_buf + mem_off), (int*)&left_node, sizeof(int));
        mem_off += sizeof(int);

        /*insert key */
        switch (AM_itab_iter->header.attrType){
          case 'c':
            memcpy((char*)(page_buf + mem_off), (char*) cvalue, AM_itab_iter->header.attrLength);
            break;
          case 'i':
            memcpy((char*)(page_buf + mem_off), (int*) &ivalue, AM_itab_iter->header.attrLength);
            break;
          case 'f':
            memcpy((char*)(page_buf + mem_off), (float*) &fvalue, AM_itab_iter->header.attrLength);
            break;
          default:
            return AME_INVALIDATTRTYPE;
            break;
        }
        mem_off += AM_itab_iter->header.attrLength;

        /* last_pt
        memcpy((char*)(page_buf + mem_off), (int*)&new_leafnode, sizeof(int));
        */

        AM_itab_iter->header.height_tree++;
        AM_itab_iter->header.num_pages++;
        AM_itab_iter->header.racine_page = nodeNum;
        AM_itab_iter->dirty = TRUE;

        error = PF_UnpinPage(AM_itab_iter->fd, nodeNum, 1);
        if (error!=PFE_OK)
          PF_ErrorHandler(error);

        printf("New root on page : %d\n", nodeNum);


        /*print_page(AM_fd, nodeNum);*/
        free(visited_path_node);
        return AME_OK;
      }

      /* PARENT IS NOT ROOT */
      else{
        parent = visited_path_node[i - 2];
        printf("Insert left node : %d, right node : %d  Into parent : %d\n", left_node, right_node, parent);

        /* get parent page ; if not full insert and return AME_OK */
        error = PF_GetThisPage(AM_itab_iter->fd, parent, &page_buf);
        if (error != PFE_OK)
          PF_ErrorHandler(error);

        mem_off = sizeof(bool_t);
        memcpy((int*) &num_keys, (char*)(page_buf+mem_off), sizeof(int));
        mem_off += sizeof(int);

        keyval_size = sizeof(int) + AM_itab_iter->header.attrLength;

        insert_pos = num_keys; /* NEED TO BE CHANGED :get the good insert_position 'ask paul'*/

        /* temp buffer hold the node with the inserted value
         * if good size, copy yo page_buf
         * else split in 2 nodes
         */
        temp_buf = malloc(keyval_size*(num_keys + 1) + sizeof(bool_t) + 2 * sizeof(int) );
        memcpy((char*) temp_buf, (char*)(page_buf), sizeof(bool_t) + 2 * sizeof(int) + keyval_size*(num_keys + 1) );

        if (insert_pos == num_keys){
          /*INSERT THE KEY IN LAST POSITION*/
          /* create a couple with the value and the ex last pointer, insert it  */
          memcpy((int*)&last_pt, (char *) (temp_buf + mem_off), sizeof(int));
          mem_off += sizeof(int) + insert_pos*keyval_size;

          /* insert right_node pagenum of couple */
          memcpy((char*)(temp_buf + mem_off), (int *)&last_pt, sizeof(int));
          mem_off += sizeof(int);

          /*insert key */
          switch (AM_itab_iter->header.attrType){
            case 'c':
              memcpy((char*)(temp_buf + mem_off), (char*) cvalue, AM_itab_iter->header.attrLength);
              break;
            case 'i':
              memcpy((char*)(temp_buf + mem_off), (int*) &ivalue, AM_itab_iter->header.attrLength);
              break;
            case 'f':
              memcpy((char*)(temp_buf + mem_off), (float*) &fvalue, AM_itab_iter->header.attrLength);
              break;
            default:
              return AME_INVALIDATTRTYPE;
              break;
          }
        }
        else{
          /*INSERT THE KEY IN ANOTHER POSITION */
          mem_off = sizeof(bool_t) + 2 * sizeof(int) + sizeof(int) + keyval_size*insert_pos; /* the last sizeof int represent the first pointer that won't move*/
          /*
           * node : header+ p4 + |p1| key1 |p2| key 2|p3| key3
           insert key4, p5 in insert_pos 0 => node : header+ p4 + |p1| key4|p5| key1 |p2| key 2|p3| key3
          */

          memmove((char*) (temp_buf + mem_off + keyval_size), (char*) (temp_buf + mem_off), keyval_size*(num_keys - insert_pos)-sizeof(int));

          /*insert the couple starting with the key */
          switch (AM_itab_iter->header.attrType){
            case 'c':
              memcpy((char*)(temp_buf + mem_off), (char*) cvalue, AM_itab_iter->header.attrLength);
              break;
            case 'i':
              memcpy((char*)(temp_buf + mem_off), (int*) &ivalue, AM_itab_iter->header.attrLength);
              break;
            case 'f':
              memcpy((char*)(temp_buf + mem_off), (float*) &fvalue, AM_itab_iter->header.attrLength);
              break;
            default:
              return AME_INVALIDATTRTYPE;
              break;
          }
          mem_off += AM_itab_iter->header.attrLength;
        }

        mem_off = sizeof(bool_t) + sizeof(int);

        /* update last AM_itab_iter */
        memcpy((char*)(temp_buf + mem_off), (int *)&right_node, sizeof(int));


        if (num_keys < AM_itab_iter->fanout -1){

          if (insert_pos == num_keys ){

            memcpy((char*)(page_buf), (char*) temp_buf, sizeof(bool_t) + 2 * sizeof(int) + keyval_size*(num_keys + 1) );
            free(temp_buf);
          }
          /* insert the new pointer and the num key */
          else{

            memcpy((char*)(page_buf), (char*) temp_buf, sizeof(bool_t) + 2 * sizeof(int) + keyval_size*(num_keys + 1) );
            free(temp_buf);
          }

          /*then update the number of key*/
          num_keys++;
          memcpy((char*)(page_buf + sizeof(bool_t)), (int*)&num_keys, sizeof(int));
          error = PF_UnpinPage(AM_itab_iter->fd, parent, 1);
          if (error !=PFE_OK)
            PF_ErrorHandler(error);

          /*print_page(AM_fd, parent);*/
          free(visited_path_node);
          return AME_OK;
        }

        else {
          /*printf("No more space in the node\n");*/
          split_pos = num_keys / 2;

          /* Allocate a page for the new node */
          error = PF_AllocPage(AM_itab_iter->fd, &right_node, &new_node_buf);
          if (error != PFE_OK){
            PF_ErrorHandler(error);
          }

          /* Update new node */
          is_leaf = 0;
          num_keys = (AM_itab_iter->fanout -1 ) - split_pos;
          memcpy((int*) &last_pt, (char*)(temp_buf + sizeof(bool_t) + sizeof(int)), sizeof(int));

          mem_off = 0;
          memcpy((char *) (new_node_buf + mem_off), (bool_t*) &is_leaf, sizeof(bool_t));
          mem_off += sizeof(bool_t);
          memcpy((char *) (new_node_buf + mem_off), (int*) &num_keys, sizeof(int));
          mem_off += sizeof(int);
          memcpy((char *) (new_node_buf + mem_off), (int*)&last_pt, sizeof(int));
          mem_off += sizeof(int);
          memcpy((char *) (new_node_buf + mem_off), (char *) (temp_buf + mem_off + keyval_size*split_pos), keyval_size * num_keys);


          /* Update old node */
          num_keys = split_pos;
          memcpy((int*) &last_pt, (char*) (temp_buf + mem_off + keyval_size*(split_pos - 1)), sizeof(int) );

          mem_off = sizeof(bool_t);
          memcpy((char *) (page_buf + mem_off), (int*) &num_keys, sizeof(int));
          mem_off += sizeof(int);
          memcpy((char *) (page_buf + mem_off), (int*) &last_pt, sizeof(int));
          mem_off += sizeof(int);
          memcpy((char *) (page_buf + mem_off), (char *)(temp_buf + mem_off), keyval_size*split_pos);

          /* get the value to push up for other nodes (aka te first value in the new node) */
          mem_off = sizeof(bool_t) + 2 * sizeof(int) + sizeof(int);
          switch (AM_itab_iter->header.attrType){
            case 'c':
              memcpy((char*) cvalue, (char*)(new_node_buf + mem_off), AM_itab_iter->header.attrLength);
              printf("Value to insert in upper node : %s\n", cvalue);
              break;
            case 'i':
              memcpy((int*) &ivalue, (char*)(new_node_buf + mem_off), AM_itab_iter->header.attrLength);
              printf("Value to insert in upper node : %d\n", ivalue);
              break;
            case 'f':
              memcpy((float*) &fvalue, (char*)(new_node_buf + mem_off), AM_itab_iter->header.attrLength);
              printf("Value to insert in upper node : %f\n", fvalue);
              break;
            default:
              return AME_INVALIDATTRTYPE;
              break;
          }

          free(temp_buf);

          error = PF_UnpinPage(AM_itab_iter->fd, right_node, 1);
          if (error!= PFE_OK)
            PF_ErrorHandler(error);

          error = PF_UnpinPage(AM_itab_iter->fd, parent, 1);
          if (error!= PFE_OK)
            PF_ErrorHandler(error);

          printf("Node %d splitted to node %d\n", parent, right_node);
          printf("pushing the key up\n\n");

          AM_itab_iter->header.num_pages++;
          AM_itab_iter->dirty = TRUE;

          left_node = parent;
        }



        return AME_NOINSERTION;
      }


    }
  }

  free(visited_path_node);
  return AME_OK;
}

int AM_CloseIndexScan(int scanDesc) {
  AM_iscantab_entry* AM_iscantab_iter;

  /* check the parameters & check the iscantab */
  if (scanDesc < 0) return AME_INVALIDSCANDESC
  if (scanDesc >= AM_iscantab_length && AM_iscantab_length !=0) return AME_INVALIDSCANDESC;

  /* set the iterator & check validity  */
  AM_iscantab_iter = AMscantab + scanDesc;
  if (AM_iscantab_iter->valid == FALSE) return AME_SCANNOTOPEN;

  /* if it's the last tail scan in the iscantab, remove the invalid ones */
  if (scanDesc == AM_iscantab_length - 1) {
    if (AM_iscantab_length > 0) AM_iscantab_length--;
    while( (AM_iscantab_length > 0) && ((AMscantab + AM_iscantab_length - 1)->valid == FALSE)) {
      AM_iscantab_length--;
    }
  }
  return AME_OK;
}





