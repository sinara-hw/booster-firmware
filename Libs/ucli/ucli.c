#include "ucli.h"

static const char * ucli_log_levels[4] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR"
};

static void ucli_print_string(ucli_ctx_t * a_ctx, const char * str)
{
	if (a_ctx->printfn != NULL) {
		while (*str) {
	        a_ctx->printfn(*str++);
	    }
	}
}


static void ucli_print_ch(ucli_ctx_t * a_ctx, char ch)
{
    if (a_ctx->printfn != NULL) {
        a_ctx->printfn(ch);
    }
}


void ucli_log(ucli_ctx_t * a_ctx, uint8_t level, const char * sFormat, ...)
{
#if UCLI_ENABLE_DYNAMIC_SETTINGS
	if (a_ctx->conf.log_level <= level)
#else
	if (UCLI_LOG_DEFAULT_LEVEL <= level)
#endif
	{
		char buf[UCLI_LOG_BUFFER_SIZE];
		char header[16];
		va_list ParamList;

		va_start(ParamList, sFormat);
		vsprintf(buf, sFormat, ParamList);
		va_end(ParamList);

		uint8_t len = 0;
		if (level <= 4) 
			len = sprintf(header, "[%s] ", ucli_log_levels[level]);

		if (len) ucli_print_string(a_ctx, header);
		ucli_print_string(a_ctx, buf);
#if UCLI_ENABLE_LOGSTASH
		ucli_logstash_push(a_ctx, level, buf);
#endif
	}
}


bool ucli_param_get_double(ucli_ctx_t *a_ctx, uint8_t argc, double * num)
{
    if (argc < a_ctx->argc) {
        *num = strtod(a_ctx->argv[argc], NULL);
        return true;
    }

    return false;
}


bool ucli_param_get_float(ucli_ctx_t *a_ctx, uint8_t argc, float * num)
{
    if (argc < a_ctx->argc) {
        *num = strtof(a_ctx->argv[argc], NULL);
        return true;
    }

    return false;
}


bool ucli_param_get_bool(ucli_ctx_t *a_ctx, uint8_t argc, bool * num)
{
	uint8_t res = 0;

    if (argc < a_ctx->argc) {
        res = strtol(a_ctx->argv[argc], NULL, 0);
        *num = (res == 1 ? true : false);
        return true;
    }

    return false;
}

bool ucli_param_get_int(ucli_ctx_t *a_ctx, uint8_t argc, int * num)
{
    if (argc < a_ctx->argc) {
        *num = strtol(a_ctx->argv[argc], NULL, 0);
        return true;
    }

    return false;
}


#if UCLI_ENABLE_DYNAMIC_SETTINGS
static void ucli_conf_command(void * a_data)
{
	ucli_ctx_t * a_ctx = (ucli_ctx_t *) a_data;
    char buf[32] = { 0x00 };
	bool conf = 0;
	ucli_param_get_bool(a_ctx, 2, &conf);

	if (!strcmp(a_ctx->argv[1], "echo")) {
		a_ctx->conf.echo_enable = conf;
		sprintf(buf, "conf echo = %d\r\n", conf);
	} else if (!strcmp(a_ctx->argv[1], "prompt")) {
		a_ctx->conf.prompt_enable = conf;
		sprintf(buf, "conf prompt = %d\r\n", conf);
	} else if (!strcmp(a_ctx->argv[1], "vt100")) {
		a_ctx->conf.vt100_enable = conf;
		sprintf(buf, "conf vt100 = %d\r\n", conf);
	} else if (!strcmp(a_ctx->argv[1], "progbar")) {
        a_ctx->conf.progbar_disabled = conf;
        sprintf(buf, "conf progbar = %d\r\n", conf);
    } else if (!strcmp(a_ctx->argv[1], "remote")) {
		a_ctx->conf.prompt_enable = !conf;
		a_ctx->conf.echo_enable = !conf;
		a_ctx->conf.vt100_enable = !conf;
        a_ctx->conf.progbar_disabled = conf;
		sprintf(buf, "conf remote = %d\r\n", conf);
	}

    ucli_print_string(a_ctx, buf);
}
#endif


