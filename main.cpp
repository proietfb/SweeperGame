#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <ctime>
#include <vector>
#include "sweeper.h"
#include "mesh.h"
#include "heep.h"
#include <iostream>
#include <stdio.h>
#include <fstream>

#define CAMERA_BACK_CAR 0
#define CAMERA_TOP_FIXED 1
#define CAMERA_TOP_CAR 2
#define CAMERA_PILOT 3
#define CAMERA_MOUSE 4
#define CAMERA_TYPE_MAX 5

float viewAlpha=20, viewBeta=40; // angoli che definiscono la vista
float eyeDist=6.0; // distanza dell'occhio dall'origine
int scrH=768, scrW=1366; // altezza e larghezza viewport (in pixels)
bool useWireframe=false;
bool useEnvmap=true;
bool useTextureCar=true;
bool useHeadlight=false;
bool useShadow=true;
bool useAntiAliasing = false;
bool crossed = false;
bool renderBillboard = true;
int cameraType=0;
const float S=100; //piano
const int nHeeps = 40;
std::vector<Heep> heeps;

float r = sqrt(2) * S / 2;
float rxBillboard = -r + static_cast<float>(rand()) / (static_cast<float> (RAND_MAX / (r*2)));
float rzBillboard = -r + static_cast<float>(rand()) / (static_cast<float> (RAND_MAX / (r*2)));

GLuint frameBf, colorBf, depthBf;
int sample = 2;

bool showminimap = true;
bool showOptions = true;
bool gameStart = false;
bool useCommands = true;
bool returnDisabled = false;
GLuint initTime;
GLuint deltaTime = 0.;
GLuint previousTime = 0.;
GLuint timeout = 20000;
GLuint restTime = timeout + 5000;
int tmpScore;

Mesh billboard((char*)"Mesh/billboardUV.obj"); //billboard body
Mesh billboardFace1((char*)"Mesh/billboardFace1.obj"); //billboard face1
Mesh billboardFace2((char*)"Mesh/billboardFace2.obj"); //billboard face2

Sweeper sweeper; // veicolo
int nstep=0; // numero di passi di FISICA fatti fin'ora
const int PHYS_SAMPLING_STEP=10; // numero di millisec che un passo di fisica simula

// Frames Per Seconds
const int fpsSampling = 3000; // lunghezza intervallo di calcolo fps
float fps=0; // valore di fps dell'intervallo precedente
int fpsNow=0; // quanti fotogrammi ho disegnato fin'ora nell'intervallo attuale
Uint32 timeLastInterval=0; // quando e' cominciato l'ultimo intervallo

int readbestScore(){ //legge il punteggio massimo
    std::fstream myfile("bestScore.txt", std::ios_base::in);
    int a;
    myfile >> a;
    myfile.close();
    return a;
}

int bestScoreDone = readbestScore();

void writeBestScore(){ //se il record viene battuto lo aggiorna
    if(bestScoreDone < tmpScore) {
        std::ofstream myfile("bestScore.txt");
        myfile << tmpScore;
        myfile.close();
    }
}

//motore per disegnare il testo in 2D a schermo
void drawTextString(const char* txt, float px, float py, GLubyte r, GLubyte g, GLubyte b){
    glPushMatrix();
    const char *character;
    glColor3ub(r,g,b);//si definisce il colore del testo
    glTranslatef(px,py,0);//moltiplico la corrente matrice con quella di traslazione
    glScalef(0,0,0);
    glRasterPos2f(px, py);//definisco la posizione in coordinate schermo da cui inizierà il testo
    for (character = txt; *character != '\0'; character++)
        glutBitmapCharacter( GLUT_BITMAP_HELVETICA_18 , *character);
    glPopMatrix();
}

//posiziona l'immondizia in modo random nel piano
void defineHeeps(){
    heeps.clear();
    for (int i=0; i<nHeeps; i++){
        float r = sqrt(2) * S / 2;
        float rx = -r + static_cast<float>(rand()) / (static_cast<float> (RAND_MAX / (r*2)));
        float rz = -r + static_cast<float>(rand()) / (static_cast<float> (RAND_MAX / (r*2)));
        Heep heep(&sweeper, rx,rz);
        heeps.push_back(heep);
    }
}

