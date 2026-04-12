/*
 * net.c — UDP LAN networking for oldflight
 *
 * Each client broadcasts its own plane state at ~30 Hz (every other tick).
 * Peers are tracked in a table; entries expire after NET_TIMEOUT_TICKS frames
 * without a fresh packet.
 *
 * Packet format: fixed-size binary struct with a 4-byte magic header "OFLT".
 * Host byte order is used (LAN-only; mixed-endian setups are uncommon).
 */

#include "net.h"
#include "flight.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef W32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

/* ------------------------------------------------------------------ */
/* Wire packet                                                          */
/* ------------------------------------------------------------------ */

#define NET_MAGIC          0x4F464C54u  /* "OFLT" */
#define NET_MAX_PEERS      15
#define NET_TIMEOUT_TICKS  180          /* ~3 s at 60 fps */

#if defined(_MSC_VER)
#pragma pack(push, 1)
#define OFLT_PACKED
#elif defined(__GNUC__)
#define OFLT_PACKED __attribute__((packed))
#else
#define OFLT_PACKED
#endif

typedef struct OFLT_PACKED {
    unsigned int   magic;
    unsigned int   planeid;
    unsigned short type;
    unsigned short alive;
    char           myname[NAME_LENGTH + 1];
    unsigned char  _pad1;               /* align to 4 */
    float          x, y, z;
    float          azimuthf;
    float          elevationf;
    short          twist;
    unsigned short _pad2;
    unsigned int   status;
    short          wheels;
    short          _pad3;
    float          rollers;
    float          rudder;
    float          elevator;
} NetPacket;

#if defined(_MSC_VER)
#pragma pack(pop)
#endif

/* ------------------------------------------------------------------ */
/* State                                                                */
/* ------------------------------------------------------------------ */

#ifdef W32
typedef SOCKET net_socket_t;
typedef int net_socklen_t;
#define NET_INVALID_SOCKET INVALID_SOCKET
#else
typedef int net_socket_t;
typedef socklen_t net_socklen_t;
#define NET_INVALID_SOCKET (-1)
#endif

static net_socket_t recv_sock  = NET_INVALID_SOCKET;
static net_socket_t send_sock  = NET_INVALID_SOCKET;
static int  net_active = 0;
static unsigned int my_id = 0;

static struct sockaddr_in dest_addr;

typedef struct {
    plane p;
    int   active;
    int   timeout;
} NetPeer;

static NetPeer peers[NET_MAX_PEERS];

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

#ifdef W32
static int net_platform_init(void)
{
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
}

static void net_platform_cleanup(void)
{
    WSACleanup();
}

static unsigned long net_process_id(void)
{
    return (unsigned long)_getpid();
}

static int net_set_nonblocking(net_socket_t sock)
{
    u_long mode = 1;
    return ioctlsocket(sock, FIONBIO, &mode);
}

static void net_close_socket(net_socket_t *sock)
{
    if (*sock != NET_INVALID_SOCKET) {
        closesocket(*sock);
        *sock = NET_INVALID_SOCKET;
    }
}
#else
static int net_platform_init(void)
{
    return 0;
}

static void net_platform_cleanup(void)
{
}

static unsigned long net_process_id(void)
{
    return (unsigned long)getpid();
}

static int net_set_nonblocking(net_socket_t sock)
{
    return fcntl(sock, F_SETFL, O_NONBLOCK);
}

static void net_close_socket(net_socket_t *sock)
{
    if (*sock != NET_INVALID_SOCKET) {
        close(*sock);
        *sock = NET_INVALID_SOCKET;
    }
}
#endif

static int peer_find(unsigned int id)
{
    int i;
    for (i = 0; i < NET_MAX_PEERS; i++)
        if (peers[i].active && (unsigned int)peers[i].p.planeid == id)
            return i;
    return -1;
}

static int peer_alloc(void)
{
    int i;
    for (i = 0; i < NET_MAX_PEERS; i++)
        if (!peers[i].active)
            return i;
    return -1;   /* table full */
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

int net_init(int port, const char *target_ip)
{
    int yes = 1;

    memset(peers, 0, sizeof(peers));
    if (net_platform_init() != 0) {
        fprintf(stderr, "[net] platform init failed\n");
        return -1;
    }
    my_id = (unsigned int)(time(NULL) ^ net_process_id());

    recv_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (recv_sock == NET_INVALID_SOCKET) {
        perror("[net] recv socket");
        net_platform_cleanup();
        return -1;
    }

    setsockopt(recv_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));
#ifdef SO_REUSEPORT
    setsockopt(recv_sock, SOL_SOCKET, SO_REUSEPORT, (const char *)&yes, sizeof(yes));
#endif

    /* Non-blocking receive */
    net_set_nonblocking(recv_sock);

    /* Bind on all interfaces so we receive incoming packets */
    {
        struct sockaddr_in bind_addr;
        memset(&bind_addr, 0, sizeof(bind_addr));
        bind_addr.sin_family      = AF_INET;
        bind_addr.sin_port        = htons((unsigned short)port);
        bind_addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(recv_sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
            perror("[net] bind");
            net_close_socket(&recv_sock);
            net_platform_cleanup();
            return -1;
        }
    }

    send_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (send_sock == NET_INVALID_SOCKET) {
        perror("[net] send socket");
        net_close_socket(&recv_sock);
        net_platform_cleanup();
        return -1;
    }
    if (!target_ip)
        setsockopt(send_sock, SOL_SOCKET, SO_BROADCAST, (const char *)&yes, sizeof(yes));

    /* Destination address */
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port   = htons((unsigned short)port);
    if (target_ip)
        dest_addr.sin_addr.s_addr = inet_addr(target_ip);
    else
        dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    net_active = 1;
    fprintf(stderr, "[net] ready — port %d, mode=%s, id=%u\n",
            port, target_ip ? target_ip : "broadcast", my_id);
    return 0;
}

