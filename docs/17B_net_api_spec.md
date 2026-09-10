# 17B - Network API Spesifikasyonu

## API
```c
int net64_init(void);
int net64_if_add(int type, u64 *out_if);
int net64_if_del(u64 iface);
int net64_send(u64 iface, u64 val);
int net64_recv(u64 iface, u64 *out_val);
int net64_stat(u64 iface, u64 *out_tx, u64 *out_rx);
```

## Sözleşme
- `if_add`: tip 0-1 dışında -1, NULL out -1.
- `send`: bilinmeyen iface -1, mailbox doluysa -1.
- `recv`: bilinmeyen iface -1, mailbox boşsa -1, NULL out -1.
- `stat`: bilinmeyen iface veya NULL out -1.

## Sonraki Adım
17C implementasyon başlatma.
