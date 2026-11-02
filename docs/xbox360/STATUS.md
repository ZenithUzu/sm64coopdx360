# Arquitectura, pruebas y límites

## Árbol actual: iteración 14, sin validar en hardware

`gfx_xsdk.c` abre presentación a 720p y comunica al motor la mitad de ancho/alto. `gfx_xenos_present` resuelve el juego a una textura RGBA y ejecuta un quad de ampliación antes de resolver al buffer nativo. La Guía mantiene su ruta de presentación XAM. Tras dibujo de sistema, se descarta color/profundidad de eDRAM antes del siguiente pase del juego.

La profundidad del target de 720p ocupa los tiles 0–719 de eDRAM y cubre el color del target de 640×360 (tiles 184–367). Por eso esas dos copias no limpian: la limpieza se hace al final, con una resolución completa a 640×360. Además, xSDK solo mantiene la profundidad jerárquica para la superficie de la última limpieza completa. Este orden se dedujo del código de xSDK; su efecto en imagen y FPS no se ha medido.

El ruido del combiner y el dither de alfa siguen a `gfx_opengl.c`. La pendiente del bias de calcomanías es -1 a 640×360 para conservar el desplazamiento de -2 a 720p.

Esta ruta compila, pero no se ha comprobado imagen, orientación, colores, transiciones de Guía ni rendimiento en consola. La iteración 11 fue la última probada. No son releases ni etiquetas publicadas.

## Módulos principales

- `platform/xsdk/CMakeLists.txt`, `build.ps1`, `prepare_assets.py`: compilación y generación local de assets.
- `src/pc/gfx/gfx_xenos.c`: estados Xenos, texturas, arena y pase de ampliación.
- `platform/xsdk/build_material_shaders.py`: especialización offline de 24 patrones desde 40 casos del motor; shader genérico para el resto.
- `include/level_script_read.h`: campos de comandos con anchuras correctas para PowerPC de 32 bits.
- `platform/xsdk/input_usb.c`: teclado XAM y mouse XBDM opcional. El teclado físico fue probado. Disponibilidad de la API de mouse no demuestra soporte del hardware conectado.
- `src/pc/audio/audio_sdl3.c`: salida S16 estéreo a 32 kHz y conversión por SDL3. La mezcla del juego sigue en el hilo principal; puede quedar sin muestras bajo carga.

El análisis local de otro XEX de SM64 identificó D3D9/D3DX9/XAudio2 y shaders por material. No se incluyó ese ejecutable, su imagen extraída, código máquina ni assets en este repositorio. La adaptación usa combiners de coopdx y código propio, no un trasplante del binario.

## Rendimiento y medición

El log registra `frame profile`, `render stats`, `heartbeat` y `present`. `render_ms` incluye envío CPU, esperas de GPU, presentación y limitador; no es una medición GPU aislada. Comparar `specialized` con `draws` antes de atribuir problemas al shader genérico. Medir siempre la misma escena/configuración.

Arena: 32768 vértices de 48 floats; fence antes de reutilizar. Las texturas antiguas conservan su almacenamiento hasta que terminan los comandos que podrían leerlo. No eliminar esos límites de vida por rendimiento.

## Pendientes

1. Validar el renderizado escalado, Guía y controles físicos en hardware.
2. Corregir colores, geometría/profundidad y compatibilidad de materiales.
3. Medir menú/exteriores y reducir costo de lotes y shaders; no prometer 60 FPS.
4. Evitar underruns de audio sin introducir carreras en el estado de mezcla.
5. Definir serialización multijugador independiente de ABI/endian. El backend UDP IPv4 compila, pero el protocolo heredado copia datos nativos; no se afirma interoperabilidad con PC/teléfono. CoopNet no está habilitado.
6. Documentar/reemplazar assets personalizados heredados para que el árbol público pueda construirse sin medios de procedencia/licencia pendiente.

Los packs y caches DynOS de PC pueden contener datos nativos incompatibles. No están validados para este destino. No reutilizar partidas/caches sin copia local de respaldo.
