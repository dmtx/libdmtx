/*
libdmtx - Data Matrix Encoding/Decoding Library
Copyright (C) 2007 Mike Laughton

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

Contact: mblaughton@users.sourceforge.net
*/

/* $Id$ */

#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL/SDL.h>

GLfloat view_rotx, view_roty, view_rotz;
GLfloat angle;

GLuint   barcodeTexture;
GLint    barcodeList;

SDL_Surface *initDisplay(void);
void DrawBarCode(void);
void ReshapeWindow(int width, int height);
void DrawGeneratedImage(SDL_Surface *screen);
void DrawBorders(SDL_Surface *screen);
void DrawPane2(SDL_Surface *screen, DmtxImage *image);
void DrawPane3(SDL_Surface *screen, DmtxImage *image);
void DrawPane4(SDL_Surface *screen, DmtxImage *image);
void DrawPane5(SDL_Surface *screen, DmtxImage *image);
void DrawPane6(SDL_Surface *screen, DmtxImage *image);
int HandleEvent(SDL_Event *event, SDL_Surface *screen);
void DrawPaneBorder(GLint x, GLint y, GLint h, GLint w);
