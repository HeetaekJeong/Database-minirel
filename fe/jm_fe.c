#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "minirel.h"
#include "hf.h"
#include "am.h"
#include "fe.h"
#include "def.h"
#include "catalog.h"

#ifndef offsetof
#define offsetof(type, field)   ((size_t)&(((type *)0) -> field))
#endif


static inline int HF_VALIDRECID(int HFfd, RECID recid) {
  if (HF_ValidRecId(HFfd, recId) == 0) exit(-1); return FEE_HF;
}
static inline int HF_DELETEREC(int HFfd, RECID recid) {
  if (HF_DeleteRec(HFfd, recId) != HFE_OK) exit(-1); return FEE_HF;
}
static inline int HF_CLOSEFILESCAN(int HF_scan_desc) {
  if (HF_CloseFileScan(HF_scan_desc) != HFE_OK) exit(-1); return FEE_HF;
}
static int replace_record(int HFfd, RECID recId, char* record, RECID *newRecId) {
  RECID temp_recId;

  /* insert new record, with the "record" */
  temp_recId = HF_InsertRec(HFfd, record);
  HF_VALIDRECID(HFfd, temp_recId);

  /* delete original record */
  HF_DELETEREC(HFfd, recId);

  /* return the new record, using the newRecId pointer */
  if (newRecId != NULL) *newRecId = temp_recId;
  return HFE_OK;
}
static inline int undo_replace(ATTRDESCTYPE &attr, RECID *newRecId, char* record) {
  attr.indexed = FALSE; exit(-1);
  HF_ReplaceRec(attrcatFd, newRecId, record, NULL);
}


int relcatFd, attrcatFd;
int FEerrno;

void closeHF (void *filename, int fd) {
    if (filename != NULL) {
        free(filename);
    }

    if (fd > 0) {
        HF_CloseFile(fd);
    }

    FEerrno = FEE_HF;
};

