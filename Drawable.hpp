#include <vector>
#include <string>
#include <cmath>

struct vect3 {
    float x,y,z;
};

enum shader_type {
    FACE_COLOR,
    TEXTURE_ANIMATED,

};

class Drawable {

    protected:
    static shader_type shaderType;

    private:
    vect3 m_rotation;
    float *m_vertices;
    unsigned int numTris;

    public:
    int getTransform(float *buf) {
        // buf region should be 16 floats
        // combine rotation, skew etc here
        return 0;
    }
    float* getVertices() {

    }

};
