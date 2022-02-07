/*
 * 3-D gear wheels.  This program is in the public domain.
 *
 * Command line options:
 *    -info      print GL implementation information
 *    -exit      automatically exit after 30 seconds
 *
 *
 * Brian Paul
 *
 *
 * Marcus Geelnard:
 *   - Conversion to GLFW
 *   - Time based rendering (frame rate independent)
 *   - Slightly modified camera that should work better for stereo viewing
 *
 *
 * Camilla Löwy:
 *   - Removed FPS counter (this is not a benchmark)
 *   - Added a few comments
 *   - Enabled vsync
 */

#if defined(_MSC_VER)
 // Make MS math.h define M_PI
 #define _USE_MATH_DEFINES
#endif

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

/**

  Draw a gear wheel.  You'll probably want to call this function when
  building a display list since we do a lot of trig here.

  Input:  inner_radius - radius of hole at center
          outer_radius - radius at center of teeth
          width - width of gear teeth - number of teeth
          tooth_depth - depth of tooth

 **/

static void
gear(GLfloat inner_radius, GLfloat outer_radius, GLfloat width,
  GLint teeth, GLfloat tooth_depth)
{
  GLint i;
  GLfloat r0, r1, r2;
  GLfloat angle, da;
  GLfloat u, v, len;

  r0 = inner_radius;
  r1 = outer_radius - tooth_depth / 2.f;
  r2 = outer_radius + tooth_depth / 2.f;

  da = 2.f * (float) M_PI / teeth / 4.f;

  glShadeModel(GL_FLAT);

  glNormal3f(0.f, 0.f, 1.f);

  /* draw front face */
  glBegin(GL_QUAD_STRIP);
  for (i = 0; i <= teeth; i++) {
    angle = i * 2.f * (float) M_PI / teeth;
    glVertex3f(r0 * (float) cos(angle), r0 * (float) sin(angle), width * 0.5f);
    glVertex3f(r1 * (float) cos(angle), r1 * (float) sin(angle), width * 0.5f);
    if (i < teeth) {
      glVertex3f(r0 * (float) cos(angle), r0 * (float) sin(angle), width * 0.5f);
      glVertex3f(r1 * (float) cos(angle + 3 * da), r1 * (float) sin(angle + 3 * da), width * 0.5f);
    }
  }
  glEnd();

  /* draw front sides of teeth */
  glBegin(GL_QUADS);
  da = 2.f * (float) M_PI / teeth / 4.f;
  for (i = 0; i < teeth; i++) {
    angle = i * 2.f * (float) M_PI / teeth;

    glVertex3f(r1 * (float) cos(angle), r1 * (float) sin(angle), width * 0.5f);
    glVertex3f(r2 * (float) cos(angle + da), r2 * (float) sin(angle + da), width * 0.5f);
    glVertex3f(r2 * (float) cos(angle + 2 * da), r2 * (float) sin(angle + 2 * da), width * 0.5f);
    glVertex3f(r1 * (float) cos(angle + 3 * da), r1 * (float) sin(angle + 3 * da), width * 0.5f);
  }
  glEnd();

  glNormal3f(0.0, 0.0, -1.0);

  /* draw back face */
  glBegin(GL_QUAD_STRIP);
  for (i = 0; i <= teeth; i++) {
    angle = i * 2.f * (float) M_PI / teeth;
    glVertex3f(r1 * (float) cos(angle), r1 * (float) sin(angle), -width * 0.5f);
    glVertex3f(r0 * (float) cos(angle), r0 * (float) sin(angle), -width * 0.5f);
    if (i < teeth) {
      glVertex3f(r1 * (float) cos(angle + 3 * da), r1 * (float) sin(angle + 3 * da), -width * 0.5f);
      glVertex3f(r0 * (float) cos(angle), r0 * (float) sin(angle), -width * 0.5f);
    }
  }
  glEnd();

  /* draw back sides of teeth */
  glBegin(GL_QUADS);
  da = 2.f * (float) M_PI / teeth / 4.f;
  for (i = 0; i < teeth; i++) {
    angle = i * 2.f * (float) M_PI / teeth;

    glVertex3f(r1 * (float) cos(angle + 3 * da), r1 * (float) sin(angle + 3 * da), -width * 0.5f);
    glVertex3f(r2 * (float) cos(angle + 2 * da), r2 * (float) sin(angle + 2 * da), -width * 0.5f);
    glVertex3f(r2 * (float) cos(angle + da), r2 * (float) sin(angle + da), -width * 0.5f);
    glVertex3f(r1 * (float) cos(angle), r1 * (float) sin(angle), -width * 0.5f);
  }
  glEnd();

  /* draw outward faces of teeth */
  glBegin(GL_QUAD_STRIP);
  for (i = 0; i < teeth; i++) {
    angle = i * 2.f * (float) M_PI / teeth;

    glVertex3f(r1 * (float) cos(angle), r1 * (float) sin(angle), width * 0.5f);
    glVertex3f(r1 * (float) cos(angle), r1 * (float) sin(angle), -width * 0.5f);
    u = r2 * (float) cos(angle + da) - r1 * (float) cos(angle);
    v = r2 * (float) sin(angle + da) - r1 * (float) sin(angle);
    len = (float) sqrt(u * u + v * v);
    u /= len;
    v /= len;
    glNormal3f(v, -u, 0.0);
    glVertex3f(r2 * (float) cos(angle + da), r2 * (float) sin(angle + da), width * 0.5f);
    glVertex3f(r2 * (float) cos(angle + da), r2 * (float) sin(angle + da), -width * 0.5f);
    glNormal3f((float) cos(angle), (float) sin(angle), 0.f);
    glVertex3f(r2 * (float) cos(angle + 2 * da), r2 * (float) sin(angle + 2 * da), width * 0.5f);
    glVertex3f(r2 * (float) cos(angle + 2 * da), r2 * (float) sin(angle + 2 * da), -width * 0.5f);
    u = r1 * (float) cos(angle + 3 * da) - r2 * (float) cos(angle + 2 * da);
    v = r1 * (float) sin(angle + 3 * da) - r2 * (float) sin(angle + 2 * da);
    glNormal3f(v, -u, 0.f);
    glVertex3f(r1 * (float) cos(angle + 3 * da), r1 * (float) sin(angle + 3 * da), width * 0.5f);
    glVertex3f(r1 * (float) cos(angle + 3 * da), r1 * (float) sin(angle + 3 * da), -width * 0.5f);
    glNormal3f((float) cos(angle), (float) sin(angle), 0.f);
  }

  glVertex3f(r1 * (float) cos(0), r1 * (float) sin(0), width * 0.5f);
  glVertex3f(r1 * (float) cos(0), r1 * (float) sin(0), -width * 0.5f);

  glEnd();

  glShadeModel(GL_SMOOTH);

  /* draw inside radius cylinder */
  glBegin(GL_QUAD_STRIP);
  for (i = 0; i <= teeth; i++) {
    angle = i * 2.f * (float) M_PI / teeth;
    glNormal3f(-(float) cos(angle), -(float) sin(angle), 0.f);
    glVertex3f(r0 * (float) cos(angle), r0 * (float) sin(angle), -width * 0.5f);
    glVertex3f(r0 * (float) cos(angle), r0 * (float) sin(angle), width * 0.5f);
  }
  glEnd();

}


