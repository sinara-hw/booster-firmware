#ifndef UCLI_H_
#define UCLI_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#define UCLI_ENABLE_DYNAMIC_SETTINGS    1
#define UCLI_ENABLE_VT100_SUPPORT       1
#define UCLI_ENABLE_HISTORY             1
#define UCLI_ENABLE_LOGSTASH            1

#if UCLI_ENABLE_DYNAMIC_SETTINGS
#define UCLI_DEFAULT_PROMPT_ENABLE      1
#define UCLI_DEFAULT_ECHO_ENABLE        1
#define UCLI_DEFAULT_VT100_ENABLE       0
#endif

#define UCLI_CMD_BUFFER_SIZE            64
#define UCLI_CMD_ARGS_MAX               5
#define UCLI_CMD_HISTORY_LEN            8
#define UCLI_MULTICODE_INPUT_MAX_LEN    3

#define UCLI_LOG_STASH_LEN              32
#define UCLI_LOG_BUFFER_SIZE            UCLI_CMD_BUFFER_SIZE
#define UCLI_LOG_DEFAULT_LEVEL          0

#define POSINC(__x)                     (((__x) < (UCLI_CMD_BUFFER_SIZE - 1)) ? (__x + 1) : (__x))
#define isprint(c)                      ((uint8_t) c >= 0x20 && (uint8_t) c <= 0x7f)

#define UCLI_DEFAULT_SPLIT			    " "
#define UCLI_CLRF                       "\r\n"
#define UCLI_DEFAULT_PROMPT             "> "
#define UCLI_DEFAULT_HELP_DELIMER       ":\r\n\t"
#define UCLI_DEFAULT_HELP_PROMPT        "Available commands:\r\n"
#define UCLI_DEFAULT_NO_HELP_PROMPT     "No help specified for this command\r\n"

#define UCLI_ERROR_CMD_TOO_SHORT_STR    "Command too short\r\n"
#define UCLI_ERROR_CMD_LACK_ARGS_STR    "Wrong number of parameters\r\n"
#define UCLI_ERROR_CMD_NOT_FOUND_STR    "Command not found\r\n"
#define UCLI_ERROR_CMD_EXECUTION_STR    "Command execution error\r\n"

#define KEY_CODE_BACKSPACE              8
#define KEY_CODE_DELETE                 27
#define KEY_CODE_RETURN                 13
#define KEY_CODE_ENTER                  10

typedef enum {
    E_CMD_OK = 0,
    E_CMD_NOT_FOUND,
    E_CMD_TOO_SHORT,
    E_CMD_EMPTY,
    E_CMD_LACK_ARGS,
    E_CMD_EXECUTION_ERROR
} ucli_cmd_status_t;

typedef enum {
    UCLI_LOG_DEBUG,
    UCLI_LOG_INFO,
    UCLI_LOG_WARN,
    UCLI_LOG_ERROR,

    UCLI_LOG_NO_LOGS
} ucl_log_level_t;

typedef struct {
	unsigned char buf[UCLI_MULTICODE_INPUT_MAX_LEN];
	unsigned char pos;	
} ucli_multicode_input_t;

typedef struct {
    unsigned char pattern[UCLI_MULTICODE_INPUT_MAX_LEN];
    unsigned char len;
    void (*fh)(void*);
} ucli_multicode_cmd_t;

typedef struct {
    const char *cmd;
    void (*fh)(void *);
    int8_t argc;
    const char *description;
    const char *help;
} ucli_cmd_t;

typedef struct {
	char items[UCLI_CMD_HISTORY_LEN][UCLI_CMD_BUFFER_SIZE];
	unsigned char head;
	unsigned char tail;
	unsigned char cpos;
	unsigned char hpos;
} ucli_ctx_history_t;

#if UCLI_ENABLE_LOGSTASH
typedef struct {
    char msg[UCLI_LOG_BUFFER_SIZE];
    uint8_t level;
} ucli_log_stash_msg_t;

typedef struct {
    ucli_log_stash_msg_t items[UCLI_LOG_STASH_LEN];
    signed char head;
    signed char tail;
    signed char count;
} ucli_ctx_log_stash_t;
#endif

typedef struct {
	bool echo_enable;
	bool prompt_enable;
    bool vt100_enable;
    bool progbar_disabled;

    uint8_t log_level;
} ucli_conf_t;

typedef struct {
    // command definitions
	ucli_multicode_cmd_t * mcmds;
	ucli_cmd_t * usr_cmds;
    ucli_cmd_t * sys_cmds;

    // commands
	char cmd[UCLI_CMD_BUFFER_SIZE];
	unsigned char cpos;

	// args
    char * argv[UCLI_CMD_ARGS_MAX];
    char arg_buf[UCLI_CMD_BUFFER_SIZE];
    unsigned char argc;

    int (*printfn)(char);

    // additional functions
#if UCLI_ENABLE_LOGSTASH
    ucli_ctx_log_stash_t logstash;
#endif
#if UCLI_ENABLE_DYNAMIC_SETTINGS
    ucli_conf_t	conf;
#endif

#if UCLI_ENABLE_HISTORY
	ucli_ctx_history_t history;
#endif

	ucli_multicode_input_t mc;
} ucli_ctx_t;

void ucli_process_chr(uint8_t chr);
void ucli_init(void * print_fn, ucli_cmd_t *a_cmds);

#if UCLI_ENABLE_VT100_SUPPORT
void ucli_vt100_clear_line(void);
void ucli_vt100_clear_screen(void);
#endif

bool ucli_param_get_double(uint8_t argc, double * num);
bool ucli_param_get_float(uint8_t argc, float * num);
bool ucli_param_get_bool(uint8_t argc, bool * num);
bool ucli_param_get_int(uint8_t argc, int * num);

bool ucli_progress_bar(int current, int start, int stop, bool clearln);
void ucli_logstash_push(uint8_t level, char * str);
void ucli_log(uint8_t level, const char * sFormat, ...);

#endif
