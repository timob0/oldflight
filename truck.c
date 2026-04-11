#include "flight.h"
#include "truck_mesh.h"

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

void make_truck(GLuint obj)
{
    glNewList(obj, GL_COMPILE);
    glShadeModel(GL_SMOOTH);
    glLineWidth(2.0f);

    glPushMatrix();
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
    draw_tri_batch(truck_orange_scoutcar_color, truck_orange_scoutcar_verts, truck_orange_scoutcar_normals, truck_orange_scoutcar_tri_count);
    draw_tri_batch(truck_black_scoutcar_color, truck_black_scoutcar_verts, truck_black_scoutcar_normals, truck_black_scoutcar_tri_count);
    draw_tri_batch(truck_darkgreen_scoutcar_color, truck_darkgreen_scoutcar_verts, truck_darkgreen_scoutcar_normals, truck_darkgreen_scoutcar_tri_count);
    draw_tri_batch(truck_darkred_scoutcar_color, truck_darkred_scoutcar_verts, truck_darkred_scoutcar_normals, truck_darkred_scoutcar_tri_count);
    draw_tri_batch(truck_windows_scoutcar_color, truck_windows_scoutcar_verts, truck_windows_scoutcar_normals, truck_windows_scoutcar_tri_count);
    draw_tri_batch(truck_carhandle_scoutcar_color, truck_carhandle_scoutcar_verts, truck_carhandle_scoutcar_normals, truck_carhandle_scoutcar_tri_count);
    draw_tri_batch(truck_gray_scoutcar_color, truck_gray_scoutcar_verts, truck_gray_scoutcar_normals, truck_gray_scoutcar_tri_count);
    glPopMatrix();

    glEndList();
}
