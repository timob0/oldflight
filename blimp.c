#include "flight.h"
#include "blimp_mesh.h"
#include <math.h>

static void draw_tri_batch(const float *color, const float *verts, const float *normals, int tri_count)
{
    int i;
    const float lx = -0.35f, ly = 0.90f, lz = 0.25f;
    glBegin(GL_TRIANGLES);
    for (i = 0; i < tri_count * 9; i += 3) {
        float nx = normals[i + 0];
        float ny = normals[i + 1];
        float nz = normals[i + 2];
        float lit = nx * lx + ny * ly + nz * lz;
        float shade = 0.35f + 0.65f * ((lit > 0.0f) ? lit : 0.0f);

        glColor3f(color[0] * shade, color[1] * shade, color[2] * shade);
        glNormal3f(nx, ny, nz);
        glVertex3f(verts[i + 0], verts[i + 1], verts[i + 2]);
    }
    glEnd();
}

void make_blimp(GLuint obj)
{
    glNewList(obj, GL_COMPILE);
    glShadeModel(GL_SMOOTH);
    glLineWidth(2.0f);

    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(270.0f, 0.0f, 0.0f, 1.0f);
    draw_tri_batch(blimp_ffffff_color, blimp_ffffff_verts, blimp_ffffff_normals, blimp_ffffff_tri_count);
    draw_tri_batch(blimp_039be5_color, blimp_039be5_verts, blimp_039be5_normals, blimp_039be5_tri_count);
    draw_tri_batch(blimp_455a64_color, blimp_455a64_verts, blimp_455a64_normals, blimp_455a64_tri_count);
    draw_tri_batch(blimp_795548_color, blimp_795548_verts, blimp_795548_normals, blimp_795548_tri_count);
    glPopMatrix();

    glEndList();
}
