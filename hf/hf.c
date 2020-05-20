#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "minirel.h"
#include "bf.h"
#include "pf.h"
#include "hf.h"
#include "def.h"
int fileread = 0;
/* 
 flag --> 0: header to page
          1: page to header
 * */
int CopyHeader(int pffd, HFHeader *hfh, int flag) {
    PFftab_ele *PFftable_cur = &(PFftable[pffd]);
    PFftable->hdrchanged = TRUE;
    if (flag == 0) {
        if (memcpy(PFftable_cur->hdr.buffer, hfh, sizeof(HFHeader)) == NULL) {
                HFerrno = HFE_INTERNAL;
                return HFE_INTERNAL;
            }
    }
    else if (flag == 1) {
        if (memcpy(hfh, PFftable_cur->hdr.buffer, sizeof(HFHeader)) == NULL) {
                HFerrno = HFE_INTERNAL;
                return HFE_INTERNAL;
            }
    }
    return HFE_OK;
}

void 	HF_Init(void){
    int i;
    PF_Init();
    HFftable = (HFftab_ele*) malloc(HF_FTAB_SIZE * sizeof(HFftab_ele));
    for (i = 0; i < HF_FTAB_SIZE; i++) {
        HFftable[i].valid = FALSE;
        HFftable[i].hfh.RecSize = 0;
        HFftable[i].hfh.RecPage = 0;
        HFftable[i].hfh.NumPg = 0;  
        HFftable[i].hfh.NumFrPgFile = 0; 
    }
    HFstable = (HFstab_ele*) malloc(MAXSCANS * sizeof(HFstab_ele));
    for (i = 0; i < MAXSCANS; i++){
        HFstable[i].valid = FALSE;
    }
}

int 	HF_CreateFile(const char *fileName, int RecSize){
    HFHeader hfh;
    int pffd;
    if (RecSize <= 0 || (RecSize >= (PF_PAGE_SIZE - sizeof(int)))) {
        HFerrno = HFE_RECSIZE;
        return HFE_RECSIZE;
    }

    if (PF_CreateFile(fileName) != PFE_OK) {
        HFerrno = HFE_PF;
        return HFE_PF;
    }

    if ((pffd = PF_OpenFile(fileName) < 0)) {
        HFerrno = HFE_PF;
        return HFE_PF;
    }

    hfh.RecSize = RecSize;
    hfh.RecPage = (double) (PAGE_SIZE) / ((double)(RecSize)+0.125);
    hfh.NumPg = 0;
    if (CopyHeader(pffd, &hfh, 0) != HFE_OK) {
        PF_CloseFile(pffd);
        HFerrno = HFE_PF;
        return HFE_PF;
    }

    if (PF_CloseFile(pffd) != PFE_OK) {
        HFerrno = HFE_PF;
        return HFE_PF;
    }

    fileread = 1;
    return HFE_OK;
}

int 	HF_DestroyFile(const char *fileName) {
    if (PF_DestroyFile(fileName) != PFE_OK) {
        HFerrno = HFE_PF;
        return HFE_PF;
    }
    return HFE_OK;
}

int 	HF_OpenFile(const char *fileName) {
    HFftab_ele *HFftable_cur; 
    int pffd, hffd;
    pffd = PF_OpenFile(fileName);
    if(pffd < 0) {
        HFerrno = HFE_PF;
        return HFE_PF;
    }

    for (hffd = 0; hffd < HF_FTAB_SIZE; hffd++) {
        HFftable_cur = &HFftable[hffd];
        if(HFftable_cur->valid == FALSE) {
            if(CopyHeader(pffd, &(HFftable_cur->hfh), 1) < 0 || HFftable_cur->hfh.RecSize <= 0 || HFftable_cur->hfh.RecPage <= 0 ) { /* CCC*/
                PF_CloseFile(pffd);
                HFerrno = HFE_PF;
                return HFE_PF;
            }
            HFftable_cur->valid = TRUE;
            HFftable_cur->pffd = pffd;
            return hffd;
        }
    }

    PF_CloseFile(pffd);
    HFerrno = HFE_FTABFULL;
    return HFE_FTABFULL;
}
int	HF_CloseFile(int HFfd) {
    int i;
    char *pagebuf;
    int error;
    HFftab_ele *HFftable_cur;
    HFstab_ele *HFstable_cur; 
    
    HFftable_cur = &(HFftable[HFfd]);
    if (HFfd < 0 || HFfd >= HF_FTAB_SIZE) {
        HFerrno = HFE_FD;
        return HFE_FD;
    }
    if(CopyHeader(HFftable_cur->pffd, &(HFftable_cur->hfh), 0) != HFE_OK) {
        return HFE_PF;
    }


    if (HFftable_cur->valid != TRUE) {
        HFerrno = HFE_FILENOTOPEN;
        return HFE_FILENOTOPEN;
    }

    HFftable_cur->valid = FALSE;
    HFftable_cur->hfh.RecSize = 0;
    HFftable_cur->hfh.RecPage = 0;
    HFftable_cur->hfh.NumPg = 0;
/*
    CopyHeader(HFftable_cur->pffd, &(HFftable_cur->hfh), 0);
 */ 
    error = PF_CloseFile(HFftable_cur->pffd);
    if (error != PFE_OK) {
        HFerrno = HFE_PF;
        return HFE_PF;
    }
    for (i = 0; i < HF_FTAB_SIZE; i++) {
        HFstable_cur = &(HFstable[i]);
        if(HFstable_cur->valid == TRUE && HFstable_cur->hffd == HFfd){
            HFerrno = HFE_SCANOPEN;
            return HFE_SCANOPEN;
        }
    }
    return HFE_OK;
}

