/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2007, 2008, 2009 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in parent directory for full terms of
 * use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file display.h
 */

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
void DrawPane2(SDL_Surface *screen, unsigned char *pxl);
void DrawPane3(SDL_Surface *screen, unsigned char *pxl);
void DrawPane4(SDL_Surface *screen, unsigned char *pxl);
void DrawPane5(SDL_Surface *screen, unsigned char *pxl);
void DrawPane6(SDL_Surface *screen, unsigned char *pxl);
int HandleEvent(SDL_Event *event, SDL_Surface *screen);
void DrawPaneBorder(GLint x, GLint y, GLint h, GLint w);
