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
#include "def.h"

#define FILE_CREATE_MASK (S_IRUSR|S_IWUSR|S_IRGRP)
int Ftable_Check(const char *filename)
{
	int i, found;
	found = 0;

	for(i = 0; i < PF_FTAB_SIZE; i++){
		if(!PFftable[i].valid)
			continue;
		if(PFftable[i].fname == NULL)
			continue;
		
		if(!strcmp(PFftable[i].fname, filename)){
/*            printf("table check, origin: %s, new: %s, i: %d, valid: %d\n", PFftable[i].fname, filename, i, PFftable[i].valid);*/
				found = 1;
				break;
		}
	}

	if(found)
			return 1;

	return 0;
}

void PF_Init(void)
{
    int i;
	/* Invoke the BF_Init() */
	BF_Init();

	/* Create the file table */
	PFftable = malloc(sizeof(PFftab_ele) * PF_FTAB_SIZE);

	/*Initialize the file table */
	for(i = 0; i < PF_FTAB_SIZE; i++){
		PFftable[i].valid = FALSE;
		PFftable[i].inode = 0;
		PFftable[i].fname = NULL;
		PFftable[i].unixfd = -1;
		PFftable[i].hdr.numpages = 0;
		PFftable[i].hdrchanged = FALSE;
	}
}

int PF_CreateFile(const char *filename)
{
	int unixfd;
	int inode;
	struct stat fileStat;
	BFreq bq;
	PFhdr_str header;

	/* File exist check */
	if(access(filename, F_OK) != -1){
        PFerrno = PFE_FD;
        return PFerrno; /* Already exists */
	}

	/* Create the file */
	if((unixfd = open(filename, O_RDWR|O_CREAT, FILE_CREATE_MASK)) < 0){
        PFerrno = PFE_UNIX;
        return PFerrno;
	}

	/* Fine the inode of the file */
	if((inode = fstat(unixfd, &fileStat)) < 0){
        PFerrno = PFE_UNIX;
        return PFerrno;
	}

	/* Initialize the header and write to the file */
	header.numpages = 0;
	if(write(unixfd, &header, sizeof(PFhdr_str)) != sizeof(PFhdr_str)){
        PFerrno = PFE_UNIX;
        return PFerrno;
	}

	/* Close the file */
	if(close(unixfd) < 0){
        PFerrno = PFE_UNIX;
        return PFerrno;
	}

	return PFE_OK;
}

int PF_DestroyFile(const char *filename)
{
    int i;
	/* File exist check */
	if(access(filename, F_OK) == -1){
        PFerrno = PFE_UNIX;
        return PFerrno;
	}

	/* Find the file by filename */
	for(i = 0; i < PF_FTAB_SIZE; i++){
		if(!strcmp(PFftable[i].fname, filename)){ /* Found the file */
			/* File open check */
			if(PFftable[i].valid){
                PFerrno = PFE_FILEOPEN;
                return PFerrno;
			}

			/* Unlink the file */
			unlink(filename);
			break;
		}
	}
    remove(filename);
/*    if (remove(filename) != 0) {
        printf("remove failed\n");
    }*/
	return PFE_OK;
}