void Initgame(){
    gameStart = false;
    useCommands = true; //i comandi di movimento sono disabilitati prima dello start
    renderBillboard = true;
    deltaTime = 0.;
    tmpScore = sweeper.score;
    //ad ogni nuova partita posiziono il billboard con la mia faccia in una posizione random
    rxBillboard = -r + static_cast<float>(rand()) / (static_cast<float> (RAND_MAX / (r*2)));
    rzBillboard = -r + static_cast<float>(rand()) / (static_cast<float> (RAND_MAX / (r*2)));
    defineHeeps();
    sweeper.Init();
    timeout = 20000;
    restTime = 25000;
    sweeper.score = 0;
    bestScoreDone = readbestScore();
    writeBestScore();
    bestScoreDone = readbestScore();
}

void drawMenu(){
    GLuint r= 0;
    GLuint g= 0;
    GLuint b= 0;
    char score[3];
    drawTextString("Rubbishes cleaned: ", scrW*32.94/100, scrH-18, r, g, b);
    sprintf(score, "%d", sweeper.score);
    drawTextString(score, scrW*46.85/100, scrH-18, r, g, b);
    drawTextString("Best: ", scrW*49.78/100,scrH-18, r,g,b);
    sprintf(score, "%d", bestScoreDone);
    drawTextString(score, scrW*53.44/100, scrH-18, r, g, b);

    if(showOptions){

        if (showminimap)
            drawTextString("Minimap (M): On", scrW*1.44/100, scrH-18, r, g, b);
        else
            drawTextString("Minimap (M): Off", scrW*1.44/100, scrH-18, r, g, b);

        switch (cameraType) {
            case CAMERA_BACK_CAR:
                drawTextString("Camera Type (F1): Back car", scrW*1.44/100, scrH-38, r, g, b);
                break;
            case CAMERA_TOP_FIXED:
                drawTextString("Camera Type (F1): Top fixed", scrW*1.44/100, scrH-38, r, g, b);
                break;
            case CAMERA_TOP_CAR:
                drawTextString("Camera Type (F1): Top car", scrW*1.44/100, scrH-38, r, g, b);
                break;
            case CAMERA_PILOT:
                drawTextString("Camera Type (F1): Pilot", scrW*1.44/100, scrH-38, r, g, b);
                break;
            case CAMERA_MOUSE:
                drawTextString("Camera Type (F1): Mouse", scrW*1.44/100, scrH-38, r, g, b);
                break;
        }

        drawTextString("FPS: ", scrW*1.44/100, scrH-58, r, g, b);
        sprintf(score, "%d", (int)(fps+0.5));
        drawTextString(score, scrW*4.78/100, scrH-58, r, g, b);

        if (useEnvmap)
            drawTextString("Sweeper Texture (F3): on",scrW*1.44/100, scrH-78, r, g, b);
        else
            drawTextString("Sweeper Texture (F3): off", scrW*1.44/100, scrH-78, r, g, b);

        if (useHeadlight)
            drawTextString("HeadLight (F4): on", scrW*1.44/100, scrH-98, r, g, b);
        else
            drawTextString("HeadLight (F4): off", scrW*1.44/100, scrH-98, r, g, b);

        if (useShadow)
            drawTextString("Shadow (F5): on", scrW*1.44/100, scrH-118, r, g, b);
        else
            drawTextString("Shadow (F5): off", scrW*1.44/100, scrH-118, r, g, b);

        drawTextString("Anti aliasing (F6): ", scrW*1.44/100, scrH-138, r, g, b);
        char txt[1];
        if (useAntiAliasing) {
            sprintf(txt,"%d",sample);
            drawTextString(txt, scrW*12.44/100, scrH-138, r, g, b);
        }
        else
            drawTextString("off", scrW*12.44/100, scrH-138, r, g, b);



        drawTextString("(Press TAB to hide options)", scrW*1.44/100, scrH-158, r, g, b);
    }

    if (!gameStart){
        drawTextString("press ENTER to start game", scrW*43.92/100, scrH-168, r, g, b);

    }
    else {
        initTime = SDL_GetTicks();
        deltaTime += initTime - previousTime;
        previousTime = initTime;
        char millisec[1];
        drawTextString("Time to go: ", scrW*32.94/100, scrH-38, r, g, b);
        if (deltaTime <= timeout) {
            sprintf(millisec, "%.1f", deltaTime/1000.);
            drawTextString(millisec, scrW*46.85/100, scrH-38, r, g, b);
        }
        else{
            if (deltaTime < restTime){
                returnDisabled = true;
                useCommands = false;
                heeps.clear();
                renderBillboard = false;
                sprintf(score, "%.1f", timeout/1000.);
                drawTextString(score, scrW*46.85/100, scrH-38, r, g, b);
                drawTextString("Time Elapsed! You cleaned ", scrW*32.57/100, scrH-168, r, g, b);
                sprintf(score, "%d", sweeper.score);
                drawTextString(score, scrW*50.51/100, scrH-168, r, g, b);
                drawTextString("rubbishes", scrW*51.97/100, scrH-168, r, g, b);
            }
            else {
                returnDisabled = false;
                Initgame();
            }
        }
        drawTextString("/", scrW*49.41/100, scrH-38, r, g, b);
        sprintf(score, "%.1f", timeout/1000.);
        drawTextString(score, scrW*49.92/100, scrH-38, r, g, b);
    }
}

