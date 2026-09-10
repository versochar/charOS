# 13I - VFS Dokümantasyon

## Kullanıcı Kılavuzu
```c
vfs64_init();
u64 ino, fd, v;
vfs64_create(&ino);
vfs64_open(ino, &fd);
vfs64_write(fd, 0xBEEF);
vfs64_read(fd, &v);
vfs64_close(fd);
```

## Durum
Tamamlandı.
