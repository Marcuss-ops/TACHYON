# TACHYON ENGINEERING RULES

Questo file contiene le regole che ogni ingegnere deve leggere e rispettare prima di scrivere o modificare codice in Tachyon.

Se una regola sembra troppo costosa, va cambiato il design, non aggirata la regola.

## 1. Regole di base

- Prima di cambiare codice, capisci il contratto del modulo.
- Ogni funzione deve avere uno scopo singolo.
- Ogni tipo deve avere una sola definizione canonica.
- Ogni dipendenza deve essere esplicita.
- Ogni stato globale va evitato.
- Ogni assunzione implicita va trasformata in codice o test.
- Ogni bug trovato deve diventare un test di regressione.

## 2. Memoria e ownership

- Usa stack per oggetti piccoli e transitori.
- Usa `unique_ptr` per ownership esclusiva.
- Usa `shared_ptr` solo se la condivisione e il lifetime condiviso sono davvero necessari.
- Non creare oggetti grandi sullo stack se possono crescere oltre una soglia ragionevole.
- Non allocare nel hot path se puoi evitarlo.
- Non nascondere l'ownership dentro puntatori grezzi.
- Ogni lifetime non ovvio va reso esplicito nel nome, nel tipo o nel commento.

## 3. Runtime e performance

- Il render loop deve essere deterministico.
- Il render loop non deve fare allocazioni inutili.
- Il render loop non deve fare I/O non necessario.
- Il render loop non deve dipendere da stato globale.
- Il caching deve usare chiavi esplicite e stabili.
- Se il cache hit dipende da un campo, quel campo deve entrare nella chiave.
- Se un dato cambia il risultato, deve cambiare anche l'invalidazione.

## 4. Grafo, dipendenze e validazione

- Ogni nodo importante deve esistere esplicitamente nel grafo.
- Un nodo root senza edge non è un caso speciale da ignorare.
- Il topo order deve includere tutti i nodi rilevanti.
- I riferimenti mancanti devono fallire in validazione, non in runtime.
- I cicli devono essere rilevati il prima possibile.
- I duplicati di ID devono fallire.
- Le relazioni padre/figlio, matte e precomp devono essere validate.

## 5. Determinismo

- A parità di input e versione, il risultato deve essere identico.
- Ogni nuovo campo che influenza il risultato deve essere parte del contratto di determinismo.
- Non introdurre randomness non controllata.
- Non basarti sull'ordine non garantito di container o filesystem.
- Se il comportamento cambia per piattaforma, deve essere documentato e testato.

## 6. Error handling

- Fallire presto è meglio che fallire tardi.
- Non silenziare errori importanti.
- Non trasformare errori strutturali in output vuoto.
- Ogni fallback deve essere intenzionale e motivato.
- Se un fallback può nascondere un bug, va accompagnato da un test.

## 7. API e naming

- I nomi devono descrivere il contratto, non l'implementazione temporanea.
- Evita abbreviazioni ambigue.
- Evita funzioni che fanno due cose diverse.
- Evita parametri con significato implicito.
- Se un parametro serve solo in certi casi, il tipo o la struttura devono dirlo chiaramente.

## 8. Strategia di debug e build

- Il profilo di debug quotidiano è `RelWithDebInfo`, non `Debug` puro.
- `build.ps1` va usato per partire dal preset più piccolo che riproduce il bug, non per lanciare la full build del repository.
- `dev-fast` è il primo tentativo quando stai iterando su un fix; `dev` è il controllo più ampio; `asan` si usa per memoria, lifetime e UB sospetta.
- Ogni bug deve avere un comando di riproduzione esplicito con preset, target e filtro test.
- Se il bug non si riproduce con un target o un filtro piccolo, restringi ancora prima di cambiare codice.
- Una build completa del repository è una verifica finale, non il punto di partenza.

## 9. Validazione del codice

- Prima di consegnare una modifica, esegui la build del target toccato.
- Se tocchi runtime o memoria, esegui anche i test interessati.
- Se tocchi caching, esegui i test di cache e di determinismo.
- Se tocchi scene spec o grafo, esegui i test di validazione e compilazione.
- Se tocchi renderer, esegui i test di output o golden fixtures.
- Non considerare una modifica finita finché il caso che hai rotto non è coperto da un test.

## 10. Cosa evitare

- Doppie definizioni di classi o concetti.
- Wrapper temporanei che restano nel codice troppo a lungo.
- Log di debug lasciati in produzione.
- Hack che aggirano un crash senza capire la causa.
- Correzioni che migliorano il sintomo ma lasciano intatto il difetto strutturale.
- Cambi di contratto non documentati.

## 11. Regola finale

Se una modifica richiede:

- più di una definizione dello stesso concetto,
- una cache key incompleta,
- un nodo implicito nel grafo,
- o un fallback che nasconde l'errore,

allora la soluzione non è pronta.

La soluzione giusta deve essere semplice da spiegare, semplice da testare e difficile da rompere.
