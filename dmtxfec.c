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

#include <stdlib.h>
#include <string.h>
#include <malloc.h>

typedef unsigned char data_t;

#define DMTX_RS_MM         8 /* Bits per symbol */
#define DMTX_RS_NN       255 /* Symbols per RS block */
#define DMTX_RS_GFPOLY 0x12d /* Field generator polynomial coefficients */

#undef NULL
#define NULL ((void *)0)

#undef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))

/* General purpose RS codec, 8-bit symbols */
static void encode_rs_char(void *rs, unsigned char *data, unsigned char *parity);
static int decode_rs_char(void *rs, unsigned char *data, int *eras_pos, int no_eras);
static void *init_rs_char(int nroots, int pad);
static void free_rs_char(void *rs);

/* Reed-Solomon codec control block */
struct rs {
   data_t *alpha_to;    /* log lookup table */
   data_t *index_of;    /* Antilog lookup table */
   data_t *genpoly;     /* Generator polynomial */
   int nroots;          /* Number of generator roots = number of parity symbols */
   int iprim;           /* prim-th root of 1, index form */
   int pad;             /* Padding bytes in shortened block */
};

/**
 *
 *
 */
static int
modnn(struct rs *rs, int x)
{
   while (x >= DMTX_RS_NN) {
      x -= DMTX_RS_NN;
      x = (x >> DMTX_RS_MM) + (x & DMTX_RS_NN);
   }

   return x;
}

/**
 *
 *
 */
static void
free_rs_char(void *p)
{
   struct rs *rs = (struct rs *)p;

   free(rs->alpha_to);
   free(rs->index_of);
   free(rs->genpoly);
   free(rs);
}

/**
 * Initialize a Reed-Solomon codec
 * nroots  = RS code generator polynomial degree (number of roots)
 * pad     = padding bytes at front of shortened block
 */
static void *
init_rs_char(int nroots, int pad)
{
   struct rs *rs;
   int i, j, sr, root, iprim;

   /* Check parameter ranges */
   if(nroots < 0 || nroots > DMTX_RS_NN)
      return NULL; /* Can't have more roots than symbol values! */

   if(pad < 0 || pad >= (DMTX_RS_NN - nroots))
      return NULL; /* Too much padding */

   rs = (struct rs *)calloc(1, sizeof(struct rs));
   if(rs == NULL)
      return NULL;

   rs->pad = pad;

   rs->alpha_to = (data_t *)malloc(sizeof(data_t) * (DMTX_RS_NN + 1));
   if(rs->alpha_to == NULL) {
      free(rs);
      return NULL;
   }
   rs->index_of = (data_t *)malloc(sizeof(data_t) * (DMTX_RS_NN + 1));
   if(rs->index_of == NULL) {
      free(rs->alpha_to);
      free(rs);
      return NULL;
   }

   /* Generate Galois field lookup tables */
   rs->index_of[0] = DMTX_RS_NN; /* log(zero) = -inf */
   rs->alpha_to[DMTX_RS_NN] = 0; /* alpha**-inf = 0 */
   sr = 1;
   for(i = 0; i < DMTX_RS_NN; i++) {
      rs->index_of[sr] = i;
      rs->alpha_to[i] = sr;
      sr <<= 1;
      if(sr & (1 << DMTX_RS_MM))
         sr ^= DMTX_RS_GFPOLY;
      sr &= DMTX_RS_NN;
   }
   if(sr != 1) {
      /* Field generator polynomial is not primitive */
      free(rs->alpha_to);
      free(rs->index_of);
      free(rs);
      return NULL;
   }

   /* Form RS code generator polynomial from its roots */
   rs->genpoly = (data_t *)malloc(sizeof(data_t)*(nroots+1));
   if(rs->genpoly == NULL) {
      free(rs->alpha_to);
      free(rs->index_of);
      free(rs);
      return NULL;
   }
   rs->nroots = nroots;

   /* Find prim-th root of 1, used in decoding */
   for(iprim = 1; (iprim % 1) != 0; iprim += DMTX_RS_NN)
      ;
   rs->iprim = iprim;

   rs->genpoly[0] = 1;
   for (i = 0,root=1; i < nroots; i++,root += 1) {
      rs->genpoly[i+1] = 1;

      /* Multiply rs->genpoly[] by  @**(root + x) */
      for (j = i; j > 0; j--) {
         if (rs->genpoly[j] != 0)
            rs->genpoly[j] = rs->genpoly[j-1] ^ rs->alpha_to[modnn(rs, rs->index_of[rs->genpoly[j]] + root)];
         else
            rs->genpoly[j] = rs->genpoly[j-1];
      }

      /* rs->genpoly[0] can never be zero */
      rs->genpoly[0] = rs->alpha_to[modnn(rs, rs->index_of[rs->genpoly[0]] + root)];
   }

   /* convert rs->genpoly[] to index form for quicker encoding */
   for(i = 0; i <= nroots; i++)
      rs->genpoly[i] = rs->index_of[rs->genpoly[i]];

   return rs;
}

