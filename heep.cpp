#include <stdio.h>
#include <iostream>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <vector>
#include "point3.h"
#include "mesh.h"
#include "sweeper.h"
#include "heep.h"

Mesh heep((char*) "Mesh/Rubbish.obj");

Sweeper *player;

void Heep::DestroyHeep(){
    cleaned = true;
    player->score += 1;
}

bool heepcleaned(float px, float pz){ //funzione che verifica se c'è collisione tra lo sweeper e l'immondizia
    bool result= false;
    float ppx, ppz;
    Point3 bbmin, bbmax;
    Point3 pbbmin, pbbmax;
    pbbmin = player->GetMesh().bbmin;
    pbbmax = player->GetMesh().bbmax;

    ppx = player->px;
    ppz = player->pz;

    if(ppx >= bbmin.X()+(px)+pbbmin.X()
        && ppz >= bbmin.Z()+pz+pbbmin.Z()
        && ppx <= bbmax.X()+px+pbbmax.X()
        && ppz <= bbmax.Z()+pz+pbbmax.Z()){
        result=true;
    }
    return result;
}


void Heep::Render(){

    if(!cleaned){
        if (heepcleaned(px,pz)) {
            //se l'auto è dentro ai limiti dell'heep distruggi
            DestroyHeep();
            return;
        }
        glDisable(GL_LIGHTING);
        glPushMatrix();

        glTranslatef(px, py, pz);
        glRotatef(angle,rx,ry,rz);
        glShadeModel(GL_FLAT);


        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
        glEnable(GL_TEXTURE_2D);
        glColor3f(1,1,1);
        glRotatef(angle, rx, ry, rz);
        glBindTexture(GL_TEXTURE_2D,4);
        heep.RendervTexture();

        glPopMatrix();
    }
}

void Heep::Init(Sweeper *p, float x, float z){
    px = x;
    py = 0;
    pz = z;
    ry = 1;
    rx = rz = 0;

    player = p;
}
