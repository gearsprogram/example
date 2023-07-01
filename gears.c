#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

static double matAlpha = 0.0;
static double timeOffset;
static double view_rotx = 180.0, view_roty = 30.0, view_rotz = 0.0;
static double sceneAngle = 0.0;
static double camDip = 0.0;
static double sunAngle2 = 0.0;
static double sunAngle3 = 0.0;
static double range = 18.0;
static double camHeight = 0.5;
#define HUDWIDTH 36
#define HUDHEIGHT 20
static double xHUDscale = HUDWIDTH / 2;
static double HUDscale = HUDHEIGHT / 2;
static int xCursor = 0;
static int yCursor = 0;
static int animPeriod = 3;
static int animIndex = 0;
static double mobileSpeed = 3.0;

// Business logic parameters
static int FAST = 0; // Fast rate of mobile shape change
static double FAST2;
static int VENUS = 0; // Venus fly trap design
static double VENUS2;
static int WARM = 0; // warm up the circuits
static double WARM2;

void gearMaterial(GLenum f,const GLfloat * ps) {
    GLfloat qqs[4];
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
    glMaterialfv(f,GL_DIFFUSE,rrs);
    GLfloat s[] = {50.0};
    glMaterialfv(f,GL_SHININESS,s);
}

//
// 0 : mobile param 1
// 1 : lights
// 2 : mobile param 2 (scene rotation)
// 3 : mobile param 3 (parameter for outer mobiles)
// 4 : mobile param 4 (wave)
//
double gearsGetTime(int lighting) {
    double f = (timeOffset + glfwGetTime());
    double m = FAST2 >= 0.5 ? 4.0 : 0.25;
    f *= m;
    f = 3 * f + 40.5 * sin(f / 81.0) + 4.5 * sin(f / 9.0) + 1.5 * sin(f / 3.0) + 0.5 * sin(f);
    f /= m;
    double timeShim;
    if (lighting == 0) {
        f *= mobileSpeed * 0.2;
        timeShim = 1.0 - cos(fmod(f,2.0 * M_PI));
        return 3.0 * f + 2.0 * timeShim;
    } else if (lighting == 1) {
        return 0.25 * f;
    } else if (lighting == 2) {
        return 0.05 * f;
    } else if (lighting == 3) {
        return 0;
        //f *= mobileSpeed * 0.45;
        //timeShim = 1.0 + sin(fmod(f,2.0 * M_PI));
        //return 4.0 * f + 3.0 * timeShim;
    } else if (lighting == 4) {
        return f ;
    } else {
        return 0.0;
    }
}

/* Calculate the cross product */
static void calcCross(double * c1, double * c2, double * c3,
        double a1, double a2, double a3,
        double b1, double b2, double b3) {
    * c1 = a2 * b3 - a3 * b2;
    * c2 = a3 * b1 - a1 * b3;
    * c3 = a1 * b2 - a2 * b1;
}

/* Normalize a vector in place */
static void normVec(double * a1, double * a2, double * a3) {
    double d = sqrt(* a1 * * a1 + * a2 * * a2 + * a3 * * a3);
    * a1 /= d;
    * a2 /= d;
    * a3 /= d;
}

/* Calculate the normal vector for a triangle with vertices
   at a,b,c */
static void triNorm(
        double a1, double a2, double a3,
        double b1, double b2, double b3,
        double c1, double c2, double c3) {
    double d1,d2,d3;
    double v1,v2,v3,w1,w2,w3;
    v1 = b1 - a1; v2 = b2 - a2; v3 = b3 - a3;
    w1 = c1 - a1; w2 = c2 - a2; w3 = c3 - a3;
    calcCross(&d1,&d2,&d3,v1,v2,v3,w1,w2,w3);
    normVec(&d1,&d2,&d3);
    glNormal3f(d1,d2,d3);
    glVertex3f(a1,a2,a3);
    glVertex3f(b1,b2,b3);
    glVertex3f(c1,c2,c3);
}

