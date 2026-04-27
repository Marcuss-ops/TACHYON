# Tachyon Modern Backgrounds API

Questo sistema permette di generare background moderni in modo modulare e riutilizzabile.

## Struttura C++ (The Engine)
La logica è stata rifattorizzata in [layer_renderer_procedural.cpp](file:///c:/Users/pater/Pyt/Tachyon/src/renderer2d/evaluated_composition/rendering/layer_renderer_procedural.cpp) per isolare i generatori di pattern.

### Pattern Disponibili:
- `noise`: Perlin noise standard per grane o movimenti fluidi.
- `aura` [NEW]: Rumore multi-ottava per mesh gradients organici.
- `waves`: Onde sinusoidali animate.
- `stripes`: Pattern a strisce procedurale.

## Utilizzo Modulare (The API)
I background sono definiti come **Componenti** nella libreria [backgrounds.json](file:///c:/Users/pater/Pyt/Tachyon/studio/library/system/backgrounds.json).

### Come "chiamare" un background in una scena:
Per usare un background "Aura Modern" in una nuova composizione, basta aggiungere un `component_instance` nel JSON della scena:

```json
{
  "component_instances": [
    {
      "component_id": "bg_aura_modern",
      "instance_id": "main_background",
      "param_values": {
        "color_primary": "#FF5500",
        "color_secondary": "#0055FF",
        "speed": "0.8"
      }
    }
  ]
}
```

### Vantaggi del sistema modulare:
1. **Clean Code**: La scena non deve definire tutti i parametri del noise, ma solo i colori e la velocità desiderati.
2. **Riutilizzabilità**: Se aggiorni il codice C++ dell'aura, tutti i background basati su quel componente si aggiorneranno automaticamente.
3. **Callable**: Funziona come una funzione; passi gli input e ottieni il risultato visuale.
