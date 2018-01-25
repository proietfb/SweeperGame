
#ifndef TMapping_H
#define TMapping_H
#ifdef __cplusplus
extern "C" {
#endif
    #include <GL/gl.h>

    class TMapping{
    public: float uvCoord[2];
    TMapping(float x, float y){
        uvCoord[0] = x;
        uvCoord[1] = y;
    }
    TMapping(){
        uvCoord[0] = uvCoord[1] = 0;
    }

    void textureCoordinate() const {
        glTexCoord2fv(uvCoord);
    }
};
#ifdef __cplusplus
}
#endif
#endif
