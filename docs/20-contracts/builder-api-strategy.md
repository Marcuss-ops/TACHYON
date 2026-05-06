---
Status: Canonical
Last reviewed: 2026-05-06
Owner: Core Team
Supersedes: N/A
---

# Builder API Strategy

## Panoramica

Il repository e l'architettura di Tachyon sono passati da un approccio **JSON-first** (ereditato da workflow legacy e web-based) a un approccio **C++ Builder-first**. 

Questo documento stabilisce le regole per l'interazione pubblica con l'engine.

## La Fonte di Verità: C++ Builder

- Le API C++ fornite nei builder (`src/presets/`) sono l'unico punto di ingresso supportato per generare contenuti complessi.
- La composizione delle scene e dei layer deve avvenire tramite struct tipizzate (`XParams`) che vengono passate ai builder.
- I builder restituiscono le struct intermedie e finali (come `LayerSpec` o `SceneSpec`) garantendo validazione type-safe a compile-time e runtime.

## Il Ruolo di JSON

La serializzazione e deserializzazione in JSON (`SceneSpec::to_json()`, `SceneSpec::from_json()`) esiste ancora ma con scopi ben definiti e limitati:

1. **Debugging & Fixtures**: Salvare lo stato intermedio della scena generata dai builder per facilitare i test di regressione (es. "builder-generated JSON compatibility tests").
2. **Caching**: Ottimizzare i tempi di caricamento per le pipeline offline in cui le scene sono statiche tra un run e l'altro.
3. **Legacy Compatibility**: Permettere a tool esterni pre-esistenti di continuare a inviare scene. 

### Regole per il JSON

- **Deprecation Plan**: Non si devono sviluppare nuove feature *esclusivamente* basate su costrutti JSON astratti (es. parametri passati come dizionari string-to-any). Le definizioni dei layer si basano sui builder C++.
- **Support Level**: Il JSON Engine mantiene compatibilità con le proprietà di base e l'architettura dei layer. I file JSON *scritti a mano* in `/scenes` non sono considerati file sorgente validi per lo sviluppo; usare sempre i test C++ per generare le scene.

## Action Items

- Mantenere la parità tra `Builder -> SceneSpec` e `SceneSpec <-> JSON`. Qualsiasi campo introdotto nei builder C++ *deve* poter essere serializzato.
- Rimuovere gradualmente la validazione legacy nei parser JSON a favore di un C++ builder forte che produca solo strutture valide.
