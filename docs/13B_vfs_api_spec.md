# 13B - VFS API Spesifikasyonu

## API
```c
int vfs64_init(void);
int vfs64_create(u64 *out_ino);
int vfs64_unlink(u64 ino);
int vfs64_open(u64 ino, u64 *out_fd);
int vfs64_close(u64 fd);
int vfs64_write(u64 fd, u64 val);
int vfs64_read(u64 fd, u64 *out_val);
```

## Sözleşme
- `create`: NULL out -1, tablo dolu -1.
- `unlink`: bilinmeyen ino -1, açık fd varsa -2.
- `open`: bilinmeyen ino -1, NULL out -1, fd tablosu dolu -1.
- `close`: bilinmeyen fd -1.
- `write/read`: bilinmeyen fd -1, read NULL out -1.

## Sonraki Adım
13C implementasyon başlatma.
