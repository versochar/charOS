# 49B - PXE & Network Boot API Spec

## API
```c
int nbp64_init(void);
int nbp64_set_server(u32 ip, u32 tftp_ip);
int nbp64_set_bootfile(const char *path);
int nbp64_discover(void);
int nbp64_poll(void);
int nbp64_boot(void);
int nbp64_reset(void);
int nbp64_state(int *out);
int nbp64_attempts(void);
```

## Donus Kurallari
- set_server: sifir adres -1. discover: sunucu yok -1, yanlis durum -2.
- poll: bootfile yoksa 1; deneme tukendi -2; DHCP->TFTP gecisi 0.
- boot: hazir degil -1, TFTP degil -2.
