/*
 * shell.c
 * Implementaion of w4118_sh
 */

#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "shell.h"

/*
 * main shell running function
 */
void runShell(void)
{
	RT_CODE rt = SUCCESS;
	char *input_str = NULL;
	int input_str_len = 0;
	BOOL save_hist = TRUE; /* whether input should be saved in history */

	rt = initProc();
	if (rt != SUCCESS) {
		printErrorCode(rt);
		exit(1);
	}

	while(1) {
		/* step 0: prompt */
		write(1,"$",1);
		
		if (TRUE == save_hist) {
			/*
			 * The timing for the input of previous loop to be saved
			 * in history is set here because it might fail to be
			 * parsed or executed, so it cannot be right after the
			 * parsing or at the end of each loop; it also cannot be
			 * right after the input is read because "history" 
			 * should not be saved into history.
			 */
			(void)saveStrToHistory(input_str,input_str_len);
		}
		save_hist = TRUE;

		/* step 1: read user input */
		rt = readUserInput(&input_str, &input_str_len, STR_MAX_LEN);
		if (rt != SUCCESS)  {
			printErrorCode(rt);
			save_hist = FALSE;
			continue;
		}
		if (validateInput(input_str, input_str_len) == FALSE) {
			/* empty input */
			save_hist = FALSE;
			continue;
		}

		/* step 2: parse user input into command pipe */
		listClear(PIPE, freePipeNodeData);

		rt = parseInputStr(input_str,input_str_len);
		if (rt != SUCCESS)  {
			printErrorCode(rt);
			continue;
		}

		/* check for "history" commands, which should not be saved */
		save_hist = checkSaveCmdToHist();

		/* step3: execute command(s) in command pipe */
		rt = runCmds();
		if (rt != SUCCESS)  {
			printErrorCode(rt);
			continue;
		}
	}

	cleanProc();
	exit(0);
}

/*
 * initialization
 * HISTORY: list of raw inputs of history runs
 # PIPE: list of command(s) of current run
 */
RT_CODE initProc(void)
{
	HISTORY = listNew();
	PIPE = listNew();

	if (HISTORY == NULL || PIPE == NULL)  {
		cleanProc();
		return ERR_ERRNO;
	}
	return SUCCESS;
}

/*
 * cleanup before exit()
 */
void cleanProc(void)
{
	listFree(HISTORY, NULL);
	listFree(PIPE, freePipeNodeData);
	return;
}

/*
 * reading user input to string
 * INPUT: str - pointer to the string
 *        str_len - pointer to the string length, the terminating nil is counted
 *        max_len - there is no limitation on the length of input, this is the
 *                  maximum length of characters each time read() reads in
 * RETURN: SUCCESS - success
 *         ERR_ERRNO - refer to errno
 *         else - pre-defined error categories
 */
RT_CODE readUserInput(char** str, int* str_len, int max_len)
{
	char buff[max_len];
	char *p = NULL;
	char *np = NULL;
	int len = 0;
	int rd = 0;

	while(1) {
		memset(buff,0,sizeof(buff));
		rd = read(0, buff, sizeof(buff));
		if (rd < 0)  {
			return ERR_ERRNO;
		} else if (rd == 0) {
			break;
		}

		if (p == NULL) {
			p = (char*)calloc(rd,sizeof(char));
			if (p == NULL) 
				return ERR_ERRNO;
		}

		strncpy(p+len,buff,rd);
		len += rd;

		if (rd < max_len) {
			/* if the returned length of read() is less than max,
			 * then all input must has been included in string
			 */
			break;
		}
		if (buff[rd-1] == '\n') {
			/* if the returned length of read() is equal to max
			 * (cannot be larger), and the buff ends with NL char
			 * i.e. 10, then all input has been included precisely
			 */
			break;
		}

		/* re-allocate p for more input */
		p = realloc(p, len+max_len);
		if (NULL == p) 
			return ERR_ERRNO;
	}

	/* 
	 * allocate np and copy contents of p to np, 
	 * discard extra space and the tailing '\n' 
	 */
	np = (char*)calloc(len, sizeof(char));
	strncpy(np, p, len-1);
	np[len-1] = '\0';
	free(p);

	*str = np;
	*str_len = len;
	return SUCCESS;
}

/*
 * validate input string
 * empty or all-space string will be discarded
 */