static GLfloat view_rotx = 20.0, view_roty = 30.0, view_rotz = 0.0;
static GLint gear1, gear2, gear3;
static GLfloat gearAngle = 0.0;
static GLfloat sceneAngle = 0.0;
static GLfloat camDip = 0.0;
int camRotate = 0;
static GLfloat sunAngle = 0.0;
static GLfloat sunAngle2 = 0.0;
static GLfloat sunAngle3 = 0.0;
static GLfloat sunAngle4 = 0.0;
static GLfloat piston = 0.0;
static GLfloat range = 40.0;
#define HUDWIDTH 36
#define HUDHEIGHT 20
static GLfloat xHUDscale = HUDWIDTH / 2;
static GLfloat HUDscale = HUDHEIGHT / 2;
//static int xCursor = -(HUDWIDTH / 2);
//static int yCursor = -(HUDHEIGHT / 2);
static int xCursor = 0;
static int yCursor = 0;

/* Calculate the cross product */
static void calcCross(float * c1, float * c2, float * c3,
        float a1, float a2, float a3,
        float b1, float b2, float b3) {
    * c1 = a2 * b3 - a3 * b2;
    * c2 = a3 * b1 - a1 * b3;
    * c3 = a1 * b2 - a2 * b1;
}

/* Normalize a vector in place */
static void normVec(float * a1, float * a2, float * a3) {
    float d = sqrt(* a1 * * a1 + * a2 * * a2 + * a3 * * a3);
    * a1 /= d;
    * a2 /= d;
    * a3 /= d;
}

/* Calculate the normal vector for a triangle with vertices
   at a,b,c and store the result in d */
static void triNorm(
        float a1, float a2, float a3,
        float b1, float b2, float b3,
        float c1, float c2, float c3) {
    float d1,d2,d3;
    float v1,v2,v3,w1,w2,w3;
    v1 = b1 - a1; v2 = b2 - a2; v3 = b3 - a3;
    w1 = c1 - a1; w2 = c2 - a2; w3 = c3 - a3;
    calcCross(&d1,&d2,&d3,v1,v2,v3,w1,w2,w3);
    normVec(&d1,&d2,&d3);
    //* d1 *= -1; * d2 *= -1; * d3 *= -1;
    glNormal3f(d1,d2,d3);
    glVertex3f(a1,a2,a3);
    glVertex3f(b1,b2,b3);
    glVertex3f(c1,c2,c3);
}

