# TUXIRO REPEAT

Generador de risers (VST3) para Windows y macOS, pensado para Ableton Live.
Cumple la misma función que KAMINO TENSION: un riser que acelera sus repeticiones
(de 1/4 a 1/64, por ejemplo) con capas de pitch, ruido y reverb, y un silencio final antes del drop.

## Compilar en GitHub (automático)

1. Crea el repositorio y sube el proyecto:

   ```bash
   cd TUXIRO-REPEAT
   git init -b main
   git add .
   git commit -m "TUXIRO REPEAT v1.0"
   gh repo create TUXIRO-REPEAT --private --source=. --push
   ```
   (sin `gh`: crea un repo vacío en github.com y usa `git remote add origin ...` + `git push -u origin main`)

2. Cada `push` lanza **Actions → Build TUXIRO REPEAT (VST3)** y compila Windows y macOS.
   Descarga los `.zip` desde la pestaña **Actions → (última ejecución) → Artifacts**.
3. Para publicar una versión descargable: `git tag v1.0.0 && git push origin v1.0.0`
   (los zips se adjuntan solos en **Releases**).

## Instalar

| Sistema | Carpeta |
|---|---|
| Windows | `C:\Program Files\Common Files\VST3\` |
| macOS | `/Library/Audio/Plug-Ins/VST3/` (o `~/Library/Audio/Plug-Ins/VST3/`) |

En macOS, si Gatekeeper lo bloquea: `xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/TUXIRO REPEAT.vst3"`

En Ableton: Preferencias → Plug-ins → activar VST3 y *Rescan*.

## Uso en Ableton

Es un **instrumento** (pista MIDI). Modos de disparo (`TRIGGER`):

- **ONE-SHOT** (por defecto): una nota MIDI lanza el riser completo; la nota transpone el tono (C3 = original).
- **GATE**: igual, pero al soltar la nota el riser se corta con un fade corto.
- **HOST**: sigue el transporte de Ableton y se repite cada `LENGTH` compases, alineado al compás.

El botón **PLAY** previsualiza sin necesitar el transporte. **DRAG AUDIO**: arrastra el WAV
(renderizado al tempo actual) a una pista de audio; un clic simple lo guarda en el Escritorio.

## Controles

- **Pestañas RATE / PITCH / NOISE**: cada una edita su curva; arrastra los dos cuadrados (inicio/fin) y elige la forma de la curva.
- **LAYER MIX**: RATE (repeticiones), PITCH (barrido continuo), NOISE (ruido con filtro ascendente), FX (reverb).
- **MODE**: LASER (cada repetición “cae” en tono), PULSE (repeticiones puras), SMOOTH (sin cortes).
- **ZAP DEPTH / LENGTH**: profundidad y duración de la caída del modo LASER.
- **LENGTH**: 1–64 compases · **END GAP**: silencio antes del drop · **TAIL**: CUT o RING OUT · **SYNC**: tempo del host (OFF = 128 BPM).
- **RANDOMIZE** y **↶** (deshacer), flechas y menú de presets.

## Licencia

Usa [JUCE](https://juce.com). Para distribuir el plugin públicamente o venderlo hace falta
cumplir su licencia (AGPLv3, o licencia comercial de JUCE).
