# Tachyon Build Guide

## Prerequisiti

- **Visual Studio 2022** (Community/Professional/Enterprise) con carico di lavoro "Sviluppo di applicazioni desktop con C++"
- **CMake 3.25+** richiesto per `MSVC_DEBUG_INFORMATION_FORMAT` / `CMP0141`
- **Ninja** (incluso in VS 2022 o installare separatamente)
- **sccache** (opzionale, velocizza le compilazioni)

Installare sccache:
```powershell
winget install Mozilla.sccache
```

## Build Rapida

Usa lo script `build.ps1` con preset CMake:

```powershell
# Build di sviluppo (raccomandato)
.\build.ps1 -RelWithDebInfo

# Build release ottimizzata
.\build.ps1 -Release

# Build debug
.\build.ps1 -Debug

# Clean build (mantiene la cache FetchContent)
.\build.ps1 -RelWithDebInfo -Clean

# Clean completo (build + dipendenze)
.\build.ps1 -RelWithDebInfo -CleanDeps

# Build e esegui test
.\build.ps1 -RelWithDebInfo -Test

# Mostra statistiche sccache
.\build.ps1 -RelWithDebInfo -ShowSccacheStats
```

## Preset CMake

Il progetto usa `CMakePresets.json` con configurazioni pre-definite:

- **debug**: Debug con simboli completi
- **release**: Release ottimizzata
- **relwithdebinfo**: Release con simboli di debug (predefinito)
- **asan**: AddressSanitizer per debug memoria
- **relwithdebinfo-unity**: Unity Build (Jumbo) per tempi ridotti del 30-50%
- **relwithdebinfo-lld**: Usa lld-link linker per link più veloce
- **relwithdebinfo-fast**: Combina Unity Build + lld-link

## sccache

sccache velocizza le compilazioni mettendo in cache i risultati. Dopo un clean, ricompila solo i file modificati.

Configurazione consigliata in `~/.config/sccache/config.toml`:
```toml
[cache]
max_size = "20GiB"
```

## FetchContent Cache

Le dipendenze (FreeType, HarfBuzz, ecc.) sono scaricate in `build-ninja/.fetchcontent`. Questa directory persiste anche dopo `--Clean`, evitando riscarichi.

## Unity Build (Jumbo)

Abilita la compilazione unificata per ridurre i tempi del 30-50%:

```powershell
.\build.ps1 -RelWithDebInfo -Unity
```

Oppure imposta in `CMakePresets.json`:
```json
"CMAKE_UNITY_BUILD": "ON"
```

## Linker veloce (lld-link)

Usa LLVM lld-link invece di link.exe per tempi di link ridotti:

```json
"CMAKE_LINKER": "C:/Program Files/LLVM/bin/lld-link.exe"
```

## Struttura Directory

- `.cache/fetchcontent/` - Cache persistente dipendenze FetchContent (gitignore)
- `build-ninja/` - Directory build Ninja (gitignore)
- `src/` - Codice sorgente TachyonCore, tachyon
- `tests/` - Test TachyonTests

## Troubleshooting

### Errore PDB (C1041)
Se vedi errori di conflitto PDB, verifica che il formato debug sia `Embedded` (/Z7):
```powershell
findstr /i "MSVC_DEBUG_INFORMATION_FORMAT" build-ninja/CMakeCache.txt
```
Dovrebbe mostrare: `CMAKE_MSVC_DEBUG_INFORMATION_FORMAT:STRING=$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>`

### sccache non trovato
Assicurati che sccache sia nel PATH o installato via winget. Lo script build.ps1 lo cerca automaticamente.

### Build fallita
Pulisci e ricompila:
```powershell
.\build.ps1 -RelWithDebInfo -Clean
```

## Comandi diretti (senza build.ps1)

Configura con preset:
```powershell
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
cmake --preset relwithdebinfo
cmake --build --preset relwithdebinfo
```

## CI/CD

Il workflow GitHub Actions (`.github/workflows/ci.yml`) usa sccache e cache FetchContent per build veloci.