#define BOLDCOUNT 5
#define BOLDFINE 25

static void boldline2(float x1,float y1,float x2,float y2,
        float *xout,float *yout) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float mag = sqrt(dx*dx + dy*dy);
    dx /= mag;
    dy /= mag;
    float temp = dx;
    dx = -dy;
    dy = temp;
    dx /= BOLDFINE;
    dy /= BOLDFINE;
    dx *= (float) BOLDCOUNT / 2.0;
    dy *= (float) BOLDCOUNT / 2.0;
    xout[0] = x1 + dx; yout[0] = y1 + dy;
    xout[1] = x1 - dx; yout[1] = y1 - dy;
    xout[2] = x2 + dx; yout[2] = y2 + dy;
    xout[3] = x2 - dx; yout[3] = y2 - dy;
}

float marqueeWidth = 0.1;

static void drawboldline2(float x1,float y1,float x2,float y2) {
    float xout[4];
    float yout[4];
    boldline2(x1,y1,x2,y2,xout,yout);
    triNorm(
        xout[0],yout[0],marqueeWidth,
        xout[1],yout[1],marqueeWidth,
        xout[2],yout[2],marqueeWidth);
    triNorm(
        xout[2],yout[2],marqueeWidth,
        xout[1],yout[1],marqueeWidth,
        xout[3],yout[3],marqueeWidth);
    triNorm(
        xout[0],yout[0],-marqueeWidth,
        xout[2],yout[2],-marqueeWidth,
        xout[1],yout[1],-marqueeWidth);
    triNorm(
        xout[1],yout[1],-marqueeWidth,
        xout[2],yout[2],-marqueeWidth,
        xout[3],yout[3],-marqueeWidth);

    triNorm(
        xout[1],yout[1], marqueeWidth,
        xout[1],yout[1],-marqueeWidth,
        xout[3],yout[3], marqueeWidth);
    triNorm(
        xout[1],yout[1],-marqueeWidth,
        xout[3],yout[3],-marqueeWidth,
        xout[3],yout[3], marqueeWidth);

    triNorm(
        xout[0],yout[0],-marqueeWidth,
        xout[0],yout[0], marqueeWidth,
        xout[2],yout[2], marqueeWidth);
    triNorm(
        xout[2],yout[2],-marqueeWidth,
        xout[0],yout[0],-marqueeWidth,
        xout[2],yout[2], marqueeWidth);
}

static void boldline(float x1,float y1,float x2,float y2,
        float *x1out,float *y1out,float *x2out,float *y2out) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float mag = sqrt(dx*dx + dy*dy);
    dx /= mag;
    dy /= mag;
    float temp = dx;
    dx = -dy;
    dy = temp;
    dx /= BOLDFINE;
    dy /= BOLDFINE;
    x1 -= (BOLDCOUNT / 2) * dx;
    x2 -= (BOLDCOUNT / 2) * dx;
    y1 -= (BOLDCOUNT / 2) * dy;
    y2 -= (BOLDCOUNT / 2) * dy;
    int i;
    for (i = 0; i < BOLDCOUNT; i += 1) {
        x1out[i] = x1;
        x2out[i] = x2;
        y1out[i] = y1;
        y2out[i] = y2;
        x1 += dx;
        x2 += dx;
        y1 += dy;
        y2 += dy;
    }
}

static void drawboldline(float x1,float y1,float x2,float y2) {
  int i;
  float x1r[BOLDCOUNT];
  float x2r[BOLDCOUNT];
  float y1r[BOLDCOUNT];
  float y2r[BOLDCOUNT];
  boldline(x1,y1,x2,y2,x1r,y1r,x2r,y2r);
  for (i = 0; i < BOLDCOUNT; i += 1) {
      glVertex3f(x1r[i],y1r[i],marqueeWidth);
      glVertex3f(x2r[i],y2r[i],marqueeWidth);
  }
}

//static GLfloat copper[4] =    {0.725 * 0.8, 0.45 * 0.8, 0.2 * 0.8, 1.0};

