/**
 * @file parser.c
 * @date 2016. 05. 16.
 * @author kmsun
 *
 * parameter file parser
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include "parser.h"

#define KB_UNIT                 (1ULL << 10)
#define MB_UNIT                 (1ULL << 20)
#define GB_UNIT                 (1ULL << 30)
#define TB_UNIT                 (1ULL << 40)
#define PB_UNIT                 (1ULL << 50)

#define MIN_TO_SEC              (60)
#define HOUR_TO_SEC             (MIN_TO_SEC * 60)
#define DAY_TO_SEC              (HOUR_TO_SEC * 24)

#define MAX_TOKEN_LEN           (128)
#define LINE_BUF_LEN            (256)

#define MATCH_TOKEN(a, b)       (((a)->len == strlen(b)) && (memcmp((a)->value, (b), (a)->len) == 0))
#define SWAP(a, b)              { typeof(a) c; (c) = (a); (a) = (b); (b) = (c); }

#define ZIPF_THETA_DEF          (1.2)
#define PARETO_SCALE_DEF        (1)
#define PARETO_SHAPE_DEF        (1)

#define PARAM_DELIMITER         ','
#define PARAM_RANGE_DELIMITER   '-'
#define SUBPARAM_DELIMITER      ':'

#define PARSER_LOG(p, ...)      if ((p)->verbose) { printf(__VA_ARGS__); }

static const char *bool_strings[] = { "FALSE", "TRUE" };

u32 iosize_max;
u32 iosize_min = 0xFFFFFFFF;

/* parser states */
enum parser_state {
    parser_state_error = 0,
    parser_state_init,
    parser_state_section_opener,
    parser_state_section_name,
    parser_state_section_closer,
    parser_state_prop_key,
    parser_state_prop_equal,
    parser_state_prop_value,
    parser_state_env_dollar,
    parser_state_env_opener,
    parser_state_env_key,
    parser_state_env_closer,
    parser_state_endline,
    parser_state_max,
};

enum parser_result {
    parser_success      = 0,    /* success          */
    parser_syntax_error = -1,   /* syntax error     */
    parser_long_token   = -2,   /* too long token   */
};

static const char parser_state_table[parser_state_max][U8_MAX] = {
    [parser_state_init] = {
        [0] = parser_state_endline,
        ['['] = parser_state_section_opener,
        ['a' ... 'z'] = parser_state_prop_key,
        ['A' ... 'Z'] = parser_state_prop_key,
    },
    [parser_state_section_opener] = {
        ['a' ... 'z'] = parser_state_section_name,
        ['A' ... 'Z'] = parser_state_section_name,
        ['0' ... '9'] = parser_state_section_name,
        ['_'] = parser_state_section_name,
    },
    [parser_state_section_name] = {
        ['a' ... 'z'] = parser_state_section_name,
        ['A' ... 'Z'] = parser_state_section_name,
        ['0' ... '9'] = parser_state_section_name,
        ['_'] = parser_state_section_name,
        [']'] = parser_state_section_closer,
    },
    [parser_state_section_closer] = {
        [0] = parser_state_endline,
    },
    [parser_state_prop_key] = {
        ['a' ... 'z'] = parser_state_prop_key,
        ['A' ... 'Z'] = parser_state_prop_key,
        ['0' ... '9'] = parser_state_prop_key,
        ['_'] = parser_state_prop_key,
        ['='] = parser_state_prop_equal,
    },
    [parser_state_prop_equal] = {
        ['a' ... 'z'] = parser_state_prop_value,
        ['A' ... 'Z'] = parser_state_prop_value,
        ['0' ... '9'] = parser_state_prop_value,
        ['_'] = parser_state_prop_value,
        ['.'] = parser_state_prop_value,
        [':'] = parser_state_prop_value,
        ['/'] = parser_state_prop_value,
        ['-'] = parser_state_prop_value,
        ['$'] = parser_state_env_dollar,
    },
    [parser_state_prop_value] = {
        [0] = parser_state_endline,
        ['a' ... 'z'] = parser_state_prop_value,
        ['A' ... 'Z'] = parser_state_prop_value,
        ['0' ... '9'] = parser_state_prop_value,
        ['_'] = parser_state_prop_value,
        ['.'] = parser_state_prop_value,
        [':'] = parser_state_prop_value,
        ['/'] = parser_state_prop_value,
        ['-'] = parser_state_prop_value,
        [','] = parser_state_prop_value,
        ['%'] = parser_state_prop_value,
    },
    [parser_state_env_dollar] = {
        ['{'] = parser_state_env_opener,
    },
    [parser_state_env_opener] = {
        ['a' ... 'z'] = parser_state_env_key,
        ['A' ... 'Z'] = parser_state_env_key,
        ['_'] = parser_state_env_key,
    },
    [parser_state_env_key] = {
        ['a' ... 'z'] = parser_state_env_key,
        ['A' ... 'Z'] = parser_state_env_key,
        ['0' ... '9'] = parser_state_env_key,
        ['_'] = parser_state_env_key,
        ['}'] = parser_state_env_closer,
    },
    [parser_state_env_closer] = {
        [0] = parser_state_endline,
    },
};

struct token {
    u16 col;
    u16 len;
    char *value;
};

struct parser {
    int verbose;
    enum parser_state state;

    const char *file_name;
    int line_num;
    int col_num;

    int cur_token_idx;
    struct token tokens[4];
    struct token *cur_token;

    char line_buf[LINE_BUF_LEN];
    struct global_params *params;
    struct job_params *job_params;
    struct job_params default_job_params;
    int end_of_token;
};

void token_strupr(struct token *token)
{
    char *str;
    int len;

    str = token->value;
    len = token->len;

    while (len) {
        *str = toupper(*str);
        str++;
        len--;
    }
}

static void parsing_error(struct parser *parser, const char *message, const struct token *token)
{
    /* print error message */
    fprintf(stderr,
            "%s:%d:%d: error: %s",
            parser->file_name, parser->line_num + 1, parser->col_num + 1, message);
    if (token) {
        fprintf(stderr, ":%.*s\n", token->len, token->value);
    } else {
        fprintf(stderr, "\n");
    }

    /* print error position */
    fprintf(stderr, " %s\n", parser->line_buf);
    fprintf(stderr, "%*s^\n", parser->col_num + 1, " ");
    exit(-1);
}

static void on_section_defined(struct parser *parser, struct token *token)
{
    PARSER_LOG(parser, "[%s\n", token->value);

    parser->params->job_params =
        (struct job_params *)realloc(parser->params->job_params,
                                     sizeof (struct job_params) * (parser->params->njobs + 1));
    parser->job_params = &parser->params->job_params[parser->params->njobs];
    memcpy(parser->job_params, &parser->default_job_params, sizeof (struct job_params));
    memcpy(parser->job_params->name, token->value, token->len);
    parser->job_params->name[token->len] = '\0';
    parser->params->njobs++;
}

