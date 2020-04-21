#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "minirel.h"
#include "bf.h"
#include "pf.h"

typedef struct PFftab_ele {
    bool_t    valid;       /* set to TRUE when a file is open. */
    ino_t     inode;       /* inode number of the file         */
    char      *fname;      /* file name                        */
    int       unixfd;      /* Unix file descriptor             */
    PFhdr_str hdr;         /* file header                      */
    short     hdrchanged;  /* TRUE if file header has changed  */
} PFftab_ele;

typedef struct PFhdr_str {
    int    numpages;      /* number of pages in the file */
} PFhdr_str;

typedef struct PFpage {
    char pagebuf[PAGE_SIZE];
} PFpage;

PFftab_ele *PFftab; 
size_t PFftab_length;


int  PF_AllocPage	(int fd, int *pagenum, char **pagebuf){

    PFpage* pfpage;
    BFreq bq;
    PFftab_ele *current_pfftab;
    int err; 

    if (fd < 0 || fd >= PFtab_length) 
        return PFE_FD;
    if (pagenum == NULL || pagebuf == NULL)
        return PFE_INVALIDPAGE;

    bq.fd = fd;
    current_pfftab = PFftab + fd;
    bq.pagenum = current_pfftab->hdr.numpages;
    bq.unixfd = current_pfftab->unixfd;
    bq.dirty = TRUE;
    
    err = BF_AllocBuf(bq, &pfpage);
    if (err != BFE_OK)
        BF_ErrorHandler(err);

    err = BF_TouchBuf(bq);
    if (err != BFE_OK)
        BF_ErrorHandler(err);

    *pagenum = bq.pagenum;
    *(pagebuf) = pfpage->pagebuf;
    current_pfftab->hdrchanged = TRUE;
    current_pfftab->hdr.numpages++;

    return PFE_OK;
}
int  PF_GetFirstPage (int fd, int *pagenum, char **pagebuf){
    *pageNum = -1;

    return PF_GetNextPage(fd, pagenum, pagebuf);
}
int  PF_GetNextPage	(int fd, int *pagenum, char **pagebuf){

    PFpage* pfpage;
    BFreq bq;
    PFftab_ele *current_pfftab;
    int err;

    current_pfftab = PFftab + fd;
    bq.fd = fd;
    bq.unixfd = current_pfftab->unixfd;
    bq.dirty = FALSE;
    bq.pagenum = (*pagenum) + 1;

    if (current_pfftab->hdr.numpages <= bq.pagenum)
        return PFE_EOF;

    err = BF_GetBuf(bq, &fpage);
    if (err < BFE_OK)
        BF_ErrorHandler(err);

    *pagebuf = fpage->pagebuf;
    *pagenum++;

    return PFE_OK;
}
int  PF_GetThisPage	(int fd, int pagenum, char **pagebuf){
    
    PFpage* pfpage;
    BFreq bq;
    PFftab_ele *current_pfftab;
    int err;

    current_pfftab = PFftab + fd;
    bq.fd = fd;
    bq.unixfd = current_pfftab->unixfd;
    bq.dirty = FALSE;
    bq.pagenum = (*pagenum);

    if (current_pfftab->hdr.numpages <= bq.pagenum)
        return PFE_EOF;

    err = BF_GetBuf(bq, &fpage);
    if (err < BFE_OK)
        BF_ErrorHandler(err);

    *pagebuf = fpage->pagebuf;

    return PFE_OK;
}
int  PF_DirtyPage	(int fd, int pagenum){
    
    BFreq bq;
    PFftab_ele *current_pfftab;
    int err;

    if (fd >= PFftab_length || fd < 0)
        return PFE_FD;

    current_pfftab = PFftab + fd;

    if (pagenum >= current_pfftab->hdr.numpages || pagenum < 0)
        return PFE_INVALIDPAGE;

    bq.fd = fd;
    bq.unixfd = current_pfftab->unixfd;
    bq.dirty = TRUE;
    bq.pagenum = pagenum;

    err = BF_TouchBuf(bq);
    if (err != BFE_OK)
        BF_ErrorHandler(err);

    return PFE_OK;
}
int  PF_UnpinPage	(int fd, int pagenum, int dirty){

    BFreq bq;
    PFftab_ele *current_pfftab;
    int err;

    if (fd >= PFftab_length || fd < 0)
        return PFE_FD;

    current_pfftab = PFftab + fd;

    if (pagenum >= current_pfftab->hdr.numpages || pagenum < 0)
        return PFE_INVALIDPAGE;

    bq.fd = fd;
    bq.unixfd = current_pfftab->unixfd;
    bq.dirty = TRUE;
    bq.pagenum = pagenum;
    
    if (dirty){
        err = BF_TouchBuf(bq);
        if (err != BFE_OK)
            BF_ErrorHandler(err);
    }
    
    err = BF_UnpinBuf(bq);
    if (err != BFE_OK)
        BF_ErrorHandler(err);

    return PFE_OK;
}

