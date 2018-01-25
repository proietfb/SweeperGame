// Sweeper.cpp
// implementazione dei metodi definiti in Sweeper.h

#include <stdio.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>
#include <vector> // la classe vector di STL
#include <ctime>

#include "sweeper.h"
#include "point3.h"
#include "mesh.h"

// var globale di tipo mesh
Mesh carlinga((char*)"Mesh/Carlinga.obj");
Mesh wheelFR((char *)"Mesh/wheel_front.obj");
Mesh wheelBR((char *)"Mesh/wheel_back.obj");

extern bool useEnvmap; // var globale esterna: per usare l'evnrionment mapping
extern bool useHeadlight; // var globale esterna: per usare i fari
extern bool useShadow; // var globale esterna: per generare l'ombra


// da invocare quando e' stato premuto/rilasciato il tasto numero "keycode"
void Controller::EatKey(int keycode, int* keymap, bool pressed_or_released)
{
  for (int i=0; i<NKEYS; i++){
    if (keycode==keymap[i]) key[i]=pressed_or_released;
  }
}

// Funzione che prepara tutto per usare un env map
void SetupEnvmapTextureCarlinga1()
{
  // facciamo binding con la texture 1
  glBindTexture(GL_TEXTURE_2D,0);
  glDisable(GL_TEXTURE_GEN_S); // disabilito la generazione automatica delle coord texture S e T
  glDisable(GL_TEXTURE_GEN_T);
  glEnable(GL_TEXTURE_2D);
  glColor3f(1,1,1); // metto il colore neutro (viene moltiplicato col colore texture, componente per componente)
  glDisable(GL_LIGHTING); // disabilito il lighting OpenGL standard (lo faccio con la texture)
}


// funzione che prepara tutto per creare le coordinate texture (s,t) da (x,y,z)
// Mappo l'intervallo [ minY , maxY ] nell'intervallo delle T [0..1]
//     e l'intervallo [ minZ , maxZ ] nell'intervallo delle S [0..1]
void SetupWheelTexture(Point3 min, Point3 max){
    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
}

Mesh Sweeper::GetMesh(){
    return carlinga;
}

// DoStep: facciamo un passo di fisica (a delta_t costante)
//
// Indipendente dal rendering.
//
// ricordiamoci che possiamo LEGGERE ma mai SCRIVERE
// la struttura controller da DoStep
void Sweeper::DoStep(){
  // computiamo l'evolversi della macchina
  static int i=5;

  float vxm, vym, vzm; // velocita' in spazio macchina

  // da vel frame mondo a vel frame macchina
  float cosf = cos(facing*M_PI/180.0);
  float sinf = sin(facing*M_PI/180.0);
  vxm = +cosf*vx - sinf*vz;
  vym = vy;
  vzm = +sinf*vx + cosf*vz;

  // gestione dello sterzo
  if (controller.key[Controller::LEFT]) sterzo+=velSterzo;
  if (controller.key[Controller::RIGHT]) sterzo-=velSterzo;
  sterzo*=velRitornoSterzo; // ritorno a volante dritto

  if (controller.key[Controller::ACC]) vzm-=accMax; // accelerazione in avanti
  if (controller.key[Controller::DEC]) vzm+=accMax; // accelerazione indietro

  // attriti (semplificando)
  vxm*=attritoX;
  vym*=attritoY;
  vzm*=attritoZ;

  // l'orientamento della macchina segue quello dello sterzo
  // (a seconda della velocita' sulla z)
  facing = facing - (vzm*grip)*sterzo;

  // rotazione mozzo ruote (a seconda della velocita' sulla z)
  float da ; //delta angolo
  da=(360.0*vzm)/(2.0*M_PI*raggioRuotaA);
  mozzoA+=da;
  da=(360.0*vzm)/(2.0*M_PI*raggioRuotaP);
  mozzoP+=da;

  // ritorno a vel coord mondo
  vx = +cosf*vxm + sinf*vzm;
  vy = vym;
  vz = -sinf*vxm + cosf*vzm;

  // posizione = posizione + velocita * delta t (ma delta t e' costante)
  px+=vx;
  py+=vy;
  pz+=vz;
}


void Controller::Init(){
  for (int i=0; i<NKEYS; i++) key[i]=false;
}

void Sweeper::Init(){
  // inizializzo lo stato della macchina
  px=pz=facing=0; // posizione e orientamento
  py=0.0;

  mozzoA=mozzoP=sterzo=0;   // stato
  vx=vy=vz=0;      // velocita' attuale
  // inizializzo la struttura di controllo
  controller.Init();

  velSterzo=2.4;         // A
  velRitornoSterzo=0.93; // B, sterzo massimo = A*B / (1-B)

  accMax = 0.0011;

  // attriti: percentuale di velocita' che viene mantenuta
  // 1 = no attrito
  // <<1 = attrito grande
  attritoZ = 0.991;  // piccolo attrito sulla Z (nel senso di rotolamento delle ruote)
  attritoX = 0.8;  // grande attrito sulla X (per non fare slittare la macchina)
  attritoY = 1.0;  // attrito sulla y nullo

  // Nota: vel max = accMax*attritoZ / (1-attritoZ)

  raggioRuotaA = 0.25;
  raggioRuotaP = 0.35;

  grip = 0.45; // quanto il facing macchina si adegua velocemente allo sterzo
}