static void on_env_refered(struct parser *parser, struct token *token)
{
    char env_key[MAX_TOKEN_LEN];
    char *env_val;

    memcpy(env_key, token->value + 2, token->len - 3);
    env_key[token->len - 3] = 0;

    env_val = getenv(env_key);
    if (env_val == NULL) {
        parser->col_num = token->col;
        parsing_error(parser, "undefined environment variable", token);
        return;
    }
    token->value = env_val;
    token->len = strlen(env_val);
}

static int to_bool(struct parser *parser, struct token *token)
{
    token_strupr(token);

    if ((MATCH_TOKEN(token, "1")) ||
        (MATCH_TOKEN(token, "TRUE"))) {
        return 1;
    }
    if ((MATCH_TOKEN(token, "0")) ||
        (MATCH_TOKEN(token, "FALSE"))) {
        return 0;
    }

    parser->col_num = token->col;
    parsing_error(parser, "expected boolean type token(0, FALSE, 1 or TRUE )", token);

    return -1;
}

static u64 to_integer(struct parser *parser, struct token *token)
{
    char *value;
    u64 res;
    u16 remains;

    value = token->value;
    res = 0;
    remains = token->len;

    while (remains) {
        if (isdigit(*value)) {
            res *= 10;
            res += *value - '0';
        } else {
            parser->col_num = token->col;
            parsing_error(parser, "expected integer token", token);
        }
        value++;
        remains--;
    }

    return res;
}

static double to_double(struct parser *parser, struct token *token)
{
    char *value;
    s64 significant;
    double base_exp;
    int mode;
    u16 remains;

    value = token->value;
    significant = 0;
    base_exp = 1;
    mode = 0;
    remains = token->len;

    while (remains) {
        if (isdigit(*value)) {
            significant *= 10;
            significant += *value - '0';
            if (mode) {
                base_exp *= 0.1;
            }
        } else if (*value == '.') {
            if (mode) {
                parser->col_num = token->col;
                parsing_error(parser, "expected endline token", token);
            }
            mode = 1;
        } else {
            parser->col_num = token->col;
            parsing_error(parser, "expected real number token", token);
        }
        value++;
        remains--;
    }

    return significant * base_exp;
}

static u64 to_seconds(struct parser *parser, struct token *token)
{
    enum state { state_init, state_digit } state;
    char *value;
    u64 res;
    u64 digit;
    u16 remains;

    value = token->value;
    res = 0;
    digit = 0;
    remains = token->len;
    state = state_init;

    token_strupr(token);

    while (remains) {
        switch (state) {
            case state_init:
                if (isdigit(*value)) {
                    digit *= 10;
                    digit += *value - '0';
                    state = state_digit;
                } else {
                    goto error;
                }
                break;
            case state_digit:
                switch (*value) {
                    case '0' ... '9':
                        digit *= 10;
                        digit += *value - '0';
                        break;
                    case 'S':
                        res += digit;
                        digit = 0;
                        state = state_init;
                        break;
                    case 'M':
                        res += digit * MIN_TO_SEC;
                        digit = 0;
                        state = state_init;
                        break;
                    case 'H':
                        res += digit * HOUR_TO_SEC;
                        digit = 0;
                        state = state_init;
                        break;
                    case 'D':
                        res += digit * DAY_TO_SEC;
                        digit = 0;
                        state = state_init;
                        break;
                    default:
                        goto error;
                }
                break;
        }
        value++;
        remains--;
    }

    return res;

error:
    parser->col_num = token->col;
    parsing_error(parser, "invalid time format", token);

    return U64_MAX;
}

static u8 to_percent(struct parser *parser, struct token *token)
{
    char *value;
    u8 res;
    u16 remains;

    value = token->value;
    res = 0;
    remains = token->len;

    while (remains) {
        if (isdigit(*value)) {
            res *= 10;
            res += *value - '0';
            if (res > 100) {
                parser->col_num = token->col;
                parsing_error(parser, "percentage out of range", token);
            }
        } else if ((*value != '%') || (remains > 1)) {
            parser->col_num = token->col;
            parsing_error(parser, "unrecognizable data unit", token);
        }
        value++;
        remains--;
    }

    return res;
}

static u64 to_bytes(struct parser *parser, struct token *token)
{
    enum state { state_begin, state_digit, state_si, state_end } state;
    char *value;
    u16 remains;
    u64 res;

    value = token->value;
    remains = token->len;
    res = 0;
    state = state_begin;

    token_strupr(token);

    while (remains) {
        switch (state) {
            case state_begin:   /* only digit is allowed */
                if (isdigit(*value)) {
                    res *= 10;
                    res += *value - '0';
                    state = state_digit;
                } else {
                    goto error;
                }
                break;
            case state_digit:  /* digit, SI prefix and 'B' are allowed */
                switch (*value) {
                    case '0' ... '9':
                        res *= 10;
                        res += *value - '0';
                        break;
                    case 'B':
                        state = state_end;
                        break;
                    case 'K':
                        res *= KB_UNIT;
                        state = state_si;
                        break;
                    case 'M':
                        res *= MB_UNIT;
                        state = state_si;
                        break;
                    case 'G':
                        res *= GB_UNIT;
                        state = state_si;
                        break;
                    case 'T':
                        res *= TB_UNIT;
                        state = state_si;
                        break;
                    default:
                        goto error;
                }
                break;
            case state_si:  /* only 'B' is allowed */
                if (*value == 'B') {
                    state = state_end;
                } else {
                    goto error;
                }
                break;
            case state_end: /* end of state-machine */
                goto error;
        }

        value++;
        remains--;
    }

    return res;
error:
    parser->col_num = token->col;
    parsing_error(parser, "unrecognizable data unit", token);

    return U64_MAX;
}

static int token_split(struct token *src, int limit, char delimiter, struct token *splited)
{
    int offset = 0;
    int count = 0;
    struct token *cur_token;

    while(offset < src->len) {
        cur_token = &splited[count];
        count++;
        if (count > limit) {
            return count;
        }

        cur_token->value = &src->value[offset];
        cur_token->col = src->col + offset;
        cur_token->len = 0;

        while ((delimiter != src->value[offset]) && (++offset < src->len));
        cur_token->len = offset - (cur_token->col - src->col);
        /* skip delimiter */
        offset++;
    }

    return count;
}

