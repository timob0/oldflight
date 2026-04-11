#include "flight.h"
#include "helicopter_mesh.h"

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

void make_helicopter(GLuint obj)
{
    glNewList(obj, GL_COMPILE);
    glShadeModel(GL_SMOOTH);
    glLineWidth(2.0f);

    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    draw_tri_batch(helicopter_darkgreen_color, helicopter_darkgreen_verts, helicopter_darkgreen_normals, helicopter_darkgreen_tri_count);
    draw_tri_batch(helicopter_windows_aircraft_color, helicopter_windows_aircraft_verts, helicopter_windows_aircraft_normals, helicopter_windows_aircraft_tri_count);
    draw_tri_batch(helicopter_black_aircraft_color, helicopter_black_aircraft_verts, helicopter_black_aircraft_normals, helicopter_black_aircraft_tri_count);
    draw_tri_batch(helicopter_orange_airplane_color, helicopter_orange_airplane_verts, helicopter_orange_airplane_normals, helicopter_orange_airplane_tri_count);
    draw_tri_batch(helicopter_darkyellow_aircraft_color, helicopter_darkyellow_aircraft_verts, helicopter_darkyellow_aircraft_normals, helicopter_darkyellow_aircraft_tri_count);
    draw_tri_batch(helicopter_gray_aircraft_color, helicopter_gray_aircraft_verts, helicopter_gray_aircraft_normals, helicopter_gray_aircraft_tri_count);
    draw_tri_batch(helicopter_red_airplane_color, helicopter_red_airplane_verts, helicopter_red_airplane_normals, helicopter_red_airplane_tri_count);
    glPopMatrix();

    glEndList();
}
