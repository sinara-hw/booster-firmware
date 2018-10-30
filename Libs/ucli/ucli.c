#include "ucli.h"

static ucli_ctx_t _ucli_ctx;

static const char * ucli_log_levels[4] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR"
};

static void ucli_print_string(const char * str)
{
	if (_ucli_ctx.printfn != NULL) {
		while (*str) {
	        _ucli_ctx.printfn(*str++);
	    }
	}
}


static void ucli_print_ch(char ch)
{
    if (_ucli_ctx.printfn != NULL) {
        _ucli_ctx.printfn(ch);
    }
}


void ucli_log(uint8_t level, const char * sFormat, ...)
{
#if UCLI_ENABLE_DYNAMIC_SETTINGS
	if (_ucli_ctx.conf.log_level <= level)
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

		if (len) ucli_print_string(header);
		ucli_print_string(buf);
#if UCLI_ENABLE_LOGSTASH
		ucli_logstash_push(level, buf);
#endif
	}
}


bool ucli_param_get_double(uint8_t argc, double * num)
{
    if (argc < _ucli_ctx.argc) {
        *num = strtod(_ucli_ctx.argv[argc], NULL);
        return true;
    }

    return false;
}


bool ucli_param_get_float(uint8_t argc, float * num)
{
    if (argc < _ucli_ctx.argc) {
        *num = strtof(_ucli_ctx.argv[argc], NULL);
        return true;
    }

    return false;
}


bool ucli_param_get_bool(uint8_t argc, bool * num)
{
	uint8_t res = 0;

    if (argc < _ucli_ctx.argc) {
        res = strtol(_ucli_ctx.argv[argc], NULL, 0);
        *num = (res == 1 ? true : false);
        return true;
    }

    return false;
}

bool ucli_param_get_int(uint8_t argc, int * num)
{
    if (argc < _ucli_ctx.argc) {
        *num = strtol(_ucli_ctx.argv[argc], NULL, 0);
        return true;
    }

    return false;
}


#if UCLI_ENABLE_DYNAMIC_SETTINGS
static void ucli_conf_command(void * a_data)
{
    char buf[32] = { 0x00 };
	bool conf = 0;
	ucli_param_get_bool(2, &conf);

	if (!strcmp(_ucli_ctx.argv[1], "echo")) {
		_ucli_ctx.conf.echo_enable = conf;
		sprintf(buf, "conf echo = %d\r\n", conf);
	} else if (!strcmp(_ucli_ctx.argv[1], "prompt")) {
		_ucli_ctx.conf.prompt_enable = conf;
		sprintf(buf, "conf prompt = %d\r\n", conf);
	} else if (!strcmp(_ucli_ctx.argv[1], "vt100")) {
		_ucli_ctx.conf.vt100_enable = conf;
		sprintf(buf, "conf vt100 = %d\r\n", conf);
	} else if (!strcmp(_ucli_ctx.argv[1], "progbar")) {
        _ucli_ctx.conf.progbar_disabled = conf;
        sprintf(buf, "conf progbar = %d\r\n", conf);
    } else if (!strcmp(_ucli_ctx.argv[1], "remote")) {
		_ucli_ctx.conf.prompt_enable = !conf;
		_ucli_ctx.conf.echo_enable = !conf;
		_ucli_ctx.conf.vt100_enable = !conf;
        _ucli_ctx.conf.progbar_disabled = conf;
		sprintf(buf, "conf remote = %d\r\n", conf);
	}

    ucli_print_string(buf);
}
#endif


