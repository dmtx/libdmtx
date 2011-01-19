/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2008, 2009, 2010 Mike Laughton

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

--------------------------------------------------------------------------
Portions of this file were derived from the Reed-Solomon encoder/decoder
released by Simon Rockliff in June 1991. It has been modified to include
only Data Matrix conditions and to integrate seamlessly with libdmtx.
--------------------------------------------------------------------------

Contact: mike@dragonflylogic.com
*/

/* $Id$ */

#define NN                      255
#define GF_NAN                  255
#define MAX_ERROR_WORD_COUNT     68
#define MAX_SYNDROME_COUNT       69

#define RETURN_FAIL_IF(c) if(c) { return DmtxFail; }

/* GF(256) log values using primitive polynomial 301 */
static unsigned char log301[] =
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

/* GF(256) antilog values using primitive polynomial 301 */
static unsigned char antilog301[] =
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

/* GF add (a + b) */
#define GfAdd(a,b) \
   ((a) ^ (b))

/* GF multiply (a * b) */
#define GfMult(a,b) \
   (((a) == 0 || (b) == 0) ? 0 : antilog301[(log301[(a)] + log301[(b)]) % NN])

/* GF multiply by antilog (a * 2**b) */
#define GfMultAntilog(a,b) \
   (((a) == 0) ? 0 : antilog301[(log301[(a)] + (b)) % NN])

/**
 *
 *
 */
static DmtxPassFail
RsEncode(DmtxMessage *message, int sizeIdx)
{
   int i, j;
   int blockStride, blockIdx;
   int blockErrorWords, symbolDataWords, symbolErrorWords, symbolTotalWords;
   unsigned char val, *bPtr;
   unsigned char g[MAX_ERROR_WORD_COUNT];
   unsigned char b[MAX_ERROR_WORD_COUNT];

   blockStride = dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks, sizeIdx);
   blockErrorWords = dmtxGetSymbolAttribute(DmtxSymAttribBlockErrorWords, sizeIdx);
   symbolDataWords = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, sizeIdx);
   symbolErrorWords = dmtxGetSymbolAttribute(DmtxSymAttribSymbolErrorWords, sizeIdx);
   symbolTotalWords = symbolDataWords + symbolErrorWords;

   /* Populate the generator polynomial */
   RsGenPoly(g, blockErrorWords);

   /* Generate error codewords for all interleaved blocks */
   for(blockIdx = 0; blockIdx < blockStride; blockIdx++)
   {
      /* Generate */
      memset(b, 0x00, sizeof(b));
      for(i = blockIdx; i < symbolDataWords; i += blockStride)
      {
         val = GfAdd(b[blockErrorWords-1], message->code[i]);

         for(j = blockErrorWords - 1; j > 0; j--)
            b[j] = GfAdd(b[j-1], GfMult(g[j], val));

         b[0] = GfMult(g[0], val);
      }

      /* Copy to output message */
      bPtr = b + blockErrorWords;
      for(i = blockIdx + symbolDataWords; i < symbolTotalWords; i += blockStride)
         message->code[i] = *(--bPtr);

      assert(b == bPtr);
   }

   return DmtxPass;
}

/**
 *
 *
 */