static GLfloat forestgreen[4] =  {  0.1 / 0.8,  0.8 / 0.8,  0.4 / 0.8,1.0};
static GLfloat skyblue[4] =      {      0.525,        0.8,      0.925,1.0};
static GLfloat vermilion[4] =    {        0.9,       0.25,        0.2,1.0};
static GLfloat vermilionS[4] =   {       0.95,      0.625,        0.6,1.0};
static GLfloat canary[4] =       {        1.0,        1.0,        0.6,1.0};
static GLfloat pink[4] =         {  0.7 / 0.8,  0.2 / 0.8,  0.3 / 0.8,1.0};
static GLfloat concrete[4] =     {        0.4,        0.4,        0.4,1.0};
static GLfloat indigo[4] =       {  0.3 / 0.8,        0.0,  0.5 / 0.8,1.0};
static GLfloat mahogany[4] =     { 0.75 / 0.8, 0.25 / 0.8,        0.0,1.0};
static GLfloat luislemon[4] =    {        0.9,        1.0,        0.2,1.0};
static GLfloat chestnut[4] =     {   0.6/0.64,   0.3/0.64,   0.2/0.64,1.0};
static GLfloat white[4] =        {        1.0,        1.0,        1.0,1.0};
static GLfloat black[4] =        {        0.0,        0.0,        0.0,1.0};
static GLfloat forestgreen2[4] = {  0.1*0.512,  0.8*0.512,  0.4*0.512,1.0};
static GLfloat skyblue2[4] =     {0.525*0.409,  0.8*0.409,0.925*0.409,1.0};
static GLfloat vermilion2[4] =   {  0.9*0.409, 0.25*0.409,  0.2*0.409,1.0};
static GLfloat vermilionS2[4] =  { 0.95*0.409,0.625*0.409,  0.6*0.409,1.0};
static GLfloat canary2[4] =      {  1.0*0.409,  1.0*0.409,  0.6*0.409,1.0};
static GLfloat pink2[4] =        {  0.7*0.512,  0.2*0.512,  0.3*0.512,1.0};
static GLfloat indigo2[4] =      {  0.3*0.512,        0.0,  0.5*0.512,1.0};
static GLfloat mahogany2[4] =    { 0.75*0.512, 0.25*0.512,        0.0,1.0};
static GLfloat luislemon2[4] =   {  0.9*0.409,      0.409,  0.2*0.409,1.0};
static GLfloat chestnut2[4] =    {   0.6*0.64,   0.3*0.64,   0.2*0.64,1.0};

typedef struct Palette {
  GLfloat * colors;
  int length;
} Palette;

Palette * pa1;
Palette * pa2;

Palette * mkPalette(int length) {
  Palette * p = malloc(sizeof(Palette));
  p->colors = malloc(sizeof(GLfloat) * 4 * length);
  p->length = length;
  return p;
}

void ldPalette(Palette * p, int i, GLfloat * c) {
  if (i < p->length) {
      p->colors[4 * i + 0] = c[0];
      p->colors[4 * i + 1] = c[1];
      p->colors[4 * i + 2] = c[2];
      p->colors[4 * i + 3] = c[3];
  }
}

/*8,6,5,4
  8
  64
  512
  4096
  32768
  262144
  2097152
  16777216
  134217728
  1073741824
  8589934592
  68719476736*/

//static GLfloat skyblue2[4] = {0.525 * 0.8, 0.8 * 0.8, 0.925 * 0.8, 1.0};
//static GLfloat tigger2[4] =  {0.725      ,      0.45,         0.2, 1.0};
//static GLfloat canary2[4] =  {  1.0 * 0.8, 1.0 * 0.8,   0.6 * 0.8, 1.0};
//static GLfloat pink2[4] =    {        0.7,       0.2,         0.3, 1.0};
//static GLfloat tigger[4] =    {0.725 / 0.8, 0.45 / 0.8, 0.2 / 0.8, 1.0};
//static GLfloat tigger2[4] =    {0.725 * 0.8 , 0.45 * 0.8,    0.2 * 0.8, 1.0};

static float sunRadius = 2.0;
static float spikeRadius = 0.5;
static float sunRadius2 = 2.0;
//static float sunThickness = 1.0;