static void ucli_help_command(void * a_data)
{
	unsigned int i = 0;
	ucli_ctx_t * a_ctx = (ucli_ctx_t *) a_data;

	if (a_ctx->argc == 1) {
	    ucli_print_string(a_ctx, UCLI_DEFAULT_HELP_PROMPT);

	    if (a_ctx->sys_cmds != NULL) {
	    	while (a_ctx->sys_cmds[i].fh) {
	    		if (a_ctx->sys_cmds[i].description) {
	    			ucli_print_string(a_ctx, a_ctx->sys_cmds[i].cmd);
	    			ucli_print_string(a_ctx, UCLI_DEFAULT_HELP_DELIMER);
	    			ucli_print_string(a_ctx, a_ctx->sys_cmds[i].description);
	    		}
	    		i++;
	    	}
	    }

	    i = 0;
	    if (a_ctx->usr_cmds != NULL) {
	    	while (a_ctx->usr_cmds[i].fh) {
	    		if (a_ctx->usr_cmds[i].description) {
	    			ucli_print_string(a_ctx, a_ctx->usr_cmds[i].cmd);
	    			ucli_print_string(a_ctx, UCLI_DEFAULT_HELP_DELIMER);
	    			ucli_print_string(a_ctx, a_ctx->usr_cmds[i].description);
	    		}
	    		i++;
	    	}
	    }
	} else if (a_ctx->argc == 2) {

		if (a_ctx->sys_cmds != NULL) {
    		while (a_ctx->sys_cmds[i].fh) {
    			uint8_t len = strlen(a_ctx->sys_cmds[i].cmd);
        		if (!strncmp(a_ctx->sys_cmds[i].cmd, a_ctx->argv[1], len)) {
        			if (a_ctx->sys_cmds[i].help != NULL) {
        				ucli_print_string(a_ctx, a_ctx->sys_cmds[i].help);
        			} else {
        				ucli_print_string(a_ctx, UCLI_DEFAULT_NO_HELP_PROMPT);
        			}
        		}
				i++;	
			}
		}

		i = 0;
		if (a_ctx->usr_cmds != NULL) {
    		while (a_ctx->usr_cmds[i].fh) {
    			uint8_t len = strlen(a_ctx->usr_cmds[i].cmd);
        		if (!strncmp(a_ctx->usr_cmds[i].cmd, a_ctx->argv[1], len)) {
        			if (a_ctx->usr_cmds[i].help != NULL) {
        				ucli_print_string(a_ctx, a_ctx->usr_cmds[i].help);
        			} else {
        				ucli_print_string(a_ctx, UCLI_DEFAULT_NO_HELP_PROMPT);
        			}
        		}
				i++;	
			}
		}
	}
}


static unsigned char ucli_count_arguments(ucli_ctx_t * a_ctx)
{
	char * ch = NULL;
	strcpy(a_ctx->arg_buf, a_ctx->cmd);

	ch = strtok(a_ctx->arg_buf, UCLI_DEFAULT_SPLIT);
	while (ch != NULL) {
        a_ctx->argv[a_ctx->argc++] = ch;
        ch = strtok(NULL, UCLI_DEFAULT_SPLIT);
    }

    return a_ctx->argc;
}


#if UCLI_ENABLE_VT100_SUPPORT
void ucli_vt100_clear_line(ucli_ctx_t * a_ctx)
{
#if UCLI_ENABLE_DYNAMIC_SETTINGS
	if (a_ctx->conf.vt100_enable)
#endif
	{
    	ucli_print_string(a_ctx, "\033[1K");
	}
}


void ucli_vt100_clear_screen(ucli_ctx_t * a_ctx)
{
#if UCLI_ENABLE_DYNAMIC_SETTINGS
	if (a_ctx->conf.vt100_enable)
#endif
	{
    	ucli_print_string(a_ctx, "\033[2J");
	}
}