RECID	HF_InsertRec(int HFfd, char *record) {
    HFftab_ele *HFftable_cur;
    int recsize;
    int pagenum, recnum;
    char *pagebuf;
    int nbyte, nbit;
    int error;
    char directory;
    RECID recid;

    HFftable_cur = &(HFftable[HFfd]);
    recsize = HFftable_cur->hfh.RecSize;

    pagenum = -1;
    recid.pagenum = pagenum;
    recid.recnum = HFE_PF;
    while (1) {
        error = PF_GetNextPage(HFftable_cur->pffd, &pagenum, &pagebuf);
/*        printf("error: %d\n", error);*/
        if (error == PFE_EOF){
           error = PF_AllocPage(HFftable_cur->pffd, &pagenum, &pagebuf);
           if (error != PFE_OK || PF_UnpinPage(HFftable_cur->pffd, pagenum, 1) != PFE_OK) {
                return recid;
           }
           HFftable_cur->hfh.NumPg++;
           pagenum--;
           continue;
        }
        else if (error != PFE_OK) {
            return recid;
        }
        for (recnum = 0; recnum < HFftable_cur->hfh.RecPage; recnum++) {
            nbyte = recnum / 8;
            nbit = recnum % 8;
            directory = pagebuf[recsize * HFftable_cur->hfh.RecPage + nbyte];


/*            printf("directory : %x, nbyte: %d, nbit: %d\n", directory & 0xff, nbyte, nbit);*/
            if (((directory >> nbit) & 0x01) == 0) {
/*                printf("insert: pagenum: %d, recnum: %d, record: %s\n", pagenum, recnum, record);*/
                if (memcpy(pagebuf + recsize * recnum, record, recsize) == NULL) {
                    PF_UnpinPage(HFftable_cur->pffd, pagenum, 0);
                    return recid;
                }
/*                printf("insert: %x-->%x, recnum: %d\n", directory & 0xff, directory | (0x01 << nbit), recnum); */
                pagebuf[recsize * HFftable_cur->hfh.RecPage + nbyte] = directory | (0x01 << nbit);
                error = PF_UnpinPage(HFftable_cur->pffd, pagenum, 1);
/*                printf("error: %d\n", error);*/
                if (error == PFE_OK) {
                    recid.pagenum = pagenum;
                    recid.recnum = recnum;
                }
                return recid;
            }
            
        }
        if (PF_UnpinPage(HFftable_cur->pffd, pagenum, 0) != PFE_OK) {
            return recid;
        }
    }

}

int 	HF_DeleteRec(int HFfd, RECID recId) {
    HFftab_ele *HFftable_cur;
    int recsize;
    char *pagebuf;
    int nbyte, nbit;
    char directory;
            
    HFftable_cur = &(HFftable[HFfd]);
    recsize = HFftable_cur->hfh.RecSize;
    if (PF_GetThisPage(HFftable_cur->pffd, recId.pagenum, &pagebuf) != PFE_OK) {
        HFerrno = HFE_PF;
        return HFE_PF;
    }

    nbyte = recId.recnum / 8;
    nbit = recId.recnum % 8;

    directory = pagebuf[recsize * HFftable_cur->hfh.RecPage + nbyte];
    
    pagebuf[recsize * HFftable_cur->hfh.RecPage + nbyte] = directory & (0xFF - (0x01 << nbit));

/*    printf("delete:: recnum: %d, directory: %x--> %x, nbyte: %d, nbit: %d\n", recId.recnum, directory & 0xff, directory & (0xFF - (0x01 << nbit)), nbyte, nbit);*/
    if (PF_UnpinPage(HFftable_cur->pffd, recId.pagenum, 1) != PFE_OK){
        HFerrno = HFE_PF;
        return HFE_PF;
    }
    return HFE_OK;
}