float cursor2x;
float cursor2y;
int dc = 0;
int vermilionPeriod = 30;
/* OpenGL draw function & timing */
static void draw(void) {
  dc += 1;
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, skyblue);
  glDisable(GL_LIGHTING);
  glDisable(GL_LIGHT0);
  glColor3f(0.1,0.4,0.8); /* set HUD color 1 */
  glBegin(GL_LINES); /* HUD outlines */
  glVertex3f(0.0      ,0.0     , 0.0);
  glVertex3f(xHUDscale,0.0     , 0.0);
  glVertex3f(0.0      ,HUDscale, 0.0);
  glVertex3f(xHUDscale,HUDscale, 0.0);
  glVertex3f(0.0      ,0.0     , 0.0);
  glVertex3f(0.0      ,HUDscale, 0.0);
  glVertex3f(xHUDscale,0.0     , 0.0);
  glVertex3f(xHUDscale,HUDscale, 0.0);
  glColor3f(0.8,0.4,0.1); /* set HUD color 2 */
  glVertex3f(0.0      , 0.0     , 0.0);
  glVertex3f(xHUDscale, 0.0     , 0.0);
  glVertex3f(0.0      ,-HUDscale, 0.0);
  glVertex3f(xHUDscale,-HUDscale, 0.0);
  glVertex3f(0.0      , 0.0     , 0.0);
  glVertex3f(0.0      ,-HUDscale, 0.0);
  glVertex3f(xHUDscale, 0.0     , 0.0);
  glVertex3f(xHUDscale,-HUDscale, 0.0);
  glColor3f(0.1,0.8,0.1); /* set HUD color 3 */
  glVertex3f(0.0       , 0.0     , 0.0);
  glVertex3f(-xHUDscale, 0.0     , 0.0);
  glVertex3f(0.0       ,-HUDscale, 0.0);
  glVertex3f(-xHUDscale,-HUDscale, 0.0);
  glVertex3f(0.0       , 0.0     , 0.0);
  glVertex3f(0.0       ,-HUDscale, 0.0);
  glVertex3f(-xHUDscale, 0.0     , 0.0);
  glVertex3f(-xHUDscale,-HUDscale, 0.0);
  glColor3f(0.8,0.8,0.1); /* set HUD color 4 */
  glVertex3f(0.0       ,0.0     , 0.0);
  glVertex3f(-xHUDscale,0.0     , 0.0);
  glVertex3f(0.0       ,HUDscale, 0.0);
  glVertex3f(-xHUDscale,HUDscale, 0.0);
  glVertex3f(0.0       ,0.0     , 0.0);
  glVertex3f(0.0       ,HUDscale, 0.0);
  glVertex3f(-xHUDscale,0.0     , 0.0);
  glVertex3f(-xHUDscale,HUDscale, 0.0);
  glEnd(); /* end HUD */
  glPushMatrix(); /* scene */
  glRotatef(camDip, 1.0, 0.0, 0.0);
  glTranslatef(0.0,0.0,-range);
  //glTranslatef(0.0,0.0,range);
  glRotatef(sceneAngle, 0.0, 1.0, 0.0);
  glTranslatef(0.0, -4.0, 0.0);
  glPushMatrix(); /* (green grid, cursor, marquee, Sol) */
  glBegin(GL_LINES); /* green grid */
  int i;
  if (1) {
      glColor3f(0.1,0.8,0.1);
      for (i = 0; i < 10; i += 1) {
          glVertex3f( 0.0 , (float) i, 0.0);
          glVertex3f( 10.0, (float) i, 0.0);
      }
      for (i = 0; i < 10; i += 1) {
          glVertex3f((float) i, 0.0 , 0.0);
          glVertex3f((float) i, 10.0, 0.0);
      }
      glEnd(); /* end green grid */
      glColor3f(0.1,0.1,0.8);
      glBegin(GL_LINES); /* blue grid */
      for (i = 0; i < 10; i += 1) {
          glVertex3f( 0.0 , 0.0, (float) i);
          glVertex3f( 10.0, 0.0, (float) i);
      }
      for (i = 0; i < 10; i += 1) {
          glVertex3f((float) i, 0.0 , 0.0);
          glVertex3f((float) i, 0.0, 10.0);
      }
      glEnd(); /* end blue grid */
      for (i = 0; i < 10; i += 1) {
          ;
          //glVertex3f( 0.0, 0.0 , (float) i);
          //glVertex3f( 0.0, 10.0, (float) i);
      }
      for (i = 0; i < 10; i += 1) {
          ;
          //glVertex3f( 0.0, (float) i, 0.0);
          //glVertex3f( 0.0, (float) i, 10.0);
      }
  }
  glTranslatef(0.0, 4.0, 0.0);
  /* apply a general 2D linear transformation
     to an array of x-y coordinates */
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  //glColor3f(0.8,0.8,0.8);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, skyblue);
  glPushMatrix(); /* (cursor, cursor 2, marquee) */

  //glBegin(GL_TRIANGLES); /* cursor 2 */
  //drawboldline2(cursor2x - 0.5, cursor2y - 0.5,cursor2x + 0.5, cursor2y + 0.5);
  //drawboldline2(cursor2x - 0.5, cursor2y + 0.5,cursor2x + 0.5, cursor2y - 0.5);
  //drawboldline2(0.0,0.0,cursor2x,cursor2y);
  //glEnd(); /* cursor 2 */

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, concrete);
  glPushMatrix(); /* platform */
  glTranslatef(0.0,-5.0,0.0);
  glBegin(GL_TRIANGLES); /* platform */
  triNorm(
       0.0,0.0, 0.0,
       0.0,0.0,10.0,
      10.0,0.0,10.0);
  triNorm(
       0.0,0.0, 0.0,
      10.0,0.0,10.0,
      10.0,0.0, 0.0);

  triNorm(
       0.0,-2.0,0.0,
       0.0, 0.0,0.0,
      10.0, 0.0,0.0);
  triNorm(
       0.0,-2.0,0.0,
      10.0, 0.0,0.0,
      10.0,-2.0,0.0);

  triNorm(
       0.0, 0.0,10.0,
       0.0,-2.0,10.0,
      10.0, 0.0,10.0);
  triNorm(
      10.0, 0.0,10.0,
       0.0,-2.0,10.0,
      10.0,-2.0,10.0);

  triNorm(
       0.0, 0.0,0.0,
       0.0,-2.0,0.0,
       0.0, 0.0,10.0);
  triNorm(
       0.0, 0.0,10.0,
       0.0,-2.0,0.0,
       0.0,-2.0,10.0);

  triNorm(
       10.0,-2.0,0.0,
       10.0, 0.0,0.0,
       10.0, 0.0,10.0);
  triNorm(
       10.0,-2.0,0.0,
       10.0, 0.0,10.0,
       10.0,-2.0,10.0);

  glEnd(); /* end platform */
  glPopMatrix(); /* end platform */

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, pink);
  int Rwidth = 1;
  for (i = 0; i < 1; i += 1) {
      glBegin(GL_TRIANGLES); /* cursor */
      drawboldline2(xCursor - 0.5, yCursor - 0.5,xCursor + 0.5, yCursor + 0.5);
      drawboldline2(xCursor - 0.5, yCursor + 0.5,xCursor + 0.5, yCursor - 0.5);
      drawboldline2(0.0,0.0,xCursor,yCursor);
      //glEnd(); /* end cursor */
      //glBegin(GL_TRIANGLES); /* marquee R */
      drawboldline2(0.0         ,4.0,0.0 + Rwidth,4.0);
      drawboldline2(0.0 + Rwidth,4.0,1.0 + Rwidth,3.0);
      drawboldline2(1.0 + Rwidth,3.0,0.0 + Rwidth,2.0);
      drawboldline2(0.0 + Rwidth,2.0,0.0         ,2.0);
      drawboldline2(0.0         ,4.0,0.0         ,0.0);
      drawboldline2(0.0 + Rwidth,2.0,1.0 + Rwidth,1.0);
      drawboldline2(1.0 + Rwidth,1.0,1.0 + Rwidth,0.0);
      glEnd(); /* end marquee R */
      glRotatef(45.0,0.0,1.0,0.0);
  }
  glPopMatrix(); /* (cursor, marquee) */
  //glRotatef(-90.0,1.0,0.0,0.0);
  //glRotatef(45.0,0.0,1.0,0.0);
  int j,k;
  int p1,p2,p3;
  // draw Sol
  // CPU utilization notes:
  //    91% idle @ k = 0..30
  //    89% idle @ k = 0..300
  // 88-89% idle @ k = 0..1000
  // 85-86% idle @ k = 0..3000
  // 84-85% idle @ k = 0..10000

  int pi = 0;
  glRotatef(sunAngle,0.0,1.0,0.0);
  for (p1 = 0; p1 < 2; p1 += 1) {
  for (p2 = 0; p2 < 2; p2 += 1) {
  for (p3 = 0; p3 < 2; p3 += 1) {
  pi += 1;
  glPushMatrix(); /* Sol */
  glTranslatef(-5.0 + 10.0 * p1,-5.0 + 10.0 * p2,-5.0 + 10.0 * p3);
  //glRotatef(pi * 15.0,0.0,1.0,0.0);
  //glRotatef(pi * 15.0,0.0,0.0,1.0);
  int rsgn = 1;
  if (pi % 2 == 0) {
      rsgn = -1;
  }
  float asgn = 0.0;
  float bsgn = 0.0;
  float csgn = 0.0;
  if (pi % 6 < 2) {
      asgn = 1.0;
  } else if (pi % 6 < 4) {
      bsgn = 1.0;
  } else {
      csgn = 1.0;
  }
  glRotatef(rsgn * sunAngle2,asgn,bsgn,csgn);
  //glRotatef(pi * 15.0,0.0,0.0,1.0);
  //glRotatef(pi * 15.0,0.0,1.0,0.0);
  int COLORS;
  int kk;
  COLORS = 11;
  for (k = 0; k < COLORS; k += 1) {
      for (j = 0; j < 4; j += 1) {
          kk = (k % COLORS) % pa1->length;
          if (pi % 2 == 0) {
              glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE,
                    pa1->colors + 4 * kk);
          } else {
              glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE,
                    pa2->colors + 4 * kk);
          }
          for (i = 0; i < 2; i += 1) {
              if (i == 2 || 1) {
                  glTranslatef(0.0,sunRadius,0.0);
                  glBegin(GL_TRIANGLES);
                  float xx[4];
                  float yy[4];
                  // line segments define arc of cone base
                  xx[0] =  1.0; yy[0] =  0.0;
                  xx[1] = 0.87; yy[1] =  0.5;
                  xx[2] =  0.5; yy[2] = 0.87;
                  xx[3] =  0.0; yy[3] =  1.0;
                  int ii;
                  for (ii = 0; ii < 4; ii += 1) {
                      xx[ii] *= spikeRadius;
                      yy[ii] *= spikeRadius;
                  }
                  for (ii = 0; ii < 3; ii += 1) {
                      triNorm(
                          xx[ii + 0],0.0,yy[ii + 0],
                          0.0,sunRadius2,0.0,
                          xx[ii + 1],0.0,yy[ii + 1]);
                      // end cap A
                      triNorm(
                          0.0,0.0,0.0,
                          xx[ii + 0],0.0,yy[ii + 0],
                          xx[ii + 1],0.0,yy[ii + 1]);

                      triNorm(
                          -xx[ii + 0],0.0,yy[ii + 0],
                          -xx[ii + 1],0.0,yy[ii + 1],
                           0.0,sunRadius2,0.0);
                      // end cap B
                      triNorm(
                          -xx[ii + 0],0.0,yy[ii + 0],
                           0.0,0.0,0.0,
                          -xx[ii + 1],0.0,yy[ii + 1]);
                  }
                  glEnd();
                  glTranslatef(0.0,-sunRadius,0.0);
              }
              glRotatef(180.0,0.0,1.0,0.0);
          }
          //glRotatef(180.0,0.0,0.0,1.0);
          //glRotatef(180.0,1.0,0.0,0.0);
          //glRotatef(120.0 + 0 * sunAngle3,0.0,0.0,1.0);
          glRotatef(15.0 + sunAngle4 / 9.0,0.0,0.0,1.0);
      }
      //glRotatef(90.0 + 0 * sunAngle4,1.0,0.0,0.0);
      glRotatef(45.0 + sunAngle3 / 16.0,1.0,0.0,0.0);
  }
  glPopMatrix(); /* end Sol */
  }
  }
  }
  glPopMatrix(); /* end (green grid, cursor, marquee, Sol) */
  glPopMatrix(); /* end scene */
}