static void ucli_clear_command(void * a_data)
{
    ucli_ctx_t * a_ctx = (ucli_ctx_t *) a_data;
    ucli_vt100_clear_screen(a_ctx);
}
#endif


#if UCLI_ENABLE_LOGSTASH
void ucli_logstash_push(ucli_ctx_t * a_ctx, uint8_t level, char * str)
{
    int next = a_ctx->logstash.head + 1;
    if (next >= UCLI_LOG_STASH_LEN)
        next = 0;

    if (next == a_ctx->logstash.tail) {
        int end = a_ctx->logstash.tail + 1;
        if (end >= UCLI_LOG_STASH_LEN) end = 0;
        a_ctx->logstash.tail = end;
        a_ctx->logstash.count--;
    }

    a_ctx->logstash.count++;
    memset(&a_ctx->logstash.items[a_ctx->logstash.head], 0x00, sizeof(ucli_log_stash_msg_t));
    strcpy(a_ctx->logstash.items[a_ctx->logstash.head].msg, str);
    a_ctx->logstash.items[a_ctx->logstash.head].level = level;
    a_ctx->logstash.head = next;
}


void ucli_logstash_show(ucli_ctx_t * a_ctx)
{
    char buff[UCLI_LOG_BUFFER_SIZE + 16];

    if (a_ctx->logstash.count == (UCLI_LOG_STASH_LEN - 1))
    {
        for (int i = a_ctx->logstash.head; i < 8; i++) {
            sprintf(buff, "[#%d] %s", a_ctx->logstash.items[i].level, a_ctx->logstash.items[i].msg);
            ucli_print_string(a_ctx, buff);
        }
        for (int i = 0; i < a_ctx->logstash.head; i++) {
            sprintf(buff, "[#%d] %s", a_ctx->logstash.items[i].level, a_ctx->logstash.items[i].msg);
            ucli_print_string(a_ctx, buff);
        }
    } else {
        for (int i = 0; i < a_ctx->logstash.count; i++) {
            sprintf(buff, "[#%d] %s", a_ctx->logstash.items[i].level, a_ctx->logstash.items[i].msg);
            ucli_print_string(a_ctx, buff);
        }
    }
}


static void ucli_logstash_command(void * a_data)
{
	ucli_ctx_t * a_ctx = (ucli_ctx_t *) a_data;
	ucli_logstash_show(a_ctx);
}
#endif


#if UCLI_ENABLE_HISTORY
static void ucli_history_push(ucli_ctx_t * a_ctx)
{
    int next = a_ctx->history.head + 1;
    if (next >= UCLI_CMD_HISTORY_LEN)
        next = 0;

    if (next == a_ctx->history.tail) {
        int end = a_ctx->history.tail + 1;
        if (end >= UCLI_CMD_HISTORY_LEN) end = 0;
        a_ctx->history.tail = end;
        a_ctx->history.hpos--;
    }

    a_ctx->history.hpos++;
    memset(a_ctx->history.items[a_ctx->history.head], 0x00, UCLI_CMD_BUFFER_SIZE);
    strcpy(a_ctx->history.items[a_ctx->history.head], a_ctx->cmd);
    a_ctx->history.head = next;
}
#endif

void ucli_prompt(ucli_ctx_t * a_ctx, unsigned char nl)
{
    if (nl) ucli_print_string(a_ctx, UCLI_CLRF);
    ucli_print_string(a_ctx, UCLI_DEFAULT_PROMPT);
}

