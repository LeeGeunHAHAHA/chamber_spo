/**
 * @file ansi_escape.h
 * @date 2016. 06. 01.
 * @author kmsun
 *
 * ANSI escape code
 */
#ifndef __NBIO_ANSI_ESCAPE_H__
#define __NBIO_ANSI_ESCAPE_H__

#define AE_SAVE_CURSOR      "\033[s"
#define AE_RESTORE_CURSOR   "\033[u"
#define AE_MOVE_UP_CURSOR   "\033[%dA"
#define AE_CLEAR_EOL        "\033[K"
#define AE_CLEAR_LINE       "\033[2K"

#endif /* __NBIO_ANSI_ESCAPE_H__ */

