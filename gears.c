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

double gearsGetTime(int lighting);

void gearMaterial(GLenum f,const GLfloat * ps) {
    GLfloat qqs[4];
    float matAlpha = (1.0+sin(gearsGetTime(2)))/2.0;
    qqs[0] = ps[0] / 2;
    qqs[1] = ps[1] / 2;
    qqs[2] = ps[2] / 2;
    qqs[3] = matAlpha;
    GLfloat qs[4];
    qs[0] = ps[0] / 4;
    qs[1] = ps[1] / 4;
    qs[2] = ps[2] / 4;
    qs[3] = matAlpha;
    GLfloat rs[4];
    rs[0] = ps[0] / 5;
    rs[1] = ps[1] / 5;
    rs[2] = ps[2] / 5;
    rs[3] = matAlpha;
    GLfloat rrs[4];
    rrs[0] = 2 * ps[0] / 5;
    rrs[1] = 2 * ps[1] / 5;
    rrs[2] = 2 * ps[2] / 5;
    rrs[3] = matAlpha;
    GLfloat hatps[4];
    hatps[0] = ps[0];
    hatps[1] = ps[1];
    hatps[2] = ps[2];
    hatps[3] = matAlpha;
    glMaterialfv(f,GL_SPECULAR,hatps);
    //glMaterialfv(f,GL_AMBIENT,rrs);
    glMaterialfv(f,GL_DIFFUSE,rrs);
    GLfloat s[] = {50.0};
    glMaterialfv(f,GL_SHININESS,s);
}

static float timeOffset;
static GLfloat view_rotx = 20.0, view_roty = 30.0, view_rotz = 0.0;
static GLint gear1, gear2, gear3;
static GLfloat sceneAngle = 0.0;
static GLfloat camDip = 0.0;
static GLfloat sunAngle = 0.0;
static GLfloat sunAngle2 = 0.0;
static GLfloat sunAngle3 = 0.0;
static GLfloat range = 10.0;
static GLfloat camHeight = -1.0;
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

#define CONEALPHA 1.0

static GLfloat forestgreen[4] =  {  0.1 / 0.8,  0.8 / 0.8,  0.4 / 0.8,CONEALPHA};
static GLfloat skyblue[4] =      {      0.525,        0.8,      0.925,CONEALPHA};
static GLfloat vermilion[4] =    {        0.9,       0.25,        0.2,CONEALPHA};
static GLfloat vermilionS[4] =   {       0.95,      0.625,        0.6,CONEALPHA};
static GLfloat canary[4] =       {        1.0,        1.0,        0.6,CONEALPHA};
static GLfloat pink[4] =         {  0.7 / 0.8,  0.2 / 0.8,  0.3 / 0.8,CONEALPHA};
static GLfloat concrete[4] =     {        0.4,        0.4,        0.4,CONEALPHA};
static GLfloat ultramarine[4] =  {       0.25,        0.0,        1.0,CONEALPHA};
static GLfloat cerulean[4] =     {        0.0,     0.0625,       0.25,CONEALPHA};
static GLfloat indigo[4] =       {  0.3 / 0.8,        0.0,  0.5 / 0.8,CONEALPHA};
static GLfloat mahogany[4] =     { 0.75 / 0.8, 0.25 / 0.8,        0.0,CONEALPHA};
static GLfloat luislemon[4] =    {        0.9,        1.0,        0.2,CONEALPHA};
static GLfloat chestnut[4] =     {   0.6/0.64,   0.3/0.64,   0.2/0.64,CONEALPHA};
static GLfloat white[4] =        {        1.0,        1.0,        1.0,CONEALPHA};
static GLfloat black[4] =        {        0.0,        0.0,        0.0,CONEALPHA};
static GLfloat forestgreen2[4] = {  0.1*0.512,  0.8*0.512,  0.4*0.512,CONEALPHA};
static GLfloat skyblue2[4] =     {0.525*0.409,  0.8*0.409,0.925*0.409,CONEALPHA};
static GLfloat vermilion2[4] =   {  0.9*0.409, 0.25*0.409,  0.2*0.409,CONEALPHA};
static GLfloat vermilionS2[4] =  { 0.95*0.409,0.625*0.409,  0.6*0.409,CONEALPHA};
static GLfloat canary2[4] =      {  1.0*0.409,  1.0*0.409,  0.6*0.409,CONEALPHA};
static GLfloat pink2[4] =        {  0.7*0.512,  0.2*0.512,  0.3*0.512,CONEALPHA};
static GLfloat indigo2[4] =      {  0.3*0.512,        0.0,  0.5*0.512,CONEALPHA};
static GLfloat mahogany2[4] =    { 0.75*0.512, 0.25*0.512,        0.0,CONEALPHA};
static GLfloat luislemon2[4] =   {  0.9*0.409,      0.409,  0.2*0.409,CONEALPHA};
static GLfloat chestnut2[4] =    {   0.6*0.64,   0.3*0.64,   0.2*0.64,CONEALPHA};
//
//static GLfloat cerulean[4] =     {        0.0,       0.25,        1.0,1.0};
typedef struct Palette {
  GLfloat * colors;
  int alloc;
  int length;
} Palette;

