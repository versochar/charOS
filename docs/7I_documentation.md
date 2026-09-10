# 7I - IPC Dokümantasyon

## Kullanıcı Kılavuzu
```c
ipc64_init();
u64 ch;
ipc64_create(&ch);
ipc64_send(ch, 0xDEAD);
u64 msg;
ipc64_recv(ch, &msg);
ipc64_destroy(ch);
```

## API Referansı
`ipc64_init`, `ipc64_create`, `ipc64_destroy`, `ipc64_send`, `ipc64_recv`.

## Not
Host-test davranış modelidir; gerçek çoklu-mesaj kuyruğu `ipc/` üzerinden yapılacaktır.

## Durum
Tamamlandı.