BOOL validateInput(char* str, int str_len)
{
	int i;
	if (NULL != str && str_len > 1) {
		for (i = 0; i < str_len - 1; i++) {
			if (FALSE == IS_SPACE(str[i]))  { 
				return TRUE; 
			}
		}
	}
	
	return FALSE;
}

/*
 * saving input string to list HISTORY for "history"
 * INPUT: str - the string, without modifications such as trimming spaces
 *        str_len - string length, the terminating nil is counted
 * RETURN: SUCCESS - success
 *         ERR_ERRNO - refer to errno
 *         else - pre-defined error categories
 */
RT_CODE saveStrToHistory(char* str, int str_len)
{
	NODE *p_node = NULL;

	if (NULL == str) 
		return ERR_NULL_PTR;
	if (1 >= str_len)  { 
		/* there is only one char '\0' or nothing */
		return SUCCESS; 
	}

	p_node = nodeNew((void*)str, str_len);
	if (p_node == NULL) 
		return ERR_ERRNO;

	listAppend(HISTORY, p_node, HISTORY_MAX, NULL);
	return SUCCESS;
}

/*
 * parsing input string into commands and save them in list PIPE for execution
 * 
 * input string contains one or more command strings delimited by PIPE_CHAR('|')
 * INPUT: str - the string, without modifications such as trimming spaces
 *        str_len - string length, the terminating nil is counted
 * RETURN: SUCCESS - success
 *         ERR_ERRNO - refer to errno
 *         else - pre-defined error categories
 */
RT_CODE parseInputStr(char* str, int str_len)
{
	RT_CODE rt = SUCCESS;
	char *cmd_str = NULL; /* one single command string */
	int len = 0;
	char *p = NULL;
	char *pp = NULL;
	BOOL not_start = TRUE; /* if true, don't put the char into cmd_str */

	for (p = str; p - str < str_len - 1; pp = p++) {
		if (IS_SPACE(*p) == TRUE && not_start == TRUE) {
			/* it is a space outside a command string, discard */
			continue;
		}

		if (*p != PIPE_CHAR) {
			/* not an useless space, nor a '|', save to cmd_str */
			if (not_start == TRUE) {
				/* start of a new cmd_str */
				not_start = FALSE;
				len = 2;
				cmd_str = (char*)calloc(len,sizeof(char));
				if (NULL == cmd_str) {
					listClear(PIPE, freePipeNodeData);
					return ERR_ERRNO;
				}
				cmd_str[len-2] = *p;
				cmd_str[len-1] = '\0';
			} else {
				/* append char to cmd_str */
				if (IS_SPACE(*p) == TRUE && 
				    IS_SPACE(*pp) == TRUE) {
					continue;
				}
				cmd_str = realloc(cmd_str, ++len);
				if (NULL == cmd_str) {
					listClear(PIPE, freePipeNodeData);
					return ERR_ERRNO;
				}
				cmd_str[len-2] = *p;
				cmd_str[len-1] = '\0';
			}


		} else if (not_start == TRUE) {
			/* a '|' shows up but there is no command ahead of it */
			continue;
		}
		
		if (*p == PIPE_CHAR || p - str == str_len - 2) {
			/* a '|' or the end of the input string */
			rt = appendCmdToPipe(cmd_str, len);
			free(cmd_str);
			cmd_str = NULL;
			if (SUCCESS != rt)  {
				listClear(PIPE, freePipeNodeData);
				return rt;
			}

			not_start = TRUE;
			len = 0;
		}
	}

	return SUCCESS;
}

/*
 * saving single command string in PIPE by appending it to the list
 * INPUT: cmd_str - the string
 *        cmd_str_len - string length, the terminating nil is counted
 * RETURN: SUCCESS - success
 *         ERR_ERRNO - refer to errno
 *         else - pre-defined error categories
 */
RT_CODE appendCmdToPipe(char* cmd_str, int cmd_str_len)
{
	NODE *node = NULL;
	CMD_ENTRY *cmd_entry = NULL;

	/* there is a limit on how many pipes are supported */
	if (PIPE_MAX <= listGetLength(PIPE)) 
		return ERR_PIPE_OVFL;

	cmd_entry = createCmdEntry(cmd_str, cmd_str_len);
	if (NULL == cmd_entry) 
		return ERR_PIPE_APPEND;

	node = nodeNew((void*)cmd_entry, sizeof(cmd_entry));
	if (NULL == node)  {
		freePipeNodeData((void*)cmd_entry);
		return ERR_ERRNO;
	}

	listAppend(PIPE, node, PIPE_MAX, freePipeNodeData);

	return SUCCESS;
}

