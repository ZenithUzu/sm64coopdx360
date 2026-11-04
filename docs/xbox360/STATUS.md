# Arquitectura, pruebas y límites

## Árbol actual: iteración 35, salas Android en el servidor oficial sin probar en hardware

`gfx_xsdk.c` abre presentación a 720p y comunica al motor esas dimensiones. Con el juego a tamaño nativo, `gfx_xenos_present` resuelve directamente al front buffer; el quad de ampliación solo corre si el tamaño interno es menor. La Guía mantiene su ruta de presentación XAM. Tras dibujo de sistema, se descarta color/profundidad de eDRAM antes del siguiente pase del juego.

La profundidad del target de 720p ocupa los tiles 0–719 de eDRAM y cubre el color del target de 640×360 (tiles 184–367). Por eso esas dos copias no limpian: la limpieza se hace al final, con una resolución completa a 640×360. Además, xSDK solo mantiene la profundidad jerárquica para la superficie de la última limpieza completa. Este orden se dedujo del código de xSDK; su efecto en imagen y FPS no se ha medido.

El ruido del combiner y el dither de alfa siguen a `gfx_opengl.c`. La pendiente del bias de calcomanías es -1 a 640×360 para conservar el desplazamiento de -2 a 720p.

La iteración 35 usa el nombre de juego `sm64coop-android` en servidores no privados, por indicación recibida en el canal de ayuda de la comunidad; en la lista oficial esas salas figuran con la misma versión v1.5.1. Unirse a ellas está sin probar. La iteración 34 crea toda sala de la consola con contraseña, generándola si falta, y rechaza rutas de mods remotas que salgan de la carpeta de descargas. La iteración 33 hace que tocar una barra la ajuste con el stick o la cruceta, de uno en uno, hasta pulsar A o B. La iteración 32, cuyo ajuste de color funcionó según el autor, añade en Opciones de pantalla la gamma de salida y el rango limitado, escritos en el LUT del display, porque xSDK deja un LUT lineal; registra el LUT que XAM carga con la Guía. La causa de los colores saturados frente a PC no está confirmada. La iteración 31 alinea los bloques de `dynamic_pool_alloc`, porque los atómicos de 64 bits de Miniaudio bloqueaban la consola al crear una partida con mods de audio. La iteración 30 cierra el cliente CoopNet antes de salir del juego. La iteración 29 hace funcionar el motor de audio de mods sin dispositivo y mezcla su salida con el audio del juego en `buffer_audio`. La iteración 28 decodifica las texturas de DynOS al dibujarlas, limita su lado a 512 píxeles y deja de cargar actores con menos de 64 MB libres, porque el pack Render96 agotó la memoria de la consola al cargar 31 actores. La iteración 27 lee los binarios de DynOS en little-endian (`BinFile`) y convierte los comandos de nivel y geometría con `data/dynos_cmd_order.h`, comprobado contra 8145 comandos de mods reales en el PC. La consola no genera packs desde fuente. FATX limita los nombres a 42 caracteres. La iteración 26 usa el mando como cursor en los menús. La iteración 25 corrige `packet_init`, que enviaba el byte alto del tipo de paquete y de los números de nivel; con ella la Xbox entró a una partida alojada en PC, a 60 FPS según el log. Falta validar dos jugadores en movimiento. La iteración 23 quita el rechazo del servidor oficial tras la autorización que el autor informa haber recibido de sus administradores: sin hacks, sin perjudicar a otros jugadores y con soporte del port a cargo del autor. El servidor rechaza con `JOIN_FAILED` a este cliente al unirse a salas de clientes oficiales; no se envía un hash falso. La iteración 22 ajustó la escala del menú y el reintento de `poll`. La iteración 21 corrigió SHA1 en big-endian, y la conexión ICE con PC se completó en consola. La iteración 20 compila CoopNet y libjuice desde `lib/xsdk` y activa las salas. La iteración 19 envía los paquetes en little-endian desde `packet_write`/`packet_read`; los bytes variables usan `packet_write_bytes`. La iteración 18 fue la última probada y vuelve a 1280×720 nativo y retira el callback de terminación, porque en la iteración 17 salir con Xbox Home congelaba la consola. La iteración 17 vuelve al lanzador indicado en `exit_to.txt`, junto al XEX, al salir desde el menú del juego; sin ese archivo sale al dashboard. La iteración 16 divide la arena en 8 bloques con fence propio y añade 8 patrones de material vistos en consola. La iteración 15 añadió tres búferes de presentación, escritura parcial de vértices y emparejado de materiales de un ciclo por sus primeros 8 comandos. La iteración 17 fue la última probada: 60 FPS casi en todas partes a 640×360 internos, y salir desde el menú del juego vuelve a Aurora sin reiniciar. Xbox Home desde la Guía no vuelve bien; la causa no está confirmada. No son releases ni etiquetas publicadas.

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

Arena: 8 bloques de 8192 vértices de 48 floats; fence antes de reutilizar cada bloque. Las texturas antiguas conservan su almacenamiento hasta que terminan los comandos que podrían leerlo. No eliminar esos límites de vida por rendimiento.

## Packs de modelos

`platform/xsdk/port_dynos_pack.py` genera una copia de un pack o mod para la consola: reduce las texturas PNG, puede quitar modelos por número de triángulos y da un informe. No modifica el original ni reduce polígonos. Un actor quitado usa el modelo del juego.

## Pendientes

1. Validar dos jugadores en movimiento: posición, objetos y progreso entre Xbox, PC y Android.
2. Probar la entrada a salas Android en el servidor oficial, y el rechazo `JOIN_FAILED` si aparece.
3. Confirmar en consola el audio de mods, las barras con el mando y un pack grande reducido.
4. Fijar los valores de color por defecto con los datos de la Guía, y comparar un fotograma con PC.
5. Aclarar `Tried to alloc` en `gNotesAndBuffersPool`, que aparece también sin mods.
6. Evitar cortes de audio sin introducir carreras en el estado de mezcla.
7. Reemplazar los archivos heredados de procedencia pendiente, para que un clon no dependa del historial.

Los packs DynOS de PC se convierten al cargarlos; un pack grande sigue sin validar en juego. No reutilizar partidas ni caches sin copia de respaldo.