static DmtxPassFail
RsDecode(unsigned char *code, int sizeIdx, int fix)
{
   int i;
   int blockStride, blockIdx;
   int blockErrorWords, blockTotalWords, blockMaxCorrectable;
   unsigned char recd[NN];
   unsigned char g[MAX_ERROR_WORD_COUNT];
   unsigned char s[MAX_SYNDROME_COUNT];
   DmtxBoolean error;

   blockStride = dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks, sizeIdx);
   blockErrorWords = dmtxGetSymbolAttribute(DmtxSymAttribBlockErrorWords, sizeIdx);
   blockMaxCorrectable = dmtxGetSymbolAttribute(DmtxSymAttribBlockMaxCorrectable, sizeIdx);

   /* Populate the generator polynomial */
   RsGenPoly(g, blockErrorWords);

   for(blockIdx = 0; blockIdx < blockStride; blockIdx++)
   {
      /* Need to query at block level because of special case at 144x144 */
      blockTotalWords = blockErrorWords + dmtxGetBlockDataSize(sizeIdx, blockIdx);

      /* Populate recd[] with data and error codewords */
      memset(recd, 0x00, sizeof(recd));
      for(i = 0; i < blockTotalWords; i++)
         recd[i] = code[blockIdx + blockStride * (blockTotalWords - 1 - i)];

      /* Calculate syndrome */
      error = RsCalcSyndrome(s, recd, blockErrorWords, blockTotalWords);

      /* Attempt to repair detected error(s) */
      if(error)
      {
         RETURN_FAIL_IF(RsFindErrorLocatorPoly(s, blockErrorWords, blockTotalWords, blockMaxCorrectable) == DmtxFail);

         /* RETURN_FAIL_IF(RsFindErrorPositions() == DmtxFail); */

         /* RETURN_FAIL_IF(RsFindErrorValues() == DmtxFail); */
      }

      /* Unable to repair */
      if(error)
         return DmtxFail;

      /* Write out codewords */
/*
      for(i = 0; i < NN; i++)
         recd[i] = (recd[i] == -1) ? 0 : antilog301[recd[i]];
*/
   }

   return DmtxPass;
}

/**
 *
 *
 */
static DmtxPassFail
RsGenPoly(unsigned char *g, int errorWordCount)
{
   int i, j;

   assert(errorWordCount <= MAX_ERROR_WORD_COUNT);

   /* Initialize all coefficients to 1 */
   memset(g, 0x01, sizeof(unsigned char) * MAX_ERROR_WORD_COUNT);

   /* Generate polynomial */
   for(i = 1; i <= errorWordCount; i++)
   {
      for(j = i - 1; j >= 0; j--)
      {
         g[j] = GfMultAntilog(g[j], i); /* g[j] *= 2**i */
         if(j > 0)
            g[j] = GfAdd(g[j], g[j-1]); /* g[j] += g[j-1] */
      }
   }

   return DmtxPass;
}

/**
 * Assume we have received bits grouped into mm-bit symbols in recd[i],
 * i=0..(nn-1),  and recd[i] is index form (ie as powers of alpha). We first
 * compute the 2*tt syndromes by substituting alpha**i into rec(X) and
 * evaluating, storing the syndromes in s[i], i=1..2tt (leave s[0] zero).
 */
static DmtxBoolean
RsCalcSyndrome(unsigned char *s, unsigned char *recd, int blockErrorWords, int blockTotalWords)
{
   int i, j;
   DmtxBoolean error = DmtxFalse;

   memset(s, 0x00, sizeof(unsigned char) * MAX_SYNDROME_COUNT);

   /* Form syndromes */
   for(i = 1; i <= blockErrorWords; i++)
   {
      /* Calculate syndrome s[i] */
      for(j = 0; j < blockTotalWords; j++)
         s[i] = GfAdd(s[i], GfMultAntilog(recd[j], i*j));

      /* Non-zero syndrome indicates error */
      if(s[i] != 0)
         error = DmtxTrue;

      /* Convert syndrome to index form */
      s[i] = log301[s[i]]; /* XXX does it still make sense to do this? */
   }

   return error;
}

#ifdef IGNORE_THIS_CODE
/**
 * My take on it
 * Note that i == 0 in code represents i == -1 in book. Not sure how to make that clear in code.
 *
 * i == 2
 * l[2] = 1
 * d[2] = s[2] + s[1]*elp[2][0]
 * d[i] = s[i] + s[i-1] * elp[i][0]
 *
 * l[3] = 1
 * d[3] = s[3] + s[2]*elp[3][0]
 * d[i] = s[i] + s[i-1] * elp[i][0]
 *
 * l[4] = 2
 * d[4] = s[4] + s[3]*elp[4][0]   + s[2]*elp[4][1]
 * d[i] = s[i] + s[i-1]*elp[i][0] + s[i-2]*elp[i][1]
 *
 * l[5] = 2
 * d[5] = s[5] + s[4]*elp[5][0]   + s[3]*elp[5][1]
 * d[i] = s[i] + s[i-1]*elp[i][0] + s[i-2]*elp[i][1]
 *
 * l[6] = 3
 * d[6] = s[6] + s[5]*elp[6][0]   + s[4]*elp[6][1]   + s[3]*elp[6][2]
 * d[i] = s[i] + s[i-1]*elp[i][0] + s[i-2]*elp[i][1] + s[i-3]*elp[i][2]
 *
 * for(j = 0, d[i] = s[i]; j < l[i]; j++)
 *    d[i] = GfSum(d[i], GfMult(s[i-j-1], elp[i][j]));
 *
 */