RECID 	HF_GetFirstRec(int HFfd, char *record) {
    RECID recid;
    recid.pagenum = 0;
    recid.recnum = 0;

    first = 1;
    return HF_GetNextRec(HFfd, recid, record);
}

RECID	HF_GetNextRec(int HFfd, RECID recId, char *record) {
    HFftab_ele *HFftable_cur;
    int recsize;
    int pagenum, recnum;
    char *pagebuf;
    int nbyte, nbit;
    char directory;
    int error;
    RECID recid;
    if (record == NULL) {
        recid.recnum = HFE_INVALIDRECORD;
        recid.pagenum = -1;
        printf("nextrec 111\n");
        return recid;
    }
    
    if (HFfd >= HF_FTAB_SIZE || HFfd < 0) {
        recid.recnum = HFE_FD;
        recid.pagenum = -1;
        printf("nextrec 222\n");
        return recid;
    }

    if (HF_ValidRecId(HFfd, recId) != TRUE){
        recid.recnum = HFE_INVALIDRECORD;
        recid.pagenum = -1;
        printf("nextrec 3333333\n");
        return recid;
    }
    recid.pagenum = -1;
    recid.recnum = HFE_PF;
   
    pagenum = recId.pagenum - 1;
    HFftable_cur = &(HFftable[HFfd]);
    recsize = HFftable_cur->hfh.RecSize;
/*    printf("recnum local: %d, input: %d\n", recnum, recId.recnum);*/
    while(1) {
        error = PF_GetNextPage(HFftable_cur->pffd, &pagenum, &pagebuf);
/*        printf("error: %d, pffd: %d\n", error, HFftable_cur->pffd);*/
        if(error == PFE_EOF) {
            recid.recnum = HFE_EOF;
            recid.pagenum = -1;
            return recid;
        }
        else if (error != PFE_OK) {
            return recid;
        }

        /* reset recnum when the pagenum increase*/
        if (recId.pagenum == pagenum) {
            recnum = recId.recnum + 1;
        }
        else {
            recnum = 0;
        }
        if (first == 1) {
            first = 0;
            recnum = 0;
        }

        for (; recnum < HFftable_cur->hfh.RecPage; recnum++) {
            nbyte = recnum / 8;
            nbit = recnum % 8;
            directory = pagebuf[recsize * HFftable->hfh.RecPage + nbyte];
/*            printf("recnum: %d, directory: %x, nbyte: %d, nbit: %d\n", recId.recnum, directory & 0xff, nbyte, nbit);*/
            if (((directory >> nbit) & 0x01) == 1) {
                if (memcpy(record, pagebuf + recsize * recnum, recsize) == NULL) {
                    PF_UnpinPage(HFftable_cur->pffd, pagenum, 0);
                    return recid;
                }

                if (PF_UnpinPage(HFftable_cur->pffd, pagenum, 0) == PFE_OK) {
                    recid.pagenum = pagenum;
                    recid.recnum = recnum;
                }
                return recid;
            }
        }

        if (PF_UnpinPage(HFftable_cur->pffd, pagenum, 0) != PFE_OK) {
            return recid;
        }
    }
}