/**
 * data_t          - a typedef for the data symbol
 * data_t data[]   - array of DMTX_RS_NN -(rs->nroots)-(rs->pad) and type data_t to be encoded
 * data_t parity[] - an array of rs->nroots and type data_t to be written with parity symbols
 * rs->nroots      - the number of roots in the RS code generator polynomial,
 *                   which is the same as the number of parity symbols in a block.
 *                   Integer variable or literal.
 * rs->pad         - the number of pad symbols in a block. Integer variable or literal.
 * rs->alpha_to    - The address of an array of DMTX_RS_NN elements to convert Galois field
 *                   elements in index (log) form to polynomial form. Read only.
 * rs->index_of    - The address of an array of DMTX_RS_NN elements to convert Galois field
 *                   elements in polynomial form to index (log) form. Read only.
 * rs->genpoly     - an array of rs->nroots+1 elements containing the generator polynomial in index form
 *
 * The memset() and memmove() functions are used. The appropriate header
 * file declaring these functions (usually <string.h>) must be included by the calling
 * program.
 */
static void
encode_rs_char(void *p, data_t *data, data_t *parity)
{
   struct rs *rs = (struct rs *)p;
   int i, j;
   data_t feedback;

   memset(parity, 0, rs->nroots * sizeof(data_t));

   for(i = 0; i < DMTX_RS_NN - (rs->nroots) - (rs->pad); i++) {
      feedback = rs->index_of[data[i] ^ parity[0]];
      if(feedback != DMTX_RS_NN) { /* feedback term is non-zero */
#ifdef UNNORMALIZED
      /* This line is unnecessary when rs->genpoly[rs->nroots] is unity, as it must
         always be for the polynomials constructed by init_rs() */
         feedback = modnn(rs, DMTX_RS_NN - rs->genpoly[rs->nroots] + feedback);
#endif
         for(j = 1; j < rs->nroots; j++)
            parity[j] ^= rs->alpha_to[modnn(rs, feedback + rs->genpoly[rs->nroots-j])];
      }

      /* Shift */
      memmove(&parity[0], &parity[1], sizeof(data_t) * (rs->nroots-1));
      if(feedback != DMTX_RS_NN)
         parity[rs->nroots-1] = rs->alpha_to[modnn(rs, feedback + rs->genpoly[0])];
      else
         parity[rs->nroots-1] = 0;
   }
}

/**
 * data_t         Typedef for the data symbol
 * data_t data[]  Array of DMTX_RS_NN data and parity symbols to be corrected in place
 * rs->nroots     the number of roots in the RS code generator polynomial,
 *                which is the same as the number of parity symbols in a block.
 *                Integer variable or literal.
 * rs->pad        Number of pad symbols in a block. Integer variable or literal.
 * rs->alpha_to   The address of an array of DMTX_RS_NN elements to convert Galois field
 *                elements in index (log) form to polynomial form. Read only.
 * rs->index_of   The address of an array of DMTX_RS_NN elements to convert Galois field
 *                elements in polynomial form to index (log) form. Read only.
 *
 * The memset(), memmove(), and memcpy() functions are used. The appropriate header
 * file declaring these functions (usually <string.h>) must be included by the calling
 * program.
 */
