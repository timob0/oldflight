#include "flight.h"
#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef W32
#include <unistd.h>
#endif

char *infile = NULL, *outfile = NULL;

static FILE *inf = NULL, *outf = NULL;
static long playback_frame = 0;
static int comm_initialized = 0;
static plane last_replay_plane;
static int have_last_replay_plane = 0;
static plane replay_planes[4];
static int replay_plane_used[4] = {0};
static int replay_plane_count = 0;

int enet = -1;
int number_messages = 0, MSG_SIZE = 0;
Plane planes[MAX_PLANES] = {0};
Plane messages[2 * MAX_PLANES] = {0};

#define LEGACY_RECORD_SIZE 82
#define LEGACY_HEADER_SIZE 4

static unsigned int be_u32(const unsigned char *p)
{
    return ((unsigned int)p[0] << 24) | ((unsigned int)p[1] << 16) |
           ((unsigned int)p[2] << 8) | (unsigned int)p[3];
}

static unsigned short be_u16(const unsigned char *p)
{
    return (unsigned short)(((unsigned int)p[0] << 8) | (unsigned int)p[1]);
}

static float be_f32(const unsigned char *p)
{
    union {
        unsigned int u;
        float f;
    } v;
    v.u = be_u32(p);
    return v.f;
}

static void decode_legacy_record(const unsigned char *rec, plane *dst)
{
    memset(dst, 0, sizeof(*dst));
    dst->planeid = (long)be_u32(rec + 0);
    dst->version = (char)rec[4];
    dst->cmd = (char)rec[5];
    dst->type = rec[6];
    dst->alive = rec[7] ? rec[7] : 1;
    dst->status = be_u16(rec + 26);
    memcpy(dst->myname, rec + 10, NAME_LENGTH + 1);
    dst->myname[NAME_LENGTH] = '\0';
    dst->azimuthf = (float)(short)be_u16(rec + 32);
    dst->elevationf = (float)(short)be_u16(rec + 34);
    dst->twist = (short)be_u16(rec + 36);
    dst->x = be_f32(rec + 38);
    dst->y = be_f32(rec + 42);
    dst->z = be_f32(rec + 46);

    /* minimal runtime defaults so the plane can render sanely */
    dst->fps_knots = TPS * (3600.0f / 6082.0f);
    dst->wheels = 10;
    dst->pilot_y = 10.0f;
    dst->pilot_z = 22.0f;
}

static int read_record(plane *p)
{
    unsigned char raw[LEGACY_RECORD_SIZE];
    size_t n;

    if (inf == NULL)
        return 0;

    n = fread(raw, 1, sizeof(raw), inf);
    if (n != sizeof(raw)) {
        if (feof(inf)) {
            fseek(inf, LEGACY_HEADER_SIZE, SEEK_SET);
            n = fread(raw, 1, sizeof(raw), inf);
            if (n != sizeof(raw))
                return 0;
        }
        else {
            return 0;
        }
    }

    decode_legacy_record(raw, p);
    return 1;
}

int InitComm(char *game)
{
    unsigned char header[LEGACY_HEADER_SIZE];
    (void)game;
    if (comm_initialized)
        return 0;
    MSG_SIZE = 50;
    playback_frame = 0;
    have_last_replay_plane = 0;

    if (infile) {
        inf = fopen(infile, "rb");
        if (!inf) {
            perror("InitComm infile");
            return -1;
        }
        if (fread(header, 1, sizeof(header), inf) != sizeof(header)) {
            return -1;
        }
    }

    if (outfile) {
        outf = fopen(outfile, "wb");
        if (!outf) {
            perror("InitComm outfile");
            return -1;
        }
        fwrite("\0\0\0\1", 1, 4, outf);
    }

    enet = -1;
    comm_initialized = 1;
    return 0;
}

int ExitComm(void)
{
    if (inf) {
        fclose(inf);
        inf = NULL;
    }
    if (outf) {
        fclose(outf);
        outf = NULL;
    }
    comm_initialized = 0;
    return 0;
}

Plane *new_msg(void)
{
    return NULL;
}

Plane lookup_plane(long id)
{
    (void)id;
    return NULL;
}

Plane *find_plane(Plane pfind)
{
    (void)pfind;
    return NULL;
}

int broadcast(char *msg)
{
    (void)msg;
    return 0;
}

int send_outdata(Plane p)
{
    if (outf) {
        /* TODO: encode and write legacy record to outf */
        (void)p;
    }
    if (net_enabled()) {
        net_send_plane(p);
    }
    return 0;
}

Plane get_last_replay_plane(void)
{
    return have_last_replay_plane ? &last_replay_plane : NULL;
}

int have_replay_plane(void)
{
    return have_last_replay_plane;
}

int get_replay_plane_count(void)
{
    return replay_plane_count;
}

Plane get_replay_plane(int index)
{
    if (index < 0 || index >= replay_plane_count)
        return NULL;
    return &replay_planes[index];
}

static void store_replay_plane(const plane *src)
{
    int i;
    for (i = 0; i < replay_plane_count; i++) {
        if (strcmp(replay_planes[i].myname, src->myname) == 0 || replay_planes[i].planeid == src->planeid) {
            replay_planes[i] = *src;
            replay_plane_used[i] = 1;
            return;
        }
    }
    if (replay_plane_count < 4) {
        replay_planes[replay_plane_count] = *src;
        replay_plane_used[replay_plane_count] = 1;
        fprintf(stderr, "new replay slot=%d name=%s type=%d\n", replay_plane_count, src->myname, src->type);
        replay_plane_count++;
    }
}

Plane get_indata(int count)
{
    static plane rec;
    int i;
    (void)count;

    if (!inf)
        return NULL;

    for (i = 0; i < 32; i++) {
        if (!read_record(&rec))
            return NULL;
        playback_frame++;
        if (rec.cmd != DATA_PACKET)
            continue;
        if (rec.myname[0] == '\0')
            continue;
        if (strcmp(rec.myname, "jim") == 0 || strcmp(rec.myname, "gary") == 0) {
            if (rec.type < C150 || rec.type > HELICOPTER)
                rec.type = (strcmp(rec.myname, "gary") == 0) ? F18 : F16;
            last_replay_plane = rec;
            have_last_replay_plane = 1;
            store_replay_plane(&rec);
            return &rec;
        }
    }
    return have_last_replay_plane ? &last_replay_plane : NULL;
}
