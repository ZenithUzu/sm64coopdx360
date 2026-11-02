# Backend xSDK de sm64coopdx360

Implementación experimental para Xbox 360. La documentación de compilación está en [BUILD.md](../../docs/xbox360/BUILD.md) y el estado actual en [STATUS.md](../../docs/xbox360/STATUS.md).

El árbol actual incorpora renderizado 640×360 en textura y presentación 720p; está compilado pero sin prueba en hardware. La iteración anterior arrancó y permitió jugar, con rendimiento variable y defectos visuales. El teclado USB físico funciona; mouse y multijugador entre plataformas no están confirmados.

Este directorio contiene código y shaders fuente, no SDKs, ROMs, assets generados ni XEXs. Los medios personalizados y precompilados heredados se excluyeron del árbol público; consulta el inventario privado documentado antes de intentar un clon limpio.

Lua 5.3.6 y zlib 1.3.1 se compilan desde `lib/xsdk`; sus avisos se mantienen. Las licencias del repositorio son por componentes: ver [LICENSE.md](../../LICENSE.md).
