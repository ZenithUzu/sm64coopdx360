# Receta reproducible de CoopDX con xSDK

## Qué se incluye

`xsdk-project.json` es una **receta fuente**, no un XEX ni una copia de otro juego. La interfaz de xSDK para Windows la detecta al seleccionar la raíz de este repositorio. Construye las correcciones y mejoras que ya están en las fuentes actuales del port; no necesita aplicar parches a un ejecutable de referencia.

- **English:** select this repository root in the xSDK GUI, choose your installed tools and built SDL3 Xenon package, keep Release, check the recipe, then build. Private ROM/assets are required. See the xSDK [EN/ES/PT guide](https://github.com/ZenithUzu/xsdk/blob/main/README-PROJECTS.md).
- **Português:** selecione a raiz deste repositório na interface do xSDK, indique as ferramentas instaladas e o SDL3 Xenon compilado, mantenha Release, verifique a receita e compile. Sua ROM e os assets privados são necessários. Consulte o [guia EN/ES/PT](https://github.com/ZenithUzu/xsdk/blob/main/README-PROJECTS.md).

## Desde la interfaz

1. Abre `Iniciar-Windows.cmd` del fork de xSDK y elige inglés, español o portugués.
2. En rutas, indica MSYS2 UCRT64, xSDK, compilador PowerPC, carpeta de trabajo y SDL3 Xenon ya compilado. Si glslang está separado, selecciona su ejecutable.
3. Selecciona la **raíz de este repositorio**, con `xsdk-project.json`, y opcionalmente tu ROM de EE. UU. La receta verifica su SHA1 antes de copiarla al archivo local ignorado.
4. Usa **Release**, con opciones CMake adicionales `[]`. **Comprobar receta del port** enumera componentes faltantes sin reconstruir GCC, descargar archivos ni ejecutar el juego. No comprueba la compatibilidad ABI de SDL ni sustituye la compilación.
5. Prepara las entradas privadas según [BUILD.md](BUILD.md). Si tienes permiso para usar las entradas heredadas, puedes activar su restauración local. La opción queda desactivada por defecto y necesita un clon completo con el commit original; una descarga ZIP no tiene ese historial.
6. Pulsa **Compilar proyecto**. La receta genera herramientas de assets, extrae la ROM, prepara assets, compila shaders y construye `sm64coopdx.xex`. **Abrir resultados** abre `<trabajo>/projects/<nombre-del-repositorio>/`.

No se instala ni lanza el juego automáticamente en Xbox. Sigue [BUILD.md](BUILD.md) para preparar los archivos privados y el directorio de ejecución; el XEX solo no representa todos los archivos necesarios.

## Desde PowerShell

```powershell
.\platform\xsdk\build.ps1 -Game `
    -XsdkRoot 'D:\tools\xsdk' `
    -Toolchain 'D:\tools\compiler' `
    -MsysRoot 'D:\msys64' `
    -SdlBuild 'D:\tools\SDL-xenon\build-xsdk' `
    -RomPath 'D:\private\baserom.us.z64' `
    -OutDir 'D:\build\coopdx' -Jobs 4 -BuildType Release
```

Las rutas son ejemplos. `-Glslang` permite usar un glslang-xenon instalado fuera del compilador. `-RestoreInheritedInputs` es opcional y explícito; consulta las restricciones de entradas privadas antes de usarlo. Microsoft XDK y VS2010 no se necesitan para esta ruta. xSDK y SDL3 deben estar construidos con herramientas compatibles.

El script mantiene el PATH UCRT64 durante la preparación **y** la compilación, para resolver las DLL del compilador; lo restaura al terminar, incluso si falla. Python, CMake, Ninja y Make salen de `-MsysRoot`. Los fallos de cada etapa detienen la siguiente. No se reconstruye el compilador por compilar el juego.

## Cómo quedan incorporadas las correcciones

| Problema tratado | Implementación que construye la receta |
| --- | --- |
| Lecturas incompatibles con PowerPC big-endian | Conversión de formatos en el port: audio, assets, comandos DynOS y paquetes; no una inversión global de todos los datos |
| Dibujos incorrectos o lentos con el combinador genérico | Generación offline de materiales especializados y shader genérico de respaldo desde `build_material_shaders.py` |
| Memoria de GPU reutilizada mientras sigue en uso | Arenas y fences en el backend Xenos; presentación y limpieza de eDRAM según el código actual |
| Colores, transparencia y ruido del combinador | Shaders, estados de render y opciones de salida del backend actual |
| Teclado y mouse USB | Fuentes de entrada del port y SDL3 Xenon; no un teclado virtual como sustituto |
| Audio, mods y red | Implementaciones actuales del port, CoopNet/libjuice y librerías fuente; la compilación no demuestra interoperabilidad en todos los casos |
| Construcciones diferentes por rutas o Debug accidental | Rutas configurables, Release por defecto y reporte local de construcción |

Las correcciones específicas del juego permanecen en este repositorio. La herramienta xSDK recibe integración de proyecto y diagnóstico; no se cambió su kernel o controlador de GPU para fingir una mejora universal. La investigación de otros ejecutables no convierte ese binario en una dependencia de esta receta ni incorpora su contenido.

Consulta [STATUS.md](STATUS.md) y el historial para el estado real de cada iteración. Esta actualización del flujo de construcción conserva esas mejoras; **no añade por sí sola una nueva optimización del motor ni garantiza FPS**.

## Registro y comparación

Después de una compilación correcta, `build-info.json` registra configuración, commit, indicador de cambios sin commit, fecha UTC y SHA256 de los ejecutables locales. `hardware_validation: not-run` significa que la construcción no constituye una prueba en Xbox. Si hay cambios sin commit, compara también el diff: el commit por sí solo no los describe.

Para comparar rendimiento, usa el mismo mapa, cámara, resolución, mods, límite de FPS y ajustes. Debug sirve para depurar, no para comparar contra Release. Incluye el reporte, el commit y el log al informar un fallo; no adjuntes ROM, assets ni binarios del SDK.

## Validación de esta integración

El 3 de noviembre de 2026 se comprobó la receta con las herramientas instaladas y una construcción Release en una carpeta de salida nueva compiló los shaders y los 630 pasos del juego. Tras corregir la herencia de módulos de PowerShell 7 al iniciar Windows PowerShell 5 desde Python, la receta terminó correctamente y produjo el XEX y el reporte local. Las pruebas host de la herramienta validaron los tres idiomas, argumentos con espacios, límites de trabajos, herramientas faltantes y rechazo de scripts fuera del repositorio. No se realizó una prueba nueva de esta integración en consola.
