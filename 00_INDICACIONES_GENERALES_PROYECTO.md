# Indicaciones generales de ConcurrentSimulator

> **Instrucción canónica para trabajo v2:** aplique DP-01–DP-11 y los contratos candidatos descritos aquí y en [DECISIONES_APROBADAS.md](docs/DECISIONES_APROBADAS.md), [CONTRATOS_V2.md](docs/CONTRATOS_V2.md) y [MIGRACION_CONTRATOS_V2.md](docs/MIGRACION_CONTRATOS_V2.md). Este diseño aún no está operativo. Las reglas v1 se conservan solo para consumidores existentes y el apartado de contexto histórico; no son tareas pendientes para v2. Castilla aún usa `std::list`; cambiarla pertenece a una fase posterior.


## Propósito y documentos de referencia

ConcurrentSimulator simula la admisión, planificación, memoria y ejecución por turnos de procesos en un CPU lógico. Esta guía conserva la separación entre requisitos docentes, decisiones aprobadas, propuestas técnicas por ratificar y contexto histórico.

Orden recomendado de lectura: [README](README.md), [especificación funcional](docs/ESPECIFICACION_FUNCIONAL.md), [requisitos del docente](docs/REQUISITOS_DOCENTE.md), [decisiones pendientes](docs/DECISIONES_PENDIENTES.md), [matriz de trazabilidad](docs/MATRIZ_TRAZABILIDAD.md), [casos de referencia](docs/CASOS_REFERENCIA.md), [contratos v1.0](contracts/README.md) y la asignación individual 01–05.

Las fuentes académicas son las notas, págs. 23–29, y la explicación del 1 de octubre de 2026, 00:03:59–00:14:22. Se excluyen las secciones ajenas de QR, multiplicación de matrices, speedup y grafos de dependencias. Los documentos académicos se mantienen fuera del repositorio.

Toda regla relevante debe identificarse como `CONFIRMADO_POR_DOCENTE`, `PROPUESTA_DOCENTE`, `DECISION_EQUIPO` o `PENDIENTE_CONFIRMACION`, con fuente y página/minuto. Las reglas v2 aprobadas rigen el diseño destino; las firmas concretas siguen sujetas a ratificación de la API candidata. La documentación canónica define el contenido de estas etiquetas.

## Alcance funcional

### Procesos y estados

Las notas enumeran ID, nombre, tamaño en bytes, estado, cuota en milisegundos y prioridad. En el destino v2 `AltaProceso` no recibe prioridad, cuota ni ticket; el proceso almacena prioridad automática y la cuota vigente se deriva con `cuotaMs(prioridad)`. La cuota del turno se fija separadamente al despacho. El contrato raíz v1.0 sigue para código existente, sin cambios en esta fase.

Estados: `READY`, `RUN`, `WAIT`, `FINISH` y `CANCEL`. El diagrama docente de la pág. 27 dibuja READY→RUN, RUN→WAIT/FINISH/CANCEL y WAIT→READY; cada flecha confirma que esa transición está representada, sin fijar disparador ni duración. La pág. 28 explica que, tras pasar el tiempo asignado a un proceso, entra otro; no especifica el estado del proceso anterior ni dibuja RUN→READY. La matriz con procedencia de cada transición está en la [especificación funcional](docs/ESPECIFICACION_FUNCIONAL.md); una flecha ausente no se considera prohibición.

### Política y unidades de tiempo

**Contrato v1.0 histórico y todavía vigente para código existente:** prioridades 1–10, quantum configurable 500 ms y aging 2,000 ms. Para el destino v2 estas políticas fueron sustituidas por DP-01, DP-04 y DP-10: consulte la tabla aprobada arriba. No atribuir las reglas v1 al docente.

**Base docente adoptada por el equipo para v2:** `AssignPriority()` calcula las bandas estrictas P1/P2/P3 (notas, pág. 24); DP-01/03 formalizan precedencia y fórmula exacta. DP-02 define TL y cuándo publicar reclasificación. Consulte [DECISIONES_APROBADAS.md](docs/DECISIONES_APROBADAS.md); esas decisiones ya no están pendientes.

