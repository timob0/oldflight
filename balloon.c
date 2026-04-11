#include "flight.h"
#include "balloon_mesh.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static GLuint basket_texture = 0;

enum BalloonColorMode {
    BALLOON_COLOR_FLAT = 0,
    BALLOON_COLOR_ENVELOPE = 1,
    BALLOON_COLOR_TOP = 2
};

static void sample_envelope_color(float x, float y, float z, float out[3])
{
    const float hull_yellow[3] = {0.96f, 0.84f, 0.16f};
    const float hull_orange[3] = {0.95f, 0.55f, 0.16f};
    const float hull_red[3]    = {0.86f, 0.23f, 0.18f};
    const float hull_navy[3]   = {0.18f, 0.22f, 0.36f};
    float theta = atan2f(z, x);
    float center = 30.0f + 5.5f * sinf(theta * 8.0f);

    if (y < 8.0f) {
        out[0] = hull_yellow[0]; out[1] = hull_yellow[1]; out[2] = hull_yellow[2];
        return;
    }
    if (y > 53.0f) {
        out[0] = hull_yellow[0]; out[1] = hull_yellow[1]; out[2] = hull_yellow[2];
        return;
    }
    if (fabsf(y - center) < 4.8f && y > 16.0f && y < 42.0f) {
        out[0] = hull_navy[0]; out[1] = hull_navy[1]; out[2] = hull_navy[2];
        return;
    }
    if (y > 40.0f) {
        out[0] = hull_orange[0]; out[1] = hull_orange[1]; out[2] = hull_orange[2];
        return;
    }
    out[0] = hull_red[0]; out[1] = hull_red[1]; out[2] = hull_red[2];
}

static void draw_tri_batch(const float *color, const float *verts, const float *normals, int tri_count, int mode)
{
    int i;
    const float lx = -0.35f, ly = 0.90f, lz = 0.25f; /* simple fake sun */
    glBegin(GL_TRIANGLES);
    for (i = 0; i < tri_count * 9; i += 3) {
        float nx = normals[i + 0];
        float ny = normals[i + 1];
        float nz = normals[i + 2];
        float base[3];
        float lit = nx * lx + ny * ly + nz * lz;
        float shade = 0.35f + 0.65f * ((lit > 0.0f) ? lit : 0.0f);

        if (mode == BALLOON_COLOR_ENVELOPE) {
            sample_envelope_color(verts[i + 0], verts[i + 1], verts[i + 2], base);
        }
        else if (mode == BALLOON_COLOR_TOP) {
            base[0] = 0.96f; base[1] = 0.84f; base[2] = 0.16f;
        }
        else {
            base[0] = color[0]; base[1] = color[1]; base[2] = color[2];
        }

        glColor3f(base[0] * shade, base[1] * shade, base[2] * shade);
        glNormal3f(nx, ny, nz);
        glVertex3f(verts[i + 0], verts[i + 1], verts[i + 2]);
    }
    glEnd();
}

static GLuint load_ppm_texture(const char *path)
{
    FILE *fp;
    char magic[3] = {0};
    int width, height, maxval;
    unsigned char *rgb = NULL;
    unsigned char *rgba = NULL;
    GLuint tex = 0;
    size_t pixels;
    int c;
    size_t i;

    fp = fopen(path, "rb");
    if (!fp) return 0;
    if (fscanf(fp, "%2s", magic) != 1 || strcmp(magic, "P6") != 0) {
        fclose(fp);
        return 0;
    }
    c = fgetc(fp);
    while (c == '#') {
        while (c != '\n' && c != EOF) c = fgetc(fp);
        c = fgetc(fp);
    }
    ungetc(c, fp);
    if (fscanf(fp, "%d %d %d", &width, &height, &maxval) != 3 || maxval != 255) {
        fclose(fp);
        return 0;
    }
    fgetc(fp);

    pixels = (size_t)width * (size_t)height;
    rgb = (unsigned char *)malloc(pixels * 3);
    rgba = (unsigned char *)malloc(pixels * 4);
    if (!rgb || !rgba) {
        free(rgb); free(rgba); fclose(fp); return 0;
    }
    if (fread(rgb, 3, pixels, fp) != pixels) {
        free(rgb); free(rgba); fclose(fp); return 0;
    }
    fclose(fp);

    for (i = 0; i < pixels; i++) {
        rgba[i * 4 + 0] = rgb[i * 3 + 0];
        rgba[i * 4 + 1] = rgb[i * 3 + 1];
        rgba[i * 4 + 2] = rgb[i * 3 + 2];
        rgba[i * 4 + 3] = 255;
    }

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

    free(rgb);
    free(rgba);
    return tex;
}