void checkBoundingBoxBillboard(){
    bool result= false;
    float ppx, ppz;
    Point3 bbmin, bbmax;
    Point3 pbbmin, pbbmax;
    pbbmin = sweeper.GetMesh().bbmin;
    pbbmax = sweeper.GetMesh().bbmax;

    ppx = sweeper.px;
    ppz = sweeper.pz;

    if(ppx >= bbmin.X()+(rxBillboard)+pbbmin.X()
        && ppz >= bbmin.Z()+rzBillboard+pbbmin.Z()
        && ppx <= bbmax.X()+rxBillboard+pbbmax.X()
        && ppz <= bbmax.Z()+rzBillboard+pbbmax.Z()){
        result=true;
    }
    if (result == true) {
        crossed = true;
    }
}

void drawBillboard(){
    glPushMatrix();
    glTranslatef(rxBillboard, 0.1, rzBillboard);
    glColor3f(1,1,1);
    billboard.RendervTexture();
    glBindTexture(GL_TEXTURE_2D,5);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_2D);
    glColor3f(1,1,1);
    billboardFace1.RendervTexture();
    billboardFace2.RendervTexture();
    glPopMatrix();
}

void drawFloor(){

    float ps[] = {0.6, 0, 0, 0};
    float pt[] = {0, 0, 0.6, 0};

    const float S=100;
    const float H=0;   // altezza
    const int K=150; //disegna K x K quads

    glBindTexture(GL_TEXTURE_2D, 3);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glColor3f(1,1,1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, ps);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, pt);

    // disegna KxK quads
    glBegin(GL_QUADS);
    glColor3f(0.6, 0.6, 0.6); // colore uguale x tutti i quads
    glNormal3f(0,1,0);       // normale verticale uguale x tutti
    for (int x=0; x<K; x++)
        for (int z=0; z<K; z++) {
            float x0=-S + 2*(x+0)*S/K;
            float x1=-S + 2*(x+1)*S/K;
            float z0=-S + 2*(z+0)*S/K;
            float z1=-S + 2*(z+1)*S/K;
            glVertex3d(x0, H, z0);
            glVertex3d(x1, H, z0);
            glVertex3d(x1, H, z1);
            glVertex3d(x0, H, z1);
        }
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

// setta le matrici di trasformazione in modo
// che le coordinate in spazio oggetto siano le coord
// del pixel sullo schemo
void  SetCoordToPixel(){
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(-1,-1,0);
  glScalef(2.0/scrW, 2.0/scrH, 1);
}