Palette * pa1;
// Palette * pa2;
Palette * pa3;

/* make object */
Palette * mkPalette(void) {
  int alloc = 4;
  int length = 0;
  Palette * p = malloc(sizeof(Palette));
  p->colors = malloc(sizeof(GLfloat) * 4 * alloc);
  p->alloc = alloc;
  p->length = length;
  return p;
}

/* grow alloc */
void grPalette(Palette * p) {
    int newalloc = 2 * p->alloc;
    GLfloat * newcolors = malloc(sizeof(GLfloat) * 4 * newalloc);
    int i;
    for (i = 0; i < p->length; i += 1) {
        newcolors[4 * i + 0] = p->colors[4 * i + 0];
        newcolors[4 * i + 1] = p->colors[4 * i + 1];
        newcolors[4 * i + 2] = p->colors[4 * i + 2];
        newcolors[4 * i + 3] = p->colors[4 * i + 3];
    }
    free(p->colors);
    p->colors = newcolors;
    p->alloc = newalloc;
}

/* load color */
void ldPalette(Palette * p, GLfloat * c) {
  int newlength = p->length + 1;
  if (newlength == p->alloc) {
      grPalette(p);
  }
  /* assert p->length < p->alloc */
  int i = p->length;
  p->colors[4 * i + 0] = c[0];
  p->colors[4 * i + 1] = c[1];
  p->colors[4 * i + 2] = c[2];
  p->colors[4 * i + 3] = c[3];
  p->length = newlength;
}

void ldPalette3i(Palette * p, int R, int G, int B) {
  GLfloat c[4];
  c[0] = (float) R / 255.0;
  c[1] = (float) G / 255.0;
  c[2] = (float) B / 255.0;
  c[3] = CONEALPHA;
  ldPalette(p,c);
}

