/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2002-2004 Phil Karn, KA9Q
Copyright (C) 2008, 2009 Mike Laughton

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

----------------------------------------------------------------------
This file is derived from various portions of the fec library
written by Phil Karn available at http://www.ka9q.net. It has
been modified to include only the specific cases used by Data
Matrix barcodes, and to integrate with the rest of libdmtx.
----------------------------------------------------------------------

Contact: mike@dragonflylogic.com
*/

/* $Id$ */

/**
 * @file dmtxfec.c
 * @brief Forward Error Correction
 */

#include <stdlib.h>
#include <string.h>

typedef unsigned char data_t;

#define DMTX_RS_MM          8 /* Bits per symbol */
#define DMTX_RS_NN        255 /* Symbols per RS block */
#define DMTX_RS_GFPOLY  0x12d /* Field generator polynomial coefficients */
#define DMTX_RS_MAX_NROOTS 68 /* Maximum value of nroots for Data Matrix */

#undef NULL
#define NULL ((void *)0)

/* Reed-Solomon codec control block
 *
 * rs->nroots  - the number of roots in the RS code generator polynomial,
 *               which is the same as the number of parity symbols in a block.
 *               Integer variable or literal.
 * rs->pad     - the number of pad symbols in a block. Integer variable or literal.
 * rs->genpoly - an array of rs->nroots+1 elements containing the generator polynomial in index form
 */
struct rs {
   data_t genpoly[DMTX_RS_MAX_NROOTS+1]; /* Generator polynomial */
   int nroots;                           /* Number of generator roots = number of parity symbols */
   int pad;                              /* Padding bytes in shortened block */
};

/* General purpose RS codec, 8-bit symbols */
static void encode_rs_char(struct rs *rs, unsigned char *data, unsigned char *parity);
static int decode_rs_char(struct rs *rs, unsigned char *data, int *eras_pos, int no_eras, int max_fixes);
static struct rs *init_rs_char(int nroots, int pad);
static void free_rs_char(struct rs **rs);

/* log(0) = -inf */
static unsigned char indexOf[] =
   { 255,   0,   1, 240,   2, 225, 241,  53,   3,  38, 226, 133, 242,  43,  54, 210,
       4, 195,  39, 114, 227, 106, 134,  28, 243, 140,  44,  23,  55, 118, 211, 234,
       5, 219, 196,  96,  40, 222, 115, 103, 228,  78, 107, 125, 135,   8,  29, 162,
     244, 186, 141, 180,  45,  99,  24,  49,  56,  13, 119, 153, 212, 199, 235,  91,
       6,  76, 220, 217, 197,  11,  97, 184,  41,  36, 223, 253, 116, 138, 104, 193,
     229,  86,  79, 171, 108, 165, 126, 145, 136,  34,   9,  74,  30,  32, 163,  84,
     245, 173, 187, 204, 142,  81, 181, 190,  46,  88, 100, 159,  25, 231,  50, 207,
      57, 147,  14,  67, 120, 128, 154, 248, 213, 167, 200,  63, 236, 110,  92, 176,
       7, 161,  77, 124, 221, 102, 218,  95, 198,  90,  12, 152,  98,  48, 185, 179,
      42, 209,  37, 132, 224,  52, 254, 239, 117, 233, 139,  22, 105,  27, 194, 113,
     230, 206,  87, 158,  80, 189, 172, 203, 109, 175, 166,  62, 127, 247, 146,  66,
     137, 192,  35, 252,  10, 183,  75, 216,  31,  83,  33,  73, 164, 144,  85, 170,
     246,  65, 174,  61, 188, 202, 205, 157, 143, 169,  82,  72, 182, 215, 191, 251,
      47, 178,  89, 151, 101,  94, 160, 123,  26, 112, 232,  21,  51, 238, 208, 131,
      58,  69, 148,  18,  15,  16,  68,  17, 121, 149, 129,  19, 155,  59, 249,  70,
     214, 250, 168,  71, 201, 156,  64,  60, 237, 130, 111,  20,  93, 122, 177, 150 };