bool LoadTexture(int textbind,char *filename){
  SDL_Surface *s = IMG_Load(filename);
  if (!s) return false;

  glBindTexture(GL_TEXTURE_2D, textbind);
  gluBuild2DMipmaps(
    GL_TEXTURE_2D,
    GL_RGB,
    s->w, s->h,
    GL_RGB,
    GL_UNSIGNED_BYTE,
    s->pixels
  );
  glTexParameteri(
  GL_TEXTURE_2D,
  GL_TEXTURE_MAG_FILTER,
  GL_LINEAR );
  glTexParameteri(
  GL_TEXTURE_2D,
  GL_TEXTURE_MIN_FILTER,
  GL_LINEAR_MIPMAP_LINEAR );
  return true;
}

void miniMappa(){
    double px = sweeper.px;
    double py = sweeper.py;
    double pz = sweeper.pz;
    double angle = sweeper.facing;
    double cosf = cos(angle*M_PI/180.0);
    double sinf = sin(angle*M_PI/180.0);

    double camd, camh, ex, ey, ez, cx, cy, cz;
    double cosff, sinff;

    camd = 8.5;
    camh = 3.0;
    ex = px + camd*sinf;
    ey = py + camh;
    ez = pz + camd*cosf;
    cx = px - camd*sinf;
    cy = py + camh;
    cz = pz - camd*cosf;
    glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);//GL_MODELVIEW: determina come i miei oggetti vengono visti nel frame con coordinate mondo
    glLoadIdentity();
    gluLookAt(ex, ey+25, ez, cx, cy, cz, 0.0, 1.0, 0.0);//definisce una trasformazione di vista
    glViewport(scrW-350, scrH-350, 350, 350);
    drawFloor();

    if (renderBillboard)
        drawBillboard();

    sweeper.Render();
    for (int i = 0; i < heeps.size(); i++) {
        heeps[i].Render();
    }

    glViewport(0, 0, scrW, scrH);
}

void drawSphere(double r, int lats, int longs) {
int i, j;
  for(i = 0; i <= lats; i++) {
     double lat0 = M_PI * (-0.5 + (double) (i - 1) / lats);
     double z0  = sin(lat0);
     double zr0 =  cos(lat0);

     double lat1 = M_PI * (-0.5 + (double) i / lats);
     double z1 = sin(lat1);
     double zr1 = cos(lat1);

     glBegin(GL_QUAD_STRIP);
     for(j = 0; j <= longs; j++) {
        double lng = 2 * M_PI * (double) (j - 1) / longs;
        double x = cos(lng);
        double y = sin(lng);

//le normali servono per l'EnvMap
        glNormal3f(x * zr0, y * zr0, z0);
        glVertex3f(r * x * zr0, r * y * zr0, r * z0);
        glNormal3f(x * zr1, y * zr1, z1);
        glVertex3f(r * x * zr1, r * y * zr1, r * z1);
     }
     glEnd();
  }
}