static void ensure_basket_texture(void)
{
    if (basket_texture != 0) return;
    basket_texture = load_ppm_texture("/Users/timo/opengl/oldflight/obj/processed-korb-256.ppm");
}

static void draw_rod(float x0, float y0, float z0, float x1, float y1, float z1)
{
    glColor3f(0.30f, 0.22f, 0.14f);
    glBegin(GL_LINES);
    glVertex3f(x0, y0, z0);
    glVertex3f(x1, y1, z1);
    glEnd();
}

static void draw_textured_basket(void)
{
    const float x0 = -3.5f, x1 = 3.5f;
    const float y0 = -3.0f, y1 = 3.0f;
    const float z0 = -3.5f, z1 = 3.5f;

    ensure_basket_texture();

    draw_rod(-6.0f, 17.0f, -6.0f, -3.2f, 3.0f, -3.2f);
    draw_rod( 6.0f, 17.0f, -6.0f,  3.2f, 3.0f, -3.2f);
    draw_rod(-6.0f, 17.0f,  6.0f, -3.2f, 3.0f,  3.2f);
    draw_rod( 6.0f, 17.0f,  6.0f,  3.2f, 3.0f,  3.2f);

    glColor3f(0.18f, 0.18f, 0.18f);
    glPushMatrix();
    glTranslatef(0.0f, 8.0f, 0.0f);
    glScalef(6.0f, 1.0f, 6.0f);
    glutSolidCube(1.0);
    glPopMatrix();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, basket_texture);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0); glVertex3f(x0, y0, z1);
    glTexCoord2f(0.5f, 0); glVertex3f(x1, y0, z1);
    glTexCoord2f(0.5f, 0.5f); glVertex3f(x1, y1, z1);
    glTexCoord2f(0, 0.5f); glVertex3f(x0, y1, z1);
    glNormal3f(0, 0, -1);
    glTexCoord2f(0, 0); glVertex3f(x1, y0, z0);
    glTexCoord2f(0.5f, 0); glVertex3f(x0, y0, z0);
    glTexCoord2f(0.5f, 0.5f); glVertex3f(x0, y1, z0);
    glTexCoord2f(0, 0.5f); glVertex3f(x1, y1, z0);
    glNormal3f(-1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(x0, y0, z0);
    glTexCoord2f(0.5f, 0); glVertex3f(x0, y0, z1);
    glTexCoord2f(0.5f, 0.5f); glVertex3f(x0, y1, z1);
    glTexCoord2f(0, 0.5f); glVertex3f(x0, y1, z0);
    glNormal3f(1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(x1, y0, z1);
    glTexCoord2f(0.5f, 0); glVertex3f(x1, y0, z0);
    glTexCoord2f(0.5f, 0.5f); glVertex3f(x1, y1, z0);
    glTexCoord2f(0, 0.5f); glVertex3f(x1, y1, z1);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    glColor3f(0.40f, 0.24f, 0.10f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(x0, y1, z0); glVertex3f(x0, y1, z1); glVertex3f(x1, y1, z1); glVertex3f(x1, y1, z0);
    glNormal3f(0, -1, 0);
    glVertex3f(x0, y0, z1); glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z0); glVertex3f(x1, y0, z1);
    glEnd();
}

void make_balloon(GLuint obj)
{
    glNewList(obj, GL_COMPILE);
    glShadeModel(GL_SMOOTH);
    glLineWidth(2.0f);

    draw_tri_batch(m_11809_hot_air_balloon_balloon_color,
                   m_11809_hot_air_balloon_balloon_verts,
                   m_11809_hot_air_balloon_balloon_normals,
                   m_11809_hot_air_balloon_balloon_tri_count,
                   BALLOON_COLOR_ENVELOPE);
    draw_tri_batch(m_11809_hot_air_balloon_top_color,
                   m_11809_hot_air_balloon_top_verts,
                   m_11809_hot_air_balloon_top_normals,
                   m_11809_hot_air_balloon_top_tri_count,
                   BALLOON_COLOR_TOP);

    draw_textured_basket();

    glEndList();
}
