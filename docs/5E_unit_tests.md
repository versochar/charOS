# 5E - Process Unit Testleri

## Kapsam
- Init, spawn parametre hataları (NULL out).
- Exit bilinmeyen PID, double-exit reddi.
- Wait READY'de ret, ZOMBIE'de başarı.
- State bilinmeyen PID reddi.

## Senaryolar
5E2 init, 5E3 spawn param, 5E4 exit/wait zinciri, 5E5 state.

## Sonuç
PASS.
