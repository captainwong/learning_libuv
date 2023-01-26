#include <stdio.h>
#include "G:/dev_libs/uv.h"

int main() {
    char buf[512];
    uv_interface_address_t* info;
    int count, i;

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    uv_interface_addresses(&info, &count);
    i = count;

    printf("Number of interfaces: %d\n", count);
    while (i--) {
        uv_interface_address_t interface_a = info[i];

        printf("Name: %s\n", interface_a.name);
        printf("Internal? %s\n", interface_a.is_internal ? "Yes" : "No");

        if (interface_a.address.address4.sin_family == AF_INET) {
            uv_ip4_name(&interface_a.address.address4, buf, sizeof(buf));
            printf("IPv4 address: %s\n", buf);
        } else if (interface_a.address.address4.sin_family == AF_INET6) {
            uv_ip6_name(&interface_a.address.address6, buf, sizeof(buf));
            printf("IPv6 address: %s\n", buf);
        }

        printf("\n");
    }

    uv_free_interface_addresses(info, count);
    return 0;
}
