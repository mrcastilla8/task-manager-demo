# Guía de trabajo para agentes

> **Fase 0, formalizada 2026-10-15:** DP-01–DP-11 constan aprobadas por coordinación; la fecha exacta de aprobación original no fue suministrada. Aplican al diseño futuro v2, no cambian v1.0. Contratos candidatos están en `contracts/v2/`; no se migran implementaciones ni se declaran integrados. Las propuestas técnicas no ratificadas permanecen separadas. Siga [CONTRATOS_V2.md](docs/CONTRATOS_V2.md), [MIGRACION_CONTRATOS_V2.md](docs/MIGRACION_CONTRATOS_V2.md) y gates de [WORKFLOW_DESARROLLO.md](docs/WORKFLOW_DESARROLLO.md). En caso de conflicto, encabezados v1 gobiernan código existente; decisiones aprobadas y contrato v2 gobiernan el destino de migración.


## Lectura obligatoria

1. Lee [README.md](README.md), [guía general](00_INDICACIONES_GENERALES_PROYECTO.md), [especificación funcional](docs/ESPECIFICACION_FUNCIONAL.md) y [matriz de trazabilidad](docs/MATRIZ_TRAZABILIDAD.md).
2. Lee [decisiones aprobadas](docs/DECISIONES_APROBADAS.md) para reglas de destino v2. [Decisiones pendientes](docs/DECISIONES_PENDIENTES.md) conserva solo el historial de deliberación; no trates DP-01–DP-11 como tareas abiertas.
3. Lee la asignación del integrante correspondiente (`01` a `05`) y luego el encabezado/API de [contratos v1.0](contracts/README.md).
4. Para decisiones académicas, consulta [REQUISITOS_DOCENTE.md](docs/REQUISITOS_DOCENTE.md). Las notas del profesor y la transcripción están fuera del repositorio; solicita al usuario los archivos si no están disponibles en la tarea.

## Precedencia y etiquetas

- `contracts/*.h` junto con `contracts/README.md` definen el contrato actualmente implementado y congelado en 1.0.
- `docs/REQUISITOS_DOCENTE.md` es la referencia canónica de evidencia académica y sus páginas/minutos; no transforma ejemplos en reglas obligatorias.
- `docs/ESPECIFICACION_FUNCIONAL.md` consolida la vista del sistema y distingue hechos, decisiones de equipo y preguntas abiertas.
- `docs/DECISIONES_PENDIENTES.md` contiene reglas no aprobadas. No se deben usar sus propuestas como instrucciones normativas.
- Cada regla debe identificarse como `CONFIRMADO_POR_DOCENTE`, `PROPUESTA_DOCENTE`, `DECISION_EQUIPO` o `PENDIENTE_CONFIRMACION` con fuente y ubicación.
- Los documentos académicos contienen temas ajenos. No importar QR, multiplicación de matrices, speedup ni grafos al simulador.

## Decisiones cerradas; detalles para ratificación técnica

DP-01–DP-11 están aprobadas para el destino v2. Los detalles de ejecución concurrente, desempates simultáneos, publicación de snapshots/eventos, overflow de tickets y retención eficiente se encuentran como propuestas no ratificadas en el registro aprobado; no conviertas propuestas en política sin aprobación.

No inventes reglas para llenar vacíos. Si una decisión impide continuar, completa primero el trabajo independiente, presenta alternativas con evidencia e impacto, y deja bloqueada solo la parte dependiente de la decisión.

## Responsabilidades de integrantes

- **01 Castilla — Modelo, estructuras y memoria** (rama `castilla`): ownership de `contracts/`, `core/ColaProcesos.*`, `core/Memoria.*` y pruebas del módulo. No define scheduling ni GUI.
- **02 Brickzon — Scheduler y planificación** (rama `brickzon`): ownership de `scheduler/` y pruebas del módulo; selecciona y ordena READY conforme a la política aprobada; no duplica la política en motores.
- **03 Aldana — Motor lógico** (rama `aldana`): ownership de `simulation/` y sus pruebas; reloj/eventos/transiciones sin threads reales; comparte semántica con el concurrente.
- **04 Ángel — Motor concurrente** (rama `angel`): ownership de `concurrent/` y sus pruebas; coordinación de workers reales y sincronización, con un solo CPU lógico simulado.
- **05 Poma — GUI e integración** (rama `poma`): ownership de `gui/`, punto de entrada y pruebas de integración visual; representa snapshots/eventos, no decide scheduler. Qt 6 es recomendación hasta ratificación.

El [workflow canónico](docs/WORKFLOW_DESARROLLO.md) define fases, dependencias, paralelismo, gates, PRs y validación. Los nombres de rama identifican espacios de trabajo, no identidades personales de GitHub. Los cambios que afecten varios módulos se coordinan con sus responsables; nadie cambia contratos compartidos unilateralmente.

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