static int parse_op_type(struct parser *parser, struct token *token)
{
    int res;

    token_strupr(token);
    if (MATCH_TOKEN(token, "READ") || MATCH_TOKEN(token, "R")) {
        res = bio_read;
    } else if (MATCH_TOKEN(token, "WRITE") || MATCH_TOKEN(token, "W")) {
        res = bio_write;
    } else if (MATCH_TOKEN(token, "FLUSH") || MATCH_TOKEN(token, "F")) {
        res = bio_flush;
    } else if (MATCH_TOKEN(token, "DEALLOCATE") || MATCH_TOKEN(token, "DEALLOC") || MATCH_TOKEN(token, "D")) {
        res = bio_deallocate;
    } else if (MATCH_TOKEN(token, "WRITE_ZEROES") || MATCH_TOKEN(token, "WRITE_ZEROS") || MATCH_TOKEN(token, "WRITE_ZERO") || MATCH_TOKEN(token, "WZ") || MATCH_TOKEN(token, "Z")) {
        res = bio_write_zeroes;
    } else if (MATCH_TOKEN(token, "WRITE_UNCORRECTABLE") || MATCH_TOKEN(token, "WRITE_UNCORRECT") || MATCH_TOKEN(token, "WRITE_UNCOR") || MATCH_TOKEN(token, "WU") || MATCH_TOKEN(token, "U")) {
        res = bio_write_uncorrect;
    } else if (MATCH_TOKEN(token, "IDENTIFY")) {
        res = bio_identify;
    } else if (MATCH_TOKEN(token, "FW_DOWNLOAD")) {
        res = bio_fw_download;
    } else {
        parsing_error(parser, "unrecognizable operation type", token);
        res = -1;
    }

    return res;
}

static void parse_operation(struct parser *parser, struct token *value_token)
{
    struct token param_tokens[10];
    struct token subparam_tokens[2];
    int num_op_tokens;
    int num_rate_tokens;
    int token_idx;
    unsigned int remained_rate;
    unsigned int rate;
    int optype;
    unsigned int optype_mask;

    remained_rate = 100;
    optype_mask = 0;
    parser->job_params->operations[0] = 0;
    parser->job_params->operations[1] = 0;
    parser->job_params->operations[2] = 0;
    parser->job_params->operations[3] = 0;
    parser->job_params->operations[4] = 0;
    parser->job_params->operations[5] = 0;
    parser->job_params->operations[6] = 0;
    parser->job_params->operations[7] = 0;

    num_op_tokens = token_split(value_token, 10, PARAM_DELIMITER, param_tokens);
    for (token_idx = 0; token_idx < num_op_tokens; token_idx++) {
        num_rate_tokens = token_split(&param_tokens[token_idx], 2, SUBPARAM_DELIMITER, subparam_tokens);
        optype = parse_op_type(parser, &subparam_tokens[0]);
        if (optype_mask & (1 << optype)) {
            parsing_error(parser, "operation redefinition", NULL);
        }
        optype_mask |= 1 << optype;
        if (num_rate_tokens == 1) {
            parser->job_params->operations[optype] = remained_rate;
            remained_rate = 0;
        } else {
            rate = to_integer(parser, &subparam_tokens[1]);
            if (rate > remained_rate) {
                parsing_error(parser, "total % of operations should equal to 100", NULL);
            }
            parser->job_params->operations[optype] = rate;
            remained_rate -= rate;
        }
    }

    if (remained_rate > 0) {
        parsing_error(parser, "total % of operations should equal to 100", NULL);
    }

    PARSER_LOG(parser,
               "OPERATION=%s:%u,%s:%u,%s:%u,%s:%u,%s:%u,%s:%u,%s:%u,%s:%u\n",
               "READ",
               parser->job_params->operations[0],
               "WRITE",
               parser->job_params->operations[1],
               "FLUSH",
               parser->job_params->operations[2],
               "DEALLOCATE",
               parser->job_params->operations[3],
               "WRITE_ZEROES",
               parser->job_params->operations[4],
               "WRITE_UNCORRECTABLE",
               parser->job_params->operations[5],
               "IDENTIFY",
               parser->job_params->operations[6],
               "FW_DOWNLOAD",
               parser->job_params->operations[7]);
}

static void parse_amount(struct parser *parser, struct token *value_token)
{
    const char *amount_type_names[] = { "INFINITE", "TIME", "IO", "BYTE" };
    struct token tokens[3];
    int num_tokens;

    token_strupr(value_token);
    num_tokens = token_split(value_token, 3, SUBPARAM_DELIMITER, tokens);

    if (MATCH_TOKEN(&tokens[0], amount_type_names[0])) {
        parser->job_params->amount_type = amount_type_infinite;
    } else if (MATCH_TOKEN(&tokens[0], amount_type_names[1])) {
        parser->job_params->amount_type = amount_type_time;
        if (num_tokens != 2) {
            parsing_error(parser, "parameter 'amount of time' is required", NULL);
        }
        parser->job_params->amount.seconds = to_seconds(parser, &tokens[1]);
    } else if (MATCH_TOKEN(&tokens[0], amount_type_names[2])) {
        parser->job_params->amount_type = amount_type_io;
        if (num_tokens != 2) {
            parsing_error(parser, "parameter 'amount of ios' is required", NULL);
        }
        parser->job_params->amount.ios = to_integer(parser, &tokens[1]);
    } else if (MATCH_TOKEN(&tokens[0], amount_type_names[3])) {
        parser->job_params->amount_type = amount_type_byte;
        if (num_tokens != 2) {
            parsing_error(parser, "parameter 'amount of bytes' is required", NULL);
        }
        if (tokens[1].value[tokens[1].len - 1] == '%') {
            parser->job_params->amount_bytes_percent_enabled = 1;
            parser->job_params->amount_bytes_percent = to_percent(parser, &tokens[1]);
        } else {
            parser->job_params->amount.bytes = to_bytes(parser, &tokens[1]);
        }
    } else {
        parsing_error(parser, "invalid i/o amount unit", &tokens[0]);
    }
    if (parser->verbose) {
        printf("AMOUNT=%s", amount_type_names[parser->job_params->amount_type]);
        if (parser->job_params->amount_type == amount_type_infinite) {
            printf("\n");
        } else if (parser->job_params->amount_type == amount_type_time) {
            printf(":%lluS\n", parser->job_params->amount.seconds);
        } else if (parser->job_params->amount_type == amount_type_io) {
            printf(":%llu\n", parser->job_params->amount.ios);
        } else if (parser->job_params->amount_type == amount_type_byte) {
            if (parser->job_params->amount_bytes_percent_enabled) {
                printf(":%u%%\n", parser->job_params->amount_bytes_percent);
            } else {
                printf(":%lluB\n", parser->job_params->amount.bytes);
            }
        }

    }
}