/*
 * creating a command entry based on input command string
 * INPUT: cmd_str - the string
 *        cmd_str_len - string length, the terminating nil is counted
 * RETURN: pointer to the allocated entry
 */
CMD_ENTRY* createCmdEntry(char* cmd_str, int cmd_str_len)
{
	CMD_ENTRY *entry = NULL;
	CMD_TYPE cmd_type = CMD_TYPE_NONE;
	ARGV argv = NULL;
	char *p = NULL;
	char *p_saved = NULL;
	char *p_limit = cmd_str + cmd_str_len - 1;
	char *cmd_name = NULL;
	char *arg = NULL;
	int len = 0;
	BOOL is_space = FALSE;

	/* extract command name */
	p_saved = cmd_str;
	for (p = p_saved; p < p_limit; p++) {
		if (TRUE == IS_SPACE(*p)) 
			break;
		++len;
	}
	if (0 == len) 
		goto CLEANUP;

	cmd_name = (char*)calloc(len+1,sizeof(char));
	if (NULL == cmd_name) 
		goto CLEANUP;
	
	strncpy(cmd_name, p_saved, len);

	/* parse command type based on command name */
	cmd_type = pharseCmdType(cmd_name, len+1);
	if (CMD_TYPE_NONE == cmd_type) 
		goto CLEANUP;
	

	/* allocate entry */
	entry = (CMD_ENTRY*)calloc(1,sizeof(CMD_ENTRY));
	if (NULL == entry) 
		goto CLEANUP;
	

	/* allocate argv, keep in mind that last entry of argv must be NULL */
	argv = argvNew(ARG_MAX);
	if (NULL == argv) 
		goto CLEANUP;
	

	/* for CMD_TYPE_EXEC, the cmd_name should go into argv[] to format
	 * argv for execv; for build-in command, it does not 
	 */
	if (CMD_TYPE_EXEC == cmd_type)  {
		arg = (char*)calloc(len+1,sizeof(char));
		if (NULL == arg) 
			goto CLEANUP;
	
		strncpy(arg, cmd_name, len);
		(void)argvAppend(argv, arg, ARG_MAX);
	}

	/* append each arg (seperated by space char) to argv */
	p_saved = p+1;
	len = 0;
	for (p++; p < p_limit; p++) {
		is_space = IS_SPACE(*p);
		if ( TRUE == is_space || p == p_limit - 1) {
			/* count in the last non-space char before '\0' */
			if (FALSE == is_space) 
				len++;

			/* end of an arg */
			if (len > 0) {
				arg = (char*)calloc(len+1,sizeof(char));
				if (NULL == arg) 
					goto CLEANUP;
	
				strncpy(arg, p_saved, len);
				if (0 > argvAppend(argv, arg, ARG_MAX)) {
					free(arg);
					goto CLEANUP;
				}
			}

			/* start of next arg */
			p_saved = p+1;
			len = 0;
			continue;
		}

		len++;
	}

	entry->cmd_type = cmd_type;
	entry->cmd_name = cmd_name;
	entry->argv = argv;

	return entry;

CLEANUP:
	free(cmd_name);
	argvFree(argv);
	free(entry);
	return NULL;
}

/*
 * phasing command type based on command name
 * INPUT: str - the string of command name
 *        len - string length, the terminating nil is counted
 * RETURN: CMD_TYPE
 */
CMD_TYPE pharseCmdType(char* str, int len)
{
	CMD_TYPE ct = CMD_TYPE_NONE;

	if (str == NULL || 1 == len) 
		return ct;

	if (TRUE == STR_MATCH(str,len,"cd",sizeof("cd"))) {
		ct = CMD_TYPE_CD;
	} else if (TRUE == STR_MATCH(str,len,"history",sizeof("history"))) {
		ct = CMD_TYPE_HIST;
	} else if (TRUE == STR_MATCH(str,len,"exit",sizeof("exit"))) {
		ct = CMD_TYPE_EXIT;
	} else {
		ct = CMD_TYPE_EXEC;
	}
	return ct;
}