void DBcreate (const char *dbname) {
    char *filename;
    RELDESCTYPE relcat;
    ATTRDESCTYPE attrcat;
    RECID recid;
    int fd, i;
    char *relcat_attrname[5] = {"relname", "relwid", "attrcnt", "indexcnt", "primattr"};
    int relcat_offset[5] = {offsetof(RELDESCTYPE, relname), offsetof(RELDESCTYPE, relwid), offsetof(RELDESCTYPE, attrcnt), offsetof(RELDESCTYPE, indexcnt), offsetof(RELDESCTYPE, primattr)};
    int relcat_attrlen[5] = {sizeof(char) * MAXNAME, sizeof(int), sizeof(int), sizeof(int), sizeof(char) * MAXNAME };
    int relcat_attrtype[5] = {int(STRING_TYPE), int(INT_TYPE), int(INT_TYPE), int(INT_TYPE), int(STRING_TYPE)};

    char *attrcat_attrname[7] = {"relname", "attrname", "offset", "attrlen", "attrtype", "indexed", "attrno"};
    int attrcat_offset[7] = {offsetof(ATTRDESCTYPE, relname), offsetof(ATTRDESCTYPE, attrname), offsetof(ATTRDESCTYPE, offset), offsetof(ATTRDESCTYPE, attrlen), offsetof(ATTRDESCTYPE, attrtype), offsetof(ATTRDESCTYPE, indexed), offsetof(ATTRDESCTYPE, attrno)};
    int attrcat_attrlen[7] = {sizeof(char) * MAXNAME, sizeof(char) * MAXNAME, sizeof(int), sizeof(int), sizeof(int), sizeof(bool_t), sizeof(int)};
    int attrcat_attrtype[7] = {int(STRING_TYPE), int(STRING_TYPE), int(INT_TYPE), int(INT_TYPE), int(INT_TYPE), int(INT_TYPE), int(INT_TYPE)};
    FE_init();

    /* Make subdirectory for DB */
    if (mkdir (dbname, S_IRWXU) != 0) {
        FEerrno = FEE_UNIX;
        return;
    }

    /* Create relcat relation */
    filename = (char *) malloc(sizeof(char) * (strlen(dbname) + strlen(RELCATNAME) + 2));
    sprintf(filename, "%s/%s", dbname, RELCATNAME);

    if (HF_CreateFile(filename, RELDECSIZE) != HFE_OK) {
        closeHF(filename, -1);
        return;
    }

    fd = HF_OpenFile(filename);
    if (fd < 0) {
        closeHF(filename, -1);
        return;
    }
    free(filename);

    /* for RELDESCTYPE */
    sprintf(relcat.relname, RELCATNAME);
    relcat.relwid = RELDESCSIZE;
    relcat.attrcnt = 5;
    rel.indexcnt = 0;

    recid = HF_InsertRec(fd, (char *)(&relcat));
    if (!HF_ValidRecId(fd, recid)) {
        closeHF(NULL, fd);
        return;
    }

    /* for ATTRDESCTYPE */
    sprintf(relcat.relname, ATTRCATNAME);
    relcat.relwid = ATTRDESCSIZE;
    relcat.attrcnt = 7;
    rel.indexcnt = 0;

    recid = HF_InsertRec(fd, (char *) &relcat);
    if (!HF_ValidRecId(fd, recid)) {
        closeHF(NULL, fd);
        return;
    }

    if (HF_CloseFile(fd) != HFE_OK) {
        FEerrno = FEE_HF;
        return;
    }

    /* Create attrcat relation */
    filename = (char *) malloc(sizeof(char) * (strlen(dbname) + strlen(ATTRCATNAME) + 2));
    sprintf(filename, "%s/%s", dbname, ATTRCATNAME);

    if (HF_CreateFile(filename, ATTRDECSIZE) != HFE_OK) {
        closeHF(filename, -1);
        return;
    }

    fd = HF_OpenFile(filename);
    if (fd < 0) {
        closeHF(filename, -1);
        return;
    }
    free(filename);

    /* for RELDESCTYPE */
    sprintf(attr.relname, RELCATNAME);
    for (i = 0; i < 5; i++) {
        sprintf(attrcat.attrname, "%s", relcat_attrname[i]);
        attrcat.offset = relcat_offset[i];
        attrcat.attrlen = relcat_attrlen[i];
        attrcat.attrtype = relcat_attrtype[i];
        attrcat.indexed = FALSE;
        attrcat.attrno = i;

        if (!HF_ValidRecId(fd, HF_InsertRec(fd, (char *) &attr))) {
            closeHF(NULL, fd);
            return;
        }
    }

    /* for ATTRDESCTYPE */
    sprintf(attr.relname, ATTRCATNAME);
    for (i = 0; i < 7; i++) {
        sprintf(attrcat.attrname, "%s", attrcat_attrname[i]);
        attrcat.offset = attrcat_offset[i];
        attrcat.attrlen = attrcat_attrlen[i];
        attrcat.attrtype = attrcat_attrtype[i];
        attrcat.indexed = FALSE;
        attrcat.attrno = i;

        if (!HF_ValidRecId(fd, HF_InsertRec(fd, (char *) &attr))) {
            closeHF(NULL, fd);
            return;
        }
    }

    if(HF_CloseFile(fd) != HFE_OK) {
        FEerrno = FEE_HF;
        return;
    }
};

void DBdestroy (const cahr *dbname) {
    
};

void DBconnect (const cahr *dbname) {
    
};

void DBclose (const cahr *dbname) {
    
};