static void parse_unwr_read(struct parser *parser, struct token *value_token)
{
    const char *result_type_names[] = { "DONTCARE", "AUTO", "ERROR", "VALUE" };
    struct token tokens[3];
    int num_tokens;

    token_strupr(value_token);
    num_tokens = token_split(value_token, 3, SUBPARAM_DELIMITER, tokens);

    if (MATCH_TOKEN(&tokens[0], result_type_names[0])) {
        parser->job_params->unwr_read_result_type = unwr_read_result_type_dontcare;
    } else if (MATCH_TOKEN(&tokens[0], result_type_names[1])) {
        parser->job_params->unwr_read_result_type = unwr_read_result_type_auto;
    } else if (MATCH_TOKEN(&tokens[0], result_type_names[2])) {
        parser->job_params->unwr_read_result_type = unwr_read_result_type_error;
    } else if (MATCH_TOKEN(&tokens[0], result_type_names[3])) {
        parser->job_params->unwr_read_result_type = unwr_read_result_type_value;
        if (num_tokens != 2) {
            parsing_error(parser, "value type must be specified", NULL);
        }
        if (MATCH_TOKEN(&tokens[1], "0")) {
            parser->job_params->unwr_read_value_type = unwr_read_value_type_zeroes;
        } else if (MATCH_TOKEN(&tokens[1], "1")) {
            parser->job_params->unwr_read_value_type = unwr_read_value_type_ones;
        } else {
            parsing_error(parser, "invalid value type, '0' or '1' is allowed", &tokens[1]);
        }
    } else {
        parsing_error(parser, "invalid result type", &tokens[0]);
    }
    if (parser->verbose) {
        printf("UNWR_READ=%s", result_type_names[parser->job_params->unwr_read_result_type]);
        if (parser->job_params->unwr_read_result_type == unwr_read_result_type_value) {
            if (parser->job_params->unwr_read_value_type == unwr_read_value_type_zeroes) {
                printf(":0\n");
            } else {
                printf(":1\n");
            }
        } else {
            printf("\n");
        }
    }
}

static void parse_dealloc_read(struct parser *parser, struct token *value_token)
{
    const char *result_type_names[] = { "DONTCARE", "AUTO", "ERROR", "VALUE" };
    struct token tokens[3];
    int num_tokens;

    token_strupr(value_token);
    num_tokens = token_split(value_token, 3, SUBPARAM_DELIMITER, tokens);

    if (MATCH_TOKEN(&tokens[0], result_type_names[0])) {
        parser->job_params->dealloc_read_result_type = dealloc_read_result_type_dontcare;
    } else if (MATCH_TOKEN(&tokens[0], result_type_names[1])) {
        parser->job_params->dealloc_read_result_type = dealloc_read_result_type_auto;
    } else if (MATCH_TOKEN(&tokens[0], result_type_names[2])) {
        parser->job_params->dealloc_read_result_type = dealloc_read_result_type_error;
    } else if (MATCH_TOKEN(&tokens[0], result_type_names[3])) {
        parser->job_params->dealloc_read_result_type = dealloc_read_result_type_value;
        if (num_tokens != 2) {
            parsing_error(parser, "value type must be specified", NULL);
        }
        if (MATCH_TOKEN(&tokens[1], "0")) {
            parser->job_params->dealloc_read_value_type = dealloc_read_value_type_zeroes;
        } else if (MATCH_TOKEN(&tokens[1], "1")) {
            parser->job_params->dealloc_read_value_type = dealloc_read_value_type_ones;
        } else {
            parsing_error(parser, "invalid value type, '0' or '1' is allowed", &tokens[1]);
        }
    } else {
        parsing_error(parser, "invalid result type", &tokens[0]);
    }
    if (parser->verbose) {
        printf("UNWR_READ=%s", result_type_names[parser->job_params->dealloc_read_result_type]);
        if (parser->job_params->dealloc_read_result_type == dealloc_read_result_type_value) {
            if (parser->job_params->dealloc_read_value_type == dealloc_read_value_type_zeroes) {
                printf(":0\n");
            } else {
                printf(":1\n");
            }
        } else {
            printf("\n");
        }
    }
}
static void parse_iosize(struct parser *parser, struct token *value_token, int optype, struct token *key_token)
{
    struct token tokens[3];
    int num_tokens;
    u64 val;

    num_tokens = token_split(value_token, 3, PARAM_RANGE_DELIMITER, tokens);

    if (num_tokens == 1) {
        val = to_bytes(parser, &tokens[0]);
        parser->job_params->iosizes[optype].iosize_min = val;
        parser->job_params->iosizes[optype].iosize_max = val;
    } else if (num_tokens == 2) {
        val = to_bytes(parser, &tokens[0]);
        parser->job_params->iosizes[optype].iosize_min = val;
        val = to_bytes(parser, &tokens[1]);
        parser->job_params->iosizes[optype].iosize_max = val;
        if (parser->job_params->iosizes[optype].iosize_min > parser->job_params->iosizes[optype].iosize_max) {
            SWAP(parser->job_params->iosizes[optype].iosize_max,
                 parser->job_params->iosizes[optype].iosize_min);
        }
    } else {
        parsing_error(parser, "invalid i/o size format", value_token);
    }
    if (optype == iosize_read || optype == iosize_write || optype == iosize_total) {
        iosize_max = MAX(iosize_max, parser->job_params->iosizes[optype].iosize_max);
        iosize_min = MIN(iosize_min, parser->job_params->iosizes[optype].iosize_min);
    }

    PARSER_LOG(parser, "%.*s=%llu,%llu\n", key_token->len, key_token->value, parser->job_params->iosizes[optype].iosize_min, parser->job_params->iosizes[optype].iosize_max);
}
static void parse_slba(struct parser *parser, struct token *value_token)
{
    const char *slba_type_names[] = { "SEQUENTIAL", "UNIFORM", "ZIPF", "PARETO", "FIXED", "JESD219", "REPLAY" };
    struct token tokens[4];
    int num_tokens;

    token_strupr(value_token);
    num_tokens = token_split(value_token, 4, SUBPARAM_DELIMITER, tokens);

    if (MATCH_TOKEN(&tokens[0], "SEQ") || MATCH_TOKEN(&tokens[0], "SEQUENTIAL")) {
        parser->job_params->slba_type = slba_type_seq;
        if (num_tokens == 1) {
            parser->job_params->slba.fixed.lba = 0;
        } else {
            parser->job_params->slba.fixed.lba = to_integer(parser, &tokens[1]);
        }
    } else if (MATCH_TOKEN(&tokens[0], "RANDOM") || MATCH_TOKEN(&tokens[0], "UNIFORM")) {
        parser->job_params->slba_type = slba_type_uniform;
    } else if (MATCH_TOKEN(&tokens[0], "ZIPF")) {
        parser->job_params->slba_type = slba_type_zipf;
        if (num_tokens == 1) {
            parser->job_params->slba.zipf.theta = ZIPF_THETA_DEF;
        } else if (num_tokens == 2) {
            parser->job_params->slba.zipf.theta = to_double(parser, &tokens[1]);
        } else {
            parsing_error(parser, "zipf distribution has only one parameter", value_token);
        }
    } else if (MATCH_TOKEN(&tokens[0], "PARETO")) {
        parser->job_params->slba_type = slba_type_pareto;
        if (num_tokens == 1) {
            parser->job_params->slba.pareto.scale = PARETO_SCALE_DEF;
            parser->job_params->slba.pareto.shape = PARETO_SHAPE_DEF;
        } else if (num_tokens == 3) {
            parser->job_params->slba.pareto.scale = to_double(parser, &tokens[1]);
            parser->job_params->slba.pareto.shape = to_double(parser, &tokens[2]);
        } else {
            parsing_error(parser, "pareto distribution has two parameters", value_token);
        }
    } else if (MATCH_TOKEN(&tokens[0], "FIXED")) {
        parser->job_params->slba_type = slba_type_fixed;
        if (num_tokens == 1) {
            parser->job_params->slba.fixed.lba = 0;
        } else {
            parser->job_params->slba.fixed.lba = to_integer(parser, &tokens[1]);
        }
    } else if (MATCH_TOKEN(&tokens[0], "JESD219")) {
        parser->job_params->slba_type = slba_type_jesd219;
        if (num_tokens == 1) {
            parser->job_params->slba.jesd219.enterprise = true;
            parser->job_params->slba.jesd219.sector_size = 512;
        } else {
            parser->job_params->slba.jesd219.enterprise = to_bool(parser, &tokens[1]);
            parser->job_params->slba.jesd219.sector_size = to_integer(parser, &tokens[2]);
        }
    } else if (MATCH_TOKEN(&tokens[0], "REPLAY")) {
        parser->job_params->slba_type = slba_type_replay;
    }

    if (parser->verbose) {
        printf("SLBA=%s", slba_type_names[parser->job_params->slba_type]);
        switch (parser->job_params->slba_type) {
            case slba_type_seq:
                printf("\n");
                break;
            case slba_type_uniform:
                printf("\n");
                break;
            case slba_type_zipf:
                printf(":%lf\n", parser->job_params->slba.zipf.theta);
                break;
            case slba_type_pareto:
                printf(":%lf:%lf\n", parser->job_params->slba.pareto.shape, parser->job_params->slba.pareto.scale);
                break;
            case slba_type_fixed:
                printf(":%llu\n", parser->job_params->slba.fixed.lba);
                break;
            case slba_type_jesd219:
                printf(":ENT %s\n", bool_strings[parser->job_params->slba.jesd219.enterprise]);
                printf(":SECTOR_SIZE: %d\n", parser->job_params->slba.jesd219.sector_size);
                break;
            case slba_type_replay:
                printf("\n");
                break;
        }
    }
}