static void ucli_help_command(void * a_data)
{
	unsigned int i = 0;

	if (_ucli_ctx.argc == 1) {
	    ucli_print_string(UCLI_DEFAULT_HELP_PROMPT);

	    if (_ucli_ctx.sys_cmds != NULL) {
	    	while (_ucli_ctx.sys_cmds[i].fh) {
	    		if (_ucli_ctx.sys_cmds[i].description) {
	    			ucli_print_string(_ucli_ctx.sys_cmds[i].cmd);
	    			ucli_print_string(UCLI_DEFAULT_HELP_DELIMER);
	    			ucli_print_string(_ucli_ctx.sys_cmds[i].description);
	    		}
	    		i++;
	    	}
	    }

	    i = 0;
	    if (_ucli_ctx.usr_cmds != NULL) {
	    	while (_ucli_ctx.usr_cmds[i].fh) {
	    		if (_ucli_ctx.usr_cmds[i].description) {
	    			ucli_print_string(_ucli_ctx.usr_cmds[i].cmd);
	    			ucli_print_string(UCLI_DEFAULT_HELP_DELIMER);
	    			ucli_print_string(_ucli_ctx.usr_cmds[i].description);
	    		}
	    		i++;
	    	}
	    }
	} else if (_ucli_ctx.argc == 2) {

		if (_ucli_ctx.sys_cmds != NULL) {
    		while (_ucli_ctx.sys_cmds[i].fh) {
    			uint8_t len = strlen(_ucli_ctx.sys_cmds[i].cmd);
        		if (!strncmp(_ucli_ctx.sys_cmds[i].cmd, _ucli_ctx.argv[1], len)) {
        			if (_ucli_ctx.sys_cmds[i].help != NULL) {
        				ucli_print_string(_ucli_ctx.sys_cmds[i].help);
        			} else {
        				ucli_print_string(UCLI_DEFAULT_NO_HELP_PROMPT);
        			}
        		}
				i++;	
			}
		}

		i = 0;
		if (_ucli_ctx.usr_cmds != NULL) {
    		while (_ucli_ctx.usr_cmds[i].fh) {
    			uint8_t len = strlen(_ucli_ctx.usr_cmds[i].cmd);
        		if (!strncmp(_ucli_ctx.usr_cmds[i].cmd, _ucli_ctx.argv[1], len)) {
        			if (_ucli_ctx.usr_cmds[i].help != NULL) {
        				ucli_print_string(_ucli_ctx.usr_cmds[i].help);
        			} else {
        				ucli_print_string(UCLI_DEFAULT_NO_HELP_PROMPT);
        			}
        		}
				i++;	
			}
		}
	}
}


static unsigned char ucli_count_arguments(void)
{
	char * ch = NULL;
	strcpy(_ucli_ctx.arg_buf, _ucli_ctx.cmd);

	ch = strtok(_ucli_ctx.arg_buf, UCLI_DEFAULT_SPLIT);
	while (ch != NULL) {
        _ucli_ctx.argv[_ucli_ctx.argc++] = ch;
        ch = strtok(NULL, UCLI_DEFAULT_SPLIT);
    }

    return _ucli_ctx.argc;
}


#if UCLI_ENABLE_VT100_SUPPORT
void ucli_vt100_clear_line(void)
{
#if UCLI_ENABLE_DYNAMIC_SETTINGS
	if (_ucli_ctx.conf.vt100_enable)
#endif
	{
    	ucli_print_string("\033[1K");
	}
}


void ucli_vt100_clear_screen(void)
{
#if UCLI_ENABLE_DYNAMIC_SETTINGS
	if (_ucli_ctx.conf.vt100_enable)
#endif
	{
    	ucli_print_string("\033[2J");
	}
}


static void ucli_clear_command(void * a_data)
{
    ucli_vt100_clear_screen();
}
#endif


#if UCLI_ENABLE_LOGSTASH
void ucli_logstash_push(uint8_t level, char * str)
{
    int next = _ucli_ctx.logstash.head + 1;
    if (next >= UCLI_LOG_STASH_LEN)
        next = 0;

    if (next == _ucli_ctx.logstash.tail) {
        int end = _ucli_ctx.logstash.tail + 1;
        if (end >= UCLI_LOG_STASH_LEN) end = 0;
        _ucli_ctx.logstash.tail = end;
        _ucli_ctx.logstash.count--;
    }

    _ucli_ctx.logstash.count++;
    memset(&_ucli_ctx.logstash.items[_ucli_ctx.logstash.head], 0x00, sizeof(ucli_log_stash_msg_t));
    strcpy(_ucli_ctx.logstash.items[_ucli_ctx.logstash.head].msg, str);
    _ucli_ctx.logstash.items[_ucli_ctx.logstash.head].level = level;
    _ucli_ctx.logstash.head = next;
}