static DmtxPassFail
RsFindErrorLocatorPoly(unsigned char *s, int errorWordCount, int totalWordCount, int maxCorrectable)
{
   unsigned char elp[MAX_ERROR_WORD_COUNT + 2][MAX_ERROR_WORD_COUNT] /* [step][order] */
   int l[MAX_ERROR_WORD_COUNT + 1];
   int d[MAX_ERROR_WORD_COUNT];

   /* i = 0 */
   memset(elp[0], 1, sizeof(unsigned char) * MAX_ERROR_WORD_COUNT);
   l[0] = 0;
   d[0] = 1;

   /* i = 1 */
   memset(elp[1], 1, sizeof(unsigned char) * MAX_ERROR_WORD_COUNT);
   l[1] = 0;
   d[1] = s[1];

   /* i = 2..errorWordCount */
   for(i = 2, iPrev = 1; /* explicit break */; iPrev = i++)
   {
      if(d[iPrev] == 0)
      {
         /* Simple case: Direct copy */
         memcpy(elp[i], elp[iPrev], sizeof(unsigned char) * MAX_ERROR_WORD_COUNT);
         l[i] = l[iPrev];
      }
      else
      {
         /* Find earlier iteration (m) that provides maximal something XXX */
         lMaxIdx = 0;
         for(m = 1; m < i; m++) /* is direction of iterations important? */
         {
            if(d[m] != 0 && l[m] > l[lMaxIdx])
               lMaxIdx = m;
         }

         /* Calculate error correction polynomial elp[i] XXX less stuck now */
         for(j = 0; j < l[i]; j++)
         {
            elp[i][j] = antilog301[(d[i] + NN - d[q] + elp[x][x]) % NN];
                                         *    /      *
                                  everything is in polynomial form though
            elp[i][j] = antilog301[(d[i] + NN - d[q] + elp[x][x]) % NN];
         }

         /* Set l */
         lTmp = l[m] + iPrev - m;
         l[i] = max(l[i], lTmp);
      }

      if(i == errorWordCount || l[i] > maxCorrectable)
         break;

      /* Calculate discrepancy d[i] */
      for(j = 0, d[i] = s[i]; j < l[i]; j++)
         d[i] = GfSum(d[i], GfMult(s[i-j-1], elp[i][j]));
   }

   return (l[i] > maxCorrectable) ? DmtxFail : DmtxPass;
}
#endif

/**
 * Find the error location polynomial elp[i] using Berlekamp-Massey
 *
 * Return DmtxFail if uncorrectable errors are present (degree of elp is > tt)
 *        otherwise DmtxPass
 *
 * kk == nn - 2*tt (either data words or data words plus padding)
 * nn - kk == nn - (nn - 2*tt) == 2 * tt == blockErrorWords (?)
 * l[] is lambda?
 * d[] is discrepancy?
 * u_lup[] is ?
 * z[] is ?
 * reg[] is ?
 * u is step
 * q is ?
 */
