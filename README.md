# charOS

Bare metal performans odaklı profesyonel kernel.

## Hakkında
charOS, x86_64 üzerinde çalışan, modüler, güvenli ve sertifikasyon odaklı bir hobby-to-professional kernel projesidir. 31A-60J serisi ile temel çekirdek, Flatpak sandbox, dağıtım ve sürümleme altyapısı tamamlanmıştır. 61-100J serisi ile profesyonel özellikler, QA, sertifikasyon ve ekosistem planlanmaktadır.

## Yol Haritası
- **31A-60J**: Temel çekirdek, oyun modülleri, Flatpak sandbox, dağıtım, sürümleme
- **61-100J**: Güvenlik, sertifikasyon, kurumsal özellikler, LTS, v1.0

Detaylı 1000 aşamalı plan: `docs/CHAROS_1000_ASAMA_PLAN.txt`
Kısa yol haritası: `CHAROS_ROADMAP_31A-60J.txt`

## Yapı
```
kernel/arch/x86_64/   # Kernel modülleri
include/arch/x86_64/  # Prototipler longmode.h
tests/host/           # Host testleri test_de64.c
build/                # Derleme çıktıları
docs/                 # Yol haritası ve dokümantasyon
```

## Derleme
```bash
cd charOS
make all
# ISO: build/charos.iso
# Test: make test-de64
```

Test sonucu: `make test-de64` → `SONUC: TUMU PASS`

## VirtualBox
ISO ile çalıştırma:
```bash
./vbox-create.sh
VBoxManage startvm "charOS-Live" --type headless
```
Ayarlar: 2 CPU, 2048 MB RAM, UEFI, SATA HDD 20 GB, DVD'ye `build/charos.iso`

## Katkı
Profesyonel kernel hedefi için 1000 aşamalı plan doks'a eklidir. Her aşama tasarım → API → uygulama → test → inceleme → güvenlik → dokümantasyon → sürüm döngüsünü takip eder.

## Lisans
GPL-3.0
