# 17I - Network Dokümantasyon

## Kullanıcı Kılavuzu
```c
net64_init();
u64 nic;
net64_if_add(1, &nic);
net64_send(nic, 0xCAFE);
u64 v;
net64_recv(nic, &v);
```

## Durum
Tamamlandı.