int	HF_GetThisRec(int HFfd, RECID recId, char *record) {
    HFftab_ele *HFftable_cur;
    int recsize;
    char *pagebuf;
    int nbyte, nbit;
    char directory;

    if (record == NULL) {
        HFerrno = HFE_INVALIDRECORD;
        return HFE_INVALIDRECORD;
    }

    if (HFfd >= HF_FTAB_SIZE || HFfd < 0) {
        HFerrno = HFE_FD;
        return HFE_FD;
    }

    if (HF_ValidRecId(HFfd, recId) != TRUE){
        HFerrno = HFE_INVALIDRECORD;
        return HFE_INVALIDRECORD;
    }
    
    HFftable_cur = &(HFftable[HFfd]);
    recsize = HFftable_cur->hfh.RecSize;

    if (PF_GetThisPage(HFftable_cur->pffd, recId.pagenum, &pagebuf) != PFE_OK) {
        HFerrno = HFE_PF;
        return HFE_PF;
    }

    nbyte = recId.recnum / 8;
    nbit = recId.recnum % 8;

    directory = pagebuf[recsize * HFftable->hfh.RecPage + nbyte];
    if (((directory >> nbit) & 0x01) == 1) {
        if (memcpy(record, pagebuf + recsize * recId.recnum, recsize) == NULL) {
            PF_UnpinPage(HFftable->pffd, recId.pagenum, 0);
            HFerrno = HFE_PF;
            return HFE_PF;
        }
        if (PF_UnpinPage(HFftable->pffd, recId.pagenum, 0) != PFE_OK){
            HFerrno = HFE_PF;
            return HFE_PF;
        }
        return HFE_OK;
    }
    else {
        PF_UnpinPage(HFftable->pffd, recId.pagenum, 0);
        HFerrno = HFE_EOF;
        return HFE_EOF;
    }
}

int 	HF_OpenFileScan(int HFfd, char attrType, int attrLength, 
			int attrOffset, int op, const char *value) {
    int i;

    if (HFfd < 0 || HFfd >= HF_FTAB_SIZE) {
        HFerrno = HFE_FD;
        return HFE_FD;
    }

    if (attrType != INT_TYPE && attrType != REAL_TYPE  && attrType != STRING_TYPE){
        HFerrno = HFE_ATTRTYPE;
        return HFE_ATTRTYPE;
    }
    if (attrLength <= 0) {
        HFerrno = HFE_ATTRLENGTH;
        return HFE_ATTRLENGTH;
    }
    if (attrOffset < 0) {
        HFerrno = HFE_ATTROFFSET;
        return HFE_ATTROFFSET;
    }
    if (op < 1 || op > 6) {
        HFerrno = HFE_OPERATOR;
        return HFE_OPERATOR;
    }

    for (i = 0; i < MAXSCANS; i++){
        if (HFstable[i].valid == FALSE) {
            HFstable[i].valid = TRUE;
            HFstable[i].hffd = HFfd;
            HFstable[i].attrType = attrType;
            HFstable[i].attrLength = attrLength;
            HFstable[i].attrOffset = attrOffset;
            HFstable[i].op = op;
            HFstable[i].value = (char *)value;
            HFstable[i].recid_cur.pagenum = -1;
            HFstable[i].recid_cur.recnum = 0;
            return i;
        }
    }
    HFerrno = HFE_STABFULL;
    return HFE_STABFULL;
}

RECID	HF_FindNextRec(int scanDesc, char *record){
    int op, flag; 
    char *value;
    int attr_int, val_int;
    float attr_float, val_float;
    int tmp;
    int attrOffset, attrLength;

    RECID recid_cur, recid;

    op = HFstable[scanDesc].op;
    value = HFstable[scanDesc].value;
    recid_cur = HFstable[scanDesc].recid_cur;
    flag = 0;

    recid.pagenum = -1;
    recid.recnum = HFE_INTERNAL;
   
    if (recid_cur.pagenum < 0) {
        recid_cur.pagenum = 0;
        first = 1;
    }
    while (!flag){
        recid_cur = HF_GetNextRec(HFstable[scanDesc].hffd, recid_cur, record);
/*        printf("recnum: %d, pagenum: %d, record: %s\n", recid_cur.recnum, recid_cur.pagenum, record); */
        if (recid_cur.pagenum < 0) {
            return recid_cur;
        }
        
        attrOffset = HFstable[scanDesc].attrOffset;
        attrLength = HFstable[scanDesc].attrLength;
/*        printf("offset: %d, length: %d\n", attrOffset, attrLength);*/
        if (value == NULL) flag = 1;
        else if (HFstable[scanDesc].attrType == INT_TYPE) {
            if (memcpy(&attr_int, record + attrOffset, attrLength) == NULL || memcpy(&val_int, value, attrLength)) {
                return recid;
            }

            if (op == EQ_OP) flag = (attr_int == val_int);
            else if (op == LT_OP) flag = (attr_int < val_int);
            else if (op == GT_OP) flag = (attr_int > val_int);
            else if (op == LE_OP) flag = (attr_int <= val_int);
            else if (op == GE_OP) flag = (attr_int >= val_int);
            else if (op == NE_OP) flag = (attr_int != val_int);
            else return recid;
        }
        else if (HFstable[scanDesc].attrType == REAL_TYPE) {
            if (memcpy(&attr_float, record + attrOffset, attrLength) == NULL || memcpy(&val_float, value, attrLength) == NULL) {
/*                printf("%p, %p\n", memcpy(&attr_float, record + attrOffset, attrLength), memcpy(&val_float, value, attrLength));
                printf("3\n");*/
                return recid;
            }

            if (op == EQ_OP) flag = (attr_float == val_float);
            else if (op == LT_OP) flag = (attr_float < val_float);
            else if (op == GT_OP) flag = (attr_float > val_float);
            else if (op == LE_OP) flag = (attr_float <= val_float);
            else if (op == GE_OP) flag = (attr_float >= val_float);
            else if (op == NE_OP) flag = (attr_float != val_float);
            else return recid;
        }
        else if (HFstable[scanDesc].attrType == REAL_TYPE) {
            tmp = strncmp(record + attrOffset, value, attrLength);

            if (op == EQ_OP) flag = (tmp == 0);
            else if (op == LT_OP) flag = (tmp < 0);
            else if (op == GT_OP) flag = (tmp > 0);
            else if (op == LE_OP) flag = (tmp <= 0);
            else if (op == GE_OP) flag = (tmp >= 0);
            else if (op == NE_OP) flag = (tmp != 0);

        }
        else return recid;
    }

    HFstable[scanDesc].recid_cur = recid_cur;
    return recid_cur;

}

