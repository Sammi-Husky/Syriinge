#pragma once

struct in_addr {
    unsigned long s_addr; // load with inet_aton()
};

struct sockaddr_in {
    short sin_family;        // e.g. AF_INET
    unsigned short sin_port; // e.g. htons(3490)
    struct in_addr sin_addr; // see struct in_addr, below
};

namespace NetLog {
    int Init();
    void InstallHooks();
}