/* subscript */
GLfloat * sbPalette(Palette * p, int i) {
  return p->colors + 4 * (i % p->length);
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

static float sunRadius = 1.0;
static float spikeRadius = 0.125;
static float sunRadius2[2] = {1.0,0.25};
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
  gearMaterial(GL_FRONT, skyblue);
  glDisable(GL_LIGHTING);
  glDisable(GL_LIGHT0);
  glDisable(GL_LIGHT1);
  glDisable(GL_LIGHT2);
  glDisable(GL_LIGHT3);
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
  glTranslatef(0.0,camHeight,0.0);
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
  /*

  glEnable(GL_LIGHT1);
  glEnable(GL_LIGHT2);
  glEnable(GL_LIGHT3);
  */
  
  //glColor3f(0.8,0.8,0.8);
  gearMaterial(GL_FRONT, skyblue);
  glPushMatrix(); /* (cursor, cursor 2, marquee) */

  glPushMatrix();
  glRotatef(sunAngle3,cursor2x,cursor2y,0.0);

  //glBegin(GL_TRIANGLES); /* cursor 2 */
  //drawboldline2(cursor2x - 0.5, cursor2y - 0.5,cursor2x + 0.5, cursor2y + 0.5);
  //drawboldline2(cursor2x - 0.5, cursor2y + 0.5,cursor2x + 0.5, cursor2y - 0.5);
  //drawboldline2(0.0,0.0,cursor2x,cursor2y);
  //glEnd(); /* cursor 2 */

  glPopMatrix();

  gearMaterial(GL_FRONT, cerulean);
  float sideWidth = 6.0;
  float platHeight = 1.0;
  for (i = 0;i < 4;i += 1) {
      float sideWidth2 = 3.0;
      float platHeight2 = 0.125;
      glPushMatrix(); /* platform */
      if (i == 1) {
          glRotatef(90.0,0.0,0.0,1.0);
          glRotatef(180.0,1.0,0.0,0.0);
          sideWidth = sideWidth2;
          platHeight = platHeight2;
      } else if (i == 2) {
          glRotatef(90.0,0.0,1.0,0.0);
          glRotatef(180.0,1.0,0.0,0.0);
      } else if (i == 3) {
          glRotatef(90.0,1.0,0.0,0.0);
          glRotatef(180.0,0.0,1.0,0.0);
      }
      glTranslatef(0.0,-5.0,0.0);
      glBegin(GL_TRIANGLES); /* platform */
      triNorm(
           0.0,0.0, 0.0,
           0.0,0.0,sideWidth,
          sideWidth,0.0,sideWidth);
      triNorm(
           0.0,0.0, 0.0,
          sideWidth,0.0,sideWidth,
          sideWidth,0.0, 0.0);

      triNorm(
           0.0,-platHeight, 0.0,
           sideWidth,-platHeight,sideWidth,
           0.0,-platHeight,sideWidth);
      triNorm(
           0.0,-platHeight, 0.0,
          sideWidth,-platHeight, 0.0,
          sideWidth,-platHeight,sideWidth);

      triNorm(
           0.0,-platHeight,0.0,
           0.0, 0.0,0.0,
          sideWidth, 0.0,0.0);
      triNorm(
           0.0,-platHeight,0.0,
          sideWidth, 0.0,0.0,
          sideWidth,-platHeight,0.0);

      triNorm(
           0.0, 0.0,sideWidth,
           0.0,-platHeight,sideWidth,
          sideWidth, 0.0,sideWidth);
      triNorm(
          sideWidth, 0.0,sideWidth,
           0.0,-platHeight,sideWidth,
          sideWidth,-platHeight,sideWidth);

      triNorm(
           0.0, 0.0,0.0,
           0.0,-platHeight,0.0,
           0.0, 0.0,sideWidth);
      triNorm(
           0.0, 0.0,sideWidth,
           0.0,-platHeight,0.0,
           0.0,-platHeight,sideWidth);

      triNorm(
           sideWidth,-platHeight,0.0,
           sideWidth, 0.0,0.0,
           sideWidth, 0.0,sideWidth);
      triNorm(
           sideWidth,-platHeight,0.0,
           sideWidth, 0.0,sideWidth,
           sideWidth,-platHeight,sideWidth);

      glEnd(); /* end platform */
      glPopMatrix(); /* end platform */
  }

  gearMaterial(GL_FRONT, pink);
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
  float disp = 2.5;
  float disp2 = disp/2.0;
  glTranslatef(-disp2 + disp * p1,-disp2 + disp * p2,-disp2 + disp * p3);
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
  int CONES;
  int kk;
  CONES = 16;
  for (k = 0; k < CONES; k += 1) {
      for (j = 0; j < 4; j += 1) {
          if (pi % 2 == 0) {
              gearMaterial(GL_FRONT, sbPalette(pa1,k));
          } else {
              gearMaterial(GL_FRONT, sbPalette(pa3,k));
          }
          glPushMatrix(); // fold
          glTranslatef(0.0,sunRadius,0.0);
          glRotatef(60.0 + sunAngle3/64.0,1.0,0.0,0.0);
          for (i = 0; i < 2; i += 1) {
              glBegin(GL_TRIANGLES);
              int FACES = 9;
              float xx[FACES];
              float yy[FACES];
              // line segments define arc of cone base
              //xx[0] =  1.0; yy[0] =  0.0;
              //xx[1] = 0.87; yy[1] =  0.5;
              //xx[2] =  0.5; yy[2] = 0.87;
              //xx[3] =  0.0; yy[3] =  1.0;
              yy[0] = xx[8] = 0.0;
              yy[1] = xx[7] = 0.19509032201612825;
              yy[2] = xx[6] = 0.3826834323650898;
              yy[3] = xx[5] = 0.5555702330196022;
              yy[4] = xx[4] = 0.7071067811865475;
              yy[5] = xx[3] = 0.8314696123025451;
              yy[6] = xx[2] = 0.9238795325112867;
              yy[7] = xx[1] = 0.9807852804032304;
              yy[8] = xx[0] = 1.0;
              int ii;
              for (ii = 0; ii < FACES; ii += 1) {
                  xx[ii] *= spikeRadius;
                  yy[ii] *= spikeRadius;
              }
              for (ii = 0; ii < FACES - 1; ii += 1) {
                  triNorm(
                      xx[ii + 0],0.0,yy[ii + 0],
                      0.0,sunRadius2[0],0.0,
                      xx[ii + 1],0.0,yy[ii + 1]);
                  // end cap A
                  triNorm(
                      0.0,-sunRadius2[1],0.0,
                      xx[ii + 0],0.0,yy[ii + 0],
                      xx[ii + 1],0.0,yy[ii + 1]);

                  triNorm(
                      -xx[ii + 0],0.0,yy[ii + 0],
                      -xx[ii + 1],0.0,yy[ii + 1],
                       0.0,sunRadius2[0],0.0);
                  // end cap B
                  triNorm(
                      -xx[ii + 0],0.0,yy[ii + 0],
                       0.0,-sunRadius2[1],0.0,
                      -xx[ii + 1],0.0,yy[ii + 1]);
              }
              glEnd();
              //glTranslatef(0.0,-sunRadius,0.0);
              glRotatef(180.0,0.0,1.0,0.0);
          }
          glPopMatrix(); // end fold
          glRotatef(45.0 + sunAngle3/27.0,0.0,0.0,1.0);
          glTranslatef(0.1,0.0,0.0);
      }
      glRotatef(60.0 + sunAngle3/64.0,1.0,0.0,0.0);
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

// 0 : mobile param 1
// 1 : lights
// 2 : mobile param 2
double gearsGetTime(int lighting) {
    float f = (double) 2.5 * (timeOffset + glfwGetTime());
    if (lighting == 0) {
        f *= 0.2;
        float timeShim = 1.0 - cos(f);
        return 5.0 * (f + timeShim);
    } else if (lighting == 1) {
        return f;
    } else {
        //float timeShim = 1.0 - cos(f);
        return 0.05 * f;
    }
}

/* update animation parameters */
static void animate(void) {
  int sceneRotate = 0;
  if (sceneRotate) {
      sceneAngle = 90 + 60.0 * (float) gearsGetTime(0);
  } else {
      sceneAngle = -45;
  }
  static GLfloat lightAngle;
  static GLfloat lightHeight;
  lightAngle = (float) gearsGetTime(1);
  lightAngle *= 0.16;
  lightHeight = 600.0 + 400.0 * sin(lightAngle * M_PI);
  lightAngle = 90 + 3450.0 * lightAngle;
  static GLfloat pos[4] = {0.0,0.0,0.0,0.0};
  pos[0] = 200.0 * cos(lightAngle * M_PI / 180.0);
  pos[1] = 200.0 * sin(lightAngle * M_PI / 180.0);
  pos[2] = lightHeight;
  glLightfv(GL_LIGHT0, GL_POSITION, pos);
  pos[0] *= -1.0;
  pos[1] *= -1.0;
  glLightfv(GL_LIGHT1, GL_POSITION, pos);
  static GLfloat temp;
  temp = pos[0];
  pos[0] = -pos[1];
  pos[1] = temp;
  glLightfv(GL_LIGHT2, GL_POSITION, pos);
  pos[0] *= -1.0;
  pos[1] *= -1.0;
  glLightfv(GL_LIGHT3, GL_POSITION, pos);
  sunAngle = 0;
  sunAngle2 = 15.0 * (float) pulseFunction2(gearsGetTime(0));
  sunAngle3 = 30.0 * (float) gearsGetTime(2);
  camDip = 5.0;
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
}
/* program & OpenGL initialization */
static void init(void) {
  /*
  static GLfloat pos[4]  = { 5.0,  5.0, 10.0, 0.0};
  static GLfloat pos2[4] = { 5.0, -5.0, 10.0, 0.0};
  static GLfloat pos3[4] = {-5.0,  5.0, 10.0, 0.0};
  static GLfloat pos4[4] = {-5.0, -5.0, 10.0, 0.0};
  */
  static GLfloat pos[4]  = { 100.0, 0.0,-10.0, 0.0};
  static GLfloat pos2[4] = {-100.0, 0.0,-10.0, 0.0};
  static GLfloat pos3[4] = { 0.0, 100.0,-10.0, 0.0};
  static GLfloat pos4[4] = { 0.0,-100.0,-10.0, 0.0};
  //static GLfloat red[4] = {0.8, 0.1, 0.0, 1.0};
  //static GLfloat green[4] = {0.0, 0.8, 0.2, 1.0};
  //static GLfloat blue[4] = {0.2, 0.2, 1.0, 1.0};
  //static GLfloat intensity[4] = {0.5, 0.5, 0.5, 1.0};
  static GLfloat intensity0[4] = {0.0, 0.0, 0.0, 1.0};
  static GLfloat intensityq[4] = {0.25, 0.25, 0.25, 1.0};
  static GLfloat intensity[4] = {1.0, 1.0, 1.0, 1.0};
  /* add a routine for duplicating a palette */
  /* construct the Psychedelic Crayola palette by
     duplicating Jovian and adding skyblue through chestnut below */
  pa1 = mkPalette(); /* Psychedelic Crayola  | Jovian */
  /*
  ldPalette(pa1,skyblue);
  ldPalette(pa1,vermilion);
  ldPalette(pa1,canary);
  ldPalette(pa1,pink);
  ldPalette(pa1,forestgreen);
  ldPalette(pa1,indigo);
  ldPalette(pa1,mahogany);
  ldPalette(pa1,luislemon);
  ldPalette(pa1,chestnut);
  */
  // ldPalette(pa1,white);
  // ldPalette(pa1,black);
  //ldPalette3i(pa1,42,52,57); /* gunmetal */
  //ldPalette3i(pa1,230,230,250); /* lavender (web) */
  // ldPalette3i(pa1,223,115,255); /* heliotrope */
  ldPalette3i(pa1,181,126,220); /* lavender (floral) */
  ldPalette3i(pa1, 80,200,120); /* emerald */
  ldPalette3i(pa1,127,255,212); /* aquamarine */
  ldPalette3i(pa1,153,102,204); /* amethyst */
  ldPalette(pa1,luislemon);
  ldPalette(pa1,canary);
  /*
  pa2 = mkPalette();
  ldPalette(pa2,skyblue2);
  ldPalette(pa2,vermilion2);
  ldPalette(pa2,canary2);
  ldPalette(pa2,pink2);
  ldPalette(pa2,forestgreen2);
  ldPalette(pa2,indigo2);
  ldPalette(pa2,mahogany2);
  ldPalette(pa2,luislemon2);
  ldPalette(pa2,chestnut2);
  ldPalette(pa2,black);
  ldPalette(pa2,white);
  */
  pa3 = mkPalette(); /* Mexican Fiesta */
  //ldPalette3i(pa3,  0,168,107); /* jade */
  //ldPalette3i(pa3, 11,218, 81); /* malachite */
  //ldPalette3i(pa3, 80,200,120); /* emerald */
  //ldPalette3i(pa3,218,112,214); /* orchid */
  //ldPalette3i(pa3,  0, 35,102); /* royal blue */
  //ldPalette3i(pa3,173,111,105); /* copper penny */
  ldPalette3i(pa3,255,215,  0); /* gold */
  ldPalette3i(pa3,255,191,  0); /* amber */
  ldPalette3i(pa3,224, 17, 95); /* ruby */
  ldPalette(pa3,vermilion);
  ldPalette(pa3,mahogany);
  ldPalette(pa3,chestnut);
  ldPalette(pa3,pink);
  ldPalette(pa3,ultramarine);
  ldPalette(pa3,cerulean);
  ldPalette3i(pa3,46,139,87); /* sea green */
  cursor2x = cursor2y = 0;
  glShadeModel(GL_SMOOTH);
  glLightfv(GL_LIGHT0, GL_POSITION, pos);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, intensity);
  glLightfv(GL_LIGHT0, GL_SPECULAR, intensity);
  //glLightfv(GL_LIGHT0, GL_AMBIENT, intensity);
  glLightfv(GL_LIGHT1, GL_POSITION, pos2);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, intensity);
  glLightfv(GL_LIGHT1, GL_SPECULAR, intensity);
  //glLightfv(GL_LIGHT1, GL_AMBIENT, intensity0);
  glLightfv(GL_LIGHT2, GL_POSITION, pos3);
  glLightfv(GL_LIGHT2, GL_DIFFUSE, intensity);
  glLightfv(GL_LIGHT2, GL_SPECULAR, intensity);
  //glLightfv(GL_LIGHT2, GL_AMBIENT, intensity0);
  glLightfv(GL_LIGHT3, GL_POSITION, pos4);
  glLightfv(GL_LIGHT3, GL_DIFFUSE, intensity);
  glLightfv(GL_LIGHT3, GL_SPECULAR, intensity);
  //glLightfv(GL_LIGHT3, GL_AMBIENT, intensity0);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_NORMALIZE);
}

