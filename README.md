# Just An Amp

VST3 de **head de bajo** hecho con JUCE 8 (misma base que JustAHead), pensado para ir **antes de `IRScene`**. No incluye cabina: la salida es line level para que el loader de IRs haga ese trabajo.

## Cadena

```
In -> Gate -> Compresor (Opto / FET) -> Pedal (SansAmp / Darkglass) -> Head -> [Mod, Delay, Reverb en el orden que elijas] -> Out
```

## Heads (DSP "inspirado", no capturas de hardware)

- **Pulga** - estado solido, rapido y brillante. Gain, Bass, Lo Mid, Hi Mid, Treble, Contour, Boost.
- **NotFieldy** - preamp de valvulas muy agresivo. Gain, Bass, Mid, Treble, Bright y ecualizador grafico de 7 bandas.
- **RoAguMiSSTruVe** - todo valvulas con potencia de mucho sag. Gain, Bass, Mid + selector de frecuencia, Treble, Ultra Lo, Ultra Hi.
- **Ryandemar** - preamp valvular y potencia firme. Igual que el anterior con Ultra Lo / Ultra Hi continuos.

## Efectos

- **Compresores**: Opto (suave, RMS, release que depende de la reduccion) y FET (rapido, ratio ajustable), con Blend paralelo.
- **Pedales**: estilo SansAmp (preamp con Blend) y estilo Darkglass (distorsion en paralelo que conserva los graves).
- **Mod**: Chorus y Flanger (solo sobre agudos), Phaser, Octaver (-1 y -2 octavas) y filtro de envolvente.
- **Delay** estereo ping-pong con TAP y **Reverb** FDN de 8 lineas; Mod, Delay y Reverb se arrastran para cambiar el orden.
- Gate de ruido, 15 presets de fabrica, interfaz escalable.

Compilacion: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` (descarga JUCE 8.0.12) y `cmake --build build --config Release`. GitHub Actions compila el VST3 de Windows en cada push.