/*
 * checking whether to save the input string to HISTORY
 *
 * following inputs will NOT be saved in HISTORY:
 * 1. empty or all-space stirng
 * 2. "history" commands, unless it is with other commands in pipes
 */
BOOL checkSaveCmdToHist()
{
	if (NULL == PIPE) 
		return FALSE; 
	if (1 < listGetLength(PIPE)) 
		return TRUE;

	CMD_ENTRY *entry = (CMD_ENTRY*)listGetHeadData(PIPE);
	if (NULL == entry || CMD_TYPE_HIST == entry->cmd_type) {
		return FALSE;
	}
	return TRUE;
}

/*
 * running command(s), either single command or multiple commands in pipes
 * RETURN: SUCCESS - success
 *         ERR_ERRNO - refer to errno
 *         else - pre-defined error categories
 */
RT_CODE runCmds(void)
{
	int size = listGetLength(PIPE);

	if (0 == size) 
		return ERR_PIPE_EMPTY_CMD;
	if (1 == size) 
		return runCmdSingle();
	return runCmdPipe();
}

/*
 * running multiple commands in pipes
 * RETURN: SUCCESS - success
 *         ERR_ERRNO - refer to errno
 *         else - pre-defined error categories
 */
RT_CODE runCmdPipe(void)
{
	RT_CODE rt = SUCCESS;
	CMD_ENTRY *entry = NULL;
	NODE *node = NULL;
	int** fd_list = NULL; /* matrix of in/out fd for each child proc */
	int* pid_list; /* list of pid of each child proc */
	int pipe_size; /* size of list PIPE, number of individual commands */
	int i = 0;
	int status;

	rt = validatePipe();
	if (SUCCESS != rt) 
		return rt;

	pipe_size = listGetLength(PIPE);
	pid_list = (int*)calloc(pipe_size, sizeof(int));
	if (NULL == pid_list) 
		return ERR_ERRNO;

	/* open pipe for each command thus child proc */
	rt = genPipeFdList(&fd_list, pipe_size);
	if (NULL == fd_list) {
		free(pid_list); 
		return rt;
	}

	/* run each command on a new child proc */
	LIST_TRAVERSE(PIPE, node) {
		entry = (CMD_ENTRY*)node->data;
		rt = runCmdExec(entry, fd_list, i, pipe_size, pid_list + i);
		if (SUCCESS != rt) 
			goto CLEANUP;
	
		i++;
	}

	/* close all pipe fds and wait for all child procs to end */
	for (i = 0; i < pipe_size; i++) {
		if (fd_list[i][INDEX_READ] != FD_STDIN) {
			if (close(fd_list[i][INDEX_READ]) < 0) {
				rt = ERR_ERRNO;
				goto CLEANUP;
			}
		}
		if (fd_list[i][INDEX_WRITE] != FD_STDOUT) {
			if (close(fd_list[i][INDEX_WRITE]) < 0) {
				rt = ERR_ERRNO;
				goto CLEANUP;
			}
		}

		if (waitpid(pid_list[i], &status, 0) < 0) {
			rt = ERR_ERRNO;
			goto CLEANUP;
		}

		free(fd_list[i]);
		fd_list[i] = NULL;
	}

CLEANUP:
	for (i = 0; i < pipe_size; i++) {
		if (NULL != fd_list[i]) 
			free(fd_list[i]);
	}
	free(fd_list);
	free(pid_list);

	return rt;
}

/*
 * generating fd matrix for each command in PIPE
 * INPUT: pfd_list - pointer to the fd matrix
 *        pipe_size - number of commands
 * RETURN: SUCCESS - success
 *         ERR_ERRNO - refer to errno
 *         else - pre-defined error categories
 */
