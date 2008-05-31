/* User include file for libfec
 * Copyright 2004, Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 */

#ifndef __FEC_H__
#define __FEC_H__

/* General purpose RS codec, 8-bit symbols */
static void encode_rs_char(void *rs,unsigned char *data,unsigned char *parity);
static int decode_rs_char(void *rs,unsigned char *data,int *eras_pos, int no_eras);
static void *init_rs_char(int symsize,int gfpoly, int fcr,int prim,int nroots, int pad);
static void free_rs_char(void *rs);

#endif
