// SPDX-License-Identifier: GPL-2.0-only
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define RESPONSE_SIZE 16384

struct query { const char *key; const char *command; const char *prefix; };

static void json_string(const char *s)
{
    putchar('"');
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') printf("\\%c", c);
        else if (c < 32) printf("\\u%04x", c);
        else putchar(c);
    }
    putchar('"');
}

static int error_json(const char *reason)
{
    printf("{\"ok\":false,\"error\":");
    json_string(reason);
    puts("}");
    return 1;
}

static long long milliseconds(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (long long)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

static int number(const char *s, unsigned min, unsigned max, unsigned *value)
{
    if (!s || !*s) return 0;
    for (const char *p = s; *p; p++) if (*p < '0' || *p > '9') return 0;
    char *end;
    errno = 0;
    unsigned long n = strtoul(s, &end, 10);
    if (errno || *end || n < min || n > max) return 0;
    *value = (unsigned)n;
    return 1;
}

static int terminal(const char *s)
{
    if (strstr(s, "\nOK\r") || strstr(s, "\nOK\n")) return 1;
    if (strstr(s, "\nERROR") || strstr(s, "+CME ERROR:") ||
        strstr(s, "+CMS ERROR:")) return -1;
    return 0;
}

/* RPMSG preserves packets. Blocking writes permit GLINK RX intent requests. */
static int query(int fd, const char *command, char *response)
{
    response[0] = 0;
    struct pollfd p = { .fd = fd, .events = POLLIN };
    char stale[4096];
    for (int n = 0; n < 32 && poll(&p, 1, 0) > 0; n++) {
        if (!(p.revents & POLLIN)) break;
        if (read(fd, stale, sizeof(stale)) <= 0) break;
    }
    size_t length = strlen(command);
    if (write(fd, command, length) != (ssize_t)length) return 0;
    size_t used = 0;
    long long deadline = milliseconds() + 4000;
    while (milliseconds() < deadline) {
        int r = poll(&p, 1, 100);
        if (r < 0) { if (errno == EINTR) continue; return 0; }
        if (!r) continue;
        if (!(p.revents & POLLIN)) return 0;
        char packet[4096];
        ssize_t n = read(fd, packet, sizeof(packet));
        if (n <= 0 || used + (size_t)n >= RESPONSE_SIZE) return 0;
        memcpy(response + used, packet, (size_t)n);
        used += (size_t)n;
        response[used] = 0;
        int t = terminal(response);
        if (t) return t > 0;
    }
    return 0;
}

/* Publish only requested result prefixes; discard unrelated unsolicited data. */
static void filtered_lines(char *response, const char *prefix)
{
    char *save = NULL;
    int first = 1;
    putchar('[');
    for (char *line = strtok_r(response, "\r\n", &save); line;
         line = strtok_r(NULL, "\r\n", &save)) {
        if (strncmp(line, prefix, strlen(prefix))) continue;
        if (!first) putchar(',');
        first = 0;
        json_string(line);
    }
    putchar(']');
}

int main(int argc, char **argv)
{
    const struct query status[] = {
        { "serving", "AT+QENG=\"servingcell\"\r", "+QENG:" },
        { "carriers", "AT+QCAINFO\r", "+QCAINFO:" },
        { "rsrp", "AT+QRSRP\r", "+QRSRP:" },
        { "sinr", "AT+QSINR\r", "+QSINR:" },
    };
    const struct query capabilities[] = {
        { "capabilities", "AT+QNWLOCK=?\r", "+QNWLOCK:" },
        { "lte", "AT+QNWLOCK=\"common/4g\"\r", "+QNWLOCK:" },
        { "nr", "AT+QNWLOCK=\"common/5g\"\r", "+QNWLOCK:" },
    };
    const struct query *queries = NULL;
    size_t count = 0;
    char command[160];
    struct query setting = { "result", command, "+QNWLOCK:" };
    unsigned freq, pci, scs, band;
    if (argc == 2 && !strcmp(argv[1], "status")) {
        queries = status; count = sizeof(status) / sizeof(*status);
    } else if (argc == 2 && !strcmp(argv[1], "serving")) {
        /* Serving cell only: the radio LED helper polls this. */
        queries = status; count = 1;
    } else if (argc == 2 && !strcmp(argv[1], "capabilities")) {
        queries = capabilities; count = sizeof(capabilities) / sizeof(*capabilities);
    } else if (argc == 4 && !strcmp(argv[1], "lock-lte") &&
               number(argv[2], 0, 262143, &freq) && number(argv[3], 0, 503, &pci)) {
        snprintf(command, sizeof(command), "AT+QNWLOCK=\"common/4g\",1,%u,%u\r", freq, pci);
        queries = &setting; count = 1;
    } else if (argc == 6 && !strcmp(argv[1], "lock-nr") &&
               number(argv[2], 1, 1007, &pci) && number(argv[3], 0, 3279165, &freq) &&
               number(argv[4], 15, 120, &scs) && number(argv[5], 1, 1024, &band) &&
               (scs == 15 || scs == 30 || scs == 60 || scs == 120)) {
        snprintf(command, sizeof(command), "AT+QNWLOCK=\"common/5g\",%u,%u,%u,%u\r", pci, freq, scs, band);
        queries = &setting; count = 1;
    } else if (argc == 2 && (!strcmp(argv[1], "unlock-lte") || !strcmp(argv[1], "unlock-nr"))) {
        snprintf(command, sizeof(command), "AT+QNWLOCK=\"common/%s\",0\r",
                 !strcmp(argv[1], "unlock-lte") ? "4g" : "5g");
        queries = &setting; count = 1;
    } else return error_json("invalid-arguments");

    alarm(25);
    int lockfd = open("/var/lock/h5-at.lock", O_CREAT | O_RDWR | O_CLOEXEC, 0600);
    if (lockfd < 0) return error_json("lock-unavailable");
    if (flock(lockfd, LOCK_EX | LOCK_NB)) { close(lockfd); return error_json("busy"); }
    int fd = open("/dev/h5-at", O_RDWR | O_CLOEXEC);
    if (fd < 0) { close(lockfd); return error_json("port-unavailable"); }
    char response[RESPONSE_SIZE];
    int all_ok = 1;
    printf("{\"queries\":{");
    for (size_t i = 0; i < count; i++) {
        int ok = query(fd, queries[i].command, response);
        all_ok &= ok;
        if (i) putchar(',');
        json_string(queries[i].key);
        printf(":{\"ok\":%s,\"lines\":", ok ? "true" : "false");
        filtered_lines(response, queries[i].prefix);
        putchar('}');
    }
    printf("},\"ok\":%s}\n", all_ok ? "true" : "false");
    close(fd);
    close(lockfd);
    return !all_ok;
}
