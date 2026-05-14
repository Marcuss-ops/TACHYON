# RustStream Core Operations Map

Documento di riferimento per il porting in C++ delle operazioni presenti in `ruststream-core`.

Obiettivo:
- mappare cosa fa davvero il codice Rust, per zona funzionale;
- separare operazioni reali da validator, builder e placeholder;
- evidenziare cosa manca o è solo parzialmente implementato.

## 1. Visione generale

Il crate espone 7 aree principali:

- `core`
- `probe`
- `audio`
- `video`
- `filters`
- `io`
- `cli`
- `server` opzionale

La pipeline moderna è costruita attorno a:

- `MediaTimelinePlan` come input canonico del render
- `AudioGraphConfig` come contratto audio
- `RenderGraph` come contratto unificato
- `RenderResult` come output unificato

In pratica il codice fa 4 cose:

1. valida configurazioni e file
2. estrae metadata media
3. costruisce filter graph FFmpeg o esegue FFmpeg
4. produce metriche, drift, errori e cache

## 2. Core

### 2.1 `core/errors.rs`

Responsabilità:
- definisce gli errori strutturati del dominio media
- assegna codici macchina leggibili
- trasporta contesto di stage, path e fallback

Operazioni:
- creazione errore con `MediaError::new`
- annotazione stage con `with_stage`
- annotazione path con `with_path`
- marcatura fallback con `with_fallback`
- conversione in stringa
- produzione di `PipelineResult`

Codici principali:
- decode
- encode
- effects
- overlay
- audio
- timeline
- pipeline
- init
- I/O
- cache

### 2.2 `core/timeline.rs`

Responsabilità:
- rappresenta il piano temporale media unico
- tiene video track, effects, overlays, output e parametri di esecuzione

Operazioni:
- `Timebase`
  - costruzione timebase
  - conversione secondi <-> timestamp
  - validazione denominator/numerator
- `VideoSegment`
  - crea segmento
  - set start/duration/timeline_start
- `VideoTrack`
  - crea track
  - marca primary
  - aggiunge segmenti
  - valida presenza di almeno un segmento
- `EffectNode`
  - crea effetto
  - set time range
  - aggiunge parametri
- `OverlayTrack`
  - crea overlay
  - set posizione
  - set time range
  - clamp opacity
  - valida duration e start_time
- `OutputConfig`
  - path
  - width/height
  - fps
  - crf
  - codec video/audio
  - audio bitrate
  - sample rate
- `MediaTimelinePlan`
  - costruzione piano
  - add track/effect/overlay
  - set output, threads, SIMD, timebase
  - abilita emergency fallback
  - trova primary track
  - validazione globale

Validazioni reali:
- almeno un video track
- timebase valido
- output path presente
- ogni track ha segmenti
- overlay con duration > 0 e start >= 0

### 2.3 `core/render_graph/*`

Responsabilità:
- definire il contratto unificato input/output della pipeline
- orchestrare stadi di processing

`graph.rs`
- crea `RenderGraph`
- attacca audio opzionale
- set config
- abilita emergency mode
- set timeout
- valida graph ID
- valida timeline
- valida audio se presente

`config.rs`
- definisce `RenderMode`
- definisce `RenderConfig`
- campi:
  - mode
  - timeout_secs
  - emit_drift_metrics
  - emit_component_metrics

`result.rs`
- definisce `RenderResult`
- contiene:
  - success
  - artifact_path
  - metrics
  - drift
  - reason_codes
  - error_message
  - error_code
  - error_stage
  - fallback_used
  - fallback_components
- builder methods:
  - success
  - failure
  - with_metrics
  - with_drift
  - with_reason_code
  - with_fallback
  - is_clean_success

`metrics.rs`
- gestisce metriche per componente
- registra attempt/success/fallback
- calcola fallback components
- calcola stage sum

`process.rs`
- pipeline end-to-end:
  - validate
  - probe
  - decode
  - effects
  - overlay
  - audio
  - encode
- aggiorna metriche per stadio
- propaga reason code
- calcola drift finale
- costruisce `RenderResult`
- include fallback handling
- espone `run_audio_graph`

`stages.rs`
- `probe_sources`
  - verifica esistenza file video, overlay e audio
- `decode_sources`
  - controlla durata segmenti
