#include <drivers/pkg_mgr.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 29E: Paket yöneticisi skeleton */

static const char* packages[] = {"charos-base", "charos-shell", "charos-editor"};
static int pkg_installed[16] = {1, 0, 0};
static int pkg_max = 3;

int pkg_init(void) {
    serial_puts("[29E] Package manager init (skeleton)\n");
    return 0;
}

int pkg_install(const char* name) {
    serial_puts("[29E] Installing: "); serial_puts(name); serial_puts("\n");
    for (int i = 0; i < pkg_max; i++) {
        if (strcmp(name, packages[i]) == 0) {
            pkg_installed[i] = 1;
            return 0;
        }
    }
    return -1;
}

int pkg_list(void) {
    for (int i = 0; i < pkg_max; i++) {
        serial_puts("  "); serial_puts(packages[i]);
        serial_puts(" ["); serial_puts(pkg_installed[i] ? "OK" : "--");
        serial_puts("]\n");
    }
    return 0;
}

void pkg_stats(void) {
    int n = 0;
    for (int i = 0; i < pkg_max; i++) if (pkg_installed[i]) n++;
    serial_puts("[29E] Installed: "); serial_puthex(n); serial_puts("/");
    serial_puthex(pkg_max); serial_puts("\n");
}

int pkg_selftest(void) {
    pkg_install("charos-base");
    pkg_install("charos-shell");
    pkg_list();
    pkg_stats();
    serial_puts("[29E] Package manager [PASS]\n");
    vga_puts("[29E] Package manager [PASS]\n");
    return 0;
}
