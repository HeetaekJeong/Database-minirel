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
typedef struct  HFftab_ele {
    bool_t valid;
    HFHeader hfh;
    int pffd;
} HFftab_ele;

/* HF scan table element. */
typedef struct  HFstab_ele {
    bool_t valid;
    int hffd;
    char attrType;
    int attrLength;
    int attrOffset;
    int op;
    char *value;
    RECID recid_cur;
} HFstab_ele;

int first;
HFftab_ele *HFftable;
HFstab_ele *HFstable;

int CopyHeader(int pffd, HFHeader *hfh) {
    PFftab_ele *PFftable_cur = &(PFftable[pffd]);
    PFftable->hdrchanged = TRUE;
    if (memcpy(PFftable_cur->hdr.buffer, sizeof(HFHeader)) == NULL) {
        return HFE_INTERNAL;
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
    PFftab_ele *PFftable;
    HFHeader hfh;
    int pffd;

    if (RecSize <= 0 || (RecSize >= (PF_PAGE_SIZE - sizeof(int)))) {
        return HFE_RECSIZE;
    }

    if (PF_CreateFile(fileName) != PFE_OK) {
        return HFE_PF;
    }

    if ((pffd = PF_OpenFile(fileName) < 0)) {
        return HFE_PF;
    }

    if (HFftab_num >= HF_FTAB_SIZE) {
        return HFE_FTABFULL;
    }

    hfh.RecSize = recSize;
    hfh.RecPage = (double) (PAGE_SIZE) / ((double)(recSize)+0.125);
    hfh.NumPg = 0;

    if (CopyHeader(pffd, &hfh) != HFE_OK) {
        PF_CloseFile(pffd);
        return PFE_PF;
    }

    if (PF_CloseFile(pffd) != PFE_OK) {
        return HFE_PF;
    }

    return HFE_OK;
}

int 	HF_DestroyFile(const char *fileName) {
    if (PF_DestroyFile(filename) != PFE_OK) {
        return HFE_PF;
    }
    return HFE_OK;
}

int 	HF_OpenFile(const char *fileName) {
    HFftab_ele *HFftable_cur; 
    int pffd, hffd;
    
    pffd = PF_OpenFile(fileName);
    if(pffd < 0) {
        return HFE_PF;
    }

    for (hffd = 0; hffd < HF_FTAB_SIZE; hffd++) {
        HFftable_cur = &HFftable[hffd];
        if(HFftable_cur->valid == FALSE) {
            if(HFftable_cur->hfh.RecSize <= 0 || HFftable_cur->hfh.RecPage <= 0 || Copyheader(pffd, &(HFftable_cur->hfh))) { // CCC
                PF_CloseFile(pffd);
                return HFE_PF;
            }
            HFftable_cur->valid = TRUE;
            HFftable_cur->pffd = pffd;

            return hffd;
        }
    }

    PF_CloseFile(pffd);
    return HFE_FTABFULL;
}
int	HF_CloseFile(int HFfd) {
    int pffd, i;
    char *pagebuf;
    HFftab_ele *HFftable_cur;
    HFstab_ele *HFstable_cur; 

    if (HFfd < 0 || HFfd >= HF_FTAB_SIZE) {
        return HFE_FD;
    }

    HFftable_cur = HFftable[HFfd];

    if (HFftable_cur->valid != TRUE) {
        return HFE_FILENOTOPEN;
    }

    HFftable_cur->valid = FALSE;
    HFftable_cur->hfh.RecSize = 0;
    HFftable_cur->hfh.RecPage = 0;
    HFftable_cur->hfh.NumPg = 0;

    CopyHeader(HFftable_cur->pffd, &(HFftable_cur->hfh));
    
    if (PF_CloseFile(pffd) != PFE_OK) {
        return HFE_PF;
    }

    for (i = 0; i < HF_FTAB_SIZE; i++) {
        HFstable_cur = HFstable[i];
        if(HFstable_cur->valid == TRUE || HFstable_cur->hffd == HFfd){
            return HFE_SCANOPEN;
        }
    }
    return HFE_OK;
}

RECID	HF_InsertRec(int HFfd, char *record) {
    HFFtab_ele *HFftable_cur;
    int recsize;
    int pagenum, recnum;
    char *pagebuf;
    int nbyte, nbit;
    int error;
    char directory;
    RECID recid;

    HFftable_cur = HFftable[HFfd];
    recsize = HFftable_cur->hfh.recsize;

    pagenum = -1;
    recid.pagenum = pagenum;
    recid.recnum = HFE_PF;

    while (1) {
        if ( == PFE_EOF){
           error = PF_AllocPage(HFftable_cur->pffd, &pagenum, &pagebuf);
           if (error != PFE_OK || PF_UnpinPage(HFftable_cur->pffd, pagenum, 1) != PFE_OK) {
                return recid;
           }
           HFftable_cur->hfh.NumPg++;
           pagenum--;
           continue;
        }
        else if (error ~= PFE_OK) {
            return recid;
        }

        for (recnum = 0; recnum < HFftable_cur->hfh.RecPage; recnum++) {
            nbyte = recnum / 8;
            nbit = recnum % 8;

            directory = pagebuf[recsize * HFftable_cur->hfh.RecPage +nbyte];

            if (((directoy >> bit) & 0x01) == 0) {
                if (memcpy(pagebuf + recsize * recnum, record, recsize) == NULL) {
                    PF_UnpinPage(HFftable_cur->pffd, pagenum, 0);
                    return recid;
                }

                pagebuf[recsize * HFftable_cur->hfh.RecPage +nbyte] = directory | (0x01 << bit);

                if (PF_UnpinPage(HFftable_cur->pffd, pagenum, 1) == PFE_OK) {
                    recid.pagenum = pagenum;
                    recid.recnum = recnum;
                }
                return recid;
            }

            if (PF_UnpinPage(HFftable_cur->pffd, pagenum, 0) != PFE_OK) {
                return recid;
            }
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

    if (PF_GetThisPage(HFftable_cur->pffd, recId.pagenum, &pagebuf) != PFE_OK) {
        return HFE_PF;
    }

    nbyte = recId.recnum / 8;
    nbit = recId.recnum % 8;

    directory = pagebuf[recsize * HFftable_cur->hfh.RecPage +nbyte];
    pagebuf[recsize * HFftable_cur->hfh.RecPage +nbyte] = directory & (0xFF - (0x01 << bit));

    if (PF_UnpinPage(HFftable_cur->pffd, recId.pagenum, 1) != PFE_OK){
        return HFE_PF;
    }
    return HFE_OK;
}

RECID 	HF_GetFirstRec(int HFfd, char *record) {
    RECID recid;
    int error;
    recid.pagenum = 0;
    recid.recnum = 0;

    first = 1;
    err = HF_GetNextRec(HFfd, recid, record);
    if(err != HFE_OK) {
        recid.recnum = error;
        recid.pagenum = error;
    }
    return recid;
}

RECID	HF_GetNextRec(int HFfd, RECID recId, char *record) {
    HFftab_ele *HFftable_cur;
    int recsize;
    int pagenum, recnum;
    char *pagebuf;
    int nbyte, nbit;
    char directory;
    RECID recid;

    if (record == NULL) {
        return HFE_WRONGPARAMETER;
    }
    
    if (HFfd >= HF_FTAB_SIZE || HFfd < 0) {
        return HFE_FD;
    }

    if (HF_ValidRecId(HFfd, recId != TRUE)){
        recid.recnum = HFE_INVALIDRECORD;
        recid.pagenum = HFE_INVALIDRECORD;
        return recId;
    }
    recid.pagenum = -1;
    recid.recnum = HFE_PF;
   
    pagenum = recId.pagenum;
    recnum = recId.recnum + 1;
    HFftable_cur = &(HFftable[HFfd]);
    recsize = HFftable_cur->hfh.recsize;

    while(1) {
        err = PF_GetThisPage(HFftable_cur->pffd, pagenum, &pagebuf);

        if(err == PFE_EOF) {
            recid.recnum = HFE_EOF;
            recid.pagenum = HFE_EOF;
            return recid;
        }
        else if (err != PFE_OK) {
            return recid;
        }

        for(; recnum < HFftable_cur.RecPage; recnum++) {
            nbyte = recnum / 8;
            bit = recnum % 8;

            directory = pagebuf[recsize * HFftable->hfh.RecPage + nbyte];

            if (((directory >> bit) & 0x01) == 1) {
                if (memcpy(recore, pagebuf + recsize * recnum, recSize) == NULL) {
                    PF_UnpinPage(HFftable_cur->pffd, pagenum, 0);
                    return recid;
                }

                if(PF_UnpinPage(HFftable_cur->pffd, pagenum, 0) == PFE_OK) {
                    recid.pagenum = pagenum;
                    recid.recnum = recnum;
                }
                return recid;
            }
        }

        if(PF_UnpinPage(HFftable_cur->pffd, pagenum, 0) != PFE_OK) {
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
        return HFE_WRONGPARAMETER;
    }

    if (HFfd >= HF_FTAB_SIZE || HFfd < 0) {
        return HFE_FD;
    }

    if (HF_ValidRecId(HFfd, recId != TRUE)){
        return HFE_INVALIDRECORD;
    }
    
    HFftable_cur = &(HFftable[HFfd]);
    recsize = HFftable_cur->hfh.recsize;

    if (PF_GetThisPage(HFftable_cur->pffd, recId.pagenum, &pagebuf) != PFE_OK) {
        return HFE_PF;
    }

    nbyte = recId.recnum / 8;
    nbit = recId.recnum % 8;

    directory = pagebuf[recsize * HFftable->hfh.RecPage + nbyte];
    if (((directory >> bit) & 0x01) == 1) {
        if (memcpy(record, pagebuf + recsize * recId.recnum, recsize) == NULL) {
            PF_UnpinPage(HFftable->pffd, recId.pagenum, 0);
            return HFE_PF;
        }
        if (PF_UnpinPage(HFftable->pffd, recId.pagenum, 0) != PFE_OK){
            return HFE_PF;
        }
        return HFE_OK;
    }
    else {
        PF_UnpinPage(HFftable->pffd, recId.pagenum, 0);
        return HFE_EOF;
    }
}

int 	HF_OpenFileScan(int HFfd, char attrType, int attrLength, 
			int attrOffset, int op, const char *value) {
    int i;

    if (HFfd < 0 || HFfd >= HFFTAB_SIZE) {
        return HFE_FD;
    }

    if (attrType != INT_TYPE && attrType != REAL_TYPE  && attrType != STRING_TYPE){
        return HFE_ATTRTYPE;
    }
    if (attrLength <= 0) {
        return HFE_ATTRLENGTH;
    }
    if (attrOffset < 0) {
        return HFE_ATTROFFSET;
    }
    if (op < 1 || op > 6) {
        return HFE_OPERATOR;
    }

    for (i = 0; i < MAXSCANS; i++){
        if (HFstable[i].valid == FALSE) {
            HFstable[i].valid = TRUE:
            HFstable[i].hffd = HFfd;
            HFstable[i].attrType = attrType;
            HFstable[i].attrLength = attrLength;
            HFstable[i].attrOffset = attrOffset;
            HFstable[i].op = op;
            HFstable[i].value = value;
            HFstable[i].recid_cur.pagenum = -1;
            HFstable[i].recid_cur.recnum = 0;
            return i;
        }
    }
    return HFE_STABFULL;
}

RECID	HF_FindNextRec(int scanDesc, char *record){
    int op, flag; 
    char *value;
    int attr_int, val_int;
    float attr_float, val_float;
    int tmp;
    int attrOffset, attrLenth;

    RECID recid_cur, recid;

    if (record == NULL) {
        recid.recnum = HFE_WRONGPARAMETER;
        recid.pagenum = HFE_WRONGPARAMETER;
        return recid;
    }
    op = HFstable[scanDesc].op;
    value = HFstable[scanDesc].value;
    recid = HFstable[scanDesc].recid_cur;
    flag = 0;

    recid.pagenum = -1;
    recid.recnum = HFE_INTERNAL;
   
    if (recid_cur.pagenum < 0) {
        recid.pagenum = 0;
        first = 1;
    }

    while (!flag){
        recid.cur = HF_GetNextRec(HFstable[scanDesc].hffd, recid, record);
        
        if (recid.pagenum < 0) return recid;
        
        attrOffset = HFstable[scanDesc].attrOffset;
        attrLength = HFstable[scanDesc].attrLength;
        if (value == NULL) flag = 1;
        else if (HFstable[scanDesc].attrType == INT_TYPE) {
            if (memcpy(&attr_int, record + attrOffset, attrLength) == NULL || memcpy(&val_int, value, .attrLength)) {
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
            if (memcpy(&attr_float, record + attrOffset, attrLength) == NULL || memcpy(&val_float, value, .attrLength)) {
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

    HFstable[scanDesc].recid_cur = recid;
    return recid;

}

int	HF_CloseFileScan(int scanDesc) {
    HFstab_ele *HFstable_cur;

    if(scanDesc < 0) {
        return HFE_SD;
    }
    HFstable_cur = &(HFstable[scanDesc]);
    if (HFstable_cur->valid == FALSE){
        return HFE_SCANNOTOPEN;
    }
    HFstable_cur->valid = FALSE;
    return HFE_OK;
}
void	HF_PrintError(const char *errString) {
    printf("HF Error: %s\n", errString);
}

bool_t         HF_ValidRecId(int HFfd, RECID recid){
    int numpg, recpg;

    numpg = HFftable[HFfd].hfh.Numpg;
    recpg = HFftable[HFfd].hfh.RecPage;

    if (recid.pagenum < 0 || recid.recnum < 0 || recid.pagenum >= numpg || recid.recnum >= recpg) {
        return FALSE;
    }
    return TRUE;
}