- `apply_effects`
  - controlla effect_type non vuoto
  - controlla range temporale valido
- `apply_overlays`
  - controlla opacity in [0,1]
- `process_audio`
  - valida config audio
  - controlla drift massimo
  - ritorna risultato placeholder
- `encode_output`
  - valida width/height/fps/crf
  - ritorna output path

### 2.4 `core/audio_graph.rs`

Responsabilità:
- definire il contratto audio dichiarativo

Operazioni:
- `AudioInput`
  - new
  - with_volume
  - with_offset
  - with_gate
- `AudioGate`
  - new
- `SyncConfig`
  - default
  - frame duration accessor
- `AudioGraphConfig`
  - new
  - add_input
  - with_output_sample_rate
  - with_output_channels
  - with_total_duration_samples
  - with_sync
  - find_inputs_by_type
  - validate
- `AudioGraphResult`
  - success
  - failure

Validazioni reali:
- graph_id non vuoto
- almeno un input
- sample rate > 0
- canali 1 o 2
- ogni input ha path non vuoto

### 2.5 `core/instrumentation.rs`

Responsabilità:
- timing e profiling

Operazioni:
- `StageTimer`
  - start
  - stop e registrazione nel profiler
- `StageMetrics`
  - stage_sum
  - has_any
- `DriftMetrics`
  - new
  - is_acceptable
- `Profiler`
  - record_stage_time
  - record_bytes_processed
  - record_frames_processed
  - record_samples_processed
  - record_cpu_time
  - total_elapsed_ms
  - get_stage_time
  - generate_report

## 3. Probe

### 3.1 `probe/mod.rs`

Responsabilità:
- estrazione metadata media nativa

Operazioni:
- `probe_full`
  - inizializza FFmpeg
  - apre input media
  - legge metadata formato
  - calcola duration
  - legge bitrate
  - legge size file
  - itera stream
  - per video:
    - legge codec
    - trova decoder
    - estrae width/height
    - estrae FPS
  - per audio:
    - legge codec
    - trova decoder
    - estrae sample rate
    - estrae channels
    - stima bit depth
  - crea `FullMetadata`
- `probe_file`
  - controlla esistenza file
  - converte path
  - richiama `probe_full`

### 3.2 `probe/cache.rs`

Responsabilità:
- cache persistente dei metadata con LRU

Operazioni:
- apertura cache su disco
- apertura cache in-memory
- `get`
- `put`
- `get_or_probe`
- `clear`
- `stats`
- eviction size-based

Dettagli implementativi:
- storage redb
- serializzazione bincode
- indice LRU in memoria
- eviction fino a 80% del limite

## 4. Audio

### 4.1 `audio/hot_kernels.rs`

Responsabilità:
- mixing e gain ad alte prestazioni
- detection CPU features
- SIMD path selection
- buffer pool thread-local

Operazioni:
- `cpu_features`
- `audio_mix`
  - fast path output vuoto
  - fast path senza input
  - selezione AVX-512 / AVX2 / SSE4.1 / scalar
- `apply_volume`
  - fast path unity gain
  - fast path gain 0
  - selezione SIMD/scalar
- `BufferPool`
  - new
  - acquire
  - release
- scalar mixing:
  - mix + clip in un solo passaggio
  - parallel chunking con Rayon
- SIMD mixing:
  - FMA
  - clipping nell’ultimo input
  - tail handling
- `apply_gate`
  - zero-fill su intervallo

### 4.2 `audio/gate_utils.rs`

Responsabilità:
- costruzione espressioni gate FFmpeg

Operazioni:
- `build_gate_expr_from_ranges`
  - ritorna `1` o `0` per range vuoti
  - filtra range invalidi
  - genera `if(between(...),0,1)` o invertito
- `build_intro_only_gate_expr`
  - gate attivo solo per intro iniziale

### 4.3 `audio/audio_mix.rs`

Responsabilità:
- builder di filter string FFmpeg audio

Operazioni:
- `build_amix_filter`
  - gestisce offset iniziale con `atrim`
  - gestisce volume in dB
  - costruisce `amix`
- `build_acrossfade_filter`
- `build_background_music_filter`
  - trim main audio
  - fade in/out
  - loop background music
  - mix a due canali
