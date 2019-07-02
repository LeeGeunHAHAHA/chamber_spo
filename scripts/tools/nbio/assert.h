/**
 * @file assert.h
 * @date 2016. 05. 31.
 * @author kmsun
 */
#ifndef __NBIO_ASSERT_H__
#define __NBIO_ASSERT_H__

#define BUILD_ASSERT(condition)     ((void)sizeof (char[1 - 2*!(condition)]))

#endif /* __NBIO_ASSERT_H__ */