static void parse_lba_range(struct parser *parser, struct token *value_token)
{
    struct token tokens[3];
    int num_tokens;

    token_strupr(value_token);
    num_tokens = token_split(value_token, 3, SUBPARAM_DELIMITER, tokens);
    if (num_tokens != 2) {
        parser->col_num = value_token->col;
        parsing_error(parser, "invalid range format", value_token);
    }
    if (tokens[0].value[tokens[0].len - 1] == '%') {
        parser->job_params->lba_range_begin_percent_enable = 1;
        parser->job_params->lba_range_begin_percent = to_percent(parser, &tokens[0]);
    } else {
        parser->job_params->lba_range_begin_percent_enable = 0;
        parser->job_params->lba_range_begin = to_bytes(parser, &tokens[0]);
    }
    if (tokens[1].value[tokens[1].len - 1] == '%') {
        parser->job_params->lba_range_size_percent_enable = 1;
        parser->job_params->lba_range_size_percent = to_percent(parser, &tokens[1]);
    } else {
        parser->job_params->lba_range_size_percent_enable = 0;
        parser->job_params->lba_range_size = to_bytes(parser, &tokens[1]);
    }
    if (parser->verbose) {
        printf("LBA_RANGE=");
        if (parser->job_params->lba_range_begin_percent_enable) {
            printf("%u%%:", parser->job_params->lba_range_begin_percent);
        } else {
            printf("%llu:", parser->job_params->lba_range_begin);
        }
        if (parser->job_params->lba_range_size_percent_enable) {
            printf("%u%%\n", parser->job_params->lba_range_size_percent);
        } else {
            printf("%llu\n", parser->job_params->lba_range_size);
        }
    }
}

static void parse_reset(struct parser *parser, struct token *value_token)
{
    struct token tokens[3];
    int num_tokens;
    u64 val;

    num_tokens = token_split(value_token, 3, PARAM_RANGE_DELIMITER, tokens);
    if (num_tokens == 1) {
        val = to_integer(parser, &tokens[0]);
        parser->job_params->reset_min = val;
        parser->job_params->reset_max = val;
    } else if (num_tokens == 2) {
        val = to_integer(parser, &tokens[0]);
        parser->job_params->reset_min = val;
        val = to_integer(parser, &tokens[1]);
        parser->job_params->reset_max = val;
        if (parser->job_params->reset_min > parser->job_params->reset_max) {
            SWAP(parser->job_params->reset_max,
                 parser->job_params->reset_min);
        }
    } else {
        parsing_error(parser, "invalid reset period format", value_token);
    }

    PARSER_LOG(parser, "RESET=%u,%u\n", parser->job_params->reset_min, parser->job_params->reset_max);
}

static void parse_stream(struct parser *parser, struct token *value_token)
{
    struct token tokens[1];
    int num_tokens;

    num_tokens = token_split(value_token, 1, PARAM_RANGE_DELIMITER, tokens);
    if (num_tokens == 1) {
        parser->job_params->num_streams = to_integer(parser, &tokens[0]);
    } else {
        parsing_error(parser, "invalid stream format", value_token);
    }

    PARSER_LOG(parser, "STREAM=%u\n", parser->job_params->num_streams);
}

