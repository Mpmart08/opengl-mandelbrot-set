// Calculate and display the Mandelbrot set
// ECE4893/8893 final project, Spring 2017

#include <iostream>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stack>

#include <GL/glut.h>
#include <GL/glext.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "complex.h"

using namespace std;

#define WIN 512
#define N   16

// Thread condition variables
pthread_mutex_t activeThreadsMutex;
pthread_mutex_t exitMutex;
pthread_cond_t exitCond;
pthread_barrier_t barrier;
int activeThreads;

// keep track of mouse location
int mouse_x1;
int mouse_y1;
int mouse_x2;
int mouse_y2;
int minX;
int maxX;
int minY;
int maxY;

// keep track of mouse click
bool mouse_pressed = false;

// Min and max complex plane values
Complex  minC(-2.0, -1.2);
Complex  maxC( 1.0,  1.8);
int      maxIt = 2000;     // Max iterations for the set computations

// 2D array that stores mandlebrot set
int mbset[WIN][WIN];

// struct that represents one frame in navigation history
struct frame {
    Complex minC;
    Complex maxC;
};

// stack to maintain navigation history
stack<frame> history;

void drawSquare()
{
    glBegin(GL_LINE_LOOP);
    glColor3f(1.0, 0.0, 0.0); // red lines
    glVertex2i(minX, WIN - minY);
    glVertex2i(minX, WIN - maxY);
    glVertex2i(maxX, WIN - maxY);
    glVertex2i(maxX, WIN - minY);
    glVertex2i(minX, WIN - minY);
    glEnd();
}

void drawMBSet()
{
    glBegin(GL_POINTS);
    for (int i = 0; i < WIN; ++i){
        for (int j = 0; j < WIN; ++j){
            if (mbset[i][j] > 5){
                if (mbset[i][j] == maxIt){
                    glColor3f(0.0, 0.0, 0.0); // black pixel
                }
                else {
                    srand(mbset[i][j]);
                    GLfloat r = ((GLfloat)rand()) / RAND_MAX;
                    GLfloat g = ((GLfloat)rand()) / RAND_MAX;
                    GLfloat b = ((GLfloat)rand()) / RAND_MAX;
                    glColor3f(r, g, b); // color determined by number of iterations
                }
                glVertex2i(i, j);
            }
        }
    }
    glEnd();
}

void* computeMBSet(void* v)
{
    unsigned long threadID = (unsigned long) v;

    GLdouble x = (maxC.real - minC.real) / WIN;
    GLdouble y = (maxC.imag - minC.imag) / WIN;

    for (int i = threadID; i < WIN; i += N) {
        for (int j = 0; j < WIN; j++){
            Complex c((x * j) + minC.real, maxC.imag - (y * i));
            Complex z = c;
            int k;
            for (k = 0; k < maxIt && z.Mag().real <= 2.0; k++){
                z = (z * z) + c;
            }
            mbset[j][i] = k;
        }
    }

    // Decrement active count and signal main if all complete
    pthread_mutex_lock(&activeThreadsMutex);
    activeThreads--;

    if (activeThreads == 0) {
        pthread_mutex_unlock(&activeThreadsMutex);
        pthread_mutex_lock(&exitMutex);
        pthread_cond_signal(&exitCond);
        pthread_mutex_unlock(&exitMutex);
    } else {
        pthread_mutex_unlock(&activeThreadsMutex);
    }
}

void generateMBSet()
{
    // pthread initialization here
    pthread_mutex_init(&exitMutex,0);
    pthread_mutex_init(&activeThreadsMutex,0);
    pthread_cond_init(&exitCond, 0);

    // Main holds the exit mutex until waiting for exitCond condition
    pthread_mutex_lock(&exitMutex);
    
    pthread_barrier_init(&barrier, NULL, N);

    activeThreads = N;
    for (int i = 0; i < N; i++) {
        pthread_t thread;
        pthread_create(&thread, 0, computeMBSet, (void*) i);
    }

    // Wait for all threads to complete
    pthread_cond_wait(&exitCond, &exitMutex);
}

void display(void)
{ // Your OpenGL display code here

    glClear(GL_COLOR_BUFFER_BIT);

    drawMBSet();

    if (mouse_pressed) {
        drawSquare();
    }

    glFlush();
}

void init()
{ // Your OpenGL initialization code here
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glShadeModel(GL_FLAT);
}

void reshape(int w, int h)
{ // Your OpenGL window reshape code here
    gluOrtho2D(0.0, WIN, WIN, 0.0);
}

void mouse(int button, int state, int x, int y)
{ // Your mouse click processing here
  // state == 0 means pressed, state != 0 means released
  // Note that the x and y coordinates passed in are in
  // PIXELS, with y = 0 at the top.
    if (state == 0){

        mouse_x1 = x;
        mouse_y1 = WIN - y;
        mouse_pressed = true;

    } else {

        mouse_pressed = false;

        if (mouse_x2 != mouse_x1 || mouse_y2 != mouse_y1){

            double width = maxC.real - minC.real;

            frame previousFrame = {minC, maxC};
            history.push(previousFrame);

            maxC = Complex(minC.real + maxX*width/WIN, minC.imag + maxY*width/WIN);
            minC = Complex(minC.real + minX*width/WIN, minC.imag + minY*width/WIN);

            generateMBSet();
        }

        glutPostRedisplay();
    }
}

void motion(int x, int y)
{ // Your mouse motion here, x and y coordinates are as above
    if (mouse_pressed) {
        mouse_x2 = x;
        mouse_y2 = WIN - y;

        if (mouse_x2 != mouse_x1 || mouse_y2 != mouse_y1) {

            if (abs(mouse_x2 - mouse_x1) > abs(mouse_y2 - mouse_y1)){
                if (mouse_y2 < mouse_y1){
                    mouse_y2 = mouse_y1 - abs(mouse_x2 - mouse_x1);
                }
                else{
                    mouse_y2 = mouse_y1 + abs(mouse_x2 - mouse_x1);
                }
            }
            else {
                if (mouse_x2 < mouse_x1){
                    mouse_x2 = mouse_x1 - abs(mouse_y2 - mouse_y1);
                }
                else{
                    mouse_x2 = mouse_x1 + abs(mouse_y2 - mouse_y1);
                }
            }

            minX = min(mouse_x2, mouse_x1);
            maxX = max(mouse_x2, mouse_x1);
            minY = min(mouse_y2, mouse_y1);
            maxY = max(mouse_y2, mouse_y1);

            glutPostRedisplay();
        }
    }
}

void keyboard(unsigned char c, int x, int y)
{ // Your keyboard processing here
    if (c == 'b' && !history.empty()) {
        frame previousFrame = history.top();
        history.pop();
        minC = previousFrame.minC;
        maxC = previousFrame.maxC;
        generateMBSet();
        glutPostRedisplay();
    }
}

int main(int argc, char** argv)
{
    // Initialize OpenGL, but only on the "master" thread or process.
    // See the assignment writeup to determine which is "master" 
    // and which is slave.
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB);
    glutInitWindowSize(WIN, WIN);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("Mandelbrot Set");
    generateMBSet();
    
    // Set your glut callbacks here
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);

    // Enter the glut main loop here
    glutMainLoop();
    return 0;
}