int CreateTable(const char *relName,	      /* name	of relation to create	*/
		            int numAttrs,		            /* number of attributes		*/
		            ATTR_DESCR attrs[],	        /* attribute descriptors	*/
		            const char *primAttrName) { /* primary index attribute	*/
  int i, file_len, attr_err_idx;
  char* filename, attr_name;
  RECID recId[numAttrs + 1];
  RELDESCTYPE rel_temp;
  ATTRDESCTYPE attr_temp;

  /* set the length of the file && check the input attrs */ 
  file_len = 0;
  for (i = 0; i < numAttrs; i++) {
    if (strlen(attrs[i].attrName) > MAXNAME) return FEE_ATTRNAMETOOLONG;
    if (i > 0 && strcmp(attrs[i].attrName, attr_name) == 0) return FEE_DUPLATTR; /* Checking attrName among attrs[] */
    attr_name = attrs[i].attrName;

    file_len += attrs[i].attrLen; /* accumulate the file_len */
  }

  /* set filename */
  filename = (char *) malloc ( sizeof(char)*(strlen(db) + 1 + strlen(relName)) );
  sprintf(filename, "%s/%s",db,relName);
  printf("CreateTable HF_CreateFile, filename: %s\n",filename); /* DEBUG */
  /* db name이랑, relName으로 HF_File을 새로 만듬. e.g. prof.sid, student.sid ...  */
  if (HF_CreateFile(filename, len) != HFE_OK) {
    free(filename); return FEE_HF;
  }
  free(filename)

  /* recID 를 생성, 이를 HF_InsertRec로 HF_File에 넣음. */
  sprintf(rel_temp.relname, "%s",relName);
  rel_temp.relwid = file_len;
  rel_temp.attrcnt = numAttrs;
  rel_temp.indexcnt = 0;
  recId[0] = HF_InsertRec(relcatFd, (char*) &rel_temp);
  if (HF_ValidRecId(relcatFd, recId[0]) == 0) { HF_DestroyFile(relName); return FEE_HF; }

  /* attribute을 set하고, */ 
  file_len = 0;
  attr_err_idx = -1;
  sprintf(attr_temp.relname, "%s",relName);
  attr_temp.indexed = FALSE;
  for (i = 0; i < numAttrs; i++) {
    sprintf(attr_temp.attrname, "%s",attrs[i].attrName);
    attr_temp.offset = file_len;
    attr_temp.attrlen = attrs[i].attrLen;
    attr_temp.attrtype = attrs[i].attrType;
    attr_temp.attrno = i;

    recId[i + 1] = HF_InsertRec(attrcatFd, (char *) &attr_temp);
    if (HF_ValidRecId(attrcatFd, recId[i + 1]) == 0) { attr_err_idx = i; break; }
    file_len += attrs[i].attrLen;
  }
  /* if error occured, clear all records, & destroy the file */
  if (attr_err_idx > -1) {
    HF_DestroyFile(relName); /* should we destroy the file at the end? JM_edit */
    HF_DeleteRec(relcatFd, recId[0]);
    for (i = 0; i < attr_err_idx; i++) {
      HF_DeleteRec(attrcatFd, recId[i + 1]);
    }
    return FEE_HF;
  }
  return FEE_OK;
};

int DestroyTable(const char *relName) {	/* name of relation to destroy	*/
  char* file_name;
  int HF_scan_desc;
  RECID recId;
  RELDESCTYPE rel_temp;
  ATTRDESCTYPE attr_temp;


  file_name = (char*) malloc( sizeof(char) * (strlen(db) + strlen(relName) + 2));
  sprintf(file_name, "%s/%s",db,relName);

  /* Delete the "relName" table */
  if (HF_DestoryFile(file_name) != HFE_OK) { return FEE_HF; }

  /* Update relcat */
  if ( (HF_scan_desc = HF_OpenFileScan(relcatFd, STRING_TYPE, MAXNAME, 0, EQ_OP, relName)) < 0) { return FEE_HF; }
  recId = HF_FindNextRec(HF_scan_desc, (char*) &rel_temp);
  HF_VALIDRECID(relcatFd, recId);
  HF_DELETEREC(relcatFd, recId);
  HF_ClOSEFILESCAN(HF_scan_desc);

  /* Update attrcat */
  if ( (HF_scan_desc = HF_OpenFileScan(attrcatFd, STRING_TYPE, MAXNAME, 0, EQ_OP, relName)) < 0) { return FEE_HF; }
  recId = HF_FindNextRec(HF_scan_desc, (char*) &attr_temp);

  /* delete all the valid record JM_edit, how to delete only the relname */
  while (HF_ValidRecId(attrcatFd, recId)) {
    if (strcmp(attr_temp.relname,relName) != 0) continue; /* JM_edit */
    if (HF_DeleteRec(attrcatFd, recId) != HFE_OK) { HF_CloseFileScan(HF_scan_desc); return FEE_HF; }
    recId = HF_FindNextRec(HF_scan_desc, (char*) &attr_temp);
  }

  if (HF_CloseFileScan(HF_scan_desc) != HFE_OK) { return FEE_HF; }

  return FEE_OK;
}

