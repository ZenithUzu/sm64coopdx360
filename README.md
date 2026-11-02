# sm64coopdx360

Port **experimental** de [sm64coopdx](https://github.com/coop-deluxe/sm64coopdx) para Xbox 360, basado en el trabajo de sm64ex-coop y sm64-port. No es una versión oficial de Nintendo ni del equipo Coop Deluxe.

Este repositorio publica código fuente y documentación. **No incluye ROM, XEX, SDKs, bibliotecas precompiladas ni paquetes de assets del juego.** No hay una descarga lista para jugar. El historial del fork procede de upstream; la limpieza del árbol actual no elimina archivos de commits anteriores.

## Estado real — 2 de noviembre de 2026

| Área | Estado |
| --- | --- |
| Compilación PowerPC y empaquetado XEX local | Funcionan en el entorno de desarrollo documentado |
| Arranque y juego | Confirmados en Xbox 360 |
| Gamepad | Implementado; probado durante el desarrollo |
| Teclado USB físico | Confirmado por el usuario; entrada de texto y controles |
| Mouse | Backend experimental dependiente de `DmGetMouseChanges` de XBDM; hardware sin confirmar |
| Audio | Funciona, pero puede sufrir cortes cuando baja el rendimiento |
| Gráficos | Persisten errores de color/profundidad y compatibilidad por validar |
| Multijugador PC/teléfono | **No funcionalidad confirmada**: falta adaptar serialización/endian del protocolo |
| Salas CoopNet | No habilitadas en esta compilación |

La última prueba de hardware corresponde a la iteración 11: aproximadamente 15–20 FPS fuera del castillo, 30–60 variables dentro, 7 FPS en Bob-omb Battlefield y 10 FPS en el menú. Son observaciones del usuario, no benchmarks controlados.

**El código actual es la iteración 13, compilada pero aún sin instalar/probar en consola.** Separa el renderizado del juego a 640×360 de la presentación a 1280×720, con un pase de ampliación GPU para conservar la ruta de la Guía. No se promete una mejora de FPS ni estabilidad de esa ruta hasta validarla en hardware.

## Construcción local

Consulta [requisitos y comandos](docs/xbox360/BUILD.md) y [arquitectura/pendientes](docs/xbox360/STATUS.md).

```powershell
.\platform\xsdk\build.ps1 -Game -RomPath 'C:\private\baserom.us.z64' -OutDir 'C:\build\sm64coopdx360'
```

El script requiere herramientas y assets privados preparados localmente. **Una ROM sola no completa actualmente un clon limpio de este árbol público:** se excluyeron también medios personalizados heredados sin una concesión de licencia suficientemente documentada. La compilación completa se validó en el árbol de desarrollo con esos archivos locales; no se afirma que el clon público sea autosuficiente. El [inventario de archivos excluidos](docs/xbox360/private-inputs.json) permite identificar lo que falta sin distribuirlo.

## Implementación

- xSDK/Xenos y SDL3 Xenon; el juego no usa un backend D3D9 del XDK oficial.
- Correcciones para CPU PowerPC big-endian, punteros de 32 bits y comandos de nivel/DynOS.
- Audio generado localmente para el ABI del destino; Lua 5.3.6 y zlib 1.3.1 compilados desde fuente.
- Arena de vértices, retirada diferida de texturas y 24 patrones de shaders especializados con respaldo genérico.
- Entrada física de teclado mediante XAM y mouse opcional mediante XBDM.
- Pruebas anfitrión de lectura de comandos y combiners; CI solo de fuentes, sin compilar/publicar el juego.

## Contribuir

Envía cambios pequeños y distingue siempre **compilado**, **instalado** y **probado en hardware**. Para errores, indica el commit, escena, configuración y log pertinente, eliminando datos personales. No adjuntes ROMs, assets extraídos, dumps de memoria, SDKs ni XEXs a issues o pull requests. Ver [CONTRIBUTING.md](CONTRIBUTING.md).

## Licencias y atribución

No existe una licencia única que cubra todo el material heredado. [LICENSE.md](LICENSE.md) delimita la licencia MIT de los módulos originales del port; no relicencia el juego, archivos de upstream ni dependencias. Se conservan avisos existentes y se documentan componentes en [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Ningún aviso del repositorio concede derechos sobre ROMs, assets o SDKs de terceros.

Créditos a Nintendo por el juego original; a n64decomp, sm64-port, djoslin0/sm64ex-coop, Coop Deluxe y sus contribuidores por las bases del proyecto; y a los autores de xSDK, SDL, Lua y zlib. Este fork no implica afiliación ni patrocinio.
