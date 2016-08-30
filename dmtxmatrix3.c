/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2008, 2009 Mike Laughton. All rights reserved.
 * Copyright 2012-2016 Vadim A. Misbakh-Soloviov. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact:
 * Vadim A. Misbakh-Soloviov <dmtx@mva.name>
 * Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxmatrix3.c
 * \brief 2D Matrix (3x3) math
 */

/**
 * \brief  Copy matrix contents
 * \param  m0 Copy target
 * \param  m1 Copy source
 * \return void
 */
extern void
dmtxMatrix3Copy(DmtxMatrix3 m0, DmtxMatrix3 m1)
{
   memcpy(m0, m1, sizeof(DmtxMatrix3));
}

/**
 * \brief  Generate identity transformation matrix
 * \param  m Generated matrix
 * \return void
 *
 *      | 1  0  0 |
 *  m = | 0  1  0 |
 *      | 0  0  1 |
 *
 *                  Transform "m"
 *            (doesn't change anything)
 *                       |\
 *  (0,1)  x----o     +--+ \    (0,1)  x----o
 *         |    |     |     \          |    |
 *         |    |     |     /          |    |
 *         +----*     +--+ /           +----*
 *  (0,0)     (1,0)      |/     (0,0)     (1,0)
 *
 */
extern void
dmtxMatrix3Identity(DmtxMatrix3 m)
{
   static DmtxMatrix3 tmp = { {1, 0, 0},
                              {0, 1, 0},
                              {0, 0, 1} };
   dmtxMatrix3Copy(m, tmp);
}

/**
 * \brief  Generate translate transformation matrix
 * \param  m Generated matrix
 * \param  tx
 * \param  ty
 * \return void
 *
 *      | 1  0  0 |
 *  m = | 0  1  0 |
 *      | tx ty 1 |
 *
 *                  Transform "m"
 *                      _____    (tx,1+ty)  x----o  (1+tx,1+ty)
 *                      \   |               |    |
 *  (0,1)  x----o       /   |      (0,1)  +-|--+ |
 *         |    |      /  /\|             | +----*  (1+tx,ty)
 *         |    |      \ /                |    |
 *         +----*       `                 +----+
 *  (0,0)     (1,0)                (0,0)     (1,0)
 *
 */
void dmtxMatrix3Translate(DmtxMatrix3 m, double tx, double ty)
{
   dmtxMatrix3Identity(m);
   m[2][0] = tx;
   m[2][1] = ty;
}

/**
 * \brief  Generate rotate transformation
 * \param  m Generated matrix
 * \param  angle
 * \return void
 *
 *     |  cos(a)  sin(a)  0 |
 * m = | -sin(a)  cos(a)  0 |
 *     |  0       0       1 |
 *                                       o
 *                  Transform "m"      /   `
 *                       ___         /       `
 *  (0,1)  x----o      |/   \       x          *  (cos(a),sin(a))
 *         |    |      '--   |       `        /
 *         |    |        ___/          `    /  a
 *         +----*                        `+  - - - - - -
 *  (0,0)     (1,0)                     (0,0)
 *
 */
extern void
dmtxMatrix3Rotate(DmtxMatrix3 m, double angle)
{
   double sinAngle, cosAngle;

   sinAngle = sin(angle);
   cosAngle = cos(angle);

   dmtxMatrix3Identity(m);
   m[0][0] = cosAngle;
   m[0][1] = sinAngle;
   m[1][0] = -sinAngle;
   m[1][1] = cosAngle;
}

/**
 * \brief  Generate scale transformation matrix
 * \param  m Generated matrix
 * \param  sx
 * \param  sy
 * \return void
 *
 *     | sx 0  0 |
 * m = | 0  sy 0 |
 *     | 0  0  1 |
 *
 *                  Transform "m"
 *                      _____     (0,sy)  x-------o (sx,sy)
 *                      \   |             |       |
 *  (0,1)  x----o       /   |      (0,1)  +----+  |
 *         |    |      /  /\|             |    |  |
 *         |    |      \ /                |    |  |
 *         +----*       `                 +----+--*
 *  (0,0)     (1,0)                (0,0)            (sx,0)
 *
 */
extern void
dmtxMatrix3Scale(DmtxMatrix3 m, double sx, double sy)
{
   dmtxMatrix3Identity(m);
   m[0][0] = sx;
   m[1][1] = sy;
}

/**
 * \brief  Generate shear transformation matrix
 * \param  m Generated matrix
 * \param  shx
 * \param  shy
 * \return void
 *
 *     | 0    shy  0 |
 * m = | shx  0    0 |
 *     | 0    0    1 |
 */
extern void
dmtxMatrix3Shear(DmtxMatrix3 m, double shx, double shy)
{
   dmtxMatrix3Identity(m);
   m[1][0] = shx;
   m[0][1] = shy;
}

/**
 * \brief  Generate top line skew transformation
 * \param  m
 * \param  b0
 * \param  b1
 * \param  sz
 * \return void
 *
 *     | b1/b0    0    (b1-b0)/(sz*b0) |
 * m = |   0    sz/b0         0        |
 *     |   0      0           1        |
 *
 *     (sz,b1)  o
 *             /|    Transform "m"
 *            / |
 *           /  |        +--+
 *          /   |        |  |
 * (0,b0)  x    |        |  |
 *         |    |      +-+  +-+
 * (0,sz)  +----+       \    /    (0,sz)  x----o
 *         |    |        \  /             |    |
 *         |    |         \/              |    |
 *         +----+                         +----+
 *  (0,0)    (sz,0)                (0,0)    (sz,0)
 *
 */