#define OFFSET_FILENAME "/Users/dbp/offset"

void readTimeOffset(void) {
    FILE * f = fopen(OFFSET_FILENAME,"r");
    float g;
    fscanf(f,"%f",& g);
    //printf("%f\n",g);
    //fflush(stdout);
    timeOffset = g;
    fclose(f);
}

void writeTimeOffset(void) {
    FILE * f = fopen(OFFSET_FILENAME,"w");
    float haltTime = timeOffset + glfwGetTime();
    fprintf(f,"%f\n",haltTime);
    printf("%f\n",haltTime);
    fflush(stdout);
    fflush(f);
    fclose(f);
}

int main(int argc, char *argv[]) {
    GLFWwindow* window;
    int width, height;
    readTimeOffset();
    if( !glfwInit() ) {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        exit( EXIT_FAILURE );
    }
    glfwWindowHint(GLFW_DEPTH_BITS, 16);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    if (argc >= 2) {
		windowWidth = 210;
		windowHeight = 210;
    } else {
		windowWidth = 1296;
		windowHeight = 576;
    }
    window = glfwCreateWindow(windowWidth, windowHeight, "Gears", NULL, NULL );
    if (!window) {
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
    while( !glfwWindowShouldClose(window) ) {
        // Update animation
        animate();
        // Draw gears
        draw();
        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    writeTimeOffset();
    // Terminate GLFW
    glfwTerminate();
    // Exit program
    exit( EXIT_SUCCESS );
}
