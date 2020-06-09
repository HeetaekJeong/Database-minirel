/* Definitions for BF layer*/

/* Definitions for PF layer*/
int PFerrno;
typedef struct PFhdr_str {
	int 	numpages;	/* number of pages in the file */
    char    buffer[PF_PAGE_SIZE];
} PFhdr_str;

typedef struct PFftab_ele {
	bool_t		valid;		/* set to TRUE when a file is open 	*/
	ino_t		inode;		/* inode number of the file 		*/
	char		*fname;		/* file name				*/
	int 		unixfd;		/* Unix file descriptor			*/
	PFhdr_str	hdr;		/* file header				*/
	short		hdrchanged;	/* TRUE if file header has changed 	*/
} PFftab_ele;

PFftab_ele *PFftable; 
size_t PFftab_length;

/* Definitions for HF layer*/

typedef struct {
    int RecSize;                 /* Record size */
    int RecPage;                 /* Number of records per page */
    int NumPg;                   /* Number of pages in file */
    int NumFrPgFile;             /* Number of free pages in the file */ 
} HFHeader;

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

int CopyHeader(int pffd, HFHeader *hfh, int flag);

int first;
HFftab_ele *HFftable;
HFstab_ele *HFstable;
int	HFerrno;