static int animPeriod = 45;
static int animIndex = 0;

// pulse function 3a and 3b

float pulseFunction(float x) {
    int i = (int) x;
    if (i % 2 == 0) {
        float f = x - (float) i;
        return x + f;
    } else {
        return (float) i + 1.0;
    }
}

float pulseFunction2(float x) {
    int i = (int) x;
    if (i % 2 == 0) {
        return (float) i;
    } else {
        float f = x - (float) i;
        return (float) i - 1.0 + f + f;
    }
}

/* update animation parameters */
static void animate(void) {
  gearAngle = 60.f * (float) glfwGetTime(); /* gear angle */
  if (camRotate) {
      sceneAngle = 90 + 15.0 * (float) glfwGetTime();
  } else {
      sceneAngle = -45.0;
  }
  camDip = 15.0 * sin(glfwGetTime());
  //sunAngle = 5.0 * (float) glfwGetTime();
  sunAngle = 0;
  sunAngle2 = 15.0 * (float) pulseFunction2(glfwGetTime());
  //sunAngle2 = 0.0;
  sunAngle3 = 30.0 * (float) glfwGetTime();
  sunAngle4 = 15.0 * (float) glfwGetTime();
  piston = fmodf(4.0 * (float) glfwGetTime(),4.f);
  piston = (piston > 2.0 ? 4.0 - piston : piston);
  camDip = 10.0 * piston;
  camDip = 0;
  animIndex += 1;
  if (0 == animIndex % animPeriod) {
      xCursor += 1;
      if (xCursor > (HUDWIDTH / 2)) {
          xCursor = -(HUDWIDTH / 2);
          yCursor += 1;
          yCursor = (yCursor > 10 ? -(HUDHEIGHT / 2) : yCursor);
      }
  }
}


