# 42B - Repository Management API Spec

## API
```c
int repo64_add(const char *name, const char *ver, const char *repo);
int repo64_find(const char *name, const char *ver_req, char *ver_out, int max);
int repo64_vercmp(const char *a, const char *b);
int repo64_count(void);
```

## Semantik
- `repo64_add`: exact (name+ver+repo) duplicate -2; NULL -1; tablo dolu -3.
- `repo64_find`: ver_req NULL/bos ise en yeni; ver_req varsa ondan dusuk olmayanlar arasinda en yeni. Bulunamazsa -2.
- `repo64_vercmp`: a<b -1, a>b 1, esit 0 (NULL icin 0).
