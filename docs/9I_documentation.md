# 9I - Drivers & HAL Dokümantasyon

## Kullanıcı Kılavuzu
```c
drvhal64_init();
u64 h;
drvhal64_register(1, &h);
drvhal64_ioctl(h, 0x01, 0);
drvhal64_unregister(h);
```

## API Referansı
`drvhal64_init`, `drvhal64_register/unregister/ioctl/state`.

## Not
Host-test davranış modelidir; gerçek MMIO sürücüler `drivers/` üzerinden yapılacaktır.

## Durum
Tamamlandı.