int PF_OpenFile(const char *filename)
{
	int unixfd;
	int PFfdsc;
	int error;
	PFhdr_str header;
	struct stat fileStat;
    int i;

/*    printf("PF_Openfile,, filename: %s\n", filename);*/
	/* Check whether the file is already open */
	if(Ftable_Check(filename)){
        printf("fileopen\n");
        PFerrno = PFE_FILEOPEN;
        return PFerrno;
    }

	/* Open the file */
	if((unixfd = open(filename, O_RDWR)) < 0){
        PFerrno = PFE_UNIX;
        return PFerrno;
	}

	PFfdsc = -1;

	/* Iterate the table and find the empty entry */
	for(i = 0; i < PF_FTAB_SIZE; i++){
		if(!PFftable[i].valid){ /* Found the target entry */
			PFfdsc = i; /* Index of the file table entry */
			if(read(unixfd, &header, sizeof(PFhdr_str)) >= 0) /* Read the header */
				break;
            PFerrno = PFE_UNIX;
            return PFerrno;
		}
	}

	/* Check whether the table is full */
	if(PFfdsc == -1){
        PFerrno = PFE_FTABFULL;
        return PFerrno;
    }

	/* Find the inode of the file */
	if((error = fstat(unixfd, &fileStat)) < 0){
        PFerrno = PFE_UNIX;
        return PFerrno;
	}
    /*
    if(PFfdsc == 0 && filenum == 1) {
            PFfdsc = PFfdsc + 1;
    }
    */
	/* Fill the file table entries accordingly */
	PFftable[PFfdsc].valid = TRUE;
	PFftable[PFfdsc].inode = fileStat.st_ino;
	PFftable[PFfdsc].fname = (char*)filename;
	PFftable[PFfdsc].unixfd = unixfd;
	PFftable[PFfdsc].hdr.numpages = header.numpages;
	PFftable[PFfdsc].hdrchanged = FALSE;

	/* Return the PF file descriptor 
    printf("filenum: %d, PFDesc: %d, fname: %s\n", filenum, PFfdsc, PFftable[0].fname);
    */
	return PFfdsc;
}

int PF_CloseFile(int fd)
{
	int unixfd;
	int error;
	PFhdr_str header;
	char **pagebuf;

	/* Invalid PF file descriptor */
	if(fd < 0 || fd > PF_FTAB_SIZE-1){
		printf("\nInvalid PF file descriptor\n");
        PFerrno = PFE_FD;
        return PFerrno;
	}

	/* Check whether the file is open */
	if(!PFftable[fd].valid){
		printf("File is not open\n");
        PFerrno = PFE_FILENOTOPEN;
        return PFerrno;
	}

	/* Find the unix file descriptor in the PF file table */
	unixfd = PFftable[fd].unixfd;

	/* 
	 * Release all the buffer pages belonging to the file to the FREE list 
	 * Dirty pages will be written to the disk
	 */
	if((error = BF_FlushBuf(fd)) != BFE_OK){
		printf("Cannot flush, not all the buffer page is unppined\n");		
        PFerrno = PFE_PAGEFREE;
        return PFerrno;
	}
	
	/* Check whether the header is changed */
	if(PFftable[fd].hdrchanged){
		/* Re-write the header in the file */
		header = PFftable[fd].hdr;
		if((error = pwrite(unixfd, &header, sizeof(PFhdr_str), 0)) < 0){
			printf("Cannot re-write the header\n");
            PFerrno = PFE_HDRWRITE;
            return PFerrno;
		}
	}

	/* Close the file */
	if((error = close(unixfd)) < 0){
		printf("Close error\n");
        PFerrno = PFE_UNIX;
        return PFerrno;
	}

	/* Invalid the corresponding entry */
/*    printf("pfclose fd: %d, valid: %d\n",fd, PFftable[fd].valid);*/
	PFftable[fd].valid = FALSE;
	return PFE_OK;
}