void ucli_logstash_show(void)
{
    char buff[UCLI_LOG_BUFFER_SIZE + 16];

    if (_ucli_ctx.logstash.count == (UCLI_LOG_STASH_LEN - 1))
    {
        for (int i = _ucli_ctx.logstash.head; i < 8; i++) {
            sprintf(buff, "[%s] %s", ucli_log_levels[_ucli_ctx.logstash.items[i].level], _ucli_ctx.logstash.items[i].msg);
            ucli_print_string(buff);
        }
        for (int i = 0; i < _ucli_ctx.logstash.head; i++) {
            sprintf(buff, "[%s] %s", ucli_log_levels[_ucli_ctx.logstash.items[i].level], _ucli_ctx.logstash.items[i].msg);
            ucli_print_string(buff);
        }
    } else {
        for (int i = 0; i < _ucli_ctx.logstash.count; i++) {
            sprintf(buff, "[%s] %s", ucli_log_levels[_ucli_ctx.logstash.items[i].level], _ucli_ctx.logstash.items[i].msg);
            ucli_print_string(buff);
        }
    }
}


static void ucli_logstash_command(void * a_data)
{
	ucli_logstash_show();
}
#endif


#if UCLI_ENABLE_HISTORY
static void ucli_history_push(void)
{
    int next = _ucli_ctx.history.head + 1;
    if (next >= UCLI_CMD_HISTORY_LEN)
        next = 0;

    if (next == _ucli_ctx.history.tail) {
        int end = _ucli_ctx.history.tail + 1;
        if (end >= UCLI_CMD_HISTORY_LEN) end = 0;
        _ucli_ctx.history.tail = end;
        _ucli_ctx.history.hpos--;
    }

    _ucli_ctx.history.hpos++;
    memset(_ucli_ctx.history.items[_ucli_ctx.history.head], 0x00, UCLI_CMD_BUFFER_SIZE);
    strcpy(_ucli_ctx.history.items[_ucli_ctx.history.head], _ucli_ctx.cmd);
    _ucli_ctx.history.head = next;
}
#endif

void ucli_prompt(unsigned char nl)
{
    if (nl) ucli_print_string(UCLI_CLRF);
    ucli_print_string(UCLI_DEFAULT_PROMPT);
}

#if UCLI_ENABLE_HISTORY
static void ucli_history_up(void * a_data)
{
    char prompt[12] = { 0x00 };
    uint8_t curr = 0;

    if (_ucli_ctx.history.head != _ucli_ctx.history.tail) {
        if (_ucli_ctx.history.cpos)
            curr = (--_ucli_ctx.history.cpos) % _ucli_ctx.history.hpos;
        else {
            _ucli_ctx.history.cpos = _ucli_ctx.history.hpos - 1;
            curr = (_ucli_ctx.history.cpos) % _ucli_ctx.history.hpos;
        }

        memset(_ucli_ctx.cmd, 0x00, UCLI_CMD_BUFFER_SIZE);
        strcpy(_ucli_ctx.cmd, _ucli_ctx.history.items[curr]);
        _ucli_ctx.cpos = strlen(_ucli_ctx.cmd);

#if UCLI_ENABLE_VT100_SUPPORT
#if UCLI_ENABLE_DYNAMIC_SETTINGS
        if (_ucli_ctx.conf.vt100_enable)
#else
        if (UCLI_ENABLE_VT100_SUPPORT)
#endif
        {
        	ucli_vt100_clear_line();
        	snprintf(prompt, sizeof(prompt), "\r(%d/%d) ", curr + 1, _ucli_ctx.history.hpos);
        } else
#endif
        	snprintf(prompt, sizeof(prompt), "\n\r(%d/%d) ", curr + 1, _ucli_ctx.history.hpos);

        ucli_print_string(prompt);
        ucli_print_string(_ucli_ctx.cmd);
    }
}


static void ucli_history_down(void * a_data)
{
    char prompt[12] = { 0x00 };

    if (_ucli_ctx.history.head != _ucli_ctx.history.tail) {
        uint8_t curr = (++_ucli_ctx.history.cpos) % _ucli_ctx.history.hpos;

        memset(_ucli_ctx.cmd, 0x00, UCLI_CMD_BUFFER_SIZE);
        strcpy(_ucli_ctx.cmd, _ucli_ctx.history.items[curr]);
        _ucli_ctx.cpos = strlen(_ucli_ctx.cmd);

#if UCLI_ENABLE_VT100_SUPPORT
#if UCLI_ENABLE_DYNAMIC_SETTINGS
        if (_ucli_ctx.conf.vt100_enable)
#else
        if (UCLI_ENABLE_VT100_SUPPORT)
#endif
        {
			ucli_vt100_clear_line();
        	snprintf(prompt, sizeof(prompt), "\r(%d/%d) ", curr + 1, _ucli_ctx.history.hpos);
        } else
#endif
        	snprintf(prompt, sizeof(prompt), "\n\r(%d/%d) ", curr + 1, _ucli_ctx.history.hpos);

        ucli_print_string(prompt);
        ucli_print_string(_ucli_ctx.cmd);
    }
}
#endif