void net_close(void)
{
    net_close_socket(&recv_sock);
    net_close_socket(&send_sock);
    net_platform_cleanup();
    net_active = 0;
    fprintf(stderr, "[net] closed\n");
}

int net_enabled(void)      { return net_active; }
unsigned int net_my_planeid(void) { return my_id; }

/* Send one plane's state to peers */
int net_send_plane(const struct plane *p)
{
    NetPacket pkt;

    if (!net_active) return 0;

    memset(&pkt, 0, sizeof(pkt));
    pkt.magic      = NET_MAGIC;
    pkt.planeid    = (p->planeid != 0) ? (unsigned int)p->planeid : my_id;
    pkt.type       = (unsigned short)p->type;
    pkt.alive      = (unsigned short)(p->alive ? p->alive : 1);
    memcpy(pkt.myname, p->myname, NAME_LENGTH + 1);
    pkt.myname[NAME_LENGTH] = '\0';
    pkt.x          = p->x;
    pkt.y          = p->y;
    pkt.z          = p->z;
    pkt.azimuthf   = p->azimuthf;
    pkt.elevationf = p->elevationf;
    pkt.twist      = (short)p->twist;
    pkt.status     = p->status;
    pkt.wheels     = (short)p->wheels;
    pkt.rollers    = p->rollers;
    pkt.rudder     = p->rudder;
    pkt.elevator   = p->elevator;

    sendto(send_sock, (const char *)&pkt, sizeof(pkt), 0,
           (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    return 1;
}

/* Drain socket, update peer table. Returns number of new/updated entries. */
int net_recv(void)
{
    NetPacket pkt;
    struct sockaddr_in src;
    net_socklen_t srclen = sizeof(src);
    int updates = 0;
    int i;
    int n;

    if (!net_active) return 0;

    /* Age out stale peers */
    for (i = 0; i < NET_MAX_PEERS; i++) {
        if (!peers[i].active) continue;
        if (--peers[i].timeout <= 0)
            peers[i].active = 0;
    }

    /* Drain all waiting packets */
    for (;;) {
        n = (int)recvfrom(recv_sock, (char *)&pkt, sizeof(pkt), 0,
                          (struct sockaddr *)&src, &srclen);
        if (n < 0) break;                          /* EAGAIN / done */
        if ((size_t)n < sizeof(pkt))   continue;   /* short packet   */
        if (pkt.magic != NET_MAGIC)    continue;   /* not ours       */
        if (pkt.planeid == my_id)      continue;   /* own echo       */

        i = peer_find(pkt.planeid);
        if (i < 0) {
            i = peer_alloc();
            if (i < 0) continue;   /* table full, drop */
            memset(&peers[i], 0, sizeof(peers[i]));
            fprintf(stderr, "[net] new peer id=%u name=%.15s type=%d\n",
                    pkt.planeid, pkt.myname, pkt.type);
        }

        {
            plane *pp      = &peers[i].p;
            pp->planeid    = (long)pkt.planeid;
            pp->type       = (short)pkt.type;
            pp->alive      = (short)(pkt.alive ? pkt.alive : 1);
            memcpy(pp->myname, pkt.myname, NAME_LENGTH + 1);
            pp->myname[NAME_LENGTH] = '\0';
            pp->x          = pkt.x;
            pp->y          = pkt.y;
            pp->z          = pkt.z;
            pp->azimuthf   = pkt.azimuthf;
            pp->elevationf = pkt.elevationf;
            pp->twist      = (short)pkt.twist;
            pp->status     = pkt.status;
            pp->wheels     = (short)pkt.wheels;
            pp->rollers    = pkt.rollers;
            pp->rudder     = pkt.rudder;
            pp->elevator   = pkt.elevator;
            /* rendering defaults */
            pp->fps_knots  = TPS * (3600.0f / 6082.0f);
            pp->pilot_y    = 10.0f;
            pp->pilot_z    = 22.0f;
            pp->obj        = (pp->type >= C150 && pp->type <= HELICOPTER) ? pp->type : F18;
        }

        peers[i].active  = 1;
        peers[i].timeout = NET_TIMEOUT_TICKS;
        updates++;
    }
    return updates;
}

/* Number of currently live peers */
int net_plane_count(void)
{
    int n = 0, i;
    for (i = 0; i < NET_MAX_PEERS; i++)
        if (peers[i].active) n++;
    return n;
}

/* Return peer plane by logical index (0-based, only active peers counted) */
struct plane *net_get_plane(int idx)
{
    int n = 0, i;
    for (i = 0; i < NET_MAX_PEERS; i++) {
        if (!peers[i].active) continue;
        if (n == idx) return &peers[i].p;
        n++;
    }
    return NULL;
}