/* change view angle, exit upon ESC */
void key( GLFWwindow* window, int k, int s, int action, int mods ) {
  if( action != GLFW_PRESS ) return;

  switch (k) {
  case GLFW_KEY_Z:
    if( mods & GLFW_MOD_SHIFT )
      view_rotz -= 5.0;
    else
      view_rotz += 5.0;
    break;
  case GLFW_KEY_ESCAPE:
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    break;
  case GLFW_KEY_UP:
    view_rotx += 5.0;
    break;
  case GLFW_KEY_DOWN:
    view_rotx -= 5.0;
    break;
  case GLFW_KEY_LEFT:
    view_roty += 5.0;
    break;
  case GLFW_KEY_RIGHT:
    view_roty -= 5.0;
    break;
  default:
    return;
  }
}

float windowWidth;
float windowHeight;

void cursor(GLFWwindow * window, double x, double y) {
    x -= windowWidth / 2.0;
    x /= windowWidth / 2.0;
    x *= 20;
    cursor2x = x;
    y -= windowHeight / 2.0;
    y /= windowHeight / 2.0;
    y *= -1 * 20 * (windowHeight / windowWidth);
    cursor2y = y;
    //printf("window width: %f\n",windowWidth);
    //printf("at %0.3f: Cursor position: %f %f\n",glfwGetTime(), x, y);
}