static unsigned char ucli_process_cmd_list(ucli_cmd_t * cmds)
{
	unsigned char i = 0;
    unsigned char ret = E_CMD_OK;

    while (cmds[i].fh) {
        uint8_t len = strlen(cmds[i].cmd);
        if (!strncmp(cmds[i].cmd, _ucli_ctx.cmd, len)) {

            // avoid passing smth like help3 to parser
            if (_ucli_ctx.cpos > len && _ucli_ctx.cmd[len] != 0x20)
                ret = E_CMD_NOT_FOUND;
            else {
                // call the handler
                ucli_count_arguments();
                if (cmds[i].argc < 0 || 
                   (cmds[i].argc == _ucli_ctx.argc - 1) ||
                   (!cmds[i].argc && _ucli_ctx.argc == 1)) {
                    	cmds[i].fh((void *) &_ucli_ctx);
                    	ret = E_CMD_OK;
                } else {
                    ret = E_CMD_LACK_ARGS;
                }
                break;
            }
        }
        i++;
    }

    if (!cmds[i].fh) {
        ret = E_CMD_NOT_FOUND;
    }

    return ret;
}


static unsigned char ucli_process_cmd(void)
{
    unsigned char ret = E_CMD_OK;

    if (!strlen(_ucli_ctx.cmd)) {
        return E_CMD_EMPTY;
    }

    if (strlen(_ucli_ctx.cmd) < 2) {
        return E_CMD_TOO_SHORT;
    }

	if (_ucli_ctx.usr_cmds != NULL) {
    	ret = ucli_process_cmd_list(_ucli_ctx.usr_cmds);
		/* 	
		  return only in case of:
	      - command success
    	  - error other than E_CMD_NOT_FOUND 
    	*/
    	if (ret == E_CMD_OK || ret != E_CMD_NOT_FOUND) 
    		return ret;
    }

    if (_ucli_ctx.sys_cmds != NULL) {
    	ret = ucli_process_cmd_list(_ucli_ctx.sys_cmds);
    }

    return ret;
}


static void ucli_postprocess_cmd(unsigned char a_response)
{
    switch(a_response) {
        case E_CMD_NOT_FOUND:
            ucli_print_string(UCLI_ERROR_CMD_NOT_FOUND_STR);
            break;

        case E_CMD_EMPTY:
            break;

        case E_CMD_LACK_ARGS:
            ucli_print_string(UCLI_ERROR_CMD_LACK_ARGS_STR);
            break;

        case E_CMD_TOO_SHORT:
            ucli_print_string(UCLI_ERROR_CMD_TOO_SHORT_STR);
            break;

        case E_CMD_OK:
#if UCLI_ENABLE_HISTORY
            ucli_history_push();
#endif
            break;
    }
}


