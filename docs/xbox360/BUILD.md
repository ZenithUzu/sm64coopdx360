# Construcción local de Xbox 360

## Requisitos externos

- Windows de 64 bits con PowerShell.
- [xSDK](https://github.com/ZenithUzu/xsdk), sus bibliotecas compiladas y su compilador host de shaders.
- Toolchain `powerpc-xsdk-elf` (el entorno de desarrollo usa GCC 15.3).
- SDL3 Xenon compilado para ese xSDK. Su backend actual no proporciona una ventana OpenGL: la presentación del juego es propia.
- MSYS2 UCRT64: CMake, Ninja, Python, GCC y Make para herramientas anfitrión.
- Una ROM US aportada por el usuario, cuyo SHA1 coincida con `sm64.us.sha1`.
- Los archivos heredados de `private-inputs.json`. El árbol público no los trae; `python platform/xsdk/restore_inherited_inputs.py` los recupera del commit de upstream en el historial, sin agregarlos a Git. Requiere un clon completo.

xSDK, SDL y el toolchain son dependencias externas; no se redistribuyen sus binarios aquí. El SDK oficial no está incluido y solo se usa externamente para herramientas de diagnóstico/transferencia. No se requiere VS2010 para el build GCC actual.

La guía de instalación completa, con los archivos que van en la consola, está en el [README](../../README.md). Un clon limpio con los archivos heredados restaurados compiló el XEX completo el 3 de noviembre de 2026, con xSDK y SDL3 ya instalados; ese XEX no se probó en consola.

## Rutas y comandos

Los valores predeterminados del script son `C:\xsdk-windows`, `C:\xsdk-tools\compiler`, `C:\SDL-xenon\build-xsdk` y herramientas anfitrión en `C:\msys64`. Las tres primeras rutas se pueden cambiar mediante `-XsdkRoot`, `-Toolchain` y `-SdlBuild`. Las rutas de las herramientas MSYS2 aún están fijadas en el script y CMakeLists; revisarlas si tu instalación difiere.

```powershell
.\platform\xsdk\build.ps1 -Game `
  -RomPath 'C:\private\baserom.us.z64' `
  -OutDir 'C:\build\sm64coopdx360'
```

El script valida la ROM, prepara herramientas anfitrión, extrae assets localmente, genera audio big-endian/32 bits, texturas, texto y animaciones; después configura Release/Ninja y construye el juego. Puede omitirse `-RomPath` cuando existe `baserom.us.z64` local. Sin `-Game` solo se construyen bibliotecas/objetos de plataforma y motor.

Reconstrucción incremental:

```powershell
& 'C:\msys64\ucrt64\bin\cmake.exe' --build 'C:\build\sm64coopdx360' --target sm64coopdx --parallel 6
if ($LASTEXITCODE -ne 0) { throw 'Build fallido' }
```

Resultados privados: `sm64coopdx` (ELF) y `sm64coopdx.xex`. Guardar ambos juntos para simbolizar crashes; no publicarlos desde este fork. Evitar el antiguo Makefile de escritorio para este destino. Los binarios de dependencias de escritorio también se retiraron del árbol público.

## Archivos excluidos

La limpieza de publicación usa `git rm --cached`: conserva las copias del entorno local, pero el clon público no las tendrá. El inventario incluye texturas/medios personalizados heredados y dependencias precompiladas, no solo datos de ROM. No se facilita su descarga ni se afirma una licencia sobre ellos. No introducirlos en Git para resolver un build local.

## Pruebas de fuente sin ROM

Desde la raíz, con GCC anfitrión:

```sh
python3 tools/verify_public_tree.py
cc -std=c99 -Wall -Wextra -Werror -Isrc/pc/gfx platform/xsdk/tests/combiner_test.c -o /tmp/combiner_test
/tmp/combiner_test
cc -std=c99 -Wall -Wextra -Werror -D_LANGUAGE_C -DF3DEX_GBI_2E -Iinclude -I. platform/xsdk/tests/level_script_read_test.c -o /tmp/level_script_test
/tmp/level_script_test
cc -std=c99 -Wall -Wextra -Werror -Isrc/pc/network/packets platform/xsdk/tests/packet_wire_test.c -o /tmp/packet_wire_test
/tmp/packet_wire_test
cc -std=c99 -Wall -Wextra -Werror -Isrc/pc/network/packets platform/xsdk/tests/packet_safe_path_test.c -o /tmp/packet_safe_path_test
/tmp/packet_safe_path_test
cc -std=c99 -Wall -Wextra -Werror -Isrc/pc/gfx platform/xsdk/tests/gfx_xsdk_lut_test.c -lm -o /tmp/gfx_xsdk_lut_test
/tmp/gfx_xsdk_lut_test
cc -std=c99 -Wall -Wextra -Werror -Idata -Isrc platform/xsdk/tests/dynos_cmd_order_test.c -o /tmp/dynos_cmd_order_test
/tmp/dynos_cmd_order_test
cc -std=c99 -Wall -Wextra -Werror -Idata platform/xsdk/tests/dynos_tex_shrink_test.c -o /tmp/dynos_tex_shrink_test
/tmp/dynos_tex_shrink_test
python3 platform/xsdk/tests/port_dynos_pack_test.py
```

La CI realiza controles de fuente; no descarga ROMs, no necesita credenciales de consola y no publica artefactos del juego.