- `build_audio_delay_filter`
- `build_audio_pitch_filter`
- `build_concat_audio_filter`

### 4.4 `audio/audio_bake.rs`

Responsabilità:
- baking audio reale tramite ffmpeg-next

Operazioni:
- `AudioBakeConfig`
  - base video
  - voiceover opzionale
  - music opzionale
  - output path
  - offset VO
  - gate ranges
  - music volume
  - sample rate
  - output AAC flag
- `build_audio_bake_filter`
  - base audio gate
  - VO delay
  - music loop
  - volume gating per frame
  - amix
  - limiter
  - fallback a `anullsrc` se non ci sono input
- `build_audio_bake_command`
  - costruisce comando ffmpeg
  - aggiunge input base/VO/music
  - `-stream_loop -1` per music
  - map video 0:v e audio filtrato
  - codec video copy
  - codec audio AAC se richiesto
- `bake_master_audio`
  - valida input
  - esegue comando
  - verifica output esistente e dimensione minima

### 4.5 `audio/audio_resample.rs`

Responsabilità:
- resampling audio con decode/resample/encode

Operazioni:
- rileva formato output
- apre input
- trova stream audio
- crea decoder
- normalizza channel layout vuoto
- colleziona packet audio
- crea output context
- `encode_wav`
  - encoder PCM_S16LE
  - resampler
  - frame writing
  - flush decoder
  - flush resampler
  - flush encoder
- `encode_aac`
  - encoder AAC
  - frame alignment
  - sample buffer con offset O(n)
  - drain encoder
  - gestione tail frame con padding
- `fix_channel_layout`
- `drain_encoder`

## 5. Video

### 5.1 `video/mod.rs`

Responsabilità:
- concatenazione video FFmpeg

Operazioni:
- `ConcatConfig`
  - inputs
  - output
  - codec
  - crf
- `concat_videos`
  - valida input non vuoti
  - controlla esistenza file
  - crea concat list temporanea
  - scrive `file '...'`
  - lancia ffmpeg concat demuxer
  - set thread count automatico
  - codec video
  - preset fast
  - crf
  - tune film
  - audio AAC
  - faststart
  - cleanup temp file

### 5.2 `video/clip_processing.rs`

Responsabilità:
- builder FFmpeg per clip centrale

Operazioni:
- `SoundEffectConfig`
- `ClipProcessingConfig`
  - input/output
  - width/height/fps
  - sound effects
  - subtitle SRT opzionale
  - font path opzionale
  - fade in/out
- `get_file_duration_sec`
- `build_clip_processing_filter`
  - scale
  - pad
  - fade in/out video
  - original audio
  - SFX input overlay
  - atrim
  - adelay
  - volume
  - amix
- `build_clip_processing_command`
  - valida input
  - probe durata
  - aggiunge input SFX esistenti
  - filter_complex
  - map video/audio
  - video H264 fast CRF 23
  - audio AAC
- `process_clip`
  - esegue ffmpeg
  - verifica output
  - verifica dimensione minima

### 5.3 `video/overlay_merge.rs`

Responsabilità:
- composizione overlay finale

Operazioni:
- `OverlayConfig`
- `MiddleClipTimestamp`
- `OverlayMergeConfig`
  - base video
  - overlays
  - output
  - middle clip timestamps
  - subtitles ASS opzionali
  - fonts dir opzionale
  - width/height/fps
- `escape_filter_path`
- `build_enable_condition`
  - enable temporale overlay
  - disabilita nelle finestre middle clip
- `build_overlay_filter_complex`
  - shift overlay con `setpts`
  - chain overlay sulla base
  - burn-in subtitles ASS opzionali
  - audio passthrough base
- `build_overlay_merge_command`
  - valida base video
  - filtra overlay validi
  - costruisce ffmpeg command
  - map video e audio
  - video libx264 fast
  - audio AAC 256k
  - avoid_negative_ts make_zero
- `merge_overlays`
  - esecuzione ffmpeg
  - verifica output esistente e dimensione minima

## 6. Filters

### `filters/mod.rs`

Responsabilità:
- builder di transizioni FFmpeg

Operazioni:
- `TransitionType`
- `build_transition`
  - fade
  - wipe left
  - wipe right
  - circle crop

## 7. I/O

### `io/sync_io.rs`

