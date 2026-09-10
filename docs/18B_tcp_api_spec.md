# 18B - TCP API Spesifikasyonu

## API
```c
int tcp64_init(void);
int tcp64_socket(u64 *out_sock);
int tcp64_close(u64 sock);
int tcp64_bind(u64 sock, u64 addr, u64 port);
int tcp64_connect(u64 sock, u64 addr, u64 port);
int tcp64_send(u64 sock, u64 val);
int tcp64_recv(u64 sock, u64 *out_val);
```

## Sözleşme
- `socket`: NULL out -1, tablo dolu -1.
- `bind`: bilinmeyen sock -1, CREATED değilse -2.
- `connect`: BOUND değilse -2, bilinmeyen -1.
- `send`: CONNECTED değilse -2, mailbox doluysa -1.
- `recv`: CONNECTED değilse -2, mailbox boşsa -1, NULL out -1.

## Sonraki Adım
18C implementasyon başlatma.