static void on_property_defined(struct parser *parser, struct token *tokens)
{
    struct token *key_token;
    struct token *value_token;

    key_token = &tokens[0];
    value_token = &tokens[1];
    token_strupr(key_token);

    if (MATCH_TOKEN(key_token, "STOP_ON_ERROR")) {
        parser->job_params->stop_on_error = to_bool(parser, value_token);
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, bool_strings[parser->job_params->stop_on_error]);
    } else if (MATCH_TOKEN(key_token, "STOP_ON_MISCOMPARE")) {
        parser->job_params->stop_on_miscompare = to_bool(parser, value_token);
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, bool_strings[parser->job_params->stop_on_miscompare]);
    } else if (MATCH_TOKEN(key_token, "COMPARE")) {
        parser->job_params->compare = to_bool(parser, value_token);
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, bool_strings[parser->job_params->compare]);
    } else if (MATCH_TOKEN(key_token, "INJECT_MISCOMPARE")) {
        parser->job_params->inject_miscompare = to_double(parser, value_token);
        PARSER_LOG(parser, "%.*s=%lf\n", key_token->len, key_token->value, parser->job_params->inject_miscompare);
    } else if (MATCH_TOKEN(key_token, "COMPARE_HEADER_ONLY")) {
        parser->job_params->compare_header_only = to_bool(parser, value_token);
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, bool_strings[parser->job_params->compare_header_only]);
    } else if (MATCH_TOKEN(key_token, "IOENGINE")) {
        memcpy(parser->job_params->ioengine, value_token->value, value_token->len);
        parser->job_params->ioengine[value_token->len] = 0;
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, parser->job_params->ioengine);
    } else if (MATCH_TOKEN(key_token, "IOENGINE_PARAM")) {
        memcpy(parser->job_params->ioengine_param, value_token->value, value_token->len);
        parser->job_params->ioengine_param[value_token->len] = 0;
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, parser->job_params->ioengine_param);
    } else if (MATCH_TOKEN(key_token, "CONTROLLER")) {
        memcpy(parser->job_params->controller, value_token->value, value_token->len);
        parser->job_params->controller[value_token->len] = 0;
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, parser->job_params->controller);
    } else if (MATCH_TOKEN(key_token, "NSID")) {
        parser->job_params->nsid = to_integer(parser, value_token);
        PARSER_LOG(parser, "%.*s=%u\n", key_token->len, key_token->value, parser->job_params->nsid);
    } else if (MATCH_TOKEN(key_token, "DEPENDENCY")) {
        memcpy(parser->job_params->dependency, value_token->value, value_token->len);
        parser->job_params->dependency[value_token->len] = 0;
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, parser->job_params->dependency);
    } else if (MATCH_TOKEN(key_token, "DELAY")) {
        parser->job_params->delay = to_seconds(parser, value_token);;
        PARSER_LOG(parser, "%.*s=%llu\n", key_token->len, key_token->value, parser->job_params->delay);
    } else if (MATCH_TOKEN(key_token, "IOLOG")) {
        memcpy(parser->params->iolog, value_token->value, value_token->len);
        parser->params->iolog[value_token->len] = 0;
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, parser->params->iolog);
    } else if (MATCH_TOKEN(key_token, "SMARTLOG")) {
        memcpy(parser->params->smartlog, value_token->value, value_token->len);
        parser->params->smartlog[value_token->len] = 0;
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, parser->params->smartlog);
    } else if (MATCH_TOKEN(key_token, "ERRORLOG")) {
        memcpy(parser->params->errorlog, value_token->value, value_token->len);
        parser->params->errorlog[value_token->len] = 0;
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, parser->params->errorlog);
    } else if (MATCH_TOKEN(key_token, "IOTRACE")) {
        memcpy(parser->params->iotrace, value_token->value, value_token->len);
        parser->params->iotrace[value_token->len] = 0;
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, parser->params->iotrace);
    } else if (MATCH_TOKEN(key_token, "JOURNAL_FILE")) {
        if (parser->job_params->journal_shm) {
            parser->col_num = key_token->col;
            parsing_error(parser, "JOURNAL_FILE and JOURNAL_SHM are mutually exclusive", key_token);
        }
        memcpy(parser->job_params->journal_file, value_token->value, value_token->len);
        parser->job_params->journal_file[value_token->len] = 0;
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, parser->job_params->journal_file);
    } else if (MATCH_TOKEN(key_token, "JOURNAL_SHM")) {
        if (parser->job_params->journal_file[0]) {
            parser->col_num = key_token->col;
            parsing_error(parser, "JOURNAL_FILE and JOURNAL_SHM are mutually exclusive", key_token);
        }
        parser->job_params->journal_shm = (int)to_integer(parser, value_token);
        PARSER_LOG(parser, "%.*s=%d\n", key_token->len, key_token->value, parser->job_params->journal_shm);
    } else if (MATCH_TOKEN(key_token, "LBA_OVERLAP")) {
        parser->job_params->lba_overlap = to_bool(parser, value_token);
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, bool_strings[parser->job_params->lba_overlap]);
    } else if (MATCH_TOKEN(key_token, "RANDOMIZE_HEADER")) {
        parser->job_params->randomize_header = to_bool(parser, value_token);
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, bool_strings[parser->job_params->randomize_header]);
    } else if (MATCH_TOKEN(key_token, "SEED")) {
        parser->job_params->seed = to_integer(parser, value_token);
        PARSER_LOG(parser, "%.*s=%u\n", key_token->len, key_token->value, parser->job_params->seed);
    } else if (MATCH_TOKEN(key_token, "IODEPTH")) {
        parser->job_params->iodepth = to_integer(parser, value_token);
        PARSER_LOG(parser, "%.*s=%u\n", key_token->len, key_token->value, parser->job_params->iodepth);
    } else if (MATCH_TOKEN(key_token, "BATCH_SUBMIT")) {
        parser->job_params->batch_submit = to_integer(parser, value_token);
        PARSER_LOG(parser, "%.*s=%u\n", key_token->len, key_token->value, parser->job_params->batch_submit);
    } else if (MATCH_TOKEN(key_token, "BATCH_COMPLETE")) {
        parser->job_params->batch_complete = to_integer(parser, value_token);
        PARSER_LOG(parser, "%.*s=%u\n", key_token->len, key_token->value, parser->job_params->batch_complete);
    } else if (MATCH_TOKEN(key_token, "IOSIZE")) {
        parse_iosize(parser, value_token, iosize_total, key_token);
    } else if (MATCH_TOKEN(key_token, "IOSIZE_ALIGN")) {
        parser->job_params->iosizes[iosize_total].iosize_align = to_bytes(parser, value_token);
        PARSER_LOG(parser, "%.*s=%u\n", key_token->len, key_token->value, parser->job_params->iosizes[iosize_total].iosize_align);
    } else if (MATCH_TOKEN(key_token, "WRITE_SIZE")) {
        parse_iosize(parser, value_token, iosize_write, key_token);
    } else if (MATCH_TOKEN(key_token, "WRITE_SIZE_ALIGN")) {
        parser->job_params->iosizes[iosize_write].iosize_align = to_bytes(parser, value_token);
        PARSER_LOG(parser, "%.*s=%u\n", key_token->len, key_token->value, parser->job_params->iosizes[iosize_write].iosize_align);
    } else if (MATCH_TOKEN(key_token, "READ_SIZE")) {
        parse_iosize(parser, value_token, iosize_read, key_token);
    } else if (MATCH_TOKEN(key_token, "READ_SIZE_ALIGN")) {
        parser->job_params->iosizes[iosize_read].iosize_align = to_bytes(parser, value_token);
        PARSER_LOG(parser, "%.*s=%u\n", key_token->len, key_token->value, parser->job_params->iosizes[iosize_read].iosize_align);
    } else if (MATCH_TOKEN(key_token, "DEALLOC_SIZE")) {
        parse_iosize(parser, value_token, iosize_deallocate, key_token);
    } else if (MATCH_TOKEN(key_token, "DEALLOC_SIZE_ALIGN")) {
        parser->job_params->iosizes[iosize_deallocate].iosize_align = to_bytes(parser, value_token);
        PARSER_LOG(parser, "%.*s=%u\n", key_token->len, key_token->value, parser->job_params->iosizes[iosize_deallocate].iosize_align);
    } else if (MATCH_TOKEN(key_token, "WRITE_ZEROES_SIZE")) {
        parse_iosize(parser, value_token, iosize_write_zeroes, key_token);
    } else if (MATCH_TOKEN(key_token, "WRITE_ZEROES_SIZE_ALIGN")) {
        parser->job_params->iosizes[iosize_write_zeroes].iosize_align = to_bytes(parser, value_token);
        PARSER_LOG(parser, "%.*s=%u\n", key_token->len, key_token->value, parser->job_params->iosizes[iosize_write_zeroes].iosize_align);
    } else if (MATCH_TOKEN(key_token, "WRITE_UNCOR_SIZE")) {
        parse_iosize(parser, value_token, iosize_write_uncorrect, key_token);
    } else if (MATCH_TOKEN(key_token, "WRITE_UNCOR_SIZE_ALIGN")) {
        parser->job_params->iosizes[iosize_write_uncorrect].iosize_align = to_bytes(parser, value_token);
        PARSER_LOG(parser, "%.*s=%u\n", key_token->len, key_token->value, parser->job_params->iosizes[iosize_write_uncorrect].iosize_align);
    } else if (MATCH_TOKEN(key_token, "SLBA")) {
        parse_slba(parser, value_token);
    } else if (MATCH_TOKEN(key_token, "SLBA_ALIGN")) {
        parser->job_params->slba_align = to_bytes(parser, value_token);
        PARSER_LOG(parser, "%.*s=%u\n", key_token->len, key_token->value, parser->job_params->slba_align);
    } else if (MATCH_TOKEN(key_token, "SEPARATE_STREAM")) {
        parser->job_params->separate_stream = to_bool(parser, value_token);
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, bool_strings[parser->job_params->separate_stream]);
    } else if (MATCH_TOKEN(key_token, "LBA_RANGE")) {
        parse_lba_range(parser, value_token);
    } else if (MATCH_TOKEN(key_token, "OPERATION")) {
        parse_operation(parser, value_token);
    } else if (MATCH_TOKEN(key_token, "AMOUNT")) {
        parse_amount(parser, value_token);
    } else if (MATCH_TOKEN(key_token, "AMOUNT_SCALE")) {
        parser->job_params->amount_scale = to_integer(parser, value_token);
        PARSER_LOG(parser, "%.*s=%u\n", key_token->len, key_token->value, parser->job_params->amount_scale);
    } else if (MATCH_TOKEN(key_token, "UNWR_READ")) {
        parse_unwr_read(parser, value_token);
    } else if (MATCH_TOKEN(key_token, "DEALLOC_READ")) {
        parse_dealloc_read(parser, value_token);
    } else if (MATCH_TOKEN(key_token, "FLUSH_PASSTHRU")) {
        parser->job_params->flush_passthru = to_bool(parser, value_token);
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, bool_strings[parser->job_params->flush_passthru]);
    } else if (MATCH_TOKEN(key_token, "RESET")) {
        parse_reset(parser, value_token);
    } else if (MATCH_TOKEN(key_token, "STREAM")) {
        parse_stream(parser, value_token);
    } else if (MATCH_TOKEN(key_token, "PRACT")) {
        parser->job_params->pract = to_double(parser, value_token);
        PARSER_LOG(parser, "%.*s=%lf\n", key_token->len, key_token->value, parser->job_params->pract);
    } else if (MATCH_TOKEN(key_token, "PRCHK")) {
        parser->job_params->prchk = to_integer(parser, value_token);
        PARSER_LOG(parser, "%.*s=%u\n", key_token->len, key_token->value, parser->job_params->prchk);
    } else if (MATCH_TOKEN(key_token, "INJECT_TAG_ERROR")) {
        parser->job_params->inject_tag_error = to_double(parser, value_token);
        PARSER_LOG(parser, "%.*s=%lf\n", key_token->len, key_token->value, parser->job_params->inject_tag_error);
    } else if (MATCH_TOKEN(key_token, "TIMEOUT")) {
        parser->job_params->timeout = to_integer(parser, value_token);
        PARSER_LOG(parser, "%.*s=%u\n", key_token->len, key_token->value, parser->job_params->timeout);
    } else if (MATCH_TOKEN(key_token, "STOP_ON_TIMEOUT")) {
        parser->job_params->stop_on_timeout = to_bool(parser, value_token);
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, bool_strings[parser->job_params->stop_on_timeout]);
    } else if (MATCH_TOKEN(key_token, "POWERCTL_DEV")) {
        memcpy(parser->params->powerctl_dev, value_token->value, value_token->len);
        PARSER_LOG(parser, "%.*s=%s\n", key_token->len, key_token->value, parser->params->powerctl_dev);
    } else if (MATCH_TOKEN(key_token, "SPOR_LEVEL")) {
        parser->params->spor_level = to_integer(parser, value_token);
        PARSER_LOG(parser, "%.*s=%u\n", key_token->len, key_token->value, parser->params->spor_level);
    } else if (MATCH_TOKEN(key_token, "SKIP_IOENGINE_EXIT")) {
        parser->params->skip_ioengine_exit = to_integer(parser, value_token);
        PARSER_LOG(parser, "%.*s=%u\n", key_token->len, key_token->value, parser->params->skip_ioengine_exit);
    } else if (MATCH_TOKEN(key_token, "HISTORY")) {
        parser->job_params->history = to_integer(parser, value_token);
        PARSER_LOG(parser, "%.*s=%u\n", key_token->len, key_token->value, parser->job_params->history);
    } else {
        parser->col_num = key_token->col;
        parsing_error(parser, "invalid property key", key_token);
    }
}

