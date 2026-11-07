# Opciones gráficas experimentales

Rama `feature/image-quality`, separada de `claude-v44-source`.

En Opciones → Pantalla:

- Resolución interna: 1280×720, 960×536 (75%, altura alineada a 8 píxeles) o 640×360. Requiere reinicio. La salida sigue siendo 720p. Menor resolución puede reducir carga de GPU; no garantiza FPS.
- Ampliación: vecino cercano para píxeles definidos o bilineal para suavizar la imagen. Requiere reinicio.
- Filtrado de texturas: cercano o bilineal. La opción tripunto se oculta en Xbox porque este backend no la implementa de forma independiente.
- Sombras originales: activa/desactiva las sombras de objetos del motor. Cambio inmediato. No son sombras dinámicas nuevas, shadow maps ni iluminación adicional.
- Gamma de salida y rango limitado: ajustes existentes para la TV.

Configuración persistente: `xbox_shadows true/false`, `xbox_upscale_filter 0/1`, `xbox_render_scale 0/1/2`. Predeterminados: sombras activas y ampliación bilineal, conservando la apariencia previa. No cambia el protocolo de red.

Comparar fuera/dentro del castillo y Bob-omb Battlefield con los mismos mods, cámara y límite de FPS. Comprobar sombras en movimiento, menús, Guía Xbox y retorno a Aurora. La compilación local no demuestra estabilidad ni rendimiento en consola.
