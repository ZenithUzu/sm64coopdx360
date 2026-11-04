# sm64coopdx360

Port **experimental y no oficial** de [sm64coopdx](https://github.com/coop-deluxe/sm64coopdx) para Xbox 360, con juego en línea y mods. No es una versión de Nintendo ni del equipo Coop Deluxe.

> **English:** unofficial Xbox 360 port of sm64coopdx. Source code only: you need your own legally obtained US ROM and must build the game yourself. See the steps below and [docs/xbox360/BUILD.md](docs/xbox360/BUILD.md).

## Antes de empezar

**Aquí solo hay código fuente.** No se publica ni se publicará el juego empaquetado: ni XEX, ni ROM, ni assets, ni SDKs. No hay descarga lista para jugar; cada persona compila su propia copia con su propia ROM. No pidas ni compartas esos archivos en issues o pull requests.

## Estado — 3 de noviembre de 2026

| Área | Estado |
| --- | --- |
| Compilación y arranque | Funcionan en Xbox 360 |
| Rendimiento | 60 FPS a 1280×720 observados por el autor; sin medición controlada en todas las zonas |
| Mando | Funciona; en los menús el stick izquierdo mueve el cursor y A hace clic |
| Barras de opciones con el mando | Implementadas; sin confirmar en consola |
| Teclado USB | Funciona |
| Ajuste de color de salida | Funciona: gamma y rango limitado en Opciones → Pantalla |
| Audio del juego | Funciona; puede cortarse si baja el rendimiento |
| Mods con Lua | Character Select con un pack de personajes carga sin caerse; el audio de mods no está confirmado de oído |
| Modelos DynOS | Se convierten al cargar; un pack grande como Render96 requiere la herramienta de port y no está confirmado en juego |
| En línea con PC | La Xbox entró a una partida alojada en PC con servidor CoopNet propio; falta validar jugadores, objetos y progreso en movimiento |
| Salas Android en el servidor oficial | La lista muestra salas de la misma versión (v1.5.1); entrar desde la Xbox **sin probar** |
| Android directo con servidor propio | Sin probar |
| Salir del juego | Vuelve al lanzador indicado en `exit_to.txt` desde el menú del juego |

## Qué necesitas

**Consola**

- Xbox 360 con homebrew ya funcionando y un lanzador como Aurora. Este proyecto no explica cómo preparar la consola.
- Disco duro o USB con unos 200 MB libres, más lo que ocupen tus mods.
- Mando de Xbox 360. El teclado USB es opcional y ayuda a escribir.
- Conexión de red para jugar en línea y una forma de copiar archivos a la consola, como FTP.

**PC con Windows de 64 bits**

- [MSYS2](https://www.msys2.org/) con UCRT64: CMake, Ninja, Python, GCC y Make. Por defecto en `C:\msys64`.
- [Git](https://git-scm.com/) y Python 3.11 o posterior con tkinter, para la interfaz de xSDK.
- [xSDK para Windows](https://github.com/ZenithUzu/xsdk), con su interfaz en inglés, español o portugués, y su compilador `powerpc-xsdk-elf`.
- [SDL3 Xenon](https://github.com/xenosworks/SDL-xenon), compilado para ese xSDK.
- Tu propia ROM de Super Mario 64 de **EE. UU.**, en formato `.z64`, cuyo SHA1 coincida con `sm64.us.sha1`.

## Instalación

### 1. Prepara las herramientas

1. Instala MSYS2 y Git.
2. Clona [xSDK](https://github.com/ZenithUzu/xsdk) en `C:\xsdk-windows` y ejecuta `Iniciar-Windows.cmd`. Esa interfaz instala dependencias y construye el compilador en `C:\xsdk-tools\compiler` y el SDK.
3. Clona SDL3 Xenon en `C:\SDL-xenon` y compílalo en `C:\SDL-xenon\build-xsdk` usando como toolchain de CMake `C:/xsdk-windows/cmake/xsdk.cmake`, en modo Release.

Si actualizas xSDK, vuelve a compilar SDL3 Xenon antes de compilar el juego, para que ambos usen el mismo SDK.

### 2. Descarga el código

Haz un clon completo, sin `--depth`, porque el siguiente paso usa el historial:

```powershell
git clone https://github.com/ZenithUzu/sm64coopdx360.git
cd sm64coopdx360
python platform\xsdk\restore_inherited_inputs.py
```

Ese script recupera del historial público de Coop Deluxe los archivos heredados que este repositorio ya no rastrea (mods incluidos, sonidos, texturas). Quedan solo en tu copia local: no los agregues a Git. Úsalo solo si tienes permiso para usar esos archivos; este proyecto no concede derechos sobre ellos.

### 3. Compila

Elige una de estas dos formas. Las dos usan la misma receta y tu ROM solo se usa localmente.

**Con la interfaz de xSDK**

1. Abre `Iniciar-Windows.cmd` y elige tu idioma.
2. Indica las rutas de MSYS2, xSDK, el compilador y SDL3 Xenon.
3. Selecciona la raíz de este repositorio, donde está `xsdk-project.json`, y tu ROM.
4. Deja **Release**, pulsa **Comprobar receta del port** y luego **Compilar proyecto**.
5. El resultado queda en `<trabajo>/projects/<nombre-del-repositorio>/`.

Más detalle en la [receta de proyecto](docs/xbox360/XSDK_RECIPE.md).

**Con PowerShell**

```powershell
.\platform\xsdk\build.ps1 -Game -RomPath 'C:\private\baserom.us.z64' -OutDir 'C:\build\sm64coopdx360'
```

Al terminar tendrás `sm64coopdx.xex` y `build-info.json` en esa carpeta. Otras rutas y opciones están en [docs/xbox360/BUILD.md](docs/xbox360/BUILD.md).

### 4. Copia los archivos a la consola

Crea una carpeta en el disco de la consola, por ejemplo `Hdd1:\sm64coopdx360`, con esta estructura:

```text
sm64coopdx360\
├── sm64coopdx.xex     el que compilaste
├── baserom.us.z64     tu ROM
├── exit_to.txt        opcional: lanzador al salir
├── lang\              carpeta del repositorio
├── palettes\          carpeta del repositorio
├── mods\              carpeta del repositorio, solo los mods que uses
└── dynos\             carpeta del repositorio
```

- `exit_to.txt` lleva en una sola línea la ruta del lanzador al que volver. Sin ese archivo, el juego sale al dashboard.
- FATX no admite nombres de más de 42 caracteres: acorta los de tus mods si hace falta.

### 5. Abre el juego

Agrega la carpeta como ruta de juegos en tu lanzador y abre `sm64coopdx.xex`. Al primer arranque se crea `save\`, con tu partida y `sm64config.txt`. Si algo falla, revisa `sm64coopdx.log`, junto al XEX.

### Actualizar

Descarga los cambios con `git pull`, vuelve a compilar y reemplaza solo `sm64coopdx.xex` con el juego cerrado. Conserva `save\`, donde están tus partidas y ajustes.

## Controles en los menús

- **Stick izquierdo:** mueve el cursor. **Cruceta:** salta entre opciones.
- **A:** hace clic. **B:** vuelve atrás.
- **Barras:** pulsa A sobre la barra, ajústala con el stick o la cruceta y pulsa A o B para soltarla.

Dentro del juego, el mando usa el mapeo normal.

## Jugar en línea

Se configura en `save\sm64config.txt`, con el juego cerrado:

| Ajuste | Valor | Uso |
| --- | --- | --- |
| `coop_network_system` | `1` | CoopNet |
| `coopnet_ip` | `net.coop64.us` | Servidor oficial, con las salas públicas de Android |
| `coopnet_ip` | La IP privada de tu servidor | Servidor propio, para jugar con PC |
| `coopnet_port` | `34197` | Puerto de CoopNet |
| `coopnet_password` | Tu contraseña | Clave de las salas que crees |

- **Servidor oficial:** la consola usa el nombre `sm64coop-android`, así que ve las salas de Android y no las de PC. Entrar a ellas está sin probar, y el servidor podría rechazarla con `JOIN_FAILED`.
- **Servidor propio:** es la forma confirmada de jugar con PC, compilando [CoopNet](https://github.com/coop-deluxe/coopnet). Protégelo: acepta solo tus direcciones y no abras su puerto en el router si no lo necesitas.
- **Tus salas son privadas:** si `coopnet_password` está vacía, la consola crea una clave y la muestra al crear la partida. Para entrar a una sala hace falta esa clave.
- **Respeta a los demás:** el autor informa que los administradores de CoopNet permiten este port si no se usan hacks ni se perjudica a otros jugadores, y que su soporte corre por cuenta del port. No uses este proyecto para eludir esas condiciones.

## Mods

- Copia cada mod a `mods\` y actívalo desde el menú al crear la partida.
- Los mods y packs pesados pueden agotar los 512 MB de la consola. Para reducirlos, usa la herramienta en tu PC:

```powershell
python platform\xsdk\port_dynos_pack.py "C:\ruta\pack" "C:\ruta\pack_xbox" --max-texture 256
python platform\xsdk\port_dynos_pack.py "C:\ruta\pack" --report
```

- Copia a la consola la carpeta generada; el original no se modifica. Los packs van en `dynos\packs\`.
- Al unirte a una sala descargas los mods de quien la creó. Entra solo a salas en las que confíes.

## Problemas comunes

| Problema | Qué revisar |
| --- | --- |
| Colores muy saturados u oscuros | Opciones → Pantalla: activa **Rango limitado (TV)** y baja **Gamma de salida (TV)** |
| No aparecen salas | `coopnet_ip`, la conexión de la consola y que estés en el mismo servidor que el anfitrión |
| No puedes entrar a una sala | Versión, contraseña y mods de la sala. `JOIN_FAILED` en el oficial: mira [Jugar en línea](#jugar-en-línea) |
| La consola se queda sin memoria con un pack | Redúcelo con `port_dynos_pack.py` |
| No compila | Rutas de `C:\msys64`, xSDK y SDL3; luego [BUILD.md](docs/xbox360/BUILD.md) |

## Cómo se logró

Estas son las correcciones que marcaron la diferencia. El detalle por iteración está en [STATUS.md](docs/xbox360/STATUS.md) y en el historial. Las cifras de FPS son observaciones en una consola, no una garantía.

**Rendimiento**

| Problema | Solución |
| --- | --- |
| Bordes dentados al dibujar a 640×360 y ampliar | El juego dibuja a 1280×720 nativo y copia directo al búfer de pantalla, sin pase de ampliación |
| Un cuadro tarde bajaba el juego a 30 FPS | Tres búferes de presentación en vez de dos |
| La CPU esperaba a la GPU en cada lote | Arena de 8 bloques de 8192 vértices; solo se espera al reutilizar el bloque más antiguo |
| Copiar vértices costaba demasiado | Se escriben solo los atributos que lee cada material |
| Shader genérico lento | 32 shaders especializados, generados al compilar a partir de 40 casos del motor; el genérico queda de respaldo |
| Limpieza de eDRAM en mal orden | Se limpia al final, porque los targets comparten tiles |
| Packs de texturas agotaban la memoria | Las texturas DynOS se decodifican al dibujarlas, con lado máximo de 512 píxeles; no se cargan más actores con menos de 64 MB libres |

**Conexión**

| Problema | Solución |
| --- | --- |
| PC y teléfonos son little-endian, la Xbox big-endian | Los paquetes viajan en little-endian y `packet_read`/`packet_write` convierten cada valor (`packet_wire.h`) |
| Cada paquete salía con tipo 0 | `packet_init` escribía el byte alto del tipo y de los números de nivel; ahora escribe el valor real |
| La conexión ICE fallaba entre consola y PC | SHA1 corregido para big-endian en libjuice, más ajustes de STUN y sockets para xSDK |
| Sin salas en la consola | CoopNet y libjuice se compilan desde fuente en `lib/xsdk` |
| Salas de Android invisibles | Nombre de juego `sm64coop-android` en servidores públicos |

**Mods**

| Problema | Solución |
| --- | --- |
| Binarios DynOS escritos en little-endian | Lectura con `BinFile` y conversión por comando de niveles y geometría (`dynos_cmd_order.h`) |
| Audio de mods sin dispositivo de salida | Miniaudio sin dispositivo, mezclado con el audio del juego |
| La consola se colgaba al crear partida con mods | `dynamic_pool_alloc` desalineaba los atómicos de 64 bits, que PowerPC no tolera; ahora conserva la alineación |

**Referencia:** se analizó un XEX de SM64 ya existente para comparar enfoques. Usa D3D9, XAudio2 y shaders por material. Este port no contiene su ejecutable, su código ni sus assets, y usa xSDK en lugar del XDK oficial.

## Reportar errores y contribuir

Indica el commit, `build-info.json`, la escena, la configuración y el log, sin IP, rutas ni nombres personales. Distingue siempre **compilado**, **instalado** y **probado en consola**. Lee [CONTRIBUTING.md](CONTRIBUTING.md); el detalle técnico está en [STATUS.md](docs/xbox360/STATUS.md).

## Licencias y atribución

Este repositorio no tiene una licencia única:

| Parte | Licencia |
| --- | --- |
| Módulos originales del port | MIT, según [LICENSE.md](LICENSE.md) |
| CoopNet (`lib/xsdk/coopnet`) | AGPL-3.0 |
| libjuice (`lib/xsdk/libjuice`) | MPL-2.0 |
| Lua y zlib | Sus propias licencias permisivas |
| xSDK | MIT, en su propio repositorio |
| Código heredado de sm64coopdx, sm64ex-coop, sm64-port y n64decomp | Sus avisos originales; no se relicencia |
| Juego, ROM y assets | Sin licencia: pertenecen a sus titulares |

El juego compilado enlaza CoopNet, así que quien **distribuya** un XEX debe cumplir la AGPL-3.0, además de los derechos del juego. Por eso este proyecto solo publica el código. La lista completa está en [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Esto no es asesoría legal.

Créditos a Nintendo por el juego original; a n64decomp, sm64-port, djoslin0/sm64ex-coop, Coop Deluxe y sus contribuidores por las bases del proyecto; y a los autores de xSDK, SDL, Lua, zlib, CoopNet y libjuice. Este fork no implica afiliación ni patrocinio.