static double BOLDTHICK = 0.1;
static void boldline2(double x1,double y1,double x2,double y2,
        double *xout,double *yout) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    double mag = sqrt(dx*dx + dy*dy);
    dx /= mag;
    dy /= mag;
    double temp = dx;
    dx = -dy;
    dy = temp;
    dx *= BOLDTHICK / 2.0;
    dy *= BOLDTHICK / 2.0;
    xout[0] = x1 + dx; yout[0] = y1 + dy;
    xout[1] = x1 - dx; yout[1] = y1 - dy;
    xout[2] = x2 + dx; yout[2] = y2 + dy;
    xout[3] = x2 - dx; yout[3] = y2 - dy;
}

double marqueeWidth = 0.05;

static void drawboldline2(double x1,double y1,double x2,double y2) {
    double xout[4];
    double yout[4];
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
static GLfloat cerulean[4] =     {        0.0,      0.125,        0.5,CONEALPHA};
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
Palette * pa2;
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
  c[0] = (double) R / 255.0;
  c[1] = (double) G / 255.0;
  c[2] = (double) B / 255.0;
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

static double sunRadius = 1.0;
static double spikeRadius = 0.125;

double cursor2x;
double cursor2y;
int dc = 0; // draw frame counter
static double bgColor[3] = {0.7225,0.8325,0.9425};
/* OpenGL draw function & timing */
static void draw(void) {
  dc += 1;
  double bgColorShade[3];
  int i;
  for (i = 0;i < 3;i += 1) {
      bgColorShade[i] = (1.0 - matAlpha) * matAlpha * bgColor[i];
  }
  glClearColor(bgColorShade[0],bgColorShade[1],bgColorShade[2],1.0 - matAlpha);
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
  camDip = 5.0;
  glRotatef(camDip, 1.0, 0.0, 0.0);
  glTranslatef(0.0,0.0,-range);
  glRotatef(view_rotx, 1.0, 0.0, 0.0);
  glRotatef(view_roty, 0.0, 1.0, 0.0);
  glTranslatef(0.0,camHeight,0.0);
  glRotatef(fmod(sceneAngle,360.0), 0.0, 1.0, 0.0);
  glTranslatef(0.0, -4.0, 0.0);
  glPushMatrix(); /* (green grid, cursor, marquee, Sol) */
  glBegin(GL_LINES); /* green grid */
  if (1) {
      glColor3f(0.1,0.8,0.1);
      for (i = 0; i < 10; i += 1) {
          glVertex3f( 0.0 , (double) i, 0.0);
          glVertex3f( 10.0, (double) i, 0.0);
      }
      for (i = 0; i < 10; i += 1) {
          glVertex3f((double) i, 0.0 , 0.0);
          glVertex3f((double) i, 10.0, 0.0);
      }
      glEnd(); /* end green grid */
      glColor3f(0.1,0.1,0.8);
      glBegin(GL_LINES); /* blue grid */
      for (i = 0; i < 10; i += 1) {
          glVertex3f( 0.0 , 0.0, (double) i);
          glVertex3f( 10.0, 0.0, (double) i);
      }
      for (i = 0; i < 10; i += 1) {
          glVertex3f((double) i, 0.0 , 0.0);
          glVertex3f((double) i, 0.0, 10.0);
      }
      glEnd(); /* end blue grid */
  }
  glTranslatef(0.0, 4.0, 0.0);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glPushMatrix(); /* (cursor, cursor 2, marquee) */
  gearMaterial(GL_FRONT, cerulean);
  double sideWidth = 6.0;
  double platHeight = 1.0;
  double sideWidth2 = 3.0;
  double platHeight2 = 0.125;
  double platRange = 4.0;
  for (i = 0;i < 4;i += 1) {
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
      glTranslatef(0.0,-platRange,0.0);
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
      glBegin(GL_TRIANGLES); /* Draw cursor */
      drawboldline2(xCursor - 0.5, yCursor - 0.5,xCursor + 0.5, yCursor + 0.5);
      drawboldline2(xCursor - 0.5, yCursor + 0.5,xCursor + 0.5, yCursor - 0.5);
      drawboldline2(0.0,0.0,xCursor,yCursor);
      /* end cursor */
      /* Draw marquee R */
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
  int j,k;
  int p1,p2,p3;
  // Draw Sol
  int ci = 0;
  int div = 5;
  for (p1 = 0; p1 < div; p1 += 1) {
  for (p2 = 0; p2 < div; p2 += 1) {
  for (p3 = 0; p3 < div; p3 += 1) {
  ci += 1;
  int outerp = p1 % 2 == 0 && p2 % 2 == 0 && p3 % 2 == 0;
  int outerp2 = ( (p1 + p2 + p3) / 2) % 2 == 0;
  int innerp = p1 >= 1 && p1 <= 3 && p2 >= 1 && p2 <= 3 && p3 >= 1 && p3 <= 3;
  int innerp2 = (p1 + p2 + p3) % 5 == 4;
  if ( !( (outerp && outerp2) || (innerp && innerp2) ) ) {
      continue;
  }
  // Cone figure parameters
  int FACES = 9;
  double xx[FACES];
  double yy[FACES];
  // line segments define arc of cone base
  yy[0] = xx[8] = 0.0;
  yy[1] = xx[7] = 0.19509032201612825;
  yy[2] = xx[6] = 0.3826834323650898;
  yy[3] = xx[5] = 0.5555702330196022;
  yy[4] = xx[4] = 0.7071067811865475;
  yy[5] = xx[3] = 0.8314696123025451;
  yy[6] = xx[2] = 0.9238795325112867;
  yy[7] = xx[1] = 0.9807852804032304;
  yy[8] = xx[0] = 1.0;
  for (i = 0; i < FACES; i += 1) {
      xx[i] *= spikeRadius;
      yy[i] *= spikeRadius;
  }
  glPushMatrix(); /* Sol */
  double solscale = 2.0 * fabs(FAST2 - 0.5);
  glScalef(solscale,solscale,solscale);
  glScalef(0.5,0.5,0.5);
  double disp = 2.75;
  double disp2 = 2 * disp;
  glTranslatef(-disp2 + disp * p1,-disp2 + disp * p2,-disp2 + disp * p3);
  int rsgn = 1;
  if (ci % 2 == 0) {
      rsgn = -1;
  }
  double asgn = 0.0;
  double bsgn = 0.0;
  double csgn = 0.0;
  if (ci % 6 < 2) {
      asgn = 1.0;
  } else if (ci % 6 < 4) {
      bsgn = 1.0;
  } else {
      csgn = 1.0;
  }
  glRotatef(fmod(rsgn * sunAngle2,360.0),asgn,bsgn,csgn);
  double mobileWave;
  double sunRadius2[2];
  int copy;
  if ( innerp ) {
      mobileWave = sin(fmod(gearsGetTime(4),2.0 * M_PI));
      glTranslatef(mobileWave,0.0,0.0);
      sunRadius2[0] = 1.25;
      sunRadius2[1] = 0.5;
      glScalef(0.9,0.9,0.9);
      copy = 3;
  } else if ( outerp ) {
      mobileWave = 0.25 * cos(fmod(gearsGetTime(4),2.0 * M_PI));
      glTranslatef(0.0,mobileWave,0.0);
      sunRadius2[0] = 1.0;
      sunRadius2[1] = 0.25;
      glScalef(0.6,0.6,0.6);
      copy = 5;
  }
  int CONES = 14.0 + 30.0 * WARM2;
  CONES = CONES <= 0 ? 0 : CONES;
  CONES = CONES >= 144 ? 144 : CONES;
  for (k = 0;k < CONES;k += 1) {
      if ( k == CONES / 2 ) {
          glRotatef(VENUS2 * 180.0,1.0,0.0,0.0);
      }
      if (outerp) {
          gearMaterial(GL_FRONT, sbPalette(pa2,k));
      } else if (ci % 2 == 0) {
          gearMaterial(GL_FRONT, sbPalette(pa1,k));
      } else {
          gearMaterial(GL_FRONT, sbPalette(pa3,k));
      }
      for (j = 0;j < copy;j += 1) {
          glPushMatrix(); // fold
          glTranslatef(0.0,sunRadius,0.0);
          glScalef(2.0,2.0,2.0);
          glRotatef(fmod(60.0 + sunAngle3/64.0,360.0),1.0,0.0,0.0);
          for (i = 0; i < 2; i += 1) {
              glBegin(GL_TRIANGLES);
              int ii;
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
              glScalef(-1.0,1.0,-1.0);
          }
          glPopMatrix(); // end fold
          glRotatef(fmod(45.0 + sunAngle3/27.0,360.0),0.0,0.0,1.0);
          glTranslatef(0.1,0.0,0.0);
      }
      glRotatef(fmod(60.0 + sunAngle3/64.0,360.0),1.0,0.0,0.0);
  }
  glPopMatrix(); /* end Sol */
  }
  }
  }
  glPopMatrix(); /* end (green grid, cursor, marquee, Sol) */
  glPopMatrix(); /* end scene */
}

static double maxFastMove = 0.01;
static double maxVenusMove = 0.01;
static double maxWarmMove = 0.01;
/* update animation parameters */
static void animate(void) {
  double rawMatAlpha = fmod(gearsGetTime(2),2.0 * M_PI);
  matAlpha = ( 1.0 + sin(rawMatAlpha) ) / 2.0;
  // calculate FAST2
  double fastMoveAbs = fabs(FAST - FAST2);
  if ( fastMoveAbs <= maxFastMove ) {
      FAST2 = FAST;
  } else {
      fastMoveAbs = fastMoveAbs >= maxFastMove ? maxFastMove : fastMoveAbs;
      double fastMoveDelta = FAST2 < FAST ? fastMoveAbs : -fastMoveAbs;
      FAST2 = FAST2 + fastMoveDelta;
  }
  // calculate VENUS2
  double venusMoveAbs = fabs(VENUS - VENUS2);
  if ( venusMoveAbs <= maxVenusMove ) {
    VENUS2 = VENUS;
  } else {
    venusMoveAbs = venusMoveAbs >= maxVenusMove ? maxVenusMove : venusMoveAbs;
    double venusMoveDelta = VENUS2 < VENUS ? venusMoveAbs : -venusMoveAbs;
    VENUS2 = VENUS2 + venusMoveDelta;
  }
  // calculate WARM2
  double warmMoveAbs = fabs(WARM - WARM2);
  if ( warmMoveAbs <= maxWarmMove ) {
    WARM2 = WARM;
  } else {
    warmMoveAbs = warmMoveAbs >= maxWarmMove ? maxWarmMove : warmMoveAbs;
    double warmMoveDelta = WARM2 < WARM ? warmMoveAbs : -warmMoveAbs;
    WARM2 = WARM2 + warmMoveDelta;
  }
  int sceneRotate = 1;
  if (sceneRotate) {
      sceneAngle = 90 + 20.0 * gearsGetTime(2);
  } else {
      sceneAngle = -45;
  }
  double lightAngle,lightHeight;
  lightAngle = 0.48 * gearsGetTime(1);
  lightHeight = 600.0 + 400.0 * sin(fmod(lightAngle,2.0 * M_PI));
  lightAngle = 1.5 + 57.0 * lightAngle;
  GLfloat pos[4] = {0.0,0.0,0.0,0.0};
  pos[0] = 200.0 * cos(fmod(lightAngle,2.0 * M_PI));
  pos[1] = 200.0 * sin(fmod(lightAngle,2.0 * M_PI));
  pos[2] = lightHeight;
  glLightfv(GL_LIGHT0, GL_POSITION, pos);
  pos[0] *= -1.0;
  pos[1] *= -1.0;
  glLightfv(GL_LIGHT1, GL_POSITION, pos);
  GLfloat temp;
  temp = pos[0];
  pos[0] = -pos[1];
  pos[1] = temp;
  glLightfv(GL_LIGHT2, GL_POSITION, pos);
  pos[0] *= -1.0;
  pos[1] *= -1.0;
  glLightfv(GL_LIGHT3, GL_POSITION, pos);
  sunAngle2 = 15.0 * gearsGetTime(0);
  sunAngle3 = 1350.0 * gearsGetTime(2);
  animIndex += 1;
  if (0 == animIndex % animPeriod) {
      xCursor += 1;
      /* ... */

      if (xCursor > (HUDWIDTH / 2)) {
          xCursor = -(HUDWIDTH / 2);
          yCursor += 1;
          yCursor = (yCursor > 10 ? -(HUDHEIGHT / 2) : yCursor);
      }
  }
}

static int sizeChange = 0;

/* change view angle, exit upon ESC */
void key(GLFWwindow * window,int k,int s,int action,int mods) {
    if (!(action == GLFW_PRESS || action == GLFW_REPEAT)) return;
    switch (k) {
    case GLFW_KEY_A:
      if ( WARM ) {
          WARM = 0;
      } else {
          WARM = 1;
      }
      break;
    case GLFW_KEY_S:
      if ( VENUS ) {
          VENUS = 0;
      } else {
          VENUS = 1;
      }
      break;
    case GLFW_KEY_T:
      if ( FAST ) {
          FAST = 0;
      } else {
          FAST = 1;
      }
      break;
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

#define MAX_RES 23
/*                   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20   21   22   23   24 */
int resWidth[] =  {300,300,300,360,360,480,480,480,480,576,576,576,600,600,720,720,800,840,864,900,1400,1024,1080,1200,1296};
int resHeight[] = {300,360,480,480,576,300,360,480,600,256,480,576,400,480,320,480,600,480,576,400, 900, 768, 480, 720, 576};
static double windowWidth;
static double windowHeight;
static int resNum = 0;
void setResolution(int n) {
  printf("[setResolution] %d\n",n);
  if (n <= 0) {
      n = 0;
  }
  if (n >= MAX_RES) {
      n = MAX_RES;
  }
  resNum = n;
  windowWidth = resWidth[n];
  windowHeight = resHeight[n];
}

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
  //printf("reshape: %f %f\n",(double) width,(double) height);
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
  glEnable(GL_LINE_SMOOTH);
  //glEnable(GL_BLEND);
  //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

#define OFFSET_FILENAME "/Users/dbp/gears/offset"
#define DEFAULT_FILENAME "/Users/dbp/gears/default"


// Read default settings
void readDefault(void) {
    FILE * f = fopen(DEFAULT_FILENAME,"r");
    if (f == NULL) {
        return;
    }
    int c;
    int parseInvert = 0;
    int parseCount = 0;
    while (1) {
        c = fgetc(f);
        if (c == EOF) {
            return;
        } else if (c == '!') {
            parseInvert = 1;
        } else if (c == '+') {
            parseCount += 1;
        } else if (c == 'x') {
            parseCount += 5;
        } else if (c == 'r') {
            if (parseCount >= MAX_RES) {
                parseCount = MAX_RES;
            }
            setResolution(parseCount);
            printf("set resolution=%d [%dx%d] \n",parseCount,
                    resWidth[parseCount],resHeight[parseCount]);
        } else if (c == 'f') {
            if (parseInvert) {
                FAST = 0;
                printf("set nofast\n");
            } else {
                FAST = 1;
                printf("set fast\n");
            }
            parseInvert = 0;
        } else if (c == 'v') {
            if (parseInvert) {
                VENUS = 0;
                printf("set novenus\n");
            } else {
                VENUS = 1;
                printf("set venus\n");
            }
            parseInvert = 0;
        } else if (c == 'w') {
            if (parseInvert) {
                WARM = 0;
                printf("set nowarm\n");
            } else {
                WARM = 1;
                printf("set warm\n");
            }
            parseInvert = 0;
        }
    }
    fflush(stdout);
}

void writeDefault(void) {
    FILE * f = fopen(DEFAULT_FILENAME,"w");
    // write fast state
    if ( FAST ) {
        fprintf(f,"f");
        printf ("set fast\n");
    } else {
        fprintf(f,"!f");
        printf("set nofast\n");
    }
    // write venus state
    if ( VENUS ) {
        fprintf(f,"v");
        printf("set venus\n");
    } else {
        fprintf(f,"!v");
        printf("set novenus\n");
    }
    // write warm state
    if ( WARM ) {
        fprintf(f,"w");
        printf("set warm\n");
    } else {
        fprintf(f,"!w");
        printf("set nowarm\n");
    }
    // write resolution number
    int savedResNum = resNum;
    int i;
    while (resNum >= 5) {
        fprintf(f,"x");
        resNum -= 5;
    }
    for (i = 0;i < resNum;i += 1) {
        fprintf(f,"+");
    }
    resNum = savedResNum;
    fprintf(f,"r");
    printf("set resolution=%d [%dx%d] \n",resNum,
        resWidth[resNum],resHeight[resNum]);
    fprintf(f,"\n");
    fflush(f);
    fclose(f);
    fflush(stdout);
}

void readTimeOffset(void) {
    FILE * f = fopen(OFFSET_FILENAME,"r");
    double g;
    fscanf(f,"%lf",& g);
    timeOffset = g;
    fclose(f);
    printf("%f\n",g);
    fflush(stdout);
}

void writeTimeOffset(void) {
    FILE * f = fopen(OFFSET_FILENAME,"w");
    double haltTime = timeOffset + glfwGetTime();
    fprintf(f,"%f\n",haltTime);
    fflush(f);
    fclose(f);
    printf("%f\n",haltTime);
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    GLFWwindow * window;
    int width, height;
    setResolution(0);
    readDefault();
    readTimeOffset();
    if ( !glfwInit() ) {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        exit( EXIT_FAILURE );
    }
    glfwWindowHint(GLFW_DEPTH_BITS, 16);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    int i;
    int cmdResSwitch = 0;
    int cmdRes = 0;
    if (argc >= 2) {
        // parse command line arguments
        for (i = 1;i < argc;i += 1) {
            if (0 == strcmp("-0",argv[i])) {
                cmdResSwitch = 1; cmdRes = 0;
            } else if (0 == strcmp("-1",argv[i])) {
                cmdResSwitch = 1; cmdRes = 1;
            } else if (0 == strcmp("-2",argv[i])) {
                cmdResSwitch = 1; cmdRes = 2;
            } else if (0 == strcmp("-3",argv[i])) {
                cmdResSwitch = 1; cmdRes = 3;
            } else if (0 == strcmp("-4",argv[i])) {
                cmdResSwitch = 1; cmdRes = 4;
            } else if (0 == strcmp("-5",argv[i])) {
                cmdResSwitch = 1; cmdRes = 5;
            } else if (0 == strcmp("-6",argv[i])) {
                cmdResSwitch = 1; cmdRes = 6;
            } else if (0 == strcmp("-7",argv[i])) {
                cmdResSwitch = 1; cmdRes = 7;
            } else if (0 == strcmp("-8",argv[i])) {
                cmdResSwitch = 1; cmdRes = 8;
            } else if (0 == strcmp("-9",argv[i])) {
                cmdResSwitch = 1; cmdRes = 9;
            } else if (0 == strcmp("-v",argv[i])) {
                VENUS = 1; // use Venus fly trap design
            } else if (0 == strcmp("-nov",argv[i])) {
                VENUS = 0; // don't use Venus fly trap design; objet d'art
            } else if (0 == strcmp("-w",argv[i])) {
                WARM = 1; // warm circuits
            } else if (0 == strcmp("-now",argv[i])) {
                WARM = 0; // don't warm circuits; cat temperature
            }
        }
    }
    if (cmdResSwitch) {
        setResolution(cmdRes);
        printf("set resolution=%d [%dx%d] \n",cmdRes,
            resWidth[cmdRes],resHeight[cmdRes]);
    }
    FAST2 = FAST;
    VENUS2 = VENUS;
    WARM2 = WARM;
    window = glfwCreateWindow(windowWidth, windowHeight, "Gears", NULL, NULL );
    if (! window) {
        fprintf( stderr, "Failed to open GLFW window\n" );
        glfwTerminate();
        exit( EXIT_FAILURE );
    }
    // Set callback functions
    glfwSetFramebufferSizeCallback(window,reshape);
    glfwSetKeyCallback(window,key);
    glfwSetCursorPosCallback(window,cursor);
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1);
    glfwGetFramebufferSize(window,& width,& height);
    reshape(window,width,height);
    // Parse command-line options
    init();
    // Main loop
    int xpos,ypos;
    while( !glfwWindowShouldClose(window) ) {
        if (sizeChange) {
            glfwGetWindowPos(window,& xpos,& ypos);
            xpos -= 5;
            //ypos -= 5;
            glfwSetWindowPos(window,xpos,ypos);
            windowWidth += 10;
            windowHeight += 10;
            // Set callback functions
            glfwSetWindowSize(window,windowWidth,windowHeight);
            glfwGetFramebufferSize(window,& width,& height);
            reshape(window,width,height);
            sizeChange = 0;
        }
        // Update animation
        animate();
        // Draw gears
        draw();
        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    writeDefault();
    writeTimeOffset();
    // Terminate GLFW
    glfwTerminate();
    // Exit program
    exit( EXIT_SUCCESS );
}
