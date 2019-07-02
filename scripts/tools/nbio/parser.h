/**
 * @file parser.h
 * @date 2016. 05. 16.
 * @author kmsun
 *
 * parameter file parser
 */
#ifndef __NBIO_PARSER_H__
#define __NBIO_PARSER_H__

#include "params.h"

int parse_param(const char *param_file, struct global_params *params, int verbose);

#endif /* __NBIO_PARSER_H__ */