int PF_AllocPage (int fd, int *pagenum, char **pagebuf)
{
    PFpage* pfpage;
    BFreq bq;
    PFftab_ele *current_pfftab;
    int err; 

    /* Invalid PF file descriptor */
	if (fd < 0 || fd > PF_FTAB_SIZE-1) {
        PFerrno = PFE_FD;
        return PFerrno;
    }
	
	/* Invaild page number or page buffer */
	if (pagenum == NULL || pagebuf == NULL) {
        PFerrno = PFE_INVALIDPAGE;
        return PFerrno;
    }

	/* Current PF file descriptor */
	current_pfftab = PFftable + fd;

	/* Wrap the BF layer request */
    bq.fd = fd;
	bq.pagenum = current_pfftab->hdr.numpages;
	bq.unixfd = current_pfftab->unixfd;
    bq.dirty = TRUE;
    
	/* Check whether the file is not open */
	if (current_pfftab->valid == FALSE){
	    PFerrno = PFE_FILENOTOPEN;
        return PFerrno;
    }
    
	/* Append the page to the file */
	err = BF_AllocBuf(bq, &pfpage);
	if (err != BFE_OK){
	    PFerrno = PFE_INVALIDPAGE;
        return PFerrno;
    }
	/* Set the dirty page */
    err = BF_TouchBuf(bq);
    if (err != BFE_OK){
	    PFerrno = PFE_INVALIDPAGE;
        return PFerrno;
    }

	/* Set the return argument */
	*pagenum = bq.pagenum;
	pfpage->pagebuf[0] = 0x00;
	*(pagebuf) = pfpage->pagebuf;
	current_pfftab->hdrchanged = TRUE;
	current_pfftab->hdr.numpages++;

	return PFE_OK;
}

int  PF_GetFirstPage (int fd, int *pagenum, char **pagebuf)
{   
	/* Invalid PF file descriptor */
	if(fd < 0 || fd > PF_FTAB_SIZE-1){
        PFerrno = PFE_FD;
        return PFerrno;
    }	
	*pagenum = -1;
   
    return PF_GetNextPage(fd, pagenum, pagebuf);
}
int  PF_GetNextPage	(int fd, int *pagenum, char **pagebuf)
{
    PFpage* pfpage;
    BFreq bq;
    PFftab_ele *current_pfftab;
    int err;

	/* Invalid PF file descriptor */
	if(fd < 0 || fd > PF_FTAB_SIZE-1){
        PFerrno = PFE_FD;
        return PFerrno;
    }

    current_pfftab = PFftable + fd;

	/* Wrap the BF layer request */
    bq.fd = fd;
    bq.unixfd = current_pfftab->unixfd;
    bq.dirty = FALSE;
    bq.pagenum = (*pagenum) + 1;

	/* Check whether the file is not open */
    if (current_pfftab->valid == FALSE){
        PFerrno = PFE_FILENOTOPEN;
        return PFerrno;
    }

	/* Invalid page number */
    if (current_pfftab->hdr.numpages <= bq.pagenum || *pagenum < -1){
        PFerrno = PFE_EOF;
        return PFerrno;
    }

	/* Get the page buffer with the page number */
    err = BF_GetBuf(bq, &pfpage);
    if (err != BFE_OK){
        PFerrno = PFE_NOUSERS;
        return PFerrno;
    }

	/* Set the return argument */
    *pagebuf = pfpage->pagebuf;
    *pagenum = *pagenum + 1;

    return PFE_OK;
}
int  PF_GetThisPage	(int fd, int pagenum, char **pagebuf)
{    
    PFpage* pfpage;
    BFreq bq;
    PFftab_ele *current_pfftab;
    int err;

	/* Invalid PF file descriptor */
	if(fd < 0 || fd > PF_FTAB_SIZE-1){
		PFerrno = PFE_FD;
        return PFerrno;
    }

    current_pfftab = PFftable + fd;

	/* Wrap the BF layer request */
    bq.fd = fd;
    bq.unixfd = current_pfftab->unixfd;
    bq.dirty = FALSE;
    bq.pagenum = pagenum;

	/* Check whether the file is open */
    if (current_pfftab->valid == FALSE){
        PFerrno = PFE_FILENOTOPEN;
        return PFerrno;
    }

	/* Invalid page number */
    if (current_pfftab->hdr.numpages <= bq.pagenum || pagenum < 0){
        PFerrno = PFE_INVALIDPAGE;
        return PFerrno;
    }

	/* Get the page buffer with the page number */
    err = BF_GetBuf(bq, &pfpage);
    if (err != BFE_OK){
        PFerrno = PFE_NOUSERS;
        return PFerrno;
    }

	/* Set the return argument */
    *pagebuf = pfpage->pagebuf;

    return PFE_OK;
}
int  PF_DirtyPage	(int fd, int pagenum)
{    
    BFreq bq;
    PFftab_ele *current_pfftab;
    int err;

	/* Invalid PF file descriptor */
	if(fd < 0 || fd > PF_FTAB_SIZE-1){
		PFerrno = PFE_FD;
        return PFerrno;
    }

    current_pfftab = PFftable + fd;

	/* Check whether the file is open */
    if (current_pfftab->valid == FALSE){
        PFerrno = PFE_FILENOTOPEN;
        return PFerrno;
    }
	
	/* Invalid page number */
    if (pagenum >= current_pfftab->hdr.numpages || pagenum < 0){
        PFerrno = PFE_INVALIDPAGE;
        return PFerrno;
    }

    bq.fd = fd;
    bq.unixfd = current_pfftab->unixfd;
    bq.dirty = TRUE;
    bq.pagenum = pagenum;

	/* Set the page as dirty */
    err = BF_TouchBuf(bq);
    if (err != BFE_OK){
        PFerrno = PFE_NOUSERS;
        return PFerrno;
    }
    return PFE_OK;
}
int  PF_UnpinPage	(int fd, int pagenum, int dirty)
{
    BFreq bq;
    PFftab_ele *current_pfftab;
    int err;

	/* Invalid PF file descriptor */
	if(fd < 0 || fd > PF_FTAB_SIZE-1){
		PFerrno = PFE_FD;
        return PFerrno;
    }

    current_pfftab = PFftable + fd;
 
	/* Check whether the file is open */
 	if (current_pfftab->valid == FALSE){
        PFerrno = PFE_FILENOTOPEN;
        return PFerrno;
    }

	/* Invalid page number */
    if (pagenum >= current_pfftab->hdr.numpages || pagenum < 0){
        PFerrno = PFE_INVALIDPAGE;
        return PFerrno;
    }

    bq.fd = fd;
    bq.unixfd = current_pfftab->unixfd;
    bq.dirty = dirty;
    bq.pagenum = pagenum;
    
    if (dirty){
        err = BF_TouchBuf(bq);
        if (err != BFE_OK){
            PFerrno = PFE_NOUSERS;
            return PFerrno;
        }
    }
    
    err = BF_UnpinBuf(bq);
    if (err != BFE_OK){
        PFerrno = PFE_NOUSERS;
        return PFerrno;
    }

    return PFE_OK;
}

