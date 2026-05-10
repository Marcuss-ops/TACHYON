# Tachyon — Regole architetturali

---
Status: Canonical
Last reviewed: 2026-05-06
Owner: Core Team
Supersedes: N/A
---

## Filosofia

Tachyon è un motore C++ headless per motion graphics deterministico, analogo ad After Effects
senza UI. L'obiettivo è produrre video di qualità professionale su CPU a bassa fascia.

## Architecture & Design Rules

Le regole architetturali di dettaglio, come il pattern dei Presets, domini approvati, pattern da evitare (es. NON usare JSON-first workflows come sorgente primaria), sono ora centralizzate e mantenute come contratti canonici nella documentazione:

👉 **[Presets Contract & Checklist](docs/10-architecture/presets-contract.md)**
👉 **[Builder API Strategy](docs/20-contracts/builder-api-strategy.md)**

Consultare i documenti linkati per i pattern esatti prima di introdurre nuove componenti.