static int parser_next_state(int cur_state, unsigned char ch)
{
    return parser_state_table[cur_state][ch];
}

static void parser_newline(struct parser *parser)
{
    parser->state = parser_state_init;
    parser->col_num = 0;
    parser->cur_token_idx = 0;
}

void syntax_error(struct parser *parser, int res)
{
    if (res == parser_syntax_error) {
        switch (parser->state) {
            case parser_state_prop_key:
                parsing_error(parser, "expected = token", NULL);
                break;
            case parser_state_prop_value:
                parsing_error(parser, "expected endline token", NULL);
                break;
            case parser_state_prop_equal:
                parsing_error(parser, "expected value token", NULL);
                break;
            case parser_state_section_opener:
                parsing_error(parser, "expected section identifier token", NULL);
                break;
            case parser_state_section_name:
                parsing_error(parser, "expected ] token", NULL);
                break;
            case parser_state_env_dollar:
                parsing_error(parser, "expected { token", NULL);
                break;
            case parser_state_env_opener:
                parsing_error(parser, "expected environment variable token", NULL);
                break;
            case parser_state_env_key:
                parsing_error(parser, "expected } token", NULL);
                break;
            case parser_state_env_closer:
                parsing_error(parser, "expected endline token", NULL);
                break;
            default:
                parsing_error(parser, "unrecognizable character!", NULL);
                break;
        }
    } else if (res == parser_long_token) {
        fprintf(stderr,
                "%s:%d:%d: error: ",
                parser->file_name, parser->line_num + 1, parser->col_num + 1);
        fprintf(stderr, "too long token '%.*s...'\n", 8, parser->cur_token->value);
        fprintf(stderr, "the maximum length of token is %d\n", MAX_TOKEN_LEN - 1);
    }
}

