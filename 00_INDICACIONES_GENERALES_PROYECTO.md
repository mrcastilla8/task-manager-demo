# Indicaciones generales de ConcurrentSimulator

> **Adenda Fase 0 (2026-10-15):** DP-01–DP-11 definen el destino v2; no alteran API v1 ni implementación existente. La política futura es P1–P3 automática por tamaño, cuota 10/20/30 ms, READY por prioridad/ticket/PID, sin aging; se conservan ambos motores sobre CPU lógico único y Qt 6/C++17/CMake. Consulte [decisiones](docs/DECISIONES_APROBADAS.md), [contratos v2 candidatos](docs/CONTRATOS_V2.md) y [migración](docs/MIGRACION_CONTRATOS_V2.md). El texto previo de quantum 500 ms/aging/prioridad manual describe v1 o antecedentes, no política activa v2. La rama de Castilla aún usa `std::list`; sustituirla es futuro trabajo, no realizado en Fase 0.


## Propósito y documentos de referencia

ConcurrentSimulator simula la admisión, planificación, memoria y ejecución por turnos de procesos en un CPU lógico. Esta guía recupera el detalle de coordinación del proyecto original y conserva la separación entre requisitos docentes, decisiones de equipo, propuestas y decisiones pendientes.

Orden recomendado de lectura: [README](README.md), [especificación funcional](docs/ESPECIFICACION_FUNCIONAL.md), [requisitos del docente](docs/REQUISITOS_DOCENTE.md), [decisiones pendientes](docs/DECISIONES_PENDIENTES.md), [matriz de trazabilidad](docs/MATRIZ_TRAZABILIDAD.md), [casos de referencia](docs/CASOS_REFERENCIA.md), [contratos v1.0](contracts/README.md) y la asignación individual 01–05.

Las fuentes académicas son las notas, págs. 23–29, y la explicación del 1 de octubre de 2026, 00:03:59–00:14:22. Se excluyen las secciones ajenas de QR, multiplicación de matrices, speedup y grafos de dependencias. Los documentos académicos se mantienen fuera del repositorio.

Toda regla relevante debe identificarse como `CONFIRMADO_POR_DOCENTE`, `PROPUESTA_DOCENTE`, `DECISION_EQUIPO` o `PENDIENTE_CONFIRMACION`, con fuente y página/minuto. La documentación canónica define el contenido de estas etiquetas.

## Alcance funcional

### Procesos y estados

Las notas enumeran ID, nombre, tamaño en bytes, estado, cuota en milisegundos y prioridad. El contrato actual añade prioridad base/actual, tiempos total/restante/espera, quantum consumido, orden de llegada y operación opcional de E/S. No agregues campos ni cambies firmas del contrato congelado v1.0 sin una migración aprobada.

Estados: `READY`, `RUN`, `WAIT`, `FINISH` y `CANCEL`. El diagrama docente de la pág. 27 dibuja READY→RUN, RUN→WAIT/FINISH/CANCEL y WAIT→READY; cada flecha confirma que esa transición está representada, sin fijar disparador ni duración. La pág. 28 explica que, tras pasar el tiempo asignado a un proceso, entra otro; no especifica el estado del proceso anterior ni dibuja RUN→READY. La matriz con procedencia de cada transición está en la [especificación funcional](docs/ESPECIFICACION_FUNCIONAL.md); una flecha ausente no se considera prohibición.

### Política y unidades de tiempo

**Contrato v1.0 histórico y todavía vigente para código existente:** prioridades 1–10, quantum configurable 500 ms y aging 2,000 ms. Para el destino v2 estas políticas fueron sustituidas por DP-01, DP-04 y DP-10: consulte la tabla aprobada arriba. No atribuir las reglas v1 al docente.

**Base docente adoptada por el equipo para v2:** `AssignPriority()` calcula las bandas estrictas P1/P2/P3 (notas, pág. 24); DP-01/03 formalizan precedencia y fórmula exacta. DP-02 define TL y cuándo publicar reclasificación. Consulte [DECISIONES_APROBADAS.md](docs/DECISIONES_APROBADAS.md); esas decisiones ya no están pendientes.

Las notas listan cuota por proceso (pág. 23). El ejemplo de la transcripción `prioridad × 10 ms` (00:11:50–00:12:07) no era requisito docente; DP-04 lo adopta como decisión fija del equipo para v2: máximo 10/20/30 ms. El quantum global v1 sigue solo para compatibilidad del código anterior.