static DmtxPassFail
RsFindErrorLocatorPoly(unsigned char *s, int errorWordCount, int totalWordCount, int maxCorrectable)
{
   int i, j, u, q;
   int d[MAX_ERROR_WORD_COUNT + 2];                         /* */
   int elp[MAX_ERROR_WORD_COUNT + 2][MAX_ERROR_WORD_COUNT]; /* Error locator polynomial */
   int l[MAX_ERROR_WORD_COUNT + 2];
   int u_lu[MAX_ERROR_WORD_COUNT + 2];

   /* Initialize table entries */
   d[0] = 0;              /* Index form */
   d[1] = s[1];           /* Index form */

   elp[0][0] = 0;         /* Index form */
   elp[1][0] = 1;         /* Polynomial form */
   for(i = 1; i < errorWordCount; i++)
   {
      elp[0][i] = GF_NAN; /* Index form */
      elp[1][i] = 0;      /* Polynomial form */
   }

   l[0] = 0;
   l[1] = 0;
   u_lu[0] = GF_NAN;
   u_lu[1] = 0;
   u = 0;

   do {
      u++;
      if(d[u] == GF_NAN)
      {
         l[u+1] = l[u];
         for(i = 0; i <= l[u]; i++)
         {
            elp[u+1][i] = elp[u][i];
            elp[u][i] = log301[elp[u][i]];
         }
      }
      else
      {
         /* search for words with greatest u_lu[q] for which d[q] != 0 */
         q = u - 1;

         while(d[q] == GF_NAN && q > 0)
            q--;

         /* Have found first non-zero d[q] */
         if(q > 0)
         {
            j = q;
            do {
               j--;
               if(d[j] != GF_NAN && u_lu[q] < u_lu[j])
                  q = j;
            } while(j > 0);
         }

         /* Found q such that d[u] != 0 and u_lu[q] is maximum store degree of new elp polynomial */
         l[u+1] = max(l[u], l[q] + u - q);

         /* Form new elp(x) */
         for(i = 0; i < errorWordCount; i++)
            elp[u+1][i] = 0;

         for(i = 0; i <= l[q]; i++)
         {
            if(elp[q][i] != GF_NAN)
               elp[u+1][i+u-q] = antilog301[(d[u] + NN - d[q] + elp[q][i]) % NN];
         }

         for(i = 0; i <= l[u]; i++)
         {
            elp[u+1][i] = GfAdd(elp[u+1][i], elp[u][i]);
            elp[u][i] = log301[elp[u][i]]; /* Convert old elp value to index */
         }
      }
      u_lu[u+1] = u - l[u+1];

      /* Form (u+1)th discrepancy */
      if(u < errorWordCount) /* No discrepancy computed on last iteration */
      {
         d[u+1] = antilog301[s[u+1]];

         for(i = 1; i <= l[u+1]; i++)
         {
            if((s[u+1-i] != GF_NAN) && (elp[u+1][i] != 0))
               d[u+1] ^= antilog301[(s[u+1-i] + log301[elp[u+1][i]]) % NN];
         }
         d[u+1] = log301[d[u+1]]; /* put d[u+1] into index form */
      }
   } while(u < errorWordCount && l[u+1] <= maxCorrectable);

   return (l[u] > maxCorrectable) ? DmtxFail : DmtxPass;
}

/**
 * 1) Find the error location polynomial elp[i] using Berlekamp-Massey
 *
 *    If the degree of elp is > tt, we cannot correct all the errors (done)
 *
 * 2) Find the error positions (using Chien Search?)
 *
 *    If the degree of elp is <= tt, we substitute alpha**i, i=1..n into the elp
 *    to get the roots, hence the inverse roots, the error location numbers.
 *
 *    If the number of errors located does not equal the degree of the elp, we
 *    have more than tt errors and cannot correct them.
 *
 * 3) Find the error values (using Forney?)
 *
 *    Solve for the error value at the error location and correct the error. The
 *    procedure is that found in Lin and Costello.
 *
 *    For the cases where the number of errors is known to be too large to
 *    correct, the information symbols as received are output (the advantage of
 *    systematic encoding is that hopefully some of the information symbols will
 *    be okay and that if we are in luck, the errors are in the parity part of
 *    the transmitted codeword).
 *
 * nn == NN == 255
 * tt == number of errors that can be corrected = maxCorrectableErrors
 * kk == nn - 2*tt (either data words or data words plus padding)
 * nn - kk == nn - (nn - 2*tt) == 2 * tt == blockErrorWords (?)
 * elp is error location polynomial
 * l[] is lambda?
 * d[] is discrepancy?
 * u_lup[] is ?
 * z[] is ?
 * reg[] is ?
 * u is step ?
 * q is ?
 */
