int  CreateTable(const char *relName,	/* name	of relation to create	*/
		int numAttrs,		/* number of attributes		*/
		ATTR_DESCR attrs[],	/* attribute descriptors	*/
		const char *primAttrName);/* primary index attribute	*/

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
void FE_Init(void);			/* FE initialization		*/