Responsabilità:
- I/O file sincrono base

Operazioni:
- `read_file_bytes`
- `write_file_bytes`

## 8. CLI

### `cli/mod.rs`

Responsabilità:
- comando riga di comando

Operazioni:
- `parse_args`
- `run_probe`
  - invoca probe
  - output JSON o testo
- `run_concat`
  - costruisce `ConcatConfig`
  - invoca concat_videos
- `run_benchmark`
  - benchmark audio_mix
  - stampa iterazioni, durata, samples/sec
- `show_info`
  - stampa version
  - stampa CPU cores
  - stampa SIMD
  - stampa feature server

## 9. Server

Stato:
- presente ma non implementato

Operazioni effettive:
- definizione `ServerConfig`
- default host/port
- `start_server` ritorna errore placeholder

## 10. Cosa manca o è solo parzialmente implementato

Questa è la parte importante per il porting.

### 10.1 Placeholder o stub

- `core/render_graph/stages.rs::process_audio`
  - non fa processing reale
  - ritorna solo `AudioGraphResult::success("", 0)`
- `core/render_graph/process.rs::run_audio_graph`
  - stesso livello di placeholder
- `server.rs`
  - server HTTP non implementato

### 10.2 Campi config non usati o usati solo in parte

- `OutputConfig.video_codec`
  - validato/trasportato ma non sempre usato nei builder
- `OutputConfig.audio_codec`
  - idem
- `OutputConfig.audio_bitrate`
  - idem
- `OutputConfig.sample_rate`
  - spesso non propagato nei path video
- `MediaTimelinePlan.allow_intermediate_files`
  - campo presente, non vedo logica runtime concreta
- `MediaTimelinePlan.emergency_fallback_enabled`
  - campo presente, ma non vedo una strategia completa di fallback runtime
- `RenderConfig.timeout_secs`
  - presente, ma non vedo enforcement nel processing mostrato
- `RenderConfig.emit_drift_metrics`
  - presente come contratto, ma non vedo branch di emissione nel pipeline shown
- `RenderConfig.emit_component_metrics`
  - idem
- `SimdLevel`
  - definito, ma non vedo uso decisivo nel routing
- `ThreadConfig`
  - definito, ma non vedo applicazione diretta nei punti FFmpeg mostrati

### 10.3 Funzionalità dichiarate ma non complete

- subtitle SRT in `clip_processing.rs`
  - campi presenti, ma il builder letto non li applica davvero
- font path in `clip_processing.rs`
  - campi presenti, ma non usati nel filter builder letto
- video effects reali
  - il render graph valida gli effect node, ma non vedo un motore effetti completo
- overlay blend modes
  - il model li contiene, ma il merge mostrato usa principalmente overlay standard + subtitles
- audio graph processing reale
  - i contratti ci sono, ma il path esecutivo completo sembra demandato ad `audio_bake`

### 10.4 Possibili gap architetturali per il porting C++

- non esiste un vero motore interno unico che realizzi tutti gli step media senza FFmpeg subprocess/bindings
- alcune aree sono builder di command/filter, non processing engine puro
- la pipeline unificata è forte sul contratto, debole sull’implementazione effettiva di effects/audio graph
- le metriche esistono, ma la raccolta completa lato esecuzione sembra parziale rispetto al modello

## 11. Priorità di porting in C++

Se devi portare il comportamento, l’ordine consigliato è:

1. `core/errors`
2. `core/timeline`
3. `probe`
4. `audio/gate_utils`
5. `audio/hot_kernels`
6. `audio/audio_mix`
7. `audio/audio_bake`
8. `video/mod`
9. `video/overlay_merge`
10. `video/clip_processing`
11. `core/render_graph`
12. `probe/cache`
13. `cli`
14. `server` solo se serve davvero

## 12. Sintesi pratica

Se vuoi portare in C++ il comportamento corrente, devi copiare soprattutto:

- validazioni input/output
- costruzione dei filter graph
- metadata probe
- mixing audio SIMD/scalar
- concatenazione e overlay video
- cache metadata
- error model e result model

Quello che invece non devi aspettarti di trovare già completo:

- HTTP server
- audio graph execution vera dentro la pipeline unificata
- video effects engine completo
- enforcement reale di tutti i campi config