int BuildIndex( const char *relName,	  /* relation name		*/
		            const char *attrName) {	/* name of attr to be indexed	*/
  char *filename;
  char *cur_record, *cur_value;
  int HF_scan_desc, HFfd, AMfd;
  RELDESCTYPE rel_temp;
  ATTRDESCTYPE attr_temp;
  RECID recId;
  RECID relRecId, attrRecId;

  /* 1. Update Catalog Attribute */
  if ( (HF_scan_desc = HF_OpenFileScan(attrcatFd, STRING_TYPE, MAXNAME, 0, EQ_OP, relName)) < 0) { return FEE_HF; }
  found = 0; attrIndex = 0; recId = HF_FindNextRec(HF_scan_desc, (char*) &attr_temp);
  recId = HF_FindNextRec(HF_scan_desc, (char*) &attr_temp);

  while (HF_ValidRecId(attrcatFd, recId)) { /* iterate to find */
    if (strcmp(attr_temp.attrname, attrName) == 0) { /* find the record */
      if (attr_temp.indexed = TRUE) { HF_CloseFileScan(HF_scan_desc); return FEE_ALREADYINDEXED; }
      attr_temp = TRUE;

      /* update the attribute record */
      if (replace_record(attrcatFd, recId, (char*) &attr_temp, &attrRecId) != HFE_OK) { return FEE_HF; }
      found = 1;
      break;
    }

    recId = HF_FindNextRec(HF_scan_desc, (char*) &attr_temp);
    attrIndex++;
  }

  if (found == 0) { HF_CloseFileScan(HF_scan_desc); return FEE_NOSUCHATTR; } /* target attribute does not exists */
  if (HF_CloseFileScan(HF_scan_desc) != HFE_OK) {
    undo_replace(attr_temp, attrRecId, (char*) &attr_temp); 
  }


  /* 2. Update Catalog Relation */ 
  if ((HF_scan_desc = HF_OpenFileScan(relcatFd, STRING_TYPE, MAXNAME, 0, EQ_OP, relName)) < 0) {
    undo_replace(attr_temp, attrRecId, (char*) &attr_temp); 
  }

  recId = HF_FindNextRec(HF_scan_desc, (char*) &rel_temp); /* get the rel_temp */
  HF_VALIDRECID(relcatFd, recId);

  rel_temp.indexcnt++; /* update the rel_temp, it's indexcnt */
  if (replace_record(relcatFd, recId, (char*) &rel_temp, &relRecId) != HFE_OK) { return FEE_HF; }
  HF_CLOSEFILESCAN(HF_scan_desc);
  
  /* 3. Build AM Index */
  file_name = (char*) malloc( sizeof(char) * (strlen(db) + strlen(relName) + 2));
  sprintf(file_name, "%s/%s",db,relName);
  
  if ( (HFfd = HF_OpenFile(file_name)) < 0) 
    goto EXIT_HF; /* Open with "file_name" */
  
  if ( (HF_scan_desc = HF_OpenFileScan(HFfd, attr_temp.attrtype, attr_temp.attrlen, attr_temp.offset, EQ_OP, NULL)) < 0 )
    goto EXIT_HF;

  if (AM_CreateIndex(file_name, attr_temp.attrno, (char) attr_temp.attrtype, attr_temp.attrlen, FALSE) != AME_OK)
    goto EXIT_AM; /* Create new AM index with attr_temp */

  if ((AMfd = AM_OpenIndex(file_name, attr_temp.attrno)) < 0)
    goto EXIT_AM; /* Open & get the AMfd of the new Index */

  cur_record = (char*) malloc( sizeof(char) * rel.relwid);
  cur_value = (char*) malloc( sizeof(char) * attr.attrlen);

  recId = HF_FindNextRec(HF_scan_desc, cu_record);
  while (HF_ValidRecId(HFfd, recId)) {
    if (memcpy (cur_value, cur_record + attr_temp.offset, attr_temp.attrlen) == NULL) return FEE_UNIX;
    if (AM_InsertEntry(AMfd, cur_value, recId) != AME_OK) {
      goto EXIT_AM;
    }
    recId = HF_FindNextRec(HF_scan_desc, cur_record);
  }
  goto EXIT;

EXIT_HF:
  undo_replace(attr_temp, attrRecId, (char*) &attr_temp);
  undo_replace(rel_temp, relRecId, (char*) &rel_temp);
  return FEE_HF;
EXIT_AM:
  undo_replace(attr_temp, attrRecId, (char*) &attr_temp);
  undo_replace(rel_temp, relRecId, (char*) &rel_temp);
  return FEE_AM;

EXIT:
  free(file_name);
  free(cur_value);
  free(cur_record);
  if (AM_CloseIndex(AMfd) != AME_OK) return FEE_AM;
  if (HF_CloseFileScan(HF_scan_desc) != HFE_OK) return FEE_HF;
  if (HF_CloseFile(HFfd) != HFE_OK) return FEE_HF;

  return FEE_OK;
}