void PF_PrintError(const char *errString)
{
	switch(PFerrno){
        case PFE_OK: return;break;
		case PFE_INVALIDPAGE: printf("PF: PFE_INVALIDPAGE\n"); break;
		case PFE_FTABFULL: printf("PF: PFE_FTABFULL\n");break;
		case PFE_FD: printf("PF: PFE_FD\n");break;
		case PFE_EOF: printf("PF: PFE_EOF\n");break;
		case PFE_FILEOPEN:printf("PF: PFE_FILEOPEN\n");break;
		case PFE_FILENOTOPEN:printf("PF: FILENOTOPEN\n");break;
        case PFE_HDRREAD:printf("PF: PFE_HDRREAD\n");break;
        case PFE_HDRWRITE:printf("PF: PFE_HDRWRITE\n");break;
        case PFE_PAGEFREE:printf("PF: PFE_PAGEFREE\n");break;
        case PFE_NOUSERS:printf("PF: PFE_NOUSERS\n");break;
        case PFE_MSGERR:printf("PF: PFE_MSGERR\n");break;
		case PFE_UNIX: printf("PF: PFE_UNIX)\n");break;

		default: printf("PF: unknown error code: %d \n", PFerrno);
	}
	printf("PF Error : %s", errString);
	exit(1);
}
