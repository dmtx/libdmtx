/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2007, 2008, 2009 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in main project directory for full terms
 * of use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file display.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "dmtx.h"
#include "rotate_test.h"
#include "display.h"

/**
 *
 *
 */
SDL_Surface *initDisplay(void)
{
   SDL_Surface *screen;

   SDL_Init(SDL_INIT_VIDEO);

   screen = SDL_SetVideoMode(968, 646, 16, SDL_OPENGL | SDL_RESIZABLE);
   if(!screen) {
      fprintf(stderr, "Couldn't set 968x646 GL video mode: %s\n", SDL_GetError());
      SDL_Quit();
      exit(2);
   }
   SDL_WM_SetCaption("GL Test", "GL Test");

   glClearColor(0.0, 0.0, 0.3, 1.0);

   return screen;
}

/**
 *
 *
 */
void DrawBarCode(void)
{
   glColor3f(0.95, 0.95, 0.95);
   glBegin(GL_QUADS);
   glTexCoord2d(0.0, 0.0); glVertex3f(-2.0, -2.0,  0.0);
   glTexCoord2d(1.0, 0.0); glVertex3f( 2.0, -2.0,  0.0);
   glTexCoord2d(1.0, 1.0); glVertex3f( 2.0,  2.0,  0.0);
   glTexCoord2d(0.0, 1.0); glVertex3f(-2.0,  2.0,  0.0);
   glEnd();
}

/**
 *
 *
 */
void ReshapeWindow(int width, int height)
{
   glViewport(2, 324, (GLint)320, (GLint)320);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 50.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}

/**
 *
 *
 */
void DrawBorders(SDL_Surface *screen)
{
   /* window and pane borders */
   DrawPaneBorder(  0,   0, 646, 968);

   DrawPaneBorder(  1,   1, 322, 322);
   DrawPaneBorder(323,   1, 322, 322);
   DrawPaneBorder(645,   1, 322, 322);

   DrawPaneBorder(  1, 323, 322, 322);
   DrawPaneBorder(323, 323, 322, 322);
   DrawPaneBorder(645, 323, 322, 322);
}

/**
 *
 *
 */
void DrawGeneratedImage(SDL_Surface *screen)
{
   /* rotate barcode surface */
   glViewport(2, 324, (GLint)320, (GLint)320);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 50.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -10.0);
   glPolygonMode(GL_FRONT, GL_FILL);
   glPolygonMode(GL_BACK, GL_LINE);
   glEnable(GL_TEXTURE_2D);

   glPushMatrix();
   glRotatef(view_rotx, 1.0, 0.0, 0.0);
   glRotatef(view_roty, 0.0, 1.0, 0.0);
   glRotatef(view_rotz, 0.0, 0.0, 1.0);
   glRotatef(angle, 0.0, 0.0, 1.0);
   glCallList(barcodeList);
   glPopMatrix();
}

/**
 *
 *
 */
void DrawPane2(SDL_Surface *screen, unsigned char *pxl)
{
   DrawPaneBorder(323, 323, 322, 322); /* XXX drawn twice */
   glRasterPos2i(1, 1);
   glDrawPixels(320, 320, GL_RGB, GL_UNSIGNED_BYTE, pxl);
}

/**
 *
 *
 */
void DrawPane3(SDL_Surface *screen, unsigned char *pxl)
{
   DrawPaneBorder(645, 323, 322, 322); /* XXX drawn twice */
   glRasterPos2i(1, 1);
   glDrawPixels(320, 320, GL_RGB, GL_UNSIGNED_BYTE, pxl);
}

/**
 *
 *
 */
void DrawPane4(SDL_Surface *screen, unsigned char *pxl)
{
   DrawPaneBorder(1, 1, 322, 322); /* XXX drawn twice */
   glRasterPos2i(1, 1);
   glDrawPixels(320, 320, GL_RGB, GL_UNSIGNED_BYTE, pxl);
}

/**
 *
 *
 */
void DrawPane5(SDL_Surface *screen, unsigned char *pxl)
{
   DrawPaneBorder(323, 1, 322, 322); /* XXX drawn twice */
   glRasterPos2i(1, 1);
   glDrawPixels(320, 320, GL_RGB, GL_UNSIGNED_BYTE, pxl);
}

/**
 *
 *
 */
void DrawPane6(SDL_Surface *screen, unsigned char *pxl)
{
   DrawPaneBorder(645, 1, 322, 322); /* XXX drawn twice */
   glRasterPos2i(1, 1);

   if(pxl != NULL)
      glDrawPixels(320, 320, GL_RGB, GL_UNSIGNED_BYTE, pxl);
}

/**
 *
 *
 */
void DrawPaneBorder(GLint x, GLint y, GLint h, GLint w)
{
   glDisable(GL_TEXTURE_2D);
   glColor3f(0.6, 0.6, 1.0);
   glPolygonMode(GL_FRONT, GL_LINE);
   glViewport(x, y, w, w);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-0.5, w-0.5, -0.5, w-0.5, -1.0, 10.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glBegin(GL_QUADS);
   glVertex2f(0, 0);
   glVertex2f(w-1, 0);
   glVertex2f(w-1, h-1);
   glVertex2f(0, h-1);
   glEnd();
}

/**
 *
 *
 */
int HandleEvent(SDL_Event *event, SDL_Surface *screen)
{
   int width, height;

   switch(event->type) {
      case SDL_VIDEORESIZE:
         screen = SDL_SetVideoMode(event->resize.w, event->resize.h, 16,
               SDL_OPENGL | SDL_RESIZABLE);
         if(screen) {
            ReshapeWindow(screen->w, screen->h);
         }
         else {
            /* Uh oh, we couldn't set the new video mode? */;
            return(1);
         }
         break;

      case SDL_QUIT:
         return(1);
         break;

      case SDL_MOUSEMOTION:
         view_rotx = ((event->motion.y-160)/2.0);
         view_roty = ((event->motion.x-160)/2.0);
         break;

      case SDL_KEYDOWN:
         switch(event->key.keysym.sym) {
            case SDLK_ESCAPE:
               return(1);
               break;
            default:
               break;
         }
         break;

      case SDL_MOUSEBUTTONDOWN:
         switch(event->button.button) {
            case SDL_BUTTON_RIGHT:
               free(texturePxl);
               texturePxl = (unsigned char *)loadTextureImage(&width, &height);
               break;
            case SDL_BUTTON_LEFT:
               fprintf(stdout, "left click\n");
               break;
            default:
               break;
         }
         break;
   }

   return(0);
}
