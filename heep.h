#ifndef HEEP_H

#define HEEP_H

class Heep{
public:
    void Init(Sweeper *player, float xpos, float zpos);
    void Render();
    void DestroyHeep();
    Heep(Sweeper *player, float xpos, float zpos) {
        Init(player,xpos,zpos);
    }
    float px, py, pz;
    float angle, rx,ry,rz;
    bool cleaned = false;
};

#endif
