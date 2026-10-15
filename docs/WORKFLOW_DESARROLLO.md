# Workflow de desarrollo de ConcurrentSimulator

> **Actualización Fase 0 (2026-10-15):** fase 0 quedó cerrada con DP-01–DP-11 aprobadas y encabezados candidatos en `contracts/v2/`. No se migran módulos en este cambio. La fase posterior se rige por [MIGRACION_CONTRATOS_V2.md](MIGRACION_CONTRATOS_V2.md): Castilla modelo/lista/memoria → Brickzon Scheduler; Aldana y Ángel en paralelo después de congelar contratos/Scheduler; Poma avanza en paralelo con mock tras congelar interfaz. Gates: G1 ratificar propuestas/API; G2 modelo; G3 Scheduler; G4 equivalencia de motores; G5 GUI/reproducción; G6 revisión integrada. El trabajo paralelo se mantiene. La matriz siguiente refleja el plan previo; quantum global y aging se refieren solo a v1/historial.

Las descripciones detalladas heredadas más abajo preservan cómo se preparó la línea v1 y la auditoría de ramas. Cuando mencionan alternativas abiertas, aging operativo o quantum de 500 ms, se refieren a v1/antecedentes; el plan de migración v2 y el resultado de Fase 0 arriba tienen precedencia para el trabajo futuro.

Este documento es la referencia del equipo para ordenar el trabajo desde la línea base actual hasta una aplicación integrada. Formaliza responsables, dependencias, actividades paralelas y gates; no aprueba decisiones funcionales pendientes ni convierte propuestas académicas en requisitos.

## Principios y estado de la línea base

- Los contratos v1.0 de `contracts/` y sus encabezados gobiernan las interfaces actuales. Un cambio compartido requiere decisión, versionado y migración coordinada.
- Las etiquetas `CONFIRMADO_POR_DOCENTE`, `PROPUESTA_DOCENTE`, `DECISION_EQUIPO` y `PENDIENTE_CONFIRMACION` conservan su significado en [requisitos docentes](REQUISITOS_DOCENTE.md), [especificación funcional](ESPECIFICACION_FUNCIONAL.md) y [decisiones pendientes](DECISIONES_PENDIENTES.md).
- Las ramas asignadas son `castilla`, `brickzon`, `aldana`, `angel` y `poma`. Identifican espacios de trabajo, no cuentas personales.
- Estado observado al preparar este workflow: PR #1 está fusionado en `master` y su línea base es `8f5b16b12c4012a97df7cacfabe8ed38c22eb318`. Modelo, cola, memoria y pruebas base están integrados. En `brickzon` existen Scheduler y archivos de pruebas; no están integrados en `master`. En `angel` existen motor concurrente, Scheduler y archivos de pruebas; no están integrados. En las ramas inspeccionadas no aparecen implementación del motor lógico ni de GUI. La existencia de archivos no demuestra que estén probados ni aprobados.
- No se encontró configuración de CI en `.github/` en la línea base inspeccionada. CMake registra actualmente `test_proceso`, `test_cola`, `test_memoria` y `test_integracion`; las ramas de Scheduler y concurrente tienen archivos de prueba adicionales, pero sus resultados no se presumen. Hasta que el equipo incorpore CI, cada PR debe aportar resultados reproducibles de validación local y los checks de GitHub se describen como no configurados, no como aprobaciones obligatorias.

## Matriz de fases