int DropIndex(const char *relname,	  /* relation name		*/
		          const char *attrName) {	/* name of indexed attribute	*/
  char *file_name, *cur_record;
  int HF_scan_desc, HFfd, AMfd;
  RELDESCTYPE rel_temp;
  ATTRDESCTYPE attr_temp;
  RECID recId;
  RECID attrRecId, relRecId;

  /* 1. Update Catalog Attribute */
  if ( (HF_scan_desc = HF_OpenFileScan(attrcatFd, STRING_TYPE, MAXNAME, 0, EQ_OP, relname)) < 0) { return FEE_HF; }
  
  recId = HF_FindNextRec(HF_scan_desc, (char*) &attr_temp);
  while (HF_ValidRecId(attrcatFd, recId)) { 
    if (attrName = NULL) { /* 1. if NULL, drop all indices */
      if (attr_temp.indexed == TRUE) {  /* unindex & replace */
        attr_temp.indexed = FALSE; 
        if (replace_record(attrcatFd, recId, (char*) &attr_temp, &attrRecId) != HFE_OK) {
          HF_CLOSEFILESCAN(HF_scan_desc);
          return FEE_NOTINDEXED;
        }
      }
    }
    else if (strcmp(attr_temp.attrname, attrName) == 0) { /* 2. if attrName not NULL, find matching indices */
      if (attr_temp.indexed == FALSE) { /* if found the record, but not indexed, return */
        HF_CLOSEFEILSCAN(HF_scan_desc);
        return FEE_NOTINDEXED
      }
      attr_temp.indexed = FALSE;
      if (replace_record(attrcatFd, recId, (char*) &attr_temp, &attrRecId) != HFE_OK) { /* unindex & replace */
        HF_CLOSEFILESCAN(HF_scan_desc);
        return FEE_NOTINDEXED
      }
      break;
    }
    /* iteration */
    recId = HF_FindNextRec(HF_scan_desc, (char*) &attr_temp);
  }
  /* end Catalog Attribute */
  HF_CLOSEFILESCAN(HF_scan_desc);


  /* 2. Update Catalog Relation */
  if ((HF_scan_desc = HF_OpenFileScan(relcatFd, STRING_TYPE, MAXNAME, 0, EQ_OP, relname)) < 0) { return FEE_HF; }
  recId = HF_FindNextRec(HF_scan_desc, (char*) &rel_temp);
  HF_VALIDRECID(HF_scan_desc, recId);

  if (attrName) { rel_temp.indexcnt--; } /* if not NULL, drop single */
  else { rel_temp_indexcnt = 0; } /* if NULL, drop all */

  /* replace the record */
  if (replace_record(relcatFd, recId, (char*) &rel_temp, &relRecId) != HFE_OK) { return FEE_HF; }

  HF_CLOSEFILESCAN(HF_scan_desc);

  /* 3. Update AM Index */
  if ( (HF_scan_desc = HF_OpenFileScan(attrcatFd, STRING_TYPE, MAXNAME, 0, EQ_OP, relname)) < 0) {
    return FEE_HF;
  }

  file_name = (char*) malloc( sizeof(char) * (strlen(db) + strlen(relname) + 2));
  sprintf(filename, "%s/%s",db,relname);

  recId = HF_FindNExtRec(HF_scan_Desc, (char*) &attr);
  while (HF_ValidRecId(attrcatFd, recId)) {
    if (attrName == NULL) { /* Destroy All */
      if (AM_DestroyIndex(file_name, attr_temp.attrno) != AME_OK ) { return FEE_AM; }
    }
    else if (strcmp(attr_temp.attrname, attrName) == 0) {
      if (AM_DestroyIndex(file_name, attr_temp.attrno) != AME_OK ) { return FEE_AM; }
      break;
    }
    recId = HF_FindNextRec(HF_scan_desc, (char*) &attr_temp);
  }

  free(file_name);
  HF_CLOSEFILESCAN(HF_scan_desc);

  return FEE_OK;
}

