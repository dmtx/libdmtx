/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (c) 2002-2004 Phil Karn, KA9Q
Copyright (c) 2008 Mike Laughton

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
 * @brief  XXX
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
 * @brief  XXX
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