| Fase | Responsable | Dependencias | Entregables | ¿Paralelizable? | Gate de integración |
|---|---|---|---|---|---|
| 0. Decisiones funcionales y contratos | Coordinación del equipo | Evidencia, v1.0, especificación y casos | Cerrada: DP-01–DP-11, casos y API v2 candidata | Cerrada en Fase 0 | Decisiones aprobadas; propuestas técnicas separadas; adopción API ratificada antes de migrar |
| 1. Modelo, estructuras y memoria | Castilla (`castilla`) | API v2 ratificada | Modelo, validación, cuota derivada, lista simple, memoria y pruebas | Base de Scheduler; otros pueden preparar mock | API pública compatible; ownership/copia/move y límites validados; integrar por PR |
| 2. Scheduler | Brickzon (`brickzon`) | Modelo v2 estable | Clasificación dinámica, selección READY prioridad/ticket/PID, Round Robin y pruebas (sin aging) | Auditoría paralela con fase 1; migración depende del modelo | Una regla común; vectores y fronteras aprobados; reingreso asigna ticket nuevo |
| 3A. Motor lógico | Aldana (`aldana`) | Contrato y Scheduler v2 estables | Cuotas, reloj, eventos, WAIT/idle, paso, cancelación, timeline y pruebas | En paralelo con 3B; GUI puede avanzar con mock | Traza por paso determinista equivalente; sin wall-clock como reloj simulado |
| 3B. Motor concurrente | Ángel (`angel`) | Contrato y Scheduler v2 estables | CPU lógico único, cuotas/tickets, workers, sincronización, cancelación, snapshots y pruebas | En paralelo con 3A | Seguridad de concurrencia y equivalencia observable con 3A |
| 4. GUI y visualización | Poma (`poma`) | `IExecutionEngine` y `EstadoSistema`; DP-08/11 antes de finalizar GUI | Ventana, tabla, cola, CPU, temporizador, mapa conceptual de memoria, timeline, log, diálogo de alta y controles | Sí, puede iniciar desde fases 1–3 con mock/fixtures. Integración real primero con 3A, luego 3B | Vistas y controles trabajan contra la interfaz; actualización segura en hilo UI; snapshots coherentes; pruebas de integración/escenarios ejecutados; Qt y selección de motores ratificados si aplican |
| 5. Integración y validación general | Todos; Poma coordina integración visual | Módulos con gates anteriores y dependencias disponibles en `master` o mecanismo de integración acordado | Integración de módulos, casos de referencia, comparación de motores, validación del sistema | Solo las correcciones independientes; secuencia de integración sigue modelo → Scheduler → motores → GUI | Build, pruebas aplicables, casos y comparación aprobados; sin conflictos ni contratos incompatibles. Crear un PR no satisface el gate |
| 6. Demostración final | Todos | Fase 5 aprobada y decisiones efectivamente acordadas | Guion reproducible, datos de entrada y evidencia de ejecución | Preparación del guion puede ir en paralelo con integración; ejecución final depende de Fase 5 | Demostrar altas, prioridades, empate/rotación, llegada dinámica, memoria, WAIT/retorno, cancelación, turnos, timeline y ambos motores si siguen en alcance; sin resultados simulados o hardcodeados presentados como ejecución real |

Los nombres de archivo en asignaciones describen entregables previstos salvo que el estado anterior indique su presencia. Usa estas distinciones en cada PR: **documentado** (se especifica), **existente** (el archivo está en una rama), **implementado** (hay comportamiento en código), **probado** (una ejecución reproducible pasó) e **integrado** (está en la línea base acordada). No infieras un estado del otro.

## Dependencias y paralelismo

```mermaid
flowchart TD
    F0[Decisiones y contratos aprobados]
    M[Modelo y memoria<br/>Castilla]
    S[Scheduler<br/>Brickzon]
    L[Motor lógico<br/>Aldana]
    C[Motor concurrente<br/>Ángel]
    G0[GUI con mock<br/>Poma]
    G1[GUI con motor lógico]
    G2[GUI con motor concurrente]
    I[Integración y validación general]
    D[Demostración final]

    F0 -->|reglas y contratos| M
    F0 -->|política aprobada| S
    M -->|modelo estable| S
    S -->|Scheduler compartido| L
    S -->|Scheduler compartido| C
    M --> L
    M --> C
    F0 -->|semántica temporal/estados| L
    F0 -->|semántica temporal/estados| C
    F0 -.->|DP-08/11 antes de terminar| G2
    G0 -.->|trabajo visual independiente| G1
    G0 -.->|mock permite avanzar temprano| G2
    L --> G1
    G1 --> G2
    L --> I
    C --> I
    S --> I
    M --> I
    G2 --> I
    I --> D
```

Las flechas sólidas expresan dependencia para integración o validación. Las punteadas indican trabajo visual que puede iniciarse con mocks o una decisión que debe cerrarse antes de completar GUI. 3A y 3B pueden desarrollarse en paralelo después de acordar las reglas compartidas; ninguno debe reimplementar la política del Scheduler.

## Fase 0 — Resolver decisiones funcionales bloqueantes

Responsable: coordinación del equipo con las personas afectadas. Revisar primero [DP-01 a DP-11](DECISIONES_PENDIENTES.md), junto con evidencia y contratos. Priorizar DP-01 (escala), DP-02 (conjunto y recálculo), DP-03 (límites/tamaños iguales), DP-04 (cuota y quantum), DP-05 (FIFO/reingreso), DP-06 (transiciones/cancelación), DP-07 (`siguientePaso()`), DP-09 (`std::list` frente a lista enlazada simple) y DP-10 (aging/prioridad dinámica). DP-08 (timeline/tiempo visual) y DP-11 (Qt/dos motores) deben resolverse antes de completar GUI, pero pueden discutirse en paralelo con módulos iniciales.

