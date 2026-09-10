# 18I - TCP Dokümantasyon

## Kullanıcı Kılavuzu
```c
tcp64_init();
u64 s;
tcp64_socket(&s);
tcp64_bind(s, 0x7F000001, 8080);
tcp64_connect(s, 0x7F000001, 9090);
tcp64_send(s, 0x1);
u64 v;
tcp64_recv(s, &v);
```

## Durum
Tamamlandı.