/* new window size */
void reshape( GLFWwindow * window, int width, int height ) {
  //printf("reshape: %f %f\n",(float) width,(float) height);
  windowWidth = width / 2;
  windowHeight = height / 2;
  GLfloat h = (GLfloat) height / (GLfloat) width;
  GLfloat xmax, znear, zfar;

  znear = 5.0f;
  zfar  = 60.0f;
  xmax  = znear * 0.5f;

  glViewport( 0, 0, (GLint) width, (GLint) height );
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glFrustum( -xmax, xmax, -xmax*h, xmax*h, znear, zfar );
  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  //glTranslatef( 0.0, 0.0, -range );
}


/* program & OpenGL initialization */
static void init(void)
{
  static GLfloat pos[4] = {5.0, 5.0, 10.0, 0.0};
  static GLfloat red[4] = {0.8, 0.1, 0.0, 1.0};
  static GLfloat green[4] = {0.0, 0.8, 0.2, 1.0};
  static GLfloat blue[4] = {0.2, 0.2, 1.0, 1.0};
  static GLfloat intensity[4] = {1.0, 1.0, 1.0, 1.0};
  //static GLfloat intensity[4] = {1.0, 1.0, 1.0, 1.0};

  pa1 = mkPalette(11);
  ldPalette(pa1,0,skyblue);
  ldPalette(pa1,1,vermilion);
  ldPalette(pa1,2,canary);
  ldPalette(pa1,3,pink);
  ldPalette(pa1,4,forestgreen);
  ldPalette(pa1,5,indigo);
  ldPalette(pa1,6,mahogany);
  ldPalette(pa1,7,luislemon);
  ldPalette(pa1,8,chestnut);
  ldPalette(pa1,9,white);
  ldPalette(pa1,10,black);
  pa2 = mkPalette(11);
  ldPalette(pa2,0,skyblue2);
  ldPalette(pa2,1,vermilion2);
  ldPalette(pa2,2,canary2);
  ldPalette(pa2,3,pink2);
  ldPalette(pa2,4,forestgreen2);
  ldPalette(pa2,5,indigo2);
  ldPalette(pa2,6,mahogany2);
  ldPalette(pa2,7,luislemon2);
  ldPalette(pa2,8,chestnut2);
  ldPalette(pa2,9,black);
  ldPalette(pa2,10,white);

  cursor2x = cursor2y = 0;

  glLightfv(GL_LIGHT0, GL_POSITION, pos);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, intensity);
  //glLightfv(GL_LIGHT0, GL_AMBIENT, intensity);
  //glLightfv(GL_LIGHT0, GL_SPECULAR, intensity);
  glEnable(GL_CULL_FACE);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);

  /* make the gears */
  gear1 = glGenLists(1);
  glNewList(gear1, GL_COMPILE);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
  gear(1.f, 4.f, 1.f, 20, 0.7f);
  glEndList();

  gear2 = glGenLists(1);
  glNewList(gear2, GL_COMPILE);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
  gear(0.5f, 2.f, 2.f, 10, 0.7f);
  glEndList();

  gear3 = glGenLists(1);
  glNewList(gear3, GL_COMPILE);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
  gear(1.3f, 2.f, 0.5f, 10, 0.7f);
  glEndList();

  glEnable(GL_NORMALIZE);
}


/* program entry */
int main(int argc, char *argv[])
{
    GLFWwindow* window;
    int width, height;

    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        exit( EXIT_FAILURE );
    }

    glfwWindowHint(GLFW_DEPTH_BITS, 16);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

    if (argc >= 2) {
		windowWidth = 210;
		windowHeight = 210;
    } else {
		windowWidth = 840;
		windowHeight = 480;
    }
    window = glfwCreateWindow(windowWidth, windowHeight, "Gears", NULL, NULL );
    if (!window)
    {
        fprintf( stderr, "Failed to open GLFW window\n" );
        glfwTerminate();
        exit( EXIT_FAILURE );
    }

    // Set callback functions
    glfwSetFramebufferSizeCallback(window, reshape);
    glfwSetKeyCallback(window, key);
    glfwSetCursorPosCallback(window, cursor);

    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval( 1 );

    glfwGetFramebufferSize(window, &width, &height);
    reshape(window, width, height);

    // Parse command-line options
    init();

    // Main loop
    while( !glfwWindowShouldClose(window) )
    {
        // Update animation
        animate();

        // Draw gears
        draw();

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Terminate GLFW
    glfwTerminate();

    // Exit program
    exit( EXIT_SUCCESS );
}