int	HF_CloseFileScan(int scanDesc) {
    HFstab_ele *HFstable_cur;

    if(scanDesc < 0) {
        HFerrno = HFE_SD;
        return HFE_SD;
    }
    HFstable_cur = &(HFstable[scanDesc]);
    if (HFstable_cur->valid == FALSE){
        HFerrno = HFE_SCANNOTOPEN;
        return HFE_SCANNOTOPEN;
    }
    HFstable_cur->valid = FALSE;
    return HFE_OK;
}
void	HF_PrintError(const char *errString) {
	switch(HFerrno){
        case HFE_OK: return;break;
		case HFE_PF: printf("HF: HFE_PF\n");break;
		case HFE_FTABFULL: printf("HF: HFE_FTABFULL\n");break;
		case HFE_STABFULL: printf("HF: HFE_STABFULL\n");break;
		case HFE_FD:printf("HF: HFE_FD\n");break;
		case HFE_SD:printf("HF: HFE_SD\n");break;
        case HFE_INVALIDRECORD:printf("HF: HFE_INVALIDRECORD\n");break;
        case HFE_EOF:printf("HF: HFE_EOF\n");break;
        case HFE_RECSIZE:printf("HF: HFE_RECSIZE\n");break;
        case HFE_FILENOTOPEN:printf("HF: HFE_FILENOTOPEN\n");break;
        case HFE_SCANNOTOPEN:printf("HF: HFE_SCANNOTOPEN\n");break;
		case HFE_ATTRTYPE: printf("HF: HFE_ATTRTYPE)\n");break;
		case HFE_ATTRLENGTH: printf("HF: HFE_ATTRLENGTH\n "); break;
		case HFE_ATTROFFSET: printf("HF: HFE_ATTROFFSET\n "); break;
		case HFE_OPERATOR: printf("HF: HFE_OPERATOR\n "); break;
		case HFE_FILE: printf("HF: HFE_FILE\n "); break;
		case HFE_INTERNAL: printf("HF: HFE_INTERNAL\n "); break;
		case HFE_PAGE: printf("HF: HFE_PAGE\n "); break;
		case HFE_INVALIDSTATS: printf("HF: HFE_INVALIDSTATS\n "); break;

		default: printf("HF: unknown error code: %d \n", HFerrno);
	}
	printf("HF Error : %s", errString);
	exit(1);
}

bool_t         HF_ValidRecId(int HFfd, RECID recid){
    int numpg, recpg;

    numpg = HFftable[HFfd].hfh.NumPg;
    recpg = HFftable[HFfd].hfh.RecPage;
    if (recid.pagenum < 0 || recid.recnum < 0 || recid.pagenum >= numpg || recid.recnum >= recpg) {
/*        printf("valid, pagenum: %d, recnum: %d, numpg: %d, recpage: %d, \n", recid.pagenum, recid.recnum, numpg, recpg);*/
        return FALSE;
    }
    return TRUE;}