void ucli_process_chr(uint8_t chr)
{
    uint8_t res = 0;
    // multi-code matching
     if ((!_ucli_ctx.mc.pos && chr == 0x1B) || _ucli_ctx.mc.buf[0] == 0x1B) {
         if (_ucli_ctx.mc.pos < UCLI_MULTICODE_INPUT_MAX_LEN) {
             unsigned char mi = 0x00;

             _ucli_ctx.mc.buf[_ucli_ctx.mc.pos++] = chr;

             while (_ucli_ctx.mcmds[mi].fh) {
                 if (!memcmp(_ucli_ctx.mcmds[mi].pattern,
                             _ucli_ctx.mc.buf,
                             _ucli_ctx.mcmds[mi].len)) {
                     _ucli_ctx.mcmds[mi].fh((void *) &_ucli_ctx);
                     _ucli_ctx.mc.pos = 0;
                     memset(_ucli_ctx.mc.buf, 0x00, UCLI_MULTICODE_INPUT_MAX_LEN);
                     break;
                 }
                 mi++;
             }

             if (_ucli_ctx.mc.pos == UCLI_MULTICODE_INPUT_MAX_LEN) {
                 _ucli_ctx.mc.pos = 0;
                 memset(_ucli_ctx.mc.buf, 0x00, UCLI_MULTICODE_INPUT_MAX_LEN);
             }

             return;
         } else {
             _ucli_ctx.mc.pos = 0;
             memset(_ucli_ctx.mc.buf, 0x00, UCLI_MULTICODE_INPUT_MAX_LEN);
         }
     }

     switch (chr) {
         case KEY_CODE_BACKSPACE:
             if (_ucli_ctx.cpos) {
                 _ucli_ctx.cmd[--_ucli_ctx.cpos] = '\0';
                 ucli_print_string("\b \b");
             }
             break;

         case KEY_CODE_RETURN:
         case KEY_CODE_ENTER:
             _ucli_ctx.cmd[POSINC(_ucli_ctx.cpos)] = '\0';
             ucli_print_string("\r\n");

 #if UCLI_ENABLE_HISTORY
             _ucli_ctx.history.cpos = 0;
 #endif

             res = ucli_process_cmd();
             ucli_postprocess_cmd(res);

             _ucli_ctx.cpos = 0;
             _ucli_ctx.argc = 0;
             memset(_ucli_ctx.cmd, 0x00, UCLI_CMD_BUFFER_SIZE);
             memset(_ucli_ctx.arg_buf, 0x00, UCLI_CMD_BUFFER_SIZE);
             memset(_ucli_ctx.argv, 0x00, UCLI_CMD_ARGS_MAX);

 #if UCLI_ENABLE_DYNAMIC_SETTINGS
             if (_ucli_ctx.conf.prompt_enable)
 #endif
             	ucli_prompt(0);
             break;
         default:
             if (_ucli_ctx.cpos < (UCLI_CMD_BUFFER_SIZE - 1) && isprint(chr)) {
                 _ucli_ctx.cmd[_ucli_ctx.cpos++] = chr;

 #if UCLI_ENABLE_DYNAMIC_SETTINGS
                 if (_ucli_ctx.conf.echo_enable)
 #endif
                 	ucli_print_ch(chr); /* echo */
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


bool ucli_progress_bar(int current, int start, int stop, bool clearln)
{
#if UCLI_ENABLE_DYNAMIC_SETTINGS
    if (!_ucli_ctx.conf.progbar_disabled)
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
            if (_ucli_ctx.conf.vt100_enable && clearln)
    #else
            if (UCLI_ENABLE_VT100_SUPPORT)
    #endif
            {
                ucli_vt100_clear_line();
                sprintf(buf, "[*] %03d%% [%s] %d/%d\r", percent, bar, current, stop);
            } else
    #endif          
                sprintf(buf, "[*] %03d%% [%s] %d/%d\r\n", percent, bar, current, stop);
            ucli_print_string(buf);
            return true;
        }
    }

    return false;
}


static void ucli_test_command(void * a_data)
{
	if (_ucli_ctx.argc == 1) {
        ucli_logstash_show();
    } else if (_ucli_ctx.argc == 3) {
        int lvl = 0;
        ucli_param_get_int(1, &lvl);
        // ucli_logstash_push(lvl, _ucli_ctx.argv[2]);
        ucli_log(lvl, "Test log with level = %d and text %s\r\n", lvl, _ucli_ctx.argv[2]);
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


void ucli_init(void * print_fn, ucli_cmd_t *a_cmds)
{
    memset(&_ucli_ctx, 0x00, sizeof(ucli_ctx_t));

#if UCLI_ENABLE_DYNAMIC_SETTINGS
	_ucli_ctx.conf.prompt_enable = UCLI_DEFAULT_PROMPT_ENABLE;
	_ucli_ctx.conf.echo_enable = UCLI_DEFAULT_ECHO_ENABLE;
	_ucli_ctx.conf.vt100_enable = UCLI_DEFAULT_VT100_ENABLE;
	_ucli_ctx.conf.log_level = UCLI_LOG_DEFAULT_LEVEL;
#endif

	_ucli_ctx.printfn = print_fn;
    _ucli_ctx.usr_cmds = a_cmds;
    _ucli_ctx.sys_cmds = sys_cmds;
    _ucli_ctx.mcmds = sys_mc_cmds;

    ucli_prompt(0);
}
