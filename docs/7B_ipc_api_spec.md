# 7B - IPC API Spesifikasyonu

## API
```c
int ipc64_init(void);
int ipc64_create(u64 *out_chan);
int ipc64_destroy(u64 chan);
int ipc64_send(u64 chan, u64 msg);
int ipc64_recv(u64 chan, u64 *out_msg);
```

## Sözleşme
- `create`: out NULL ise -1, tablo doluysa -1, başarıda 0 + yeni chan.
- `destroy`: bilinmeyen chan -1.
- `send`: bilinmeyen chan -1, mailbox doluysa -1.
- `recv`: bilinmeyen chan -1, mailbox boşsa -1, out NULL ise -1.

## Uyumluluk
Mevcut IPC çekirdeği değişmez; yeni API opsiyonel uzantıdır.

## Sonraki Adım
7C implementasyon başlatma.
