# 16I - Virtio Dokümantasyon

## Kullanıcı Kılavuzu
```c
virt64_init();
u64 d;
virt64_add(1, &d);
virt64_kick(d, 0);
u64 p;
virt64_poll(d, 0, &p);
virt64_ack(d, 0);
```

## Durum
Tamamlandı.