Para cada decisión: analizar fuente académica y su etiqueta; comparar alternativas; localizar contratos y módulos afectados; acordar una opción; registrar decisión, fuente y alcance; actualizar especificación y casos de referencia; y solo entonces autorizar implementación. Si se requiere cambiar la API congelada, preparar nueva versión y migración coordinada. El criterio de salida es que cada módulo sepa qué puede implementar y qué permanece bloqueado; tareas independientes continúan.

## Fase 1 — Modelo, estructuras de datos y memoria

Responsable: **Castilla**, rama `castilla`. Revisar primero lo integrado en `master`: `contracts/`, `core/ColaProcesos.*`, `core/Memoria.*` y pruebas base. El modelo, cola, memoria y pruebas ya existen en la línea base inspeccionada; la asignación consiste en auditar y completar faltantes, no rehacer lo ya integrado.

Verificar semántica de altas, búsqueda, eliminación y filtros de `ColaProcesos`; validación, admisión, capacidad insuficiente y liberación de memoria; y compatibilidad con contratos. Resolver DP-09 antes de reemplazar la estructura implementada. Los archivos de prueba base registrados por CMake son `tests/test_proceso.cpp`, `tests/test_cola.cpp`, `tests/test_memoria.cpp` y `tests/test_integracion.cpp`.

**Gate:** faltantes delimitados y completados, comportamiento validado con pruebas ejecutadas, documentación/API consistente e integración aceptada. La existencia de archivos en `master` no declara el trabajo probado por esta fase.

## Fase 2 — Scheduler

Responsable: **Brickzon**, rama `brickzon`. Auditar la implementación y pruebas ya presentes en esa rama antes de cambiarla. Entregables previstos: `scheduler/Scheduler.h`, `scheduler/Scheduler.cpp`, `scheduler/SchedulerConfig.h`, `tests/test_scheduler_prioridad.cpp`, `tests/test_scheduler_empate.cpp` y `tests/test_scheduler_aging.cpp`.

El Scheduler selecciona READY y ordena la cola según política aprobada; no crea workers, temporizadores visuales ni interfaz gráfica, ni duplica su regla en los motores. Revisar prioridades, configuración, filtrado de estados, vacío, desempate y aging aprobado. El empate de llegada no implementa por sí mismo Round Robin: al reingresar un proceso a READY después de quantum, la coordinación/motor asigna nuevo `ordenLlegada` o ticket conforme a DP-05. Pruebas presentes en una rama no equivalen a pruebas ejecutadas ni integradas.

**Gate:** selección determinista y pruebas/casos de referencia ejecutados y revisados; dependencias contractuales disponibles en `master` o integración coordinada documentada.

<a id="fase-3--motores-de-ejecucion"></a>
## Fase 3 — Motores de ejecución

Los dos frentes comparten contratos, Scheduler, reloj lógico y semántica aprobada. Su desarrollo puede ser paralelo, pero deben converger en los mismos casos observables. La regla del proyecto es un solo CPU lógico, aunque el motor concurrente use varios hilos del sistema.

### Fase 3A — Motor lógico

Responsable: **Aldana**, rama `aldana`. Crear `simulation/SimulationEngine.h/.cpp` y `simulation/SimulationClock.h/.cpp`; la implementación aún no estaba presente en las ramas auditadas al crear este documento. Implementar ejecución sin threads reales, reloj en milisegundos simulados, estados, quantum/cuota conforme a reglas aprobadas, WAIT/retorno, finalización, cancelación, eventos y timeline.

Pruebas previstas: `tests/test_simulation_quantum.cpp`, `test_simulation_wait.cpp`, `test_simulation_finish.cpp`, `test_simulation_cancel.cpp` y `test_simulation_dynamic_process.cpp`. Cubrir respectivamente intervalos y tiempo restante; bloqueo y espera E/S; terminación y liberación; cancelación idempotente/activa; y alta dinámica con snapshot y rechazo de memoria según contrato. La llamada `siguientePaso()` queda sin semántica temporal exacta hasta resolver DP-07; no fijar por anticipado si significa evento, transición, turno o quantum.

**Gate:** motor determinista, snapshots/eventos/timeline coherentes, pruebas ejecutadas, semántica de paso acordada y resultados contrastables con casos aprobados. No introduce otro Scheduler.