/* alpha**-inf = 0 */
static unsigned char alphaTo[] =
   {   1,   2,   4,   8,  16,  32,  64, 128,  45,  90, 180,  69, 138,  57, 114, 228,
     229, 231, 227, 235, 251, 219, 155,  27,  54, 108, 216, 157,  23,  46,  92, 184,
      93, 186,  89, 178,  73, 146,   9,  18,  36,  72, 144,  13,  26,  52, 104, 208,
     141,  55, 110, 220, 149,   7,  14,  28,  56, 112, 224, 237, 247, 195, 171, 123,
     246, 193, 175, 115, 230, 225, 239, 243, 203, 187,  91, 182,  65, 130,  41,  82,
     164, 101, 202, 185,  95, 190,  81, 162, 105, 210, 137,  63, 126, 252, 213, 135,
      35,  70, 140,  53, 106, 212, 133,  39,  78, 156,  21,  42,  84, 168, 125, 250,
     217, 159,  19,  38,  76, 152,  29,  58, 116, 232, 253, 215, 131,  43,  86, 172,
     117, 234, 249, 223, 147,  11,  22,  44,  88, 176,  77, 154,  25,  50, 100, 200,
     189,  87, 174, 113, 226, 233, 255, 211, 139,  59, 118, 236, 245, 199, 163, 107,
     214, 129,  47,  94, 188,  85, 170, 121, 242, 201, 191,  83, 166,  97, 194, 169,
     127, 254, 209, 143,  51, 102, 204, 181,  71, 142,  49,  98, 196, 165, 103, 206,
     177,  79, 158,  17,  34,  68, 136,  61, 122, 244, 197, 167,  99, 198, 161, 111,
     222, 145,  15,  30,  60, 120, 240, 205, 183,  67, 134,  33,  66, 132,  37,  74,
     148,   5,  10,  20,  40,  80, 160, 109, 218, 153,  31,  62, 124, 248, 221, 151,
       3,   6,  12,  24,  48,  96, 192, 173, 119, 238, 241, 207, 179,  75, 150,   0 };

/**
 * @brief  Calculate x mod NN (255)
 * @param  x
 * @return Modulus
 */
static int
modnn(int x)
{
   while (x >= DMTX_RS_NN) {
      x -= DMTX_RS_NN;
      x = (x >> DMTX_RS_MM) + (x & DMTX_RS_NN);
   }

   return x;
}

/**
 * @brief  Free memory used for Reed Solomon struct
 * @param  p
 * @return rs
 */
static void
free_rs_char(struct rs **p)
{
   if(*p != NULL)
      free(*p);

   *p = NULL;
}

/**
 * @brief  Initialize a Reed-Solomon codec
 * @param  nroots RS code generator polynomial degree (number of roots)
 * @param  pad Padding bytes at front of shortened block
 * @return Pointer to rs struct
 */
static struct rs *
init_rs_char(int nroots, int pad)
{
   struct rs *rs;
   int i, j, root;

   assert(nroots <= DMTX_RS_MAX_NROOTS);

   /* Can't have more roots than symbol values */
   if(nroots < 0 || nroots > DMTX_RS_NN)
      return NULL;

   /* Too much padding */
   if(pad < 0 || pad >= (DMTX_RS_NN - nroots))
      return NULL;

   rs = (struct rs *)calloc(1, sizeof(struct rs));
   if(rs == NULL)
      return NULL;

   /* Form RS code generator polynomial from its roots */
   rs->pad = pad;
   rs->nroots = nroots;

   rs->genpoly[0] = 1;
   for(i = 0, root = 1; i < nroots; i++, root++) {
      rs->genpoly[i+1] = 1;

      /* Multiply rs->genpoly[] by @**(root + x) */
      for(j = i; j > 0; j--) {
         rs->genpoly[j] = (rs->genpoly[j] == 0) ? rs->genpoly[j-1] :
               rs->genpoly[j-1] ^ alphaTo[modnn(indexOf[rs->genpoly[j]] + root)];
      }

      /* rs->genpoly[0] can never be zero */
      rs->genpoly[0] = alphaTo[modnn(indexOf[rs->genpoly[0]] + root)];
   }

   /* convert rs->genpoly[] to index form for quicker encoding */
   for(i = 0; i <= nroots; i++)
      rs->genpoly[i] = indexOf[rs->genpoly[i]];

   return rs;
}

/**
 * @brief  Encode array of values with Reed-Solomon
 * @param  rs Reed-Solomon codec control block
 * @param  data Array of data_t to be encoded
 * @param  parity Array of data_t to be written with parity symbols
 * @return void
 */
