/*
 * shell.h
 * basic data structures, macros, function declarations, etc
 */

#include <stdio.h>
#include <stdlib.h>

/* macros */
#define HISTORY_MAX 100  /* maximum number of entries in history */
#define PIPE_MAX 50 /* maximum number of pipes supported */
#define ARG_MAX 101 /* maximum number of arguments in one command */
#define STR_MAX_LEN 256 /* maximum length of single read() from input */
#define PIPE_CHAR '|'
#define FD_STDIN 0
#define FD_STDOUT 1
#define INDEX_READ 0
#define INDEX_WRITE 1

#define LIST_TRAVERSE(_list_,_node_) \
	for (_node_ = _list_->head; _node_ != NULL; _node_ = nodeGetNext(_node_))

#define ARGV_TRAVERSE(_argv_,_arg_,_c_) \
	for (_c_ = 0; (_arg_ = _argv_[c]) != NULL; _c_++)

#define IS_SPACE(s) (isspace(s) ? TRUE : FALSE)

#define STR_MATCH(s1,n1,s2,n2) ((n1 == n2) && (0 == strcmp(s1,s2)) ? TRUE : FALSE)

/* data structures */
typedef enum bool
{
	FALSE,
	TRUE
}BOOL;

typedef enum rtcode
{
	SUCCESS,
	ERR_ERRNO,
	ERR_NULL_PTR,
	ERR_MEM_ALLOC,
	ERR_BAD_PARAM,
	ERR_PIPE_EMPTY_CMD,
	ERR_PIPE_APPEND,
	ERR_PIPE_OVFL,
	ERR_PIPE_NOT_SUPPT,
	ERR_HIST_NOT_FOUND,
	ERR_MAX
}RT_CODE;

typedef enum cmdtype 
{
	CMD_TYPE_NONE,
	CMD_TYPE_EXEC,
	CMD_TYPE_EXIT,
	CMD_TYPE_CD,
	CMD_TYPE_HIST,
	CMD_TYPE_MAX
}CMD_TYPE;

typedef struct mynode
{
	void* data;
	int size;
	struct mynode* next;
}NODE;

typedef struct mylist
{
	NODE* head;
	NODE* tail;
	int length;
}LIST;

typedef struct cmdentry
{
	CMD_TYPE cmd_type;
	char* cmd_name;
	char** argv;
}CMD_ENTRY;

typedef void (*FREE_FUNC)(void*);

typedef char** ARGV; /* argument vector */


LIST *HISTORY; /* list of history lines of user input */
LIST *PIPE; /* list of command entries in pipes for each run */

/* functions */
static inline ARGV argvNew(int count)
{
	ARGV v = calloc(count, sizeof(char*));
	if (NULL != v) 
		v[0] = (char*)NULL;
	return v;
}

static inline void argvFree(ARGV argv)
{
	if (NULL == argv) 
		return;

	char *arg = NULL;
	int c = 0;

	ARGV_TRAVERSE(argv,arg,c) 
		free(arg);
	free(argv);
}

static inline int argvAppend(ARGV argv, char* arg, int max)
{
	char *s;
	int c;
	
	ARGV_TRAVERSE(argv,s,c);
	if (c < max - 1) {
		argv[c] = arg;
		argv[c+1] = (char*)NULL;
		return c;
	} else {
		return -1;
	}
}

static inline NODE* nodeNew(void *data, int size) 
{
	NODE *ptr = (NODE*)calloc(1,sizeof(NODE));
	if (ptr == NULL) 
		return NULL;

	ptr->data = data;
	ptr->size = size;
	ptr->next = NULL;

	return ptr;
}

static inline void nodeFree(NODE *node, FREE_FUNC pf)
{
	if (node == NULL) 
		return;
	if (pf == NULL) 
		pf = free;
	if (node->data != NULL) 
		pf(node->data);
	free(node);
	return;
}

static inline void* nodeGetData(NODE *node)
{
	return node->data;
}

static inline int nodeGetSize(NODE *node)
{
	return node->size;
}

static inline NODE* nodeGetNext(NODE *node)
{
	return node->next;
}

static inline LIST* listNew(void)
{
	LIST *ptr = (LIST*)calloc(1,sizeof(LIST));
	if (ptr == NULL) 
		return NULL;

	ptr->head = NULL;
	ptr->tail = NULL;
	ptr->length = 0;

	return ptr;
}

static inline void listClear(LIST *list, FREE_FUNC pf)
{
	if (NULL == list) 
		return;

	NODE *ptr = list->head;
	NODE *nptr = NULL;
	while (ptr != NULL) {
		nptr = ptr->next;
		nodeFree(ptr, pf);
		ptr = nptr;
	}

	list->head = NULL;
	list->tail = NULL;
	list->length = 0;

	return;
}

static inline void listFree(LIST *list, FREE_FUNC pf)
{
	if (list == NULL) 
		return;

	listClear(list, pf);
	free(list);
	return;
}

static inline void listAppend(LIST *list, NODE *node, int max_len, FREE_FUNC pf)
{
	if (list == NULL || node == NULL) 
		return;
	if (list->head == NULL) {
		/* list empty, first node */
		list->head = node;
		list->tail = node;
		list->length += 1;
	} else if (list->length < max_len) {
		/* list not full */
		NODE *cur_tail = list->tail;
		
		cur_tail->next = node;
		list->tail = node;
		list->length += 1;
	} else {
		/* list full */
		NODE *cur_head = list->head;
		NODE *cur_tail = list->tail;

		list->head = cur_head->next;
		nodeFree(cur_head, pf);

		cur_tail->next = node;
		list->tail = node;
	}
	return;
}

static inline int listGetLength(LIST *list)
{
	return list->length;
}

static inline void* listGetHeadData(LIST *list)
{
	NODE *node = list->head;
	if (NULL == node) 
		return NULL;
	return node->data;
}


void runShell(void);
RT_CODE initProc(void);
void cleanProc(void);
void printErrorCode(RT_CODE rt);
RT_CODE readUserInput(char** str, int* str_len, int max_len);
BOOL validateInput(char* str, int str_len);
RT_CODE saveStrToHistory(char* str, int str_len);
RT_CODE parseInputStr(char* str, int str_len);
RT_CODE appendCmdToPipe(char* cmd_str, int cmd_str_len);
CMD_ENTRY* createCmdEntry(char* cmd_str, int cmd_str_len);
CMD_TYPE pharseCmdType(char* str, int len);
BOOL checkSaveCmdToHist();
RT_CODE runCmds(void);
RT_CODE runCmdPipe(void);
RT_CODE validatePipe(void);
RT_CODE genPipeFdList(int*** pfd_list, int pipe_size);
RT_CODE runCmdSingle(void);
RT_CODE runCmdExecSingle(CMD_ENTRY* entry);
RT_CODE runCmdExec(CMD_ENTRY* entry, int** fd_list,
		int pipe_index, int pipe_size, int* ppid);
RT_CODE runCmdExit(CMD_ENTRY *entry);
RT_CODE runCmdCd(CMD_ENTRY *entry);
RT_CODE runCmdHist(CMD_ENTRY *entry);
RT_CODE runCmdHistShow(void);
RT_CODE runCmdHistClear(void);
RT_CODE runCmdHistExec(int offset);
int str2int(char* str);
void freePipeNodeData(void *data);