### Fase 3B — Motor concurrente

Responsable: **Ángel**, rama `angel`. Auditar primero código ya existente en `angel`; existen archivos de motor concurrente y pruebas en esa rama, aún no integrados en `master`. Entregables: `concurrent/ConcurrentEngine.h/.cpp`, `ProcessThread.h/.cpp`, `Synchronization.h` y `tests/test_concurrent_basic.cpp`, `test_concurrent_pause.cpp`, `test_concurrent_cancel.cpp`, `test_concurrent_thread_shutdown.cpp`, `test_concurrent_race_safety.cpp`.

Un coordinador consulta al Scheduler y otorga un turno; los workers esperan permiso mediante `std::condition_variable`, ejecutan la ráfaga autorizada y reportan resultado. Proteger snapshots, procesos, memoria, eventos y timeline con mutex; no hacer busy waiting ni mantener el mutex compartido durante esperas que impidan completar un worker o cancelación. Implementar pausa/reanudación, paso según DP-07, cancelación, reinicio, cierre y `join()` de cada thread. Las pruebas deben cubrir alta/validación/quantum/memoria, pausa e inicio, cancelación e interacción RUN/WAIT según DP-06, destrucción y reinicio, y acceso concurrente a snapshots/productor sin carreras.

Comprobar expresamente orden de reingreso/Round Robin, aging, WAIT, cancelación activa, carreras, deadlocks y cierre de workers. El reloj lógico avanza solo según ejecución simulada; intervalos reales de coordinación no son quantum. No actualizar widgets desde workers.

**Gate:** pruebas concurrentes ejecutadas, terminación sin deadlocks/threads huérfanos, un único CPU lógico y equivalencia con el motor lógico en casos aprobados. La rama contiene archivos, pero no se afirma aquí ningún PASS.

<a id="fase-4--gui-y-visualizacion"></a>
## Fase 4 — GUI y visualización

Responsable: **Poma**, rama `poma`. GUI aún no aparecía implementada en las ramas inspeccionadas. Puede avanzar desde temprano con un mock y snapshots de `EstadoSistema`, sin esperar al fin de motores. Las rutas propuestas son `gui/MainWindow.*`, `ProcessTable.*`, `QueueWidget.*`, `CPUWidget.*`, `MemoryWidget.*`, `TimelineWidget.*`, `EventLogWidget.*`, `NewProcessDialog.*` y `main.cpp`; coordinar cualquier cambio de CMake fuera de esta documentación.

Construir controles de modo, quantum, velocidad visual, alta, inicio/pausa/paso/reinicio; tabla, cola READY, CPU, mapa conceptual de memoria, timeline y log. El diálogo propone nombre, tamaño, tiempo de CPU, prioridad y campos E/S opcionales, validando mediante las reglas/contrato vigentes. Mockear la interfaz `IExecutionEngine` completa y fixtures para READY, RUN parcial, retorno de quantum, WAIT, FINISH/CANCEL, memoria múltiple y CPU sin READY. Mantener todas las reglas de scheduling fuera de widgets.

La GUI consume `IExecutionEngine` y `EstadoSistema`; integrar primero motor lógico y después concurrente. Las actualizaciones originadas en worker deben llegar de forma segura al hilo principal (p. ej., signals/slots o consulta con `QTimer`, si se adopta Qt). El temporizador refresca representación, no avanza ni modifica tiempo simulado. El estado RUN transitorio debe poder visualizarse desde snapshots sin alterar el tiempo simulado. Timeline muestra intervalos CPU `[inicio, fin)` con anchura proporcional; memoria muestra ocupantes y espacio disponible sin direcciones físicas. Qt 6, dos motores, cadencia/refresco, escala/retención del timeline se sujetan a DP-08/11 y acuerdos vigentes. Las velocidades visuales `0.25x`, `0.5x`, `1x`, `2x`, `5x` son sugerencias antiguas del equipo, no del docente.

La asignación original no fija nombres obligatorios para tests GUI; tratar estos como escenarios de integración y escoger el mecanismo con el equipo: renderizar snapshots equivalentes del mock y motores; alta válida/rechazo de memoria; cancelación y PID inexistente; pausa/paso/reinicio; RUN transitorio; seguridad del hilo UI; orden y proporción del timeline; mapa conceptual; y velocidad visual independiente del reloj.