RT_CODE genPipeFdList(int*** pfd_list, int pipe_size)
{
	int **fd_list = NULL;
	int prev_out = 0;
	int pipe_fd[2];
	int i;

	if (NULL == pfd_list) 
		return ERR_NULL_PTR;

	fd_list = (int**)calloc(pipe_size, sizeof(int*));
	if (NULL == fd_list) 
		return ERR_ERRNO;

	for (i = 0; i < pipe_size; i++) {
		fd_list[i] = (int*)calloc(2, sizeof(int));
		if (NULL == fd_list[i]) {
			free(fd_list);
			return ERR_ERRNO;
		}

		/* no need to pipe() for the last iteration */
		if (i != pipe_size - 1 && pipe(pipe_fd) < 0) {
			free(fd_list);
			return ERR_ERRNO;
		}

		/* the fd-in of the current cmd is the fd-out of previous cmd */
		if (i == 0) {
			fd_list[i][INDEX_READ] = FD_STDIN;
		} else {
			fd_list[i][INDEX_READ] = prev_out;
		}
		
		/* the fd-out of the current cmd is the fd-in of new pipe */
		if (i == pipe_size - 1) {
			fd_list[i][INDEX_WRITE] = FD_STDOUT;
		} else {
			fd_list[i][INDEX_WRITE] = pipe_fd[INDEX_WRITE];
		}

		/* save the fd-out of new pipe for the next cmd */
		prev_out = (pipe_fd[INDEX_READ]);
	}

	*pfd_list = fd_list;

	return SUCCESS;
}

/*
 * validating the PIPE
 *
 * following conditions lead to an invalid PIPE:
 * 1. empty PIPE
 * 2. one or more empty command(s) in PIPE
 * 3. one or more command(s) in PIPE are build-in, e.g. cd, history
 * RETURN: SUCCESS - success
 *         ERR_ERRNO - refer to errno
 *         else - pre-defined error categories
 */
RT_CODE validatePipe(void)
{
	NODE *node = NULL;
	CMD_ENTRY *entry = NULL;

	if (0 == listGetLength(PIPE)) return ERR_PIPE_EMPTY_CMD;

	LIST_TRAVERSE(PIPE, node) {
		if (NULL == node) 
			return ERR_PIPE_EMPTY_CMD;

		entry = (CMD_ENTRY*)node->data;
		if (NULL == entry) 
			return ERR_PIPE_EMPTY_CMD;

		switch (entry->cmd_type) {
		case CMD_TYPE_EXIT:
		case CMD_TYPE_HIST:
		case CMD_TYPE_CD:
			return ERR_PIPE_NOT_SUPPT;

		default:
			break;
		}
	}
	return SUCCESS;
}

/*
 * running single command, i.e. no pipes
 * dispense to individual command handler based on command types
 * RETURN: SUCCESS - success
 *         ERR_ERRNO - refer to errno
 *         else - pre-defined error categories
 */
RT_CODE runCmdSingle(void)
{
	RT_CODE rt = SUCCESS;
	CMD_ENTRY *entry = (CMD_ENTRY*)listGetHeadData(PIPE);

	if (NULL == entry) 
		return ERR_PIPE_EMPTY_CMD;

	switch (entry->cmd_type) {
	case CMD_TYPE_EXEC:
		rt = runCmdExecSingle(entry);
		break;

	case CMD_TYPE_EXIT:
		rt = runCmdExit(entry);
		break;

	case CMD_TYPE_CD:
		rt = runCmdCd(entry);
		break;

	case CMD_TYPE_HIST:
		rt = runCmdHist(entry);
		break;

	default:
		break;
	}

	return rt;
}

/*
 * running single "exec" command
 * any non-built-in command is treated as "exec" by system call execv()
 * INPUT: entry - command entry containing command name and argument vector
 * RETURN: SUCCESS - success
 *         ERR_ERRNO - refer to errno
 *         else - pre-defined error categories
 */
RT_CODE runCmdExecSingle(CMD_ENTRY* entry)
{
	int pid = 0;
	int rt = 0;
	char* cmd_name = entry->cmd_name;
	ARGV argv = entry->argv;

	if (NULL == cmd_name || NULL == argv) 
		return ERR_NULL_PTR;

	pid = fork();
	if (pid < 0) 
		return ERR_ERRNO;

	if (pid == 0) {
		/* child proc */
		execv(cmd_name,argv);

		/* error */
		printErrorCode(ERR_ERRNO);
		exit(1);
	} else {
		/* parent proc */
		while (1) {
			rt = wait(0);
			if (rt < 0) return ERR_ERRNO;
			if (rt == pid) break;
		}
	}

	return SUCCESS;
}

/*
 * running "exec" command in pipes
 * INPUT: entry - command entry containing command name and argument vector
 *        fd_list - list of fd for all commands
 *        pipe_index - the index of this command in fd_list
 *        pipe_size - the total number of commands
 * OUTPUT: ppid - pid of the child proc for this command
 * RETURN: SUCCESS - success
 *         ERR_ERRNO - refer to errno
 *         else - pre-defined error categories
 */