// attiva una luce di openGL per simulare un faro della macchina
void Sweeper::DrawHeadlight(float x, float y, float z, int lightN, bool useHeadlight) const{
  int usedLight=GL_LIGHT1 + lightN;

  if(useHeadlight)
  {
  glEnable(usedLight);

  float col0[4]= {0.8,0.8,0.0,  1};
  glLightfv(usedLight, GL_DIFFUSE, col0);

  float col1[4]= {0.5,0.5,0.0,  1};
  glLightfv(usedLight, GL_AMBIENT, col1);

  float tmpPos[4] = {x,y,z,  1}; // ultima comp=1 => luce posizionale
  glLightfv(usedLight, GL_POSITION, tmpPos );

  float tmpDir[4] = {0,0,-1,  0}; // ultima comp=1 => luce posizionale
  glLightfv(usedLight, GL_SPOT_DIRECTION, tmpDir );

  glLightf (usedLight, GL_SPOT_CUTOFF, 30);
  glLightf (usedLight, GL_SPOT_EXPONENT,5);

  glLightf(usedLight,GL_CONSTANT_ATTENUATION,0);
  glLightf(usedLight,GL_LINEAR_ATTENUATION,1);
  }
  else
   glDisable(usedLight);
}

// funzione che disegna tutti i pezzi della macchina
// (carlinga, + 4 route)
// (da invocarsi due volte: per la macchina, e per la sua ombra)
// (se usecolor e' falso, NON sovrascrive il colore corrente
//  e usa quello stabilito prima di chiamare la funzione)
void Sweeper::RenderAllParts(bool usecolor) const{

  // disegna la carliga con una mesh
  glPushMatrix();
  if (!useEnvmap) {
    if (usecolor) glColor3f(1,0,0);     // colore rosso, da usare con Lighting
  }
  else {
    if (usecolor) SetupEnvmapTextureCarlinga1();
  }
  carlinga.RendervTexture(); // rendering delle mesh carlinga usando normali per vertice

  glPopMatrix();

  for (int i=0; i<2; i++) {
    // i==0 -> disegno ruote destre.
    // i==1 -> disegno ruote sinistre.
    int sign;
    if (i==0) sign=1;
    else sign=-1;
    glPushMatrix();
    if (i==1) {
        glTranslatef(0,+wheelFR.Center().Y(), 0);
        glRotatef(180, 0,0,1 );
        glTranslatef(0,-wheelFR.Center().Y(), 0);
    }

    glTranslate(  wheelFR.Center() );
    glRotatef( sign*sterzo,0,1,0);
    glRotatef(+sign*mozzoA,1,0,0);
    glTranslate( -wheelFR.Center() );

    if (usecolor) glColor3f(.6,.6,.6);
    if (usecolor) SetupWheelTexture(wheelFR.bbmin,wheelFR.bbmax);
    wheelFR.RendervTexture();
    glDisable(GL_TEXTURE_2D);
    if (usecolor) glColor3f(0.9,0.9,0.9);
    glPopMatrix();

    glPushMatrix();
    if (i==1) {
        glTranslatef(0,+wheelBR.Center().Y(), 0);
        glRotatef(180, 0,0,1 );
        glTranslatef(0,-wheelBR.Center().Y(), 0);
    }

    glTranslate(  wheelBR.Center() );
    glRotatef(+sign*mozzoA,1,0,0);
    glTranslate( -wheelBR.Center() );

    if (usecolor) glColor3f(.6,.6,.6);
    if (usecolor) SetupWheelTexture(wheelBR.bbmin,wheelBR.bbmax);
    wheelBR.RendervTexture();
    glPopMatrix();
  }
}
// disegna a schermo
void Sweeper::Render() const{
  // sono nello spazio mondo
  glPushMatrix();

  glTranslatef(px,py,pz);
  glRotatef(facing, 0,1,0);

  // sono nello spazio MACCHINA
  DrawHeadlight(-0.3,0,0, 0, useHeadlight); // accendi faro sinistro
  DrawHeadlight(+0.3,0,0, 0, useHeadlight); // accendi faro destro

  RenderAllParts(true);

  // ombra!
  if(useShadow)
  {
    glColor3f(0.4, 0.4, 0.4); // colore fisso
    glTranslatef(0,0.01,0); // alzo l'ombra di un epsilon per evitare z-fighting con il pavimento
    glScalef(1.01,0,1.01);  // appiattisco sulla Y, ingrandisco dell'1% sulla Z e sulla X
    glDisable(GL_LIGHTING); // niente lighing per l'ombra
    RenderAllParts(false);  // disegno la macchina appiattita

    glEnable(GL_LIGHTING);
  }
  glPopMatrix();

}