// setto la posizione della camera
void setCamera(){

        double px = sweeper.px;
        double py = sweeper.py;
        double pz = sweeper.pz;
        double angle = sweeper.facing;
        double cosf = cos(angle*M_PI/180.0);
        double sinf = sin(angle*M_PI/180.0);
        double camd, camh, ex, ey, ez, cx, cy, cz;
        double cosff, sinff;

// controllo la posizione della camera a seconda dell'opzione selezionata
        switch (cameraType) {
            case CAMERA_BACK_CAR:
                    camd = 5.5;
                    camh = 2.5;
                    ex = px + camd*sinf;
                    ey = py + camh;
                    ez = pz + camd*cosf;
                    cx = px - camd*sinf;
                    cy = py + camh;
                    cz = pz - camd*cosf;
                    gluLookAt(ex,ey,ez,cx,cy,cz,0.0,1.0,0.0);
                    break;
            case CAMERA_TOP_FIXED:
                    camd = 0.5;
                    camh = 2.0;
                    angle = sweeper.facing;
                    cosff = cos(angle*M_PI/240.0);
                    sinff = sin(angle*M_PI/180.0);
                    ex = px + camd*sinff;
                    ey = py + camh;
                    ez = pz + camd*cosff;
                    cx = px - camd*sinf;
                    cy = py + camh;
                    cz = pz - camd*cosf;
                    gluLookAt(ex,ey,ez,cx,cy,cz,0.0,1.0,0.0);
                    break;
            case CAMERA_TOP_CAR:
                    camd = 0.5;
                    camh = 5.0;
                    ex = px + camd*sinf;
                    ey = py + camh;
                    ez = pz + camd*cosf;
                    cx = px - camd*sinf;
                    cy = py + camh;
                    cz = pz - camd*cosf;
                    gluLookAt(ex,ey+5,ez,cx,cy,cz,0.0,1.0,0.0);
                    break;
            case CAMERA_PILOT:
                    camd = 0.5;
                    camh = 2.5;
                    ex = px + camd*sinf;
                    ey = py + camh;
                    ez = pz + camd*cosf;
                    cx = px - camd*sinf;
                    cy = py + camh;
                    cz = pz - camd*cosf;
                    gluLookAt(ex,ey,ez,cx,cy,cz,0.0,1.0,0.0);
                    break;
            case CAMERA_MOUSE:
                    glTranslatef(0,0,-eyeDist);
                    glRotatef(viewBeta,  1,0,0);
                    glRotatef(viewAlpha, 0,1,0);
                    break;
            }
}

void drawSky() {
int H = 100;

  if (useWireframe) {
    glDisable(GL_TEXTURE_2D);
    glColor3f(0,0,0);
    glDisable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    drawSphere(100.0, 20, 20);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glColor3f(1,1,1);
    glEnable(GL_LIGHTING);
  }
  else
  {
        glBindTexture(GL_TEXTURE_2D,2);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP); // Env map
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP);
        glColor3f(1,1,1);
        glDisable(GL_LIGHTING);

        drawSphere(100.0, 20, 20);

        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
  }

}


void setAntiAliasing(int s){
    // genero un nuovo frame buffer
    glGenFramebuffers(1,&frameBf);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBf);
    // genero un nuovo color buffer
    glGenRenderbuffers(1, &colorBf);
    glBindRenderbuffer(GL_RENDERBUFFER, colorBf);
    //definisce storage, formato, dimensioni(in pixels) e numero di samples del buffer
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, s, GL_RGBA8, scrW, scrH);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorBf);
    //definisce un dept buffer
    glGenRenderbuffers(1, &depthBf);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBf);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, s, GL_DEPTH_COMPONENT, scrW, scrH);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBf);

}

void antiAliasing(){
    if(!useAntiAliasing){
        useAntiAliasing = true;
        setAntiAliasing(sample);
        glEnable(GL_MULTISAMPLE);
    }
    else if(sample == 16) {
        glDeleteFramebuffers(1, &frameBf);
        glDeleteRenderbuffers(1, &colorBf);
        glDeleteRenderbuffers(1, &depthBf);
        glDisable(GL_MULTISAMPLE);
        useAntiAliasing = false;
        sample = 2;
    }
    else {
        sample *= 2;
        setAntiAliasing(sample);
        glEnable(GL_MULTISAMPLE);
    }
}

