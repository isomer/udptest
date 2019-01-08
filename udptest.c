/* Test error reporting on connected UDP sockets.
 * This has very little useful error reporting, it's expected to be run under strace -T.
 * This will send a UDP packet (consisting of 64 \0's) to the given IP:53, using a connect()'d UDP socket.  Trying
 * as close as possible to emulate the glibc resolver behaviour.
 * usage:
 *   udptest [-r] IP
 * IP  the IP to attempt to send the packet to.
 * If -r is passed, then the IP_RECVERR sockopt will be turned on to the socket.
 */
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <err.h>
#include <getopt.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

#define TRY(x) if ((x) == -1) err(1, #x)

int main(int argc, char *argv[]) {
    int fd;
    static const int one = 1;

    TRY(fd = socket(AF_INET, SOCK_DGRAM|SOCK_CLOEXEC|SOCK_NONBLOCK, 0));

    if (getopt(argc, argv, "r") == 'r')
        TRY(setsockopt(fd, SOL_IP, IP_RECVERR, &one, sizeof(one)));

    struct sockaddr_in sa = {
        .sin_family = AF_INET,
        .sin_port = htons(53),
        .sin_addr.s_addr = inet_addr(argv[optind]),
    };

    TRY(connect(fd, (struct sockaddr *)&sa, sizeof(sa)));

    char buf[64] = { 0, };
    TRY(send(fd, buf, sizeof(buf), MSG_NOSIGNAL));

    struct pollfd pev[1] = {
        {
        .fd = fd,
        .events = POLLIN,
        .revents = 0,
        },
    };
    TRY(poll(pev, sizeof(pev) / sizeof(pev[0]), 5000));
    TRY(close(fd));
    printf("Result:%s%s (%x)\n",
            (pev[0].revents & POLLIN) ? " POLLIN": "",
            (pev[0].revents & POLLERR) ? " POLLERR": "",
            pev[0].revents);

}
