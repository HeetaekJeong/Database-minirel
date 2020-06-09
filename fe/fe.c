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

int  CreateTable(const char *relName,	/* name	of relation to create	*/
		int numAttrs,		/* number of attributes		*/
		ATTR_DESCR attrs[],	/* attribute descriptors	*/
		const char *primAttrName) { /* primary index attribute	*/

    
};

int  DestroyTable(const char *relName);	/* name of relation to destroy	*/

int  BuildIndex(const char *relName,	/* relation name		*/
		const char *attrName);	/* name of attr to be indexed	*/

int  DropIndex(const char *relname,	/* relation name		*/
		const char *attrName);	/* name of indexed attribute	*/

int  PrintTable(const char *relName);	/* name of relation to print	*/

int  LoadTable(const char *relName,	/* name of target relation	*/
		const char *fileName);	/* file containing tuples	*/

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