#if UCLI_ENABLE_HISTORY
static void ucli_history_up(void * a_data)
{
    ucli_ctx_t * a_ctx = (ucli_ctx_t *) a_data;
    char prompt[12] = { 0x00 };
    uint8_t curr = 0;

    if (a_ctx->history.head != a_ctx->history.tail) {
        if (a_ctx->history.cpos)
            curr = (--a_ctx->history.cpos) % a_ctx->history.hpos;
        else {
            a_ctx->history.cpos = a_ctx->history.hpos - 1;
            curr = (a_ctx->history.cpos) % a_ctx->history.hpos;
        }

        memset(a_ctx->cmd, 0x00, UCLI_CMD_BUFFER_SIZE);
        strcpy(a_ctx->cmd, a_ctx->history.items[curr]);
        a_ctx->cpos = strlen(a_ctx->cmd);

#if UCLI_ENABLE_VT100_SUPPORT
#if UCLI_ENABLE_DYNAMIC_SETTINGS
        if (a_ctx->conf.vt100_enable)
#else
        if (UCLI_ENABLE_VT100_SUPPORT)
#endif
        {
        	ucli_vt100_clear_line(a_ctx);
        	snprintf(prompt, sizeof(prompt), "\r(%d/%d) ", curr + 1, a_ctx->history.hpos);
        } else
#endif
        	snprintf(prompt, sizeof(prompt), "\n\r(%d/%d) ", curr + 1, a_ctx->history.hpos);

        ucli_print_string(a_ctx, prompt);
        ucli_print_string(a_ctx, a_ctx->cmd);
    }
}


static void ucli_history_down(void * a_data)
{
    ucli_ctx_t * a_ctx = (ucli_ctx_t *) a_data;
    char prompt[12] = { 0x00 };

    if (a_ctx->history.head != a_ctx->history.tail) {
        uint8_t curr = (++a_ctx->history.cpos) % a_ctx->history.hpos;

        memset(a_ctx->cmd, 0x00, UCLI_CMD_BUFFER_SIZE);
        strcpy(a_ctx->cmd, a_ctx->history.items[curr]);
        a_ctx->cpos = strlen(a_ctx->cmd);

#if UCLI_ENABLE_VT100_SUPPORT
#if UCLI_ENABLE_DYNAMIC_SETTINGS
        if (a_ctx->conf.vt100_enable)
#else
        if (UCLI_ENABLE_VT100_SUPPORT)
#endif
        {
			ucli_vt100_clear_line(a_ctx);
        	snprintf(prompt, sizeof(prompt), "\r(%d/%d) ", curr + 1, a_ctx->history.hpos);
        } else
#endif
        	snprintf(prompt, sizeof(prompt), "\n\r(%d/%d) ", curr + 1, a_ctx->history.hpos);

        ucli_print_string(a_ctx, prompt);
        ucli_print_string(a_ctx, a_ctx->cmd);
    }
}
#endif

static unsigned char ucli_process_cmd_list(ucli_ctx_t * a_ctx, ucli_cmd_t * cmds)
{
	unsigned char i = 0;
    unsigned char ret = E_CMD_OK;

    while (cmds[i].fh) {
        uint8_t len = strlen(cmds[i].cmd);
        if (!strncmp(cmds[i].cmd, a_ctx->cmd, len)) {

            // avoid passing smth like help3 to parser
            if (a_ctx->cpos > len && a_ctx->cmd[len] != 0x20)
                ret = E_CMD_NOT_FOUND;
            else {
                // call the handler
                ucli_count_arguments(a_ctx);
                if (cmds[i].argc < 0 || 
                   (cmds[i].argc == a_ctx->argc - 1) || 
                   (!cmds[i].argc && a_ctx->argc == 1)) {
                    	cmds[i].fh((void *)a_ctx);
                } else {
                    ret = E_CMD_LACK_ARGS;
                }
            }	
            break;
        }
        i++;
    }

    if (!cmds[i].fh) {
        ret = E_CMD_NOT_FOUND;
    }

    return ret;
}


static unsigned char ucli_process_cmd(ucli_ctx_t * a_ctx)
{
    unsigned char ret = E_CMD_OK;

    if (!strlen(a_ctx->cmd)) {
        return E_CMD_EMPTY;
    }

    if (strlen(a_ctx->cmd) < 2) {
        return E_CMD_TOO_SHORT;
    }

	if (a_ctx->usr_cmds != NULL) {
    	ret = ucli_process_cmd_list(a_ctx, a_ctx->usr_cmds);
		/* 	
		  return only in case of:
	      - command success
    	  - error other than E_CMD_NOT_FOUND 
    	*/
    	if (ret == E_CMD_OK || ret != E_CMD_NOT_FOUND) 
    		return ret;
    }

    if (a_ctx->sys_cmds != NULL) {
    	ret = ucli_process_cmd_list(a_ctx, a_ctx->sys_cmds);
    }

    return ret;
}


