# Just An Amp

VST3 de **head de bajo** hecho con JUCE 8 (misma base que JustAHead), pensado para ir **antes de `IRScene`**. No incluye cabina: la salida es line level para que el loader de IRs haga ese trabajo.

## Cadena

```
In -> Gate -> Compresor (Opto / FET) -> Pedal (SansAmp / Darkglass) -> Head -> [Chorus, Flanger, Phaser, Octaver, Env Filter, Delay, Reverb en el orden que elijas] -> Out
```

## Heads (DSP "inspirado", no capturas de hardware)

Son seis. Los dos ultimos son rigs dobles (amp limpio + amp sucio en paralelo, con Dirty Mix); el pedal entra solo al lado sucio.

- **Pulga** - estado solido, rapido y brillante. Gain, Bass, Lo Mid, Hi Mid, Treble, Contour, Boost.
- **NotFieldy** - preamp de valvulas muy agresivo. Gain, Bass, Mid, Treble, Bright y ecualizador grafico de 7 bandas.
- **RoAguMiSSTruVe** - todo valvulas con potencia de mucho sag. Gain, Bass, Mid + selector de frecuencia, Treble, Ultra Lo, Ultra Hi.
- **Ryandemar** - preamp valvular y potencia firme. Igual que el anterior con Ultra Lo / Ultra Hi continuos.
- **Canciller** - dos amps de estado solido en paralelo (limpio y sucio), estilo de rig de Justin Chancellor.
- **TimmyC** - SVT limpio mas SVT empujado a la distorsion, estilo de rig de Tim Commerford.

## Efectos

- **Compresores**: Opto (suave, RMS, release que depende de la reduccion) y FET (rapido, ratio ajustable), con Blend paralelo.
- **Pedales**: estilo SansAmp (preamp con Blend) y estilo Darkglass (distorsion en paralelo que conserva los graves).
- **Efectos independientes, cada uno con su ON/OFF** (se pueden usar varios a la vez): Chorus y Flanger (solo sobre los agudos), Phaser, Octaver al estilo OC-2 (OCT -1, OCT +1, TONE, DIRECT) y filtro de envolvente.
- **Delay** estereo ping-pong con TAP y **Reverb** FDN de 8 lineas, tambien con ON/OFF. Los 7 efectos se arrastran por el encabezado para cambiar el orden de la cadena.
- Gate de ruido, medidores de entrada y salida, indicador de reduccion del compresor, 21 presets de fabrica, interfaz escalable.

Compilacion: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` (descarga JUCE 8.0.12) y `cmake --build build --config Release`. GitHub Actions compila el VST3 de Windows en cada push.