static void
encode_rs_char(struct rs *rs, data_t *data, data_t *parity)
{
   int i, j;
   data_t feedback;

   memset(parity, 0, rs->nroots * sizeof(data_t));

   for(i = 0; i < DMTX_RS_NN - rs->nroots - rs->pad; i++) {
      feedback = indexOf[data[i] ^ parity[0]];
      if(feedback != DMTX_RS_NN) { /* feedback term is non-zero */
         for(j = 1; j < rs->nroots; j++)
            parity[j] ^= alphaTo[modnn(feedback + rs->genpoly[rs->nroots - j])];
      }

      /* Shift */
      memmove(&parity[0], &parity[1], sizeof(data_t) * (rs->nroots-1));
      if(feedback != DMTX_RS_NN)
         parity[rs->nroots-1] = alphaTo[modnn(feedback + rs->genpoly[0])];
      else
         parity[rs->nroots-1] = 0;
   }
}

/**
 * @brief  Decode and correct array of values with Reed-Solomon
 * @param  rs Reed-Solomon codec control block
 * @param  data Array of data and parity symbols to be corrected in place
 * @param  eras_pos
 * @param  no_eras
 * @param  max_fixes
 * @return count
 */
static int
decode_rs_char(struct rs *rs, data_t *data, int *eras_pos, int no_eras, int max_fixes)
{
   int deg_lambda, el, deg_omega;
   int i, j, r, k;
   data_t u, q, tmp, num1, num2, den, discr_r;
   int syn_error, count;

   /* Err+Eras Locator poly and syndrome poly */
   data_t lambda[DMTX_RS_MAX_NROOTS+1];
   data_t s[DMTX_RS_MAX_NROOTS];
   data_t b[DMTX_RS_MAX_NROOTS+1];
   data_t t[DMTX_RS_MAX_NROOTS+1];
   data_t omega[DMTX_RS_MAX_NROOTS+1];
   data_t root[DMTX_RS_MAX_NROOTS];
   data_t reg[DMTX_RS_MAX_NROOTS+1];
   data_t loc[DMTX_RS_MAX_NROOTS];

   /* Form the syndromes; i.e., evaluate data(x) at roots of g(x) */
   for(i = 0; i < rs->nroots; i++)
      s[i] = data[0];

   for(j = 1; j < DMTX_RS_NN - rs->pad; j++) {
      for(i = 0; i< rs->nroots; i++) {
         s[i] = (s[i] == 0) ? data[j] : data[j] ^ alphaTo[modnn(indexOf[s[i]] + (1+i))];
      }
   }

   /* Convert syndromes to index form, checking for nonzero condition */
   syn_error = 0;
   for(i = 0; i < rs->nroots; i++) {
      syn_error |= s[i];
      s[i] = indexOf[s[i]];
   }

   /* If syndrome is zero, data[] is a codeword and there are no errors to
      correct. So return data[] unmodified */
   if(syn_error == 0) {
      count = 0;
      goto finish;
   }
   else if(max_fixes == 0) {
      count = -1;
      goto finish;
   }

   memset(&lambda[1], 0, rs->nroots * sizeof(lambda[0]));
   lambda[0] = 1;

   if(no_eras > 0) {
      /* Init lambda to be the erasure locator polynomial */
      lambda[1] = alphaTo[modnn(DMTX_RS_NN - 1 - eras_pos[0])];
      for(i = 1; i < no_eras; i++) {
         u = modnn(DMTX_RS_NN - 1 - eras_pos[i]);
         for(j = i+1; j > 0; j--) {
            tmp = indexOf[lambda[j - 1]];
            if(tmp != DMTX_RS_NN)
               lambda[j] ^= alphaTo[modnn(u + tmp)];
         }
      }
   }
   for(i = 0; i < rs->nroots + 1; i++)
      b[i] = indexOf[lambda[i]];

   /* Begin Berlekamp-Massey algorithm to determine error+erasure
      locator polynomial */
   r = no_eras;
   el = no_eras;
   while(++r <= rs->nroots) { /* r is the step number */
      /* Compute discrepancy at the r-th step in poly-form */
      discr_r = 0;
      for(i = 0; i < r; i++) {
         if((lambda[i] != 0) && (s[r-i-1] != DMTX_RS_NN)) {
            discr_r ^= alphaTo[modnn(indexOf[lambda[i]] + s[r-i-1])];
         }
      }
      discr_r = indexOf[discr_r]; /* Index form */
      if(discr_r == DMTX_RS_NN) {
         /* 2 lines below: B(x) <-- x*B(x) */
         memmove(&b[1], b, rs->nroots * sizeof(b[0]));
         b[0] = DMTX_RS_NN;
      }
      else {
         /* 7 lines below: T(x) <-- lambda(x) - discr_r*x*b(x) */
         t[0] = lambda[0];
         for(i = 0 ; i < rs->nroots; i++) {
            t[i+1] = (b[i] != DMTX_RS_NN) ? lambda[i+1] ^ alphaTo[modnn(discr_r + b[i])] : lambda[i+1];
         }
         if(2 * el <= r + no_eras - 1) {
            el = r + no_eras - el;
            /* 2 lines below: B(x) <-- inv(discr_r) * lambda(x) */
            for(i = 0; i <= rs->nroots; i++)
               b[i] = (lambda[i] == 0) ? DMTX_RS_NN : modnn(indexOf[lambda[i]] - discr_r + DMTX_RS_NN);
         }
         else {
            /* 2 lines below: B(x) <-- x*B(x) */
            memmove(&b[1], b, rs->nroots * sizeof(b[0]));
            b[0] = DMTX_RS_NN;
         }
         memcpy(lambda, t, (rs->nroots+1) * sizeof(t[0]));
      }
   }

   /* Convert lambda to index form and compute deg(lambda(x)) */
   deg_lambda = 0;
   for(i = 0; i < rs->nroots + 1; i++) {
      lambda[i] = indexOf[lambda[i]];
      if(lambda[i] != DMTX_RS_NN)
         deg_lambda = i;
   }

   /* Find roots of the error+erasure locator polynomial by Chien search */
   memcpy(&reg[1], &lambda[1], rs->nroots * sizeof(reg[0]));
   count = 0; /* Number of roots of lambda(x) */
   for(i = 1,k=0; i <= DMTX_RS_NN; i++,k = modnn(k+1)) {
      q = 1; /* lambda[0] is always 0 */
      for(j = deg_lambda; j > 0; j--) {
         if(reg[j] != DMTX_RS_NN) {
            reg[j] = modnn(reg[j] + j);
            q ^= alphaTo[reg[j]];
         }
      }

      if(q != 0)
         continue; /* Not a root */

      /* Store root (index-form) and error location number */
      root[count] = i;
      loc[count] = k;

      /* If we've already found max possible roots, abort the search to save time */
      if(++count == deg_lambda)
         break;
   }

   if(deg_lambda != count) {
      /* deg(lambda) unequal to number of roots => uncorrectable error detected */
      count = -1;
      goto finish;
   }

   /* Compute err+eras evaluator poly omega(x) = s(x)*lambda(x)
      (modulo x**rs->nroots). in index form. Also find deg(omega). */
   deg_omega = deg_lambda-1;
   for(i = 0; i <= deg_omega;i++) {
      tmp = 0;
      for(j=i;j >= 0; j--) {
         if((s[i - j] != DMTX_RS_NN) && (lambda[j] != DMTX_RS_NN))
            tmp ^= alphaTo[modnn(s[i - j] + lambda[j])];
      }
      omega[i] = indexOf[tmp];
   }

   /* Compute error values in poly-form. num1 = omega(inv(X(l))), num2 =
      inv(X(l))**0 and den = lambda_pr(inv(X(l))) all in poly-form */
   for(j = count - 1; j >= 0; j--) {
      num1 = 0;
      for(i = deg_omega; i >= 0; i--) {
         if(omega[i] != DMTX_RS_NN)
            num1 ^= alphaTo[modnn(omega[i] + i * root[j])];
      }
      num2 = alphaTo[modnn(DMTX_RS_NN)];
      den = 0;

      /* lambda[i+1] for i even is the formal derivative lambda_pr of lambda[i] */
      for(i = min(deg_lambda, rs->nroots - 1) & ~1; i >= 0; i -=2) {
         if(lambda[i+1] != DMTX_RS_NN)
            den ^= alphaTo[modnn(lambda[i+1] + i * root[j])];
      }

      /* Apply error to data */
      if(num1 != 0 && loc[j] >= rs->pad) {
         data[loc[j] - rs->pad] ^= alphaTo[modnn(indexOf[num1] +
               indexOf[num2] + DMTX_RS_NN - indexOf[den])];
      }
   }

finish:
   if(eras_pos != NULL) {
      for(i = 0; i < count; i++)
         eras_pos[i] = loc[i];
   }

   return count;
}
