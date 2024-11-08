/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 * FreeBSD log2 patch from graphviz port to FreeBSD 9, courtesy of
 * Christoph Moench-Tegeder <cmt@burggraben.net>
 */

// It is recommended to include this header instead of <math.h> or
// <cmath> whenever the log2(x) function is needed.  See bug 1467.

#ifndef MATH_H
#define MATH_H

/**
 * @file
 * @ingroup core
 * log2() macro definition; to deal with \bugid{1467}.
 */

#include <cmath>

#ifdef __FreeBSD__

#if __FreeBSD_version <= 704101 || (__FreeBSD_version >= 800000 && __FreeBSD_version < 802502) ||  \
    (__FreeBSD_version >= 900000 && __FreeBSD_version < 900027)
#define log2(x) (std::log(x) / M_LN2)
#endif /* __FreeBSD_version */

#endif /* __FreeBSD__ */

#endif /* MATH_H */