extern void
dmtxMatrix3LineSkewTop(DmtxMatrix3 m, double b0, double b1, double sz)
{
   assert(b0 >= DmtxAlmostZero);

   dmtxMatrix3Identity(m);
   m[0][0] = b1/b0;
   m[1][1] = sz/b0;
   m[0][2] = (b1 - b0)/(sz*b0);
}

/**
 * \brief  Generate top line skew transformation (inverse)
 * \param  m
 * \param  b0
 * \param  b1
 * \param  sz
 * \return void
 */
extern void
dmtxMatrix3LineSkewTopInv(DmtxMatrix3 m, double b0, double b1, double sz)
{
   assert(b1 >= DmtxAlmostZero);

   dmtxMatrix3Identity(m);
   m[0][0] = b0/b1;
   m[1][1] = b0/sz;
   m[0][2] = (b0 - b1)/(sz*b1);
}

/**
 * \brief  Generate side line skew transformation
 * \param  m
 * \param  b0
 * \param  b1
 * \param  sz
 * \return void
 */
extern void
dmtxMatrix3LineSkewSide(DmtxMatrix3 m, double b0, double b1, double sz)
{
   assert(b0 >= DmtxAlmostZero);

   dmtxMatrix3Identity(m);
   m[0][0] = sz/b0;
   m[1][1] = b1/b0;
   m[1][2] = (b1 - b0)/(sz*b0);
}

/**
 * \brief  Generate side line skew transformation (inverse)
 * \param  m
 * \param  b0
 * \param  b1
 * \param  sz
 * \return void
 */
extern void
dmtxMatrix3LineSkewSideInv(DmtxMatrix3 m, double b0, double b1, double sz)
{
   assert(b1 >= DmtxAlmostZero);

   dmtxMatrix3Identity(m);
   m[0][0] = b0/sz;
   m[1][1] = b0/b1;
   m[1][2] = (b0 - b1)/(sz*b1);
}

/**
 * \brief  Multiply two matrices to create a third
 * \param  mOut
 * \param  m0
 * \param  m1
 * \return void
 */
extern void
dmtxMatrix3Multiply(DmtxMatrix3 mOut, DmtxMatrix3 m0, DmtxMatrix3 m1)
{
   int i, j, k;
   double val;

   for(i = 0; i < 3; i++) {
      for(j = 0; j < 3; j++) {
         val = 0.0;
         for(k = 0; k < 3; k++) {
            val += m0[i][k] * m1[k][j];
         }
         mOut[i][j] = val;
      }
   }
}

/**
 * \brief  Multiply two matrices in place
 * \param  m0
 * \param  m1
 * \return void
 */
extern void
dmtxMatrix3MultiplyBy(DmtxMatrix3 m0, DmtxMatrix3 m1)
{
   DmtxMatrix3 mTmp;

   dmtxMatrix3Copy(mTmp, m0);
   dmtxMatrix3Multiply(m0, mTmp, m1);
}

/**
 * \brief  Multiply vector and matrix
 * \param  vOut Vector (output)
 * \param  vIn Vector (input)
 * \param  m Matrix to be multiplied
 * \return DmtxPass | DmtxFail
 */
extern int
dmtxMatrix3VMultiply(DmtxVector2 *vOut, DmtxVector2 *vIn, DmtxMatrix3 m)
{
   double w;

   w = vIn->X*m[0][2] + vIn->Y*m[1][2] + m[2][2];
   if(fabs(w) <= DmtxAlmostZero) {
      vOut->X = FLT_MAX;
      vOut->Y = FLT_MAX;
      return DmtxFail;
   }

   vOut->X = (vIn->X*m[0][0] + vIn->Y*m[1][0] + m[2][0])/w;
   vOut->Y = (vIn->X*m[0][1] + vIn->Y*m[1][1] + m[2][1])/w;

   return DmtxPass;
}

/**
 * \brief  Multiply vector and matrix in place
 * \param  v Vector (input and output)
 * \param  m Matrix to be multiplied
 * \return DmtxPass | DmtxFail
 */
extern int
dmtxMatrix3VMultiplyBy(DmtxVector2 *v, DmtxMatrix3 m)
{
   int success;
   DmtxVector2 vOut;

   success = dmtxMatrix3VMultiply(&vOut, v, m);
   *v = vOut;

   return success;
}

/**
 * \brief  Print matrix contents to STDOUT
 * \param  m
 * \return void
 */
extern void
dmtxMatrix3Print(DmtxMatrix3 m)
{
   fprintf(stdout, "%8.8f\t%8.8f\t%8.8f\n", m[0][0], m[0][1], m[0][2]);
   fprintf(stdout, "%8.8f\t%8.8f\t%8.8f\n", m[1][0], m[1][1], m[1][2]);
   fprintf(stdout, "%8.8f\t%8.8f\t%8.8f\n", m[2][0], m[2][1], m[2][2]);
   fprintf(stdout, "\n");
}