**Gate:** aplicación interactiva integrada con los motores disponibles, actualización UI segura, escenarios reproducibles y decisiones Qt/DP-08/11 cerradas para el alcance final. Una pantalla estática o un mock por sí solo no completa la fase.

## Fase 5 — Integración y validación general

Participan los cinco responsables; Poma coordina integración visual sin modificar unilateralmente módulos ajenos. Orden de integración recomendado: modelo/memoria → Scheduler → motor lógico → motor concurrente → GUI; integrar primero el motor lógico en GUI y luego el concurrente. Antes de integrar una dependencia, confirmar que está en `master` o acordar/documentar integración coordinada entre ramas. Comparar ambos motores con las mismas entradas, decisiones y criterio de paso.

La línea base dispone de CMake 3.20+, C++17 y CTest. Cuando se implementen cambios, el PR debe ejecutar configuración, build y pruebas aplicables, por ejemplo:

```powershell
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Esas instrucciones no son resultados de pruebas de este workflow. Si hay pruebas específicas de motores/GUI, documentar comando, configuración, entorno y resultado. Al no haber CI detectado en la línea base, marcar automatización como pendiente hasta que se añada; no inventar checks obligatorios. **No fusionar** mientras existan pruebas fallidas, conflictos sin resolver o contratos incompatibles. El gate se satisface con validación ejecutada y revisada, dependencias disponibles y casos acordados cubiertos; que exista un PR no basta.

## Fase 6 — Demostración

Preparar entradas y pasos reproducibles para alta de procesos, prioridad, empate y rotación, llegada dinámica, carga/rechazo/liberación de memoria, WAIT/retorno, cancelación, ejecución por turnos, timeline y comparación de motores si ambos permanecen en alcance. Explicar la política realmente aprobada. Guardar evidencia adecuada y no presentar mocks, snapshots inventados o datos hardcodeados como ejecución real.

## Workflow de Git y Pull Requests

Para cada cambio de implementación o documentación del equipo:

1. Consultar estado y actualizar referencias remotas (`git fetch origin`); inspeccionar `origin/master` y cambios ya fusionados.
2. Sincronizar la rama propia con cuidado, verificando commits y cambios locales primero. Elegir merge/rebase según coordinación de la rama; no imponer rebase ni force-push en ramas compartidas.
3. Leer asignación, contratos, especificación, casos y decisiones aprobadas pertinentes.
4. Limitarse al módulo y reglas autorizados; coordinar cambios transversales antes de editar interfaces compartidas.
5. Añadir/actualizar pruebas y ejecutar validaciones relevantes; reportar solo resultados realmente observados.
6. Revisar diff, archivos tocados, compatibilidad, conflictos, enlaces/documentación y estado de las decisiones.
7. Crear commits descriptivos y publicar la rama.
8. Abrir PR hacia `master`, declarar dependencias y solicitar revisión.
9. Revisar CI disponible, conflictos y compatibilidad con las dependencias; si no hay CI, reportar validación local y automatización pendiente.
10. Fusionar solo tras revisión/aprobación, gates satisfechos y dependencias integrables. No fusionar con fallos, conflictos pendientes ni interfaces incompatibles.

Cada PR de implementación informa responsable/módulo, issue si existe, requisitos, decisiones aplicadas, archivos, dependencias, pruebas y resultados, capturas/demo cuando apliquen, limitaciones y confirmación de aprobación para cualquier cambio contractual. En un cambio transversal, registrar propuesta, responsables revisores, impacto contractual, versión/migración e integración de módulos afectados. No asumir cambios de API por comentario informal.

## Decisiones que pueden bloquear trabajo

DP-01–DP-11 ya no bloquean por falta de decisión funcional. La migración espera ratificación de las propuestas técnicas/API v2 y sigue los gates G1–G6 de [MIGRACION_CONTRATOS_V2.md](MIGRACION_CONTRATOS_V2.md). El trabajo independiente avanza en paralelo con contratos estables y mocks.

## Fuentes y validación documental

Este workflow enlaza la [especificación funcional](ESPECIFICACION_FUNCIONAL.md), [decisiones pendientes](DECISIONES_PENDIENTES.md), [casos de referencia](CASOS_REFERENCIA.md), [matriz de trazabilidad](MATRIZ_TRAZABILIDAD.md), [contratos v1.0](../contracts/README.md), las cinco asignaciones individuales y las [reglas para agentes](../AGENTS.md). Las reglas académicas se consultan en [REQUISITOS_DOCENTE.md](REQUISITOS_DOCENTE.md); no se publican los documentos fuente.
