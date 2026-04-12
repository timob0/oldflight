#pragma once

/*
 * net.h — UDP LAN networking for oldflight
 *
 * Usage:
 *   net_init(port, NULL)         broadcast mode (auto-discover peers)
 *   net_init(port, "x.x.x.x")   unicast to specific host
 *   net_send_plane(p)            broadcast one plane's state
 *   net_recv()                   drain socket, update peer table
 *   net_get_plane(i)             get received peer plane by index
 *   net_plane_count()            number of live peers
 *   net_my_planeid()             our unique ID (filter own echoes)
 *   net_enabled()                1 if init succeeded
 *   net_close()                  shutdown
 */

#define NET_DEFAULT_PORT 7070

struct plane;   /* forward decl — defined in flight.h */

int            net_init(int port, const char *target_ip);
void           net_close(void);
int            net_enabled(void);
unsigned int   net_my_planeid(void);
int            net_send_plane(const struct plane *p);
int            net_recv(void);
int            net_plane_count(void);
struct plane  *net_get_plane(int idx);