**Distinciones necesarias:** FIFO es orden de llegada; prioridad elige qué clase/proceso READY gana; FIFO desempata procesos iguales; Round Robin describe la rotación al reingresar después de un turno. El Scheduler compara `ordenLlegada`, pero el motor concurrente revisado no actualiza ese campo al agotar quantum. El motor es quien tendría que asignar el nuevo orden de READY según una regla que el equipo apruebe (DP-05); no corrijas código en tareas documentales.

### Avance temporal y salidas

El contrato expresa el tiempo simulado en milisegundos. Los conceptos de tiempo simulado y demora visual son independientes. Como sugerencia de equipo, los documentos originales mencionan quantum de 500 ms y un paso visual de 800 ms reales; la unidad exacta de `siguientePaso()` sigue pendiente (DP-07). Velocidades visuales sugeridas por el equipo: `0.25x`, `0.5x`, `1x`, `2x` y `5x`; no son requisitos del profesor.

Registrar eventos de admisión, despacho, reclasificación, cuota, WAIT/retorno, FINISH, CANCEL e idle conforme a v2; no hay evento de aging activo. RUN→READY por cuota es decisión de equipo para v2, no se atribuye a una flecha docente. La GUI reproduce RUN transitorio desde snapshots/eventos sin avanzar reloj. Timeline muestra CPU atribuido y hueco idle sin PID ficticio. Ambos motores se comparan con los mismos pasos y resultados observables.

## Requisitos visuales

La pantalla integra controles, tabla de procesos, cola READY con prioridades, CPU lógico, mapa conceptual de memoria, timeline, registro de eventos y estado del simulador. Las notas muestran atributos del proceso (pág. 23), el flujo proceso/cola/memoria/procesador (pág. 29), y la comparación visual de serial frente a time-sharing (pág. 28).

El timeline es una vista gráfica de intervalos sobre eje temporal: cada segmento representa una ráfaga de CPU y tiene anchura proporcional a su duración. Es distinto de la tabla, la cola, el log y cualquier archivo exportado; no se solicita exportar PNG, HTML o PDF. Escala, CPU idle, retención y desplazamiento están pendientes (DP-08). La memoria debe mostrar procesos cargados y proporción de ocupación, no solo dos totales; no se presuponen direcciones o administración avanzada.

El documento de Poma recomienda Qt 6. Es una recomendación del equipo, no un requisito docente ni una dependencia ratificada. Las velocidades indicadas arriba también son sugerencias del equipo.

## Arquitectura y contratos

**Decisión de equipo vigente:** mantener un motor lógico sin threads reales y un motor concurrente con `std::thread`, mutex y variables de condición. Ambos representan un solo CPU lógico; los workers reales no equivalen a núcleos simulados. La explicación docente de time-sharing no obliga por sí sola a implementar dos motores.

La GUI depende únicamente de `IExecutionEngine` y snapshots `EstadoSistema`; no selecciona prioridades ni toca widgets desde workers. Los contratos compartidos están en `contracts/Proceso.h`, `Estado.h`, `Configuracion.h`, `EstadoSistema.h`, `Evento.h` e `IExecutionEngine.h`. La API común v1.0 está congelada: no redefinirla con ejemplos copiados de documentos antiguos.

## Responsabilidades y rutas

| Integrante | Responsable | Rama de trabajo | Entregables principales | Límites |
|---|---|---|---|---|
| 1. Modelo y memoria | Castilla | `castilla` | `contracts/`; `core/ColaProcesos.h/.cpp`; `core/Memoria.h/.cpp`; pruebas de proceso, cola y memoria | No implementa scheduler, threads ni GUI |
| 2. Scheduler | Brickzon | `brickzon` | `scheduler/Scheduler.h/.cpp`; `scheduler/SchedulerConfig.h`; pruebas de prioridad, empate y aging | No implementa GUI, threads ni timers visuales |
| 3. Motor lógico | Aldana | `aldana` | `simulation/SimulationEngine.h/.cpp`; `simulation/SimulationClock.h/.cpp`; cinco pruebas de simulación | No implementa GUI, workers ni política propia de prioridad |
| 4. Motor concurrente | Ángel | `angel` | `concurrent/ConcurrentEngine.h/.cpp`; `ProcessThread.h/.cpp`; `Synchronization.h`; cinco pruebas concurrentes | No duplica Scheduler ni actualiza Qt |
| 5. GUI e integración | Poma | `poma` | `gui/MainWindow.*`, tabla, cola, CPU, memoria, timeline, log y diálogo de alta; `main.cpp` | No decide política de scheduling ni sincroniza internamente los workers |