Las notas listan cuota por proceso (pág. 23). El ejemplo de la transcripción `prioridad × 10 ms` (00:11:50–00:12:07) no era requisito docente; DP-04 lo adopta como decisión fija del equipo para v2: máximo 10/20/30 ms. El quantum global v1 sigue solo para compatibilidad del código anterior.

**Distinciones necesarias:** el Scheduler común compara prioridad descendente, ticket ascendente y PID ascendente; el motor asigna ticket nuevo al reingresar READY tras WAIT/cuota. La observación de `ordenLlegada` y quantum pertenece a auditoría histórica v1, no modifica las reglas ya aprobadas para v2.

### Avance temporal y salidas

El tiempo del modelo usa milisegundos simulados, independientes del tiempo físico y de animación. Por DP-07, `siguientePaso()` exige PAUSADO y representa una ráfaga hasta cuota/WAIT/FINISH/CANCEL; sin READY pero con WAIT avanza idle hasta el despertar más próximo y difiere el despacho al siguiente paso. El modo continuo itera esa misma unidad. Velocidades visuales son opcionales y no alteran reloj/cuota.

Registrar eventos de admisión, despacho, reclasificación, cuota, WAIT/retorno, FINISH, CANCEL e idle conforme a v2; no hay evento de aging activo. RUN→READY por cuota es decisión de equipo para v2, no se atribuye a una flecha docente. La GUI reproduce RUN transitorio desde snapshots/eventos sin avanzar reloj. Timeline muestra CPU atribuido y hueco idle sin PID ficticio. Ambos motores se comparan con los mismos pasos y resultados observables.

## Requisitos visuales

La pantalla integra controles, tabla de procesos, cola READY con prioridades, CPU lógico, mapa conceptual de memoria, timeline, registro de eventos y estado del simulador. Las notas muestran atributos del proceso (pág. 23), el flujo proceso/cola/memoria/procesador (pág. 29), y la comparación visual de serial frente a time-sharing (pág. 28).

El timeline muestra bloques proporcionales solo para CPU realmente ejecutado por PID positivo. Idle son huecos inferidos desde 0 hasta el tiempo del snapshot, antes/entre/después de bloques, sin asignar PID ni crear bloques idle. Historial abarca la sesión completa con scroll/zoom; la representación eficiente no descarta eventos. La memoria muestra ocupación proporcional por PID y espacio libre, sin direcciones inventadas.

DP-11 aprueba Qt 6 + C++17 + CMake como decisión de equipo, no como requisito atribuido al docente. Los controles no exponen quantum global configurable ni prioridad manual.

## Arquitectura y contratos

**Decisión de equipo aprobada:** mantener un motor lógico sin threads reales y un motor concurrente con threads reales; ambos implementan la misma interfaz y Scheduler, y representan un CPU lógico único. Los workers no equivalen a núcleos simulados.

La GUI depende de una interfaz común candidata `IExecutionEngine` y snapshots `EstadoSistema`; no selecciona prioridades ni toca widgets desde workers. Los headers raíz `contracts/*.h` siguen como API v1.0 del código actual; `contracts/v2/` contiene candidatos no operativos hasta ratificación e integración.

## Responsabilidades y rutas

| Integrante | Responsable | Rama de trabajo | Entregables principales | Límites |
|---|---|---|---|---|
| 1. Modelo y memoria | Castilla | `castilla` | `contracts/`; `core/ColaProcesos.h/.cpp`; `core/Memoria.h/.cpp`; pruebas de proceso, cola y memoria | No implementa scheduler, threads ni GUI |
| 2. Scheduler | Brickzon | `brickzon` | `scheduler/Scheduler.h/.cpp`; `scheduler/SchedulerConfig.h`; pruebas de prioridad, empate y tickets; revisar/eliminar test v1 de aging | No implementa GUI, threads ni timers visuales |
| 3. Motor lógico | Aldana | `aldana` | `simulation/SimulationEngine.h/.cpp`; `simulation/SimulationClock.h/.cpp`; cinco pruebas de simulación | No implementa GUI, workers ni política propia de prioridad |
| 4. Motor concurrente | Ángel | `angel` | `concurrent/ConcurrentEngine.h/.cpp`; `ProcessThread.h/.cpp`; `Synchronization.h`; cinco pruebas concurrentes | No duplica Scheduler ni actualiza Qt |
| 5. GUI e integración | Poma | `poma` | `gui/MainWindow.*`, tabla, cola, CPU, memoria, timeline, log y diálogo de alta; `main.cpp` | No decide política de scheduling ni sincroniza internamente los workers |