/* Esegue il Rendering della scena */
void rendering(SDL_Window *win){

  if(useAntiAliasing)	glBindFramebuffer(GL_FRAMEBUFFER, frameBf);
  // un frame in piu'!!!
  fpsNow++;

  glLineWidth(3); // linee larghe

  // settiamo il viewport
  glViewport(0,0, scrW, scrH);

  // colore sfondo = bianco
  glClearColor(1,1,1,1);

  // settiamo la matrice di proiezione
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  gluPerspective( 70, //fovy, in gradi rispetto alla coordinata y
		((float)scrW) / scrH,//aspect Y/X, rispetto ad x
		0.2,//distanza dell'osservatore rispetto al Near clipping Plane
		1000  //distanza del FAR CLIPPING PLANE in coordinate vista
  );

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();

  // riempe tutto lo screen buffer di pixel color sfondo
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  // setto la posizione luce
  float tmpv[4] = {0,1,2,  0}; // ultima comp=0 => luce direzionale
  glLightfv(GL_LIGHT0, GL_POSITION, tmpv );

  setCamera();

  static float tmpcol[4] = {1,1,1,  1};
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tmpcol);
  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 127);

  glEnable(GL_LIGHTING);

  drawSky(); // disegna il cielo come sfondo
  if (renderBillboard)
    drawBillboard(); //disegna la biilboard con la mia immagine
  drawFloor(); // disegna il suolo

  sweeper.Render(); // disegna la macchina

  for(int i = 0; i<heeps.size();i++)
    heeps[i].Render();

  if(showminimap) miniMappa();

  // attendiamo la fine della rasterizzazione di
  // tutte le primitive mandate

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  checkBoundingBoxBillboard();

  if (crossed == true) {
      timeout += 5000;
      restTime = timeout + 5000;
      rxBillboard = -r + static_cast<float>(rand()) / (static_cast<float> (RAND_MAX / (r*2)));
      rzBillboard = -r + static_cast<float>(rand()) / (static_cast<float> (RAND_MAX / (r*2)));
      crossed = false;
  }

// disegnamo i fps (frame x sec) come una barra a sinistra.
// (vuota = 0 fps, piena = 100 fps)
  SetCoordToPixel();
  drawMenu();

  glBegin(GL_QUADS);
  float y=scrH*fps/100;
  float ramp=fps/100;
  glColor3f(1-ramp,0,ramp);
  glVertex2d(10,0);
  glVertex2d(10,y);
  glVertex2d(0,y);
  glVertex2d(0,0);
  glEnd();

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);

  glFinish();

  // Anti aliasing
  if(useAntiAliasing){
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      glDrawBuffer(GL_BACK); //disegno prima, scambio poi
      glBlitFramebuffer(0, 0, scrW, scrH, 0, 0, scrW, scrH, GL_COLOR_BUFFER_BIT,
          GL_NEAREST);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  // ho finito: buffer di lavoro diventa visibile
  SDL_GL_SwapWindow(win);
}