Las ramas identifican espacios de trabajo, no usuarios personales de GitHub. El [workflow canónico](docs/WORKFLOW_DESARROLLO.md) explica fases, dependencias, trabajo paralelo, gates y PRs. Esta tabla indica ownership funcional; no certifica que los entregables estén implementados o probados.

## Workflow de desarrollo

Resuelve primero las decisiones que bloquean reglas compartidas, completa modelo y Scheduler, desarrolla ambos motores en paralelo cuando contratos y política estén estables, y permite que GUI avance temprano con un mock. Consulta [docs/WORKFLOW_DESARROLLO.md](docs/WORKFLOW_DESARROLLO.md) para el proceso completo de implementación, integración, validación y demostración.

Los contratos v1.0 y sus unidades/errores prevalecen sobre snippets antiguos. Si una firma de la asignación discrepa, enlaza el encabezado actual y registra la necesidad como migración pendiente en lugar de publicar otra API.

## Dependencias entre módulos

- Modelo y contratos son base de Scheduler, motores y GUI. Mantener firmas estables.
- Scheduler depende del contrato `Proceso`; ambos motores reutilizan su selector y configuración.
- El motor lógico depende de contratos, modelo y Scheduler; puede usar dobles de prueba temporales mientras se implementan esas dependencias.
- El motor concurrente depende de contratos, modelo y Scheduler; no necesita GUI para pruebas.
- GUI puede comenzar con un mock de `IExecutionEngine` y snapshots de `EstadoSistema`; luego integra primero el motor lógico y después el concurrente.

## Secuencia de integración sugerida por el equipo

1. Congelar contratos v1.0 y fijar responsables.
2. Completar modelo, cola y memoria con pruebas.
3. Desarrollar Scheduler y escenarios aislados.
4. Construir GUI inicial con mock para trabajar en paralelo.
5. Integrar y comparar primero el motor lógico.
6. Integrar el motor concurrente y verificar sincronización/terminación.
7. Comparar ambos motores con los mismos escenarios aprobados.
8. Completar integración GUI, demo y validación del equipo.

La secuencia no resuelve decisiones pendientes ni autoriza cambios de API.

## Escenarios mínimos de demostración del equipo

| Caso | Entrada | Resultado esperado bajo la política vigente o pendiente |
|---|---|---|
| A. Mayor prioridad | A P8, B P5, C P3 | A primero si permanece la escala de equipo 1–10 |
| B. Empate | A y B P8, órdenes de llegada distintos | FIFO al elegir inicialmente; RR solo si el motor actualiza el orden al reingresar |
| C. Aging | Proceso de menor prioridad espera varios intervalos | Aumenta +1 por 2,000 ms, hasta 10, según decisión de equipo |
| D. WAIT | Proceso tiene operación E/S configurada | RUN→WAIT y luego READY según política del motor; fuente académica no fija tiempos |
| E. Alta dinámica | Agregar un proceso durante ejecución | Visible en lista/READY si es admitido; recálculo por rango pendiente |
| F. Cancelación | Usuario cancela READY, RUN o WAIT | Reflejar CANCEL según comportamiento actual; efecto de cancelar RUN debe aprobarse |
| G. Motores | Misma configuración y entradas en lógico y concurrente | Snapshots, reloj, eventos y timeline equivalentes bajo semántica de paso aprobada |

Los casos reproducibles completos están en [CASOS_REFERENCIA.md](docs/CASOS_REFERENCIA.md).

## Criterio de cierre del proyecto

- Aplicación GUI integrada con lista, cola, CPU, memoria, timeline, log y controles.
- Altas, cancelaciones, pausa, reinicio y paso de acuerdo con contrato y decisiones ratificadas.
- Prioridad, aging, estados, E/S y tiempos coherentes entre módulos.
- Si se mantienen dos motores, resultados comparables con escenarios comunes y sincronización segura en el concurrente.
- Requisitos académicos distinguidos de propuestas y políticas del equipo; decisiones pendientes resueltas antes de afirmar conformidad.
- Compilación y pruebas reproducibles documentadas para la entrega final. Esta guía no afirma que se hayan ejecutado durante la auditoría documental.
