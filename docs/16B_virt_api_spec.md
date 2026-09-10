# 16B - Virtio API Spesifikasyonu

## API
```c
int virt64_init(void);
int virt64_add(int type, u64 *out_dev);
int virt64_del(u64 dev);
int virt64_kick(u64 dev, u64 q);
int virt64_poll(u64 dev, u64 q, u64 *out_pending);
int virt64_ack(u64 dev, u64 q);
```

## Sözleşme
- `add`: tip 0-2 dışında -1, NULL out -1.
- `kick/poll/ack`: bilinmeyen dev -1, q>=4 -1, poll NULL out -1.
- `ack`: pending==0 ise -1.

## Sonraki Adım
16C implementasyon başlatma.