RT_CODE runCmdExec(CMD_ENTRY* entry, int** fd_list,
		int pipe_index, int pipe_size, int* ppid)
{
	int pid = 0;
	int i;
	char* cmd_name = entry->cmd_name;
	ARGV argv = entry->argv;

	if (NULL == cmd_name || NULL == argv ||
	    NULL == fd_list || NULL == ppid) { 
		return ERR_NULL_PTR; 
	}

	pid = fork();
	if (pid < 0)  {
		return ERR_ERRNO; 
	} else if (pid > 0) {
		/* parent proc should only return the pid */
		*ppid = pid;
		return SUCCESS;
	}
	
	/* child proc */

	/* copy the pipe-read fd to stdin, except for the first proc */
	if (fd_list[pipe_index][INDEX_READ] != FD_STDIN) {
		if (dup2(fd_list[pipe_index][INDEX_READ], FD_STDIN) < 0) {
			return ERR_ERRNO;
		}
	}
	/* copy the pipe-write fd to stdout, except for the last proc */
	if (fd_list[pipe_index][INDEX_WRITE] != FD_STDOUT) {
		if (dup2(fd_list[pipe_index][INDEX_WRITE], FD_STDOUT) < 0) {
			return ERR_ERRNO;
		}
	}

	/* close all fds in the fd_list except for stdin and stdout */
	for (i = 0; i < pipe_size; i++) {
		if (fd_list[i][INDEX_READ] != FD_STDIN) {
			if (close(fd_list[i][INDEX_READ]) < 0) {
				return ERR_ERRNO;
			}
		}
		if (fd_list[i][INDEX_WRITE] != FD_STDOUT) {
			if (close(fd_list[i][INDEX_WRITE]) < 0) {
				return ERR_ERRNO;
			}
		}
	}

	execv(cmd_name,argv);

	/* error */
	printErrorCode(ERR_ERRNO);
	exit(1);
}

/*
 * running single "exit" command
 * INPUT: entry - command entry containing command name and argument vector
 * RETURN: SUCCESS - success
 *         ERR_ERRNO - refer to errno
 *         else - pre-defined error categories
 */
RT_CODE runCmdExit(CMD_ENTRY *entry)
{
	ARGV argv = entry->argv;

	/* exit takes no argument 
	 * I know checking argv here is kind of worthless, but just in case
	 * someone accidentally pastes a string starting with "exit" and Oops
	 */
	if (NULL == argv) 
		return ERR_NULL_PTR;

	if (NULL != argv[0]) 
		return ERR_BAD_PARAM; 

	cleanProc();
	exit(0);
}

/*
 * running single "cd" command
 * INPUT: entry - command entry containing command name and argument vector
 * RETURN: SUCCESS - success
 *         ERR_ERRNO - refer to errno
 *         else - pre-defined error categories
 */
RT_CODE runCmdCd(CMD_ENTRY *entry)
{
	ARGV argv = entry->argv;

	/* cd takes exactly one argument */
	if (NULL == argv) 
		return ERR_NULL_PTR;

	if (NULL == argv[0] || NULL != argv[1]) 
		return ERR_BAD_PARAM;

	if (chdir(argv[0]) < 0) {
		return ERR_ERRNO;
	}

	return SUCCESS;
}

/*
 * running single "history" command
 * dispense this command into following:
 * 1. "history": show HISTORY
 * 2. "history -c": clear HISTORY
 * 3. "history [offset]": run the command with index [offset] in HISTORY
 * INPUT: entry - command entry containing command name and argument vector
 * RETURN: SUCCESS - success
 *         ERR_ERRNO - refer to errno
 *         else - pre-defined error categories
 */
RT_CODE runCmdHist(CMD_ENTRY *entry)
{
	ARGV argv = entry->argv;
	char *arg = NULL;
	int offset = 0;

	/* history takes no more than one argument */
	if (NULL == argv) 
		return ERR_NULL_PTR;

	if (NULL != argv[0] && NULL != argv[1]) 
		return ERR_BAD_PARAM;

	arg = argv[0];
	if (NULL == arg)  { 
		/* "history" */
		return runCmdHistShow(); 
	}

	if (TRUE == STR_MATCH(arg, strlen(arg), "-c", sizeof("-c")-1)) {
		/* "history -c */
		return runCmdHistClear();
	}

	offset = str2int(arg);
	if (offset >= 0) {
		/* "history [offset] */
		return runCmdHistExec(offset);
	}

	return ERR_BAD_PARAM;
}