Las ramas identifican espacios de trabajo, no usuarios personales de GitHub. El [workflow canónico](docs/WORKFLOW_DESARROLLO.md) explica fases, dependencias, trabajo paralelo, gates y PRs. Esta tabla indica ownership funcional; no certifica que los entregables estén implementados o probados.

## Workflow de desarrollo

La fase de decisiones está cerrada. Ratifica las propuestas técnicas que afectan el detalle concurrente/API, migra primero modelo y Scheduler, desarrolla ambos motores en paralelo con contrato estable y permite que GUI avance con un mock. Consulta [docs/WORKFLOW_DESARROLLO.md](docs/WORKFLOW_DESARROLLO.md) para gates.

Para código integrado rige v1.0. Para diseño futuro y trabajo v2 rigen DP-01–DP-11 y los contratos candidatos, sin tratarlos como API operativa antes de ratificación. Las alternativas de `DECISIONES_PENDIENTES.md` son deliberación histórica; no son tareas de decisión aún abiertas.

## Dependencias entre módulos

- Modelo y contratos son base de Scheduler, motores y GUI. Mantener firmas estables.
- Scheduler depende del contrato `Proceso`; ambos motores reutilizan su selector y configuración.
- El motor lógico depende de contratos, modelo y Scheduler; puede usar dobles de prueba temporales mientras se implementan esas dependencias.
- El motor concurrente depende de contratos, modelo y Scheduler; no necesita GUI para pruebas.
- GUI puede comenzar con un mock de `IExecutionEngine` y snapshots de `EstadoSistema`; luego integra primero el motor lógico y después el concurrente.

## Secuencia de integración sugerida por el equipo

1. Preservar v1.0; revisar/adoptar contratos candidatos v2 y ratificar propuestas.
2. Migrar modelo, validaciones, lista enlazada y memoria.
3. Migrar Scheduler y escenarios aislados sin aging.
4. Construir GUI inicial con mock para trabajar en paralelo.
5. Integrar y comparar primero el motor lógico.
6. Integrar el motor concurrente y verificar sincronización/terminación.
7. Comparar ambos motores con los mismos escenarios aprobados.
8. Completar integración GUI, demo y validación del equipo.

La secuencia prepara migración; añadir headers candidatos por sí solo no hace la API operativa.

## Escenarios mínimos de demostración del equipo

| Caso | Entrada | Resultado esperado bajo la política destino v2 |
|---|---|---|
| A. Prioridad/cuota | Tamaños clasifican P1/P2/P3 | Clase automática; cuota máxima 10/20/30 ms |
| B. Empate/RR | READY de igual prioridad; un proceso agota cuota | Menor ticket primero; al reingresar recibe ticket nuevo |
| C. Espera | Proceso pasa RUN→WAIT | Al despertar entra READY con ticket nuevo; CPU idle aparece como hueco inferido |
| D. Alta dinámica | Alta válida durante RUN / alta rechazada | RUN y cuota fijada continúan; rechazo no muta TL/prioridades |
| E. Cancelación | READY, RUN o WAIT | CANCEL terminal; contabilizar solo CPU simulada ejecutada |
| F. Motores | Misma entrada y secuencia de pasos | Igual reloj, estados, eventos y timeline entre motores |

Los casos reproducibles completos están en [CASOS_REFERENCIA.md](docs/CASOS_REFERENCIA.md).

## Criterio de cierre del proyecto

- Aplicación GUI integrada con lista, cola, CPU, memoria, timeline, log y controles.
- Altas, cancelaciones, pausa, reinicio y paso de acuerdo con contrato y decisiones ratificadas.
- Prioridad/cuota, estados, E/S y tiempos coherentes; no aging activo.
- Los dos motores comparten interfaz y Scheduler y dan resultados equivalentes con sincronización segura.
- Evidencia académica separada de decisiones del equipo y de propuestas técnicas por ratificar.
- Compilación y pruebas reproducibles documentadas para la entrega final. Esta guía no afirma que se hayan ejecutado durante la auditoría documental.
