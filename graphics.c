/* Source file for graphics.c, controlling gameboy graphics
  Author: Max Croucher
  Email: mpccroucher@gmail.com
  May 2025
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <GL/freeglut.h>
#include <time.h>
#include "graphics.h"

clock_t start,end;

double frametime = 0;
uint8_t framecount_offset = 0;
#define FRAMETIME_BUFSIZE 10

uint8_t xoffset = 0;
uint32_t dot = 0;

GLubyte texture[144][160][3];

void init_screen_tex(GLubyte texture[144][160][3]) {
    /* initialise the screen texture */
    for (int i=0; i<144; i++) {
        float r = (float)((i+xoffset)%144) / 144.0 * 255.0;
        for (int j=0; j<160; j++) {
            float g = (float)j / 160 * 255.0;
            texture[i][j][0] = (GLubyte)r;
            texture[i][j][1] = (GLubyte)g;
            texture[i][j][2] = (GLubyte)0;
        }
    }
    for (int i=40; i < 60; i++) {
        for (int j=40; j < 60; j++) {
            texture[i][j][0] = (GLubyte)0;
            texture[i][j][1] = (GLubyte)0;
            texture[i][j][2] = (GLubyte)0;
            
        }
            
    }
    glEnable(GL_TEXTURE_2D);
    glTexImage2D(GL_TEXTURE_2D,0,3,160,144,0,GL_RGB, GL_UNSIGNED_BYTE, texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void gl_tick(void) {
    /* update graphics */
    glClear(GL_COLOR_BUFFER_BIT);
    glPushMatrix();
    //glColor3f(0.0,1.0,0.0);
    init_screen_tex(texture);
    glBegin(GL_POLYGON);
        glTexCoord2f(0,0); glVertex2f(0,0);
        glTexCoord2f(0,1); glVertex2f(0,1);
        glTexCoord2f(1,1); glVertex2f(1,1);
        glTexCoord2f(1,0); glVertex2f(1,0);
    glEnd();
	glDisable(GL_TEXTURE_2D);

    glPopMatrix();
    glutSwapBuffers();
}


void reshape_window(int w, int h) {
    /* Ensure window is accurately scaled*/
    glViewport(0,0,(GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,1,0,1,-1.0,1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}


void init_graphics(int *argc, char *argv[]) {
    /* Main init procedure for graphics */
    glutInit(argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(320,288);
    glutInitWindowPosition(100,100);
    glutCreateWindow("GB Emu");
    glClearColor(0.0,0.0,0.0,0.0);
    glShadeModel(GL_FLAT);
    glutDisplayFunc(gl_tick);
    glutReshapeFunc(reshape_window);
    start = clock();
    //glutMainLoop();
}


void tick_graphics(uint8_t *ram) {
    /* Main tick procedure for graphics */
    
    dot++;
    if (dot == 70224) {
        dot = 0;
        xoffset++;
        if (xoffset == 144) xoffset = 0;
        glutMainLoopEvent();
        glutPostRedisplay();
        end = clock();
        frametime += ((double)(end-start)) / CLOCKS_PER_SEC;
        framecount_offset++;
        if (framecount_offset == (FRAMETIME_BUFSIZE-1)) {
            framecount_offset = 0;
            fprintf(stderr, "framerate: %.2fHz\n", 1.0/(frametime/FRAMETIME_BUFSIZE));
            frametime = 0;
        }
        printf("framerate: %.2fHz\n", 1.0/(frametime/10));
        start = clock();
    }
}