int main(int argc, char* argv[]) {
    SDL_Window *win;
    SDL_GLContext mainContext;
    Uint32 windowID;
    static int keymap[Controller::NKEYS] = {SDLK_a, SDLK_d, SDLK_w, SDLK_s};

    glutInit(&argc, argv);

      // inizializzazione di SDL
      SDL_Init( SDL_INIT_VIDEO);


      SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 ); //definito un numero minimo di bit al depthBuffer
      SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

      // facciamo una finestra di scrW x scrH pixels
      win=SDL_CreateWindow("City cleaner", 0, 0, scrW, scrH, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

      //Create our opengl context and attach it to our window
      mainContext=SDL_GL_CreateContext(win);

      GLenum err = glewInit(); //init di glew per l'anti aliasing
      if(GLEW_OK != err) return -1;

      glEnable(GL_DEPTH_TEST);
      glEnable(GL_LIGHTING);
      glEnable(GL_LIGHT0);
      glEnable(GL_NORMALIZE); // opengl, per favore, rinormalizza le normali
                              // prima di usarle

      glFrontFace(GL_CW); // consideriamo Front Facing le facce ClockWise
      glEnable(GL_COLOR_MATERIAL);
      glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
      glEnable(GL_POLYGON_OFFSET_FILL); // caro openGL sposta i
                                        // frammenti generati dalla
                                        // rasterizzazione poligoni
      glPolygonOffset(1,1);             // indietro di 1

      glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); //qualità di texture, colore, ecc durante l'interpolazione
                                                        //GL_NICEST miglior qualità che può essere scelta

      if (!LoadTexture(0,(char *)"texture/Ducato_D.jpg")) return 0;
      if (!LoadTexture(2,(char *)"texture/sky_ok.jpg")) return -1;
      if (!LoadTexture(3,(char *)"texture/tar.jpg")) return -1;
      if (!LoadTexture(4,(char *)"texture/T_BlitzGS_rubbish_01_c.bmp")) return -1;
      if (!LoadTexture(5,(char *)"texture/ColorRun.jpg")) return -1;

      defineHeeps();
      bool done=0;
      while (!done) {
        SDL_Event e;
        // guardo se c'e' un evento:
        if (SDL_PollEvent(&e)) {
         // se si: processa evento
         switch (e.type) {
          case SDL_KEYDOWN:
            if (gameStart && useCommands) {
                sweeper.controller.EatKey(e.key.keysym.sym, keymap , true);
            }
            if (e.key.keysym.sym==SDLK_F1) cameraType=(cameraType+1)%CAMERA_TYPE_MAX;
            if (e.key.keysym.sym==SDLK_F3) useEnvmap=!useEnvmap;
            if (e.key.keysym.sym==SDLK_F4) useHeadlight=!useHeadlight;
            if (e.key.keysym.sym==SDLK_F5) useShadow=!useShadow;
            if (e.key.keysym.sym==SDLK_F6) antiAliasing();
            if (e.key.keysym.sym==SDLK_m) {
                if (!showminimap) showminimap = true;
                else showminimap = false;
            }
            if (e.key.keysym.sym==SDLK_TAB) {
                if (!showOptions) showOptions = true;
                else showOptions = false;
            }
            if(e.key.keysym.sym==SDLK_RETURN && returnDisabled == false) {
                gameStart=true;
                deltaTime = 0;
                previousTime = SDL_GetTicks();
            }
            break;
          case SDL_KEYUP:
              sweeper.controller.EatKey(e.key.keysym.sym, keymap , false);
            break;
          case SDL_QUIT:
              done=1;   
	      break;
          case SDL_WINDOWEVENT:
             // dobbiamo ridisegnare la finestra
              if (e.window.event==SDL_WINDOWEVENT_EXPOSED)
                rendering(win);
              else{
               windowID = SDL_GetWindowID(win);
               if (e.window.windowID == windowID)  {
                 switch (e.window.event)  {
                      case SDL_WINDOWEVENT_SIZE_CHANGED:  {
                         scrW = e.window.data1;
                         scrH = e.window.data2;
                         glViewport(0,0,scrW,scrH);
			 rendering(win);
                         break;
                      }
                 }
               }
             }
          break;

          case SDL_MOUSEMOTION:
            if (e.motion.state & SDL_BUTTON(1) & cameraType==CAMERA_MOUSE) {
              viewAlpha+=e.motion.xrel;
              viewBeta +=e.motion.yrel;
              if (viewBeta<+5) viewBeta=+5; //per non andare sotto la macchina
              if (viewBeta>+90) viewBeta=+90;
            }
            break;

         case SDL_MOUSEWHEEL:
           if (e.wheel.y < 0 ) {
             // avvicino il punto di vista (zoom in)
             eyeDist=eyeDist*0.9;
             if (eyeDist<1) eyeDist = 1;
           };
           if (e.wheel.y > 0 ) {
             // allontano il punto di vista (zoom out)
             eyeDist=eyeDist/0.9;
           };
         break;
         }
        } else {
          // nessun evento: siamo IDLE
          Uint32 timeNow=SDL_GetTicks(); // che ore sono?

          if (timeLastInterval+fpsSampling<timeNow) {
            fps = 1000.0*((float)fpsNow) /(timeNow-timeLastInterval);
            fpsNow=0;
            timeLastInterval = timeNow;
          }

          bool doneSomething=false;
          int guardia=0; // sicurezza da loop infinito

          // finche' il tempo simulato e' rimasto indietro rispetto
          // al tempo reale...
          while (nstep*PHYS_SAMPLING_STEP < timeNow ) {
            sweeper.DoStep();
            nstep++;
            doneSomething=true;
            timeNow=SDL_GetTicks();
            if (guardia++>1000) {done=true; break;} // siamo troppo lenti!
          }
	  if(doneSomething)
	        rendering(win);
        }
      }
    SDL_GL_DeleteContext(mainContext);
    SDL_DestroyWindow(win);
    SDL_Quit ();
    return (0);
}
