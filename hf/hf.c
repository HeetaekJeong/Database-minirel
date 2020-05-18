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

/* HF file table element. */
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
    RECID current;
} HFstab_ele;

HFftab_ele HFftable;
HFstab_ele HFstable;


void 	HF_Init(void){
    int i;
    PF_Init();
    HFftable = (HFftab_ele*) malloc(HF_FTAB_SIZE * sizeof(HFftab_ele));

    for (i = 0; i < HF_FTAB_SIZE; i++) {
        HFftable[i].valid = FALSE;
        HFftable[i].hfh.RecSize = 0;
        HFftable[i].hfh.RecPage = 0;
        HFftable[i].hfh.NumPg = 0;  
        HFftable[i].hfh.NumFrPgFile = 0; // CCC
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

    if (PF_CreateFile(fileName) != PFE_OK) {
        return HFE_PF;
    }

    if ((pffd = PF_OpenFile(fileName) < 0)) {
        return HFE_PF;
    }

    hfh.RecSize = recSize;
    hfh.RecPage = (double) (PAGE_SIZE) / ((double)(recSize)+0.125);
    hfh.NumPg = 0;

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
            if( HFftable_cur->hfheader.RecSize <= 0 || HFftable_cur->hfheader.RecPage <= 0) { // CCC
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
    int pffd;
    
    if (PF_CloseFile(pffd) != PFE_OK) {
        return HFE_PF;
    }

    HFftable[HFfd].valid = FALSE;
    HFftable[HFfd].hfheader.RecSize = 0;
    HFftable[HFfd].hfheader.RecPage = 0;
    HFftable[HFfd].hfheader.NumPg = 0;

    return HFE_OK;
}
RECID	HF_InsertRec(int fileDesc, char *record);
int 	HF_DeleteRec(int fileDesc, RECID recId);
RECID 	HF_GetFirstRec(int fileDesc, char *record);
RECID	HF_GetNextRec(int fileDesc, RECID recId, char *record);
int	HF_GetThisRec(int fileDesc, RECID recId, char *record);
int 	HF_OpenFileScan(int fileDesc, char attrType, int attrLength, 
			int attrOffset, int op, const char *value);
RECID	HF_FindNextRec(int scanDesc, char *record);
int	HF_CloseFileScan(int scanDesc);
void	HF_PrintError(const char *errString);
bool_t         HF_ValidRecId(int fileDesc, RECID recid);
