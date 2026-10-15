# Guía de trabajo para agentes

## Lectura obligatoria

1. Lee [README.md](README.md), [guía general](00_INDICACIONES_GENERALES_PROYECTO.md), [especificación funcional](docs/ESPECIFICACION_FUNCIONAL.md) y [matriz de trazabilidad](docs/MATRIZ_TRAZABILIDAD.md).
2. Lee [decisiones pendientes](docs/DECISIONES_PENDIENTES.md) antes de implementar reglas de prioridad, tiempo, transiciones o GUI.
3. Lee la asignación del integrante correspondiente (`01` a `05`) y luego el encabezado/API de [contratos v1.0](contracts/README.md).
4. Para decisiones académicas, consulta [REQUISITOS_DOCENTE.md](docs/REQUISITOS_DOCENTE.md). Las notas del profesor y la transcripción están fuera del repositorio; solicita al usuario los archivos si no están disponibles en la tarea.

## Precedencia y etiquetas

- `contracts/*.h` junto con `contracts/README.md` definen el contrato actualmente implementado y congelado en 1.0.
- `docs/REQUISITOS_DOCENTE.md` es la referencia canónica de evidencia académica y sus páginas/minutos; no transforma ejemplos en reglas obligatorias.
- `docs/ESPECIFICACION_FUNCIONAL.md` consolida la vista del sistema y distingue hechos, decisiones de equipo y preguntas abiertas.
- `docs/DECISIONES_PENDIENTES.md` contiene reglas no aprobadas. No se deben usar sus propuestas como instrucciones normativas.
- Cada regla debe identificarse como `CONFIRMADO_POR_DOCENTE`, `PROPUESTA_DOCENTE`, `DECISION_EQUIPO` o `PENDIENTE_CONFIRMACION` con fuente y ubicación.
- Los documentos académicos contienen temas ajenos. No importar QR, multiplicación de matrices, speedup ni grafos al simulador.

## Decisiones abiertas que afectan implementación

Consulta DP-01 a DP-11. En particular: escala 1–3 vs 1–10; conjunto TL y recálculo dinámico; cuota individual vs quantum global 500 ms; FIFO y actualización de orden al reingresar; aging vs recálculo por tamaño; semántica de `siguientePaso()`; transiciones/cancelación; timeline idle/escala/historial; mapa conceptual de memoria; `std::list` vs lista enlazada simple; y aprobación de Qt 6/dos motores.

No inventes reglas para llenar vacíos. Si una decisión impide continuar, completa primero el trabajo independiente, presenta alternativas con evidencia e impacto, y deja bloqueada solo la parte dependiente de la decisión.

## Responsabilidades de integrantes

- **01 Modelo, estructuras y memoria:** `contracts/`, `core/ColaProcesos.*`, `core/Memoria.*`; no define scheduling ni GUI.
- **02 Scheduler:** `scheduler/`; selecciona y ordena READY conforme a la política aprobada; no duplica la política en motores.
- **03 Motor lógico:** `simulation/`; reloj/eventos/transiciones sin threads reales; debe compartir semántica con el concurrente.
- **04 Motor concurrente:** `concurrent/`; coordinación de workers reales y sincronización, con un solo CPU lógico simulado.
- **05 GUI e integración:** `gui/` y punto de entrada; representa snapshots/eventos, no decide scheduler. Qt 6 es recomendación hasta ratificación.

Los cinco documentos individuales describen los entregables y límites, pero la especificación canónica y los contratos determinan precedencia.

## Restricciones de cambio

- No alteres `contracts/*.h`, campos, enums, unidades, errores ni métodos virtuales mientras v1.0 esté congelado. Una necesidad aprobada requiere una propuesta de nueva versión y migración explícita.
- No cambies código C++, tests o CMake durante una tarea solo documental.
- No declares eliminada una decisión de equipo porque no aparezca en las notas del docente.
- No copies pseudocódigo académico incompleto como interfaz o código normativo.
- Mantén documentación y ejemplos sincronizados; evita ejemplos de API obsoletos.
- No publiques archivos académicos originales sin autorización.

## Evidencia para declarar trabajo terminado

Relaciona cada requisito con archivos y criterio verificable en `docs/MATRIZ_TRAZABILIDAD.md`. Para documentación, revisa enlaces relativos, coherencia de etiquetas/fuentes y ejemplos; comprueba `git diff --check` y confirma que los cambios se limitan al alcance autorizado. Para implementación, aporta pruebas reproducibles y resultados reales; no afirmes pruebas de compilación, concurrencia ni GUI que no ejecutaste.