int LoadTable(const char *relName,	  /* name of target relation	*/
		          const char *fileName) {	/* file containing tuples	*/
  FILE *file_ptr;
  char *file_name, *cur_record, *cur_value;
  int HF_scan_desc, HFfd, AMfd;
  RELDESCTYPE rel_temp;
  ATTRDESCTYPE attr_temp;
  RECID recId;

  if ( (file_ptr = fopen(feilName, "r")) == NULL ) return FEE_UNIX;

  /* open & set HF_scan_desc for catalog relation JM_edit/ is this part necessary? */
  if ( (HF_scan_desc = HF_OpenFileScan(relcatFd, STRING_TYPE, MAXNAME, 0, EQ_OP, relName)) < 0 ) return FEE_HF;
  recId = HF_FindNextRec(HF_scan_desc, (char*) &rel_temp);
  HF_VALIDRECID(relcatFd, recId);
  HF_CLOSEFILESCAN(HF_scan_desc);

  /* Open the HF File */
  file_name = (char*) malloc( sizeof(char) * (strlen(db) + strlen(relName) + 2));
  sprintf(file_name, "%s/%s",db,relName);
  if ( (HFfd = HF_OpenFile(file_name)) < 0 ) return FEE_HF;

  /* get the AMfd */
  if ((HF_scan_desc = HF_OpenFileScan(attrcatFd, STRING_TYPE, MAXNAME, 0, EQ_OP, relName)) < 0) return FEE_HF;
  recId = HF_FindNextRec(HF_scan_desc, (char*) &attr_temp);
  HF_VALIDRECID(attrcatFd, recId);

  if (attr.indexed) {
    if ( (AMfd = AM_OpenIndex(filename, attr_temp.attrno)) < 0) return FEE_AM;
    cur_value = (char*) malloc( sizeof(char) * attr_temp.attrlen);
  }

  /* Insert Records from the input file till end */
  cur_record = (char*) malloc( sizeof(char)*rel_temp.relwid);
  while ( fread(cur_record, rel_temp.relwid, 1, file_ptr) == 1 ) {
    /* */
    recId = HF_InsertRec(HFfd, cur_record);
    HF_VALIDRECID(HFfd, recId);

    if (attr_temp.indexed) {
      if (memcpy(cur_value, cur_record + attr_temp.offset, attr_temp.attrlen) == NULL) return FEE_UNIX;
      if (attr_temp.indexed && AM_InsertEntry(AMfd, cur_value, recId) != AME_OK) return FEE_AM;
    }
  }

  /* exit */
  if (HF_CloseFile(HFfd) != HFE_OK) return FEE_HF;
  if (attr_temp.indexed && AM_CloseIndex(AMfd) != AME_OK) return FEE_AM;
  if (attr.indexed) free(cur_value);
  free(cur_record);
  free(file_name);
  fclose(file_ptr);

  return FEE_OK;
}

int  PrintTable(const char *relName);	/* name of relation to print	*/
int  HelpTable(const char *relName);	/* name of relation		*/

/*
 * Prototypes for QU layer functions
 */

int  Select(const char *srcRelName,	/* source relation name         */
		const char *selAttr,	/* name of selected attribute   */
		int op,			/* comparison operator          */
		int valType,		/* type of comparison value     */
		int valLength,		/* length if type = STRING_TYPE */
		char *value,		/* comparison value             */
		int numProjAttrs,	/* number of attrs to print     */
		char *projAttrs[],	/* names of attrs of print      */
		char *resRelName);       /* result relation name         */

int  Join(REL_ATTR *joinAttr1,		/* join attribute #1            */
		int op,			/* comparison operator          */
		REL_ATTR *joinAttr2,	/* join attribute #2            */
		int numProjAttrs,	/* number of attrs to print     */
		REL_ATTR projAttrs[],	/* names of attrs to print      */
		char *resRelName);	/* result relation name         */

int  Insert(const char *relName,	/* target relation name         */
		int numAttrs,		/* number of attribute values   */
		ATTR_VAL values[]);	/* attribute values             */

int  Delete(const char *relName,	/* target relation name         */
		const char *selAttr,	/* name of selection attribute  */
		int op,			/* comparison operator          */
		int valType,		/* type of comparison value     */
		int valLength,		/* length if type = STRING_TYPE */
		char *value);		/* comparison value             */


void FE_PrintError(const char *errmsg);	/* error message		*/
void FE_Init(void){			/* FE initialization		*/
    AM_Init();
};