static void ucli_postprocess_cmd(ucli_ctx_t * a_ctx, unsigned char a_response)
{
    switch(a_response) {
        case E_CMD_NOT_FOUND:
            ucli_print_string(a_ctx, UCLI_ERROR_CMD_NOT_FOUND_STR);
            break;

        case E_CMD_EMPTY:
            break;

        case E_CMD_LACK_ARGS:
            ucli_print_string(a_ctx, UCLI_ERROR_CMD_LACK_ARGS_STR);
            break;

        case E_CMD_TOO_SHORT:
            ucli_print_string(a_ctx, UCLI_ERROR_CMD_TOO_SHORT_STR);
            break;

        case E_CMD_OK:
#if UCLI_ENABLE_HISTORY
            ucli_history_push(a_ctx);
#endif
            break;
    }
}


void ucli_process_chr(ucli_ctx_t * a_ctx, uint8_t chr)
{
    uint8_t res = 0;

    // multi-code matching
    if ((!a_ctx->mc.pos && chr == 0x1B) || a_ctx->mc.buf[0] == 0x1B) {
        if (a_ctx->mc.pos < UCLI_MULTICODE_INPUT_MAX_LEN) {
            unsigned char mi = 0x00;

            a_ctx->mc.buf[a_ctx->mc.pos++] = chr;

            while (a_ctx->mcmds[mi].fh) {
                if (!memcmp(a_ctx->mcmds[mi].pattern,
                            a_ctx->mc.buf,
                            a_ctx->mcmds[mi].len)) {
                    a_ctx->mcmds[mi].fh((void *) a_ctx);
                    a_ctx->mc.pos = 0;
                    memset(a_ctx->mc.buf, 0x00, UCLI_MULTICODE_INPUT_MAX_LEN);
                    break;
                }
                mi++;
            }

            if (a_ctx->mc.pos == UCLI_MULTICODE_INPUT_MAX_LEN) {
                a_ctx->mc.pos = 0;
                memset(a_ctx->mc.buf, 0x00, UCLI_MULTICODE_INPUT_MAX_LEN);
            }

            return;
        } else {
            a_ctx->mc.pos = 0;
            memset(a_ctx->mc.buf, 0x00, UCLI_MULTICODE_INPUT_MAX_LEN);
        }
    }

    switch (chr) {
        case KEY_CODE_BACKSPACE:
            if (a_ctx->cpos) {
                a_ctx->cmd[--a_ctx->cpos] = '\0';
                ucli_print_string(a_ctx, "\b \b");
            }
            break;

        case KEY_CODE_RETURN:
        case KEY_CODE_ENTER:
            a_ctx->cmd[POSINC(a_ctx->cpos)] = '\0';
            ucli_print_string(a_ctx, "\r\n");

#if UCLI_ENABLE_HISTORY
            a_ctx->history.cpos = 0;
#endif

            res = ucli_process_cmd(a_ctx);
            ucli_postprocess_cmd(a_ctx, res);

            a_ctx->cpos = 0;
            a_ctx->argc = 0;
            memset(a_ctx->cmd, 0x00, UCLI_CMD_BUFFER_SIZE);
            memset(a_ctx->arg_buf, 0x00, UCLI_CMD_BUFFER_SIZE);
            memset(a_ctx->argv, 0x00, UCLI_CMD_ARGS_MAX);

#if UCLI_ENABLE_DYNAMIC_SETTINGS
            if (a_ctx->conf.prompt_enable)
#endif
            	ucli_prompt(a_ctx, 0);
            break;
        default:
            if (a_ctx->cpos < (UCLI_CMD_BUFFER_SIZE - 1) && isprint(chr)) {
                a_ctx->cmd[a_ctx->cpos++] = chr;

#if UCLI_ENABLE_DYNAMIC_SETTINGS                
                if (a_ctx->conf.echo_enable)
#endif
                	ucli_print_ch(a_ctx, chr); /* echo */
            }
            break;
    }
}