/*
 * running "history" command
 * RETURN: SUCCESS - success
 *         ERR_ERRNO - refer to errno
 *         else - pre-defined error categories
 */
RT_CODE runCmdHistShow(void)
{
	NODE *p_node = NULL;
	int c = 0;
	char* str = NULL;

	if (HISTORY == NULL) 
		return ERR_NULL_PTR;

	LIST_TRAVERSE(HISTORY,p_node) {
		str = (char*)nodeGetData(p_node);
		if (nodeGetSize(p_node) > 0) {
			printf("%3d\t%s\n",c,str);
			c++;
		}
	}
	return SUCCESS;
}

/*
 * running "history -c" command
 * RETURN: SUCCESS - success
 *         ERR_ERRNO - refer to errno
 *         else - pre-defined error categories
 */
RT_CODE runCmdHistClear(void)
{
	listClear(HISTORY, NULL);
	return SUCCESS;
}

/*
 * running "history [offset]" command
 * INPUT: offset - the index in HISTORY
 * RETURN: SUCCESS - success
 *         ERR_ERRNO - refer to errno
 *         else - pre-defined error categories
 */
RT_CODE runCmdHistExec(int offset)
{
	RT_CODE rt = SUCCESS;
	NODE *p_node = NULL;
	int c = 0;
	char* input_str = NULL;
	int input_str_len = 0;

	if (HISTORY == NULL) 
		return ERR_NULL_PTR;

	LIST_TRAVERSE(HISTORY,p_node) {
		if (offset == c)  {
			input_str = (char*)nodeGetData(p_node);
			input_str_len = nodeGetSize(p_node);
			break;
		}
		c++;
	}

	if (NULL == input_str || 0 == input_str_len)  { 
		/* no entry was found with index [offset] */
		return ERR_HIST_NOT_FOUND; 
	}

	/*
	 * Since "history" does not work in pipes, there will be 
	 * no other commands in the PIPE now, simply clear the PIPE
	 * and parse the string of HISTORY[offset] and run it
	 */
	listClear(PIPE, freePipeNodeData);

	rt = parseInputStr(input_str,input_str_len);
	if (rt != SUCCESS) 
		return rt;

	rt = runCmds();
	if (rt != SUCCESS) 
		return rt;

	return SUCCESS;
}

/*
 * printing error message based on input error categories
 * 
 * generally error messages have the format "error: xxx", where xxx can be:
 * 1. standard message derived from the 'errno', should it exists
 * 2. pre-defined message for specific errors
 * 3. "unknown" when no category was specified
 */
void printErrorCode(RT_CODE rt)
{
	char *str = NULL;
	extern int errno;

	switch (rt) {
	case ERR_ERRNO:
		str = strerror(errno);
		break;

	case ERR_NULL_PTR:
		str = "unexpected null pointer";
		break;

	case ERR_MEM_ALLOC:
		str = "failed to allocate memory";
		break;

	case ERR_BAD_PARAM:
		str = "bad parameters";
		break;

	case ERR_PIPE_EMPTY_CMD:
		str = "empty command in pipe";
		break;

	case ERR_PIPE_APPEND:
		str = "failed to add command in pipe";
		break;

	case ERR_PIPE_OVFL:
		str = "pipe size exceeds limit";
		break;

	case ERR_PIPE_NOT_SUPPT:
		str = "command not supported in pipe";
		break;

	case ERR_HIST_NOT_FOUND:
		str = "history offset is out of range";
		break;

	default:
		str = "unknown";
		break;
	}

	printf("error: %s\n", str);
	return;
}

/* easy implementation for small positive integers */
int str2int(char *str)
{
	int num = 0;
	int i = 0;

	for(; i < strlen(str); i++) {
		if (str[i] >= '0' && str[i] <= '9') {
			num = 10 * num + (str[i] - '0');
			continue;
		}
		return -1;
	}
	return num;
}

/* free function for nodes containing CMD_ENTRY* as data */
void freePipeNodeData(void *data)
{
	CMD_ENTRY *entry = (CMD_ENTRY*)data;

	argvFree(entry->argv);
	free(entry->cmd_name);
	free(entry);
	return;
}
