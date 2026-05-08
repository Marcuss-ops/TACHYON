---
Status: Draft
Last reviewed: 2026-05-06
Owner: Core Team
Supersedes: N/A
---

 2. Definisci la fonte di verità  - Un tipo, una struttura, una
  semantica devono vivere in un solo posto.  - Se hai alias legacy, dillo chiaramente: usa  shared_contracts.h come source of truth.  - Molti bug nascono da
  duplicati tra include/ e src/, o tra namespace diversi.  3. Non fidarti del summary senza build    Un agente può dire “fatto” anche quando il codice compila solo
  in parte.  - Regola pratica: ogni blocco importante deve finire con:      - build      - test  - controllo warning  - Nel tuo repo i warning sono spesso errori,
  quindi anche un dettaglio piccolo rompe tutto.  4. Evita API inventate  - Quando chiedi una  modifica, l’agente deve usare i nomi già esistenti nel repo.  - Se
  non è sicuro, deve cercare prima nel codice, non inventare campi o file.  - Questo vale  soprattutto per:      - struct      - enum      - signature di funzioni
  - path dei file  5. Cambia una cosa alla volta  - Meglio 3 patch piccole che una  mega-refactor.  - Dopo ogni blocco:      - compila      - correggi      - poi
  continui  - Se fai tutto insieme, gli errori si moltiplicano e non capisci più cosa  li ha generati.  6. Proteggi la compatibilità  - Se stai sostituendo un
  contratto, mantieni wrapper o alias finché tutto il resto non è migrato.  - Rimuovere  subito i vecchi tipi spesso rompe parser, serializer e runtime insieme.  -
  Prima compatibilità, poi pulizia.  7. Aggiorna codice e docs insieme  - Se cambia un  contratto, aggiorna anche:      - parser      - compiler      -
  serialization      - docs  - I tuoi errori recenti sono stati spesso da “documentato come fatto”  ma non ancora chiuso end-to-end.  8. Usa checklist tecniche per
  l’agente  Prima di considerare un task chiuso, chiedi esplicitamente:  - il codice compila?  - ci  sono warning?  - i test passano?  - sono stati aggiornati
  tutti i call site?  - ci sono alias o duplicati residui?  9. Fai scrivere all’agente meno, ma più  preciso  - Meglio: “aggiungi line_index in TextAnimatorContext
  e aggiorna i call site”  - Peggio: “sistemi il text system”  - Più il prompt è preciso, meno  l’agente improvvisa.  10. Tratta il build come verità, non il prose
  - Il codice decide.  - Il summary dell’agente è utile solo se confermato da build/test.  -  Se il build fallisce, il lavoro non è finito anche se la descrizione
  suona bene.  La regola più importante  - Ogni agente deve essere visto come un collaboratore  veloce, non come una fonte di verità.  - Tu devi imporre:      -
  confini      - validazione      - ordine di lavoro      - single source of truth 