static DmtxBoolean
RsRepairErrors(unsigned char *recd, unsigned char *s, int blockErrorWords, int blockTotalWords)
{
   DmtxBoolean error = DmtxTrue;

/*
   int i, j, u, q;
   int l[MAX_ERROR_WORD_COUNT + 2];
   int u_lu[MAX_ERROR_WORD_COUNT + 2];
   int root[tt], loc[tt], z[tt+1], err[nn], reg[tt+1];
   int count = 0;

   u++;

   // elp has degree has degree > tt hence cannot solve
   if(l[u] > tt)
   {
      // could return error flag if desired
      // convert recd[] to polynomial form or output as is
      for(i = 0; i < NN; i++)
         recd[i] = (recd[i] == -1) ? 0 : antilog301[recd[i]];
   }
   else // can correct error
   {
      // put elp into index form
      for(i = 0; i <= l[u]; i++)
         elp[u][i] = log301[elp[u][i]];

      //
      // Find error positions (Chien?)
      //

      // find roots of the error location polynomial
      for(i = 1; i <= l[u]; i++)
         reg[i] = elp[u][i];

      count = 0;
      for(i = 1; i <= NN; i++)
      {
         q = 1;
         for(j = 1; j <= l[u]; j++)
         {
            if(reg[j] != -1)
            {
               reg[j] = (reg[j] + j) % NN;
               q ^= antilog301[reg[j]];
            }
         }

         if(!q)
         {
            // store root and error location number indices
            root[count] = i;
            loc[count] = NN - i;
            count++;
         }
      }

      //
      // Find error values (Forney?)
      //

      if(count == l[u]) // no. roots = degree of elp hence <= tt errors
      {
         // form polynomial z(x)
         for(i = 1; i <= l[u]; i++)        // Z[0] = 1 always - do not need
         {
            if((s[i] != -1) && (elp[u][i] != -1))
               z[i] = antilog301[s[i]] ^ antilog301[elp[u][i]];
            else if((s[i] != -1) && (elp[u][i] == -1))
               z[i] = antilog301[s[i]];
            else if((s[i] == -1) && (elp[u][i] != -1))
               z[i] = antilog301[elp[u][i]];
            else
               z[i] = 0;

            for(j = 1; j < i; j++)
            {
               if(s[j] != -1 && elp[u][i-j] != -1)
                  z[i] ^= antilog301[(elp[u][i-j] + s[j]) % NN];
            }
            z[i] = log301[z[i]]; // put into index form
         }

         // evaluate errors at locations given by error location numbers loc[i]
         for(i = 0; i < NN; i++)
         {
            err[i] = 0;
            recd[i] = (recd[i] == -1) ? 0 : antilog301[recd[i]]; // convert recd[] to polynomial form
         }

         // compute numerator of error term first
         for(i = 0; i < l[u]; i++)
         {
            err[loc[i]] = 1; // accounts for z[0]
            for(j = 1; j <= l[u]; j++)
            {
               if(z[j]!=-1)
                  err[loc[i]] ^= antilog301[(z[j]+j*root[i]) % NN];
            }
            if(err[loc[i]] != 0)
            {
               err[loc[i]] = log301[err[loc[i]]];
               q = 0; // form denominator of error term
               for(j = 0; j < l[u]; j++)
               {
                  if(j != i)
                     q += log301[1^antilog301[(loc[j]+root[i]) % NN]];
               }
               q = q % NN;
               err[loc[i]] = antilog301[(err[loc[i]]-q+nn) % NN];
               recd[loc[i]] ^= err[loc[i]]; // recd[i] must be in polynomial form
            }
         }
      }
      else // no. roots != degree of elp => >tt errors and cannot solve
      {
         // could return error flag if desired
         // convert recd[] to polynomial form or output as is
         for(i = 0; i < NN; i++)
            recd[i] = (recd[i] == -1) ? 0 : antilog301[recd[i]];
      }
   }
*/

   return error;
}