static int
decode_rs_char(void *p, data_t *data, int *eras_pos, int no_eras)
{
   struct rs *rs = (struct rs *)p;
   int deg_lambda, el, deg_omega;
   int i, j, r,k;
   data_t u,q,tmp,num1,num2,den,discr_r;
   int syn_error, count;

   /* Err+Eras Locator poly and syndrome poly */
   data_t *lambda = malloc((rs->nroots+1) * sizeof(data_t));
   data_t *s      = malloc(rs->nroots * sizeof(data_t));
   data_t *b      = malloc((rs->nroots+1) * sizeof(data_t));
   data_t *t      = malloc((rs->nroots+1) * sizeof(data_t));
   data_t *omega  = malloc((rs->nroots+1) * sizeof(data_t));
   data_t *root   = malloc(rs->nroots * sizeof(data_t));
   data_t *reg    = malloc((rs->nroots+1) * sizeof(data_t));
   data_t *loc    = malloc(rs->nroots * sizeof(data_t));

   /* Form the syndromes; i.e., evaluate data(x) at roots of g(x) */
   for(i = 0; i < rs->nroots; i++)
      s[i] = data[0];

   for(j = 1; j < DMTX_RS_NN - rs->pad; j++) {
      for(i = 0; i< rs->nroots; i++) {
         s[i] = (s[i] == 0) ? data[j] : data[j] ^ rs->alpha_to[modnn(rs, rs->index_of[s[i]] + (1+i))];
      }
   }

   /* Convert syndromes to index form, checking for nonzero condition */
   syn_error = 0;
   for(i = 0; i < rs->nroots; i++) {
      syn_error |= s[i];
      s[i] = rs->index_of[s[i]];
   }

   /* If syndrome is zero, data[] is a codeword and there are no errors to
      correct. So return data[] unmodified */

   /* XXX add check here to return if error correction is not required */

   if(!syn_error) {
      count = 0;
      goto finish;
   }
   memset(&lambda[1], 0, rs->nroots * sizeof(lambda[0]));
   lambda[0] = 1;

   if(no_eras > 0) {
      /* Init lambda to be the erasure locator polynomial */
      lambda[1] = rs->alpha_to[modnn(rs, DMTX_RS_NN - 1 - eras_pos[0])];
      for(i = 1; i < no_eras; i++) {
         u = modnn(rs, DMTX_RS_NN - 1 - eras_pos[i]);
         for(j = i+1; j > 0; j--) {
            tmp = rs->index_of[lambda[j - 1]];
            if(tmp != DMTX_RS_NN)
               lambda[j] ^= rs->alpha_to[modnn(rs,u + tmp)];
         }
      }
   }
   for(i = 0; i < rs->nroots + 1; i++)
      b[i] = rs->index_of[lambda[i]];

   /* Begin Berlekamp-Massey algorithm to determine error+erasure
      locator polynomial */
   r = no_eras;
   el = no_eras;
   while(++r <= rs->nroots) { /* r is the step number */
      /* Compute discrepancy at the r-th step in poly-form */
      discr_r = 0;
      for(i = 0; i < r; i++) {
         if((lambda[i] != 0) && (s[r-i-1] != DMTX_RS_NN)) {
            discr_r ^= rs->alpha_to[modnn(rs, rs->index_of[lambda[i]] + s[r-i-1])];
         }
      }
      discr_r = rs->index_of[discr_r]; /* Index form */
      if(discr_r == DMTX_RS_NN) {
         /* 2 lines below: B(x) <-- x*B(x) */
         memmove(&b[1], b, rs->nroots * sizeof(b[0]));
         b[0] = DMTX_RS_NN;
      }
      else {
         /* 7 lines below: T(x) <-- lambda(x) - discr_r*x*b(x) */
         t[0] = lambda[0];
         for(i = 0 ; i < rs->nroots; i++) {
            t[i+1] = (b[i] != DMTX_RS_NN) ? lambda[i+1] ^ rs->alpha_to[modnn(rs, discr_r + b[i])] : lambda[i+1];
         }
         if(2 * el <= r + no_eras - 1) {
            el = r + no_eras - el;
            /* 2 lines below: B(x) <-- inv(discr_r) * lambda(x) */
            for(i = 0; i <= rs->nroots; i++)
               b[i] = (lambda[i] == 0) ? DMTX_RS_NN : modnn(rs, rs->index_of[lambda[i]] - discr_r + DMTX_RS_NN);
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
      lambda[i] = rs->index_of[lambda[i]];
      if(lambda[i] != DMTX_RS_NN)
         deg_lambda = i;
   }

   /* Find roots of the error+erasure locator polynomial by Chien search */
   memcpy(&reg[1], &lambda[1], rs->nroots * sizeof(reg[0]));
   count = 0; /* Number of roots of lambda(x) */
   for(i = 1,k=rs->iprim-1; i <= DMTX_RS_NN; i++,k = modnn(rs,k+rs->iprim)) {
      q = 1; /* lambda[0] is always 0 */
      for(j = deg_lambda; j > 0; j--) {
         if(reg[j] != DMTX_RS_NN) {
            reg[j] = modnn(rs,reg[j] + j);
            q ^= rs->alpha_to[reg[j]];
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
            tmp ^= rs->alpha_to[modnn(rs,s[i - j] + lambda[j])];
      }
      omega[i] = rs->index_of[tmp];
   }

   /* Compute error values in poly-form. num1 = omega(inv(X(l))), num2 =
      inv(X(l))**0 and den = lambda_pr(inv(X(l))) all in poly-form */
   for(j = count-1; j >=0; j--) {
      num1 = 0;
      for(i = deg_omega; i >= 0; i--) {
         if(omega[i] != DMTX_RS_NN)
            num1   ^= rs->alpha_to[modnn(rs,omega[i] + i * root[j])];
      }
      num2 = rs->alpha_to[modnn(rs, DMTX_RS_NN)];
      den = 0;

      /* lambda[i+1] for i even is the formal derivative lambda_pr of lambda[i] */
      for(i = MIN(deg_lambda, rs->nroots - 1) & ~1; i >= 0; i -=2) {
         if(lambda[i+1] != DMTX_RS_NN)
            den ^= rs->alpha_to[modnn(rs,lambda[i+1] + i * root[j])];
      }

      /* Apply error to data */
      if(num1 != 0 && loc[j] >= rs->pad) {
         data[loc[j] - rs->pad] ^= rs->alpha_to[modnn(rs, rs->index_of[num1] +
               rs->index_of[num2] + DMTX_RS_NN - rs->index_of[den])];
      }
   }

finish:
   if(eras_pos != NULL) {
      for(i=0;i<count;i++)
         eras_pos[i] = loc[i];
   }

   free(lambda);
   free(s);
   free(b);
   free(t);
   free(omega);
   free(root);
   free(reg);
   free(loc);

   return count;
}