static int parse_char(struct parser *parser, unsigned char ch)
{
    enum parser_state new_state;

    new_state = parser_next_state(parser->state, ch);
    if (new_state == parser_state_error) {
        return parser_syntax_error;
    }
    if (new_state != parser->state) {
        switch(new_state) {
            case parser_state_section_name:
            case parser_state_prop_key:
            case parser_state_prop_value:
            case parser_state_env_dollar:
                parser->cur_token = &parser->tokens[parser->cur_token_idx++];
                parser->cur_token->col = parser->col_num;
                parser->cur_token->len = 0;
                parser->cur_token->value = &parser->line_buf[parser->col_num];
                break;
            case parser_state_env_opener:
            case parser_state_env_key:
            case parser_state_env_closer:
                break;
            default:
                if (parser->cur_token) {
                    switch (parser->state) {
                        case parser_state_prop_value:
                            on_property_defined(parser, parser->cur_token - 1);
                            break;
                        case parser_state_section_name:
                            on_section_defined(parser, parser->cur_token);
                            break;
                        case parser_state_env_closer:
                            on_env_refered(parser, parser->cur_token);
                            on_property_defined(parser, parser->cur_token - 1);
                            break;
                        default:
                            break;
                    }
                    parser->cur_token = NULL;
                }
                break;
        }
        parser->end_of_token = 0;
    } else {
        if (parser->end_of_token) {
            syntax_error(parser, parser_syntax_error);
            exit(-1);
        }
    }

    if (parser->cur_token) {
        parser->cur_token->len++;
        if (parser->cur_token->len >= MAX_TOKEN_LEN) {
            return parser_long_token;
        }
    }

    parser->state = new_state;

    return parser_success;
}

static int read_line(FILE *file, int max_line_byte, char *line)
{
    int ch;
    int col = 0;

    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\r') {
            /* treat carriage return as end-of-line */
            line[col] = 0;
            col++;
        } else if (ch == '\n') {
            line[col] = 0;
            break;
        } else if ((ch == '#') || (ch == ';')) {
            line[col] = 0;
            /* ignore comment until the next line feed */
            while ((ch = fgetc(file)) != EOF) {
                if (ch == '\n') {
                    break;
                }
            }
            break;
        } else {
            line[col] = ch;
            col++;
        }
        if (col == max_line_byte) {
            /* line buffer overflow */
            return -1;
        }
    }

    return col;
}

static int parse_line(struct parser *parser)
{
    int res;

    parser_newline(parser);

    do {
        if (isspace(parser->line_buf[parser->col_num])) {
            parser->end_of_token = 1;
        } else {
            res = parse_char(parser, parser->line_buf[parser->col_num]);
            if (res != parser_success) {
                syntax_error(parser, res);
                return -1;
            }
        }
    } while (parser->line_buf[parser->col_num++]);

    return 0;
}

int parse_param(const char *param_file, struct global_params *params, int verbose)
{
    FILE *file;
    int line_bytes;
    struct parser parser;

    file = fopen(param_file, "r");
    if (file == 0) {
        fprintf(stderr, "could not open parameter file '%s'\n", param_file);
        return -1;
    }

    parser.verbose = verbose;
    parser.file_name = param_file;
    parser.line_num = 0;
    parser.params = params;
    memset(params, 0, sizeof (struct global_params));
    parser.job_params = NULL;
    parser.cur_token = NULL;
    parser.end_of_token = 0;
    parser.job_params = &parser.default_job_params;
    memset(&parser.default_job_params, 0, sizeof (struct job_params));
    strcpy(parser.default_job_params.name, "NONAME");
    parser.default_job_params.iodepth = 1;
    parser.default_job_params.batch_complete = 1;
    parser.default_job_params.lba_range_begin_percent_enable = 0;
    parser.default_job_params.lba_range_begin = 0;
    parser.default_job_params.lba_range_size_percent_enable = 1;
    parser.default_job_params.lba_range_size_percent = 100;
    parser.default_job_params.unwr_read_result_type = unwr_read_result_type_value;
    parser.default_job_params.unwr_read_value_type = unwr_read_value_type_ones;
    parser.default_job_params.dealloc_read_result_type = dealloc_read_result_type_value;
    parser.default_job_params.dealloc_read_value_type = dealloc_read_value_type_ones;
    parser.default_job_params.pract = 0.5;
    parser.default_job_params.prchk = 7;
    parser.default_job_params.amount_scale = 1;
    parser.default_job_params.iosizes[iosize_read].iosize_align = 0;
    parser.default_job_params.iosizes[iosize_write].iosize_align = 0;
    parser.default_job_params.iosizes[iosize_deallocate].iosize_align = 0;
    parser.default_job_params.iosizes[iosize_write_zeroes].iosize_align = 0;
    parser.default_job_params.iosizes[iosize_write_uncorrect].iosize_align = 0;

    PARSER_LOG(&parser, ";---- beginning of %s ----\n", param_file);
    while (!feof(file)) {
        line_bytes = read_line(file, LINE_BUF_LEN, parser.line_buf);
        if (line_bytes < 0) {
            fprintf(stderr, "%s:%d: error: line buffer overflow!\n", param_file, parser.line_num + 1);
            return -1;
        }
        if (line_bytes == 0) {
            continue;
        }
        parse_line(&parser);
        parser.line_num++;
    }
    fclose(file);
    PARSER_LOG(&parser, ";---- end of %s ----\n", param_file);

    if (parser.params->njobs == 0) {
        parser.params->job_params = (struct job_params *)malloc(sizeof (struct job_params));
        memcpy(&parser.params->job_params[0], &parser.default_job_params, sizeof (struct job_params));
        parser.params->njobs++;
    }

    return 0;
}

