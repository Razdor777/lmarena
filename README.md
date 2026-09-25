# have fun <3

game version: 1.21.44 | cmake recommended

## Сборка

```bat
dev.bat            :: быстрая сборка для итераций (build-dev, без /GL + /LTCG)
dev.bat release    :: релизная сборка (build)
```

Почему это в разы быстрее, чем `cmake -S . -B build` на каждую правку, и как настроить
окружение (Defender, SSD, диагностика MSBuild-логом) — в [FAST_BUILD.md](FAST_BUILD.md).