static ucli_multicode_cmd_t sys_mc_cmds[] = {
#if UCLI_ENABLE_HISTORY
    { { 0x1b, 0x5b, 0x41 }, 3, ucli_history_up },
    { { 0x1b, 0x5b, 0x42 }, 3, ucli_history_down },
#endif
    // null
    {{ 0x00, 0, 0x00 }}
};


bool ucli_progress_bar(ucli_ctx_t * a_ctx, int current, int start, int stop, bool clearln)
{
#if UCLI_ENABLE_DYNAMIC_SETTINGS
    if (a_ctx->conf.progbar_disabled)
#endif
    {
        char buf[32];
        char bar[10] = { 0x00 };
        memset(bar, 0x20, 10);

        if (start < stop && current >= start && current <= stop) 
        {
            float scale = ((float) current / (stop - start));
            int progress = scale * 10;
            int percent = scale * 100;
            memset(bar, 0x23, progress);

    #if UCLI_ENABLE_VT100_SUPPORT
    #if UCLI_ENABLE_DYNAMIC_SETTINGS
            if (a_ctx->conf.vt100_enable && clearln)
    #else
            if (UCLI_ENABLE_VT100_SUPPORT)
    #endif
            {
                ucli_vt100_clear_line(a_ctx);
                sprintf(buf, "[*] %03d%% [%s] %d/%d\r", percent, bar, current, stop);
            } else
    #endif          
                sprintf(buf, "[*] %03d%% [%s] %d/%d\r\n", percent, bar, current, stop);
            ucli_print_string(a_ctx, buf);
            return true;
        }
    }

    return false;
}


static void ucli_test_command(void * a_data)
{
	ucli_ctx_t * a_ctx = (ucli_ctx_t *) a_data;
	if (a_ctx->argc == 1) {
        ucli_logstash_show(a_ctx);
    } else if (a_ctx->argc == 3) {
        int lvl = 0;
        ucli_param_get_int(a_ctx, 1, &lvl);
        // ucli_logstash_push(a_ctx, lvl, a_ctx->argv[2]);
        ucli_log(a_ctx, lvl, "Test log with level = %d and text %s\r\n", lvl, a_ctx->argv[2]);
    }
}

static ucli_cmd_t sys_cmds[] = {
    { 
      	"help", 
      	ucli_help_command, 
      	-1,
		"Displays this menu\r\n"
    },
#if UCLI_ENABLE_DYNAMIC_SETTINGS
    { 
      	"conf", 
      	ucli_conf_command, 
      	2,
		"Configure uCLI settings\r\n"
    },
#endif
#if UCLI_ENABLE_VT100_SUPPORT
	{ 
      	"clear", 
      	ucli_clear_command, 
      	0x00,
		"Clear screen\r\n"
    },
#endif
#if UCLI_ENABLE_LOGSTASH
    { 
      	"logstash", 
      	ucli_logstash_command,
      	0x00
    },
#endif
    { 
      	"test", 
      	ucli_test_command,
      	-1
    },
    // null
    { 0x00, 0x00, 0x00  }
};


void ucli_init(ucli_ctx_t *a_ctx, ucli_cmd_t *a_cmds)
{
#if UCLI_ENABLE_DYNAMIC_SETTINGS
	a_ctx->conf.prompt_enable = UCLI_DEFAULT_PROMPT_ENABLE;
	a_ctx->conf.echo_enable = UCLI_DEFAULT_ECHO_ENABLE;
	a_ctx->conf.vt100_enable = UCLI_DEFAULT_VT100_ENABLE;
	a_ctx->conf.log_level = UCLI_LOG_DEFAULT_LEVEL;
#endif

    a_ctx->usr_cmds = a_cmds;
    a_ctx->sys_cmds = sys_cmds;
    a_ctx->mcmds = sys_mc_cmds;

    ucli_prompt(a_ctx, 0);
}