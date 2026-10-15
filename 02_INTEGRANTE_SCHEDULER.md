# Integrante 2 — Scheduler y política de planificación

**Responsable:** Brickzon · **Rama:** `brickzon` · **Workflow:** [Fase 2](docs/WORKFLOW_DESARROLLO.md#fase-2--scheduler)

Módulo de planificación de CPU lógico de `ConcurrentSimulator`. Implementa la política vigente acordada por el equipo, desacoplada de la interfaz gráfica y de los motores de ejecución.

## Objetivo y límites

Decidir qué proceso `READY` recibe el CPU lógico y construir el orden de PIDs `READY` que la GUI y los motores representan.
Scheduler no depende de GUI, workers, temporizadores gráficos, hilos del sistema ni del motor concreto. Los motores (`SimulationEngine` y `ConcurrentEngine`) llaman al mismo Scheduler; no duplican su política.

## Decisión vigente del equipo

La política implementada y verificada es Priority Scheduling con desempate por FIFO/orden de llegada y aging:

- Prioridad válida 1–10; 10 es la mayor en la escala actual del equipo.
- Solo se seleccionan procesos en estado `READY`.
- Gana mayor `prioridadActual`.
- En caso de empate en `prioridadActual`, gana el de menor `ordenLlegada` (FIFO).
- Si empatan en `prioridadActual` y `ordenLlegada`, desempata el de menor `pid`.
- Cada intervalo completo de espera `READY` de 2,000 ms suma +1 de prioridad, sin superar 10.
- Al asignar CPU, el Scheduler restaura `prioridadActual` a `prioridadBase` y reinicia `tiempoEspera` a 0 (`alAsignarCPU`).

Todo lo anterior es `DECISION_EQUIPO`, no requisito atribuido al docente. La implementación de aging acumula deltas simulados de manera determinista. Procesos que no están en `READY` (`WAIT`, `RUN`, `FINISH`, `CANCEL`) no acumulan tiempo de espera ni sufren aging.

## Algoritmo docente y precedencia

Las notas académicas (pág. 24) asignan prioridades 1, 2 y 3 por bandas de tamaño con cortes estrictos `A+H` y `A+2H`. En la pág. 25, `ObtenerMaximaPrioridad()` conserva el proceso cuyo valor numérico de prioridad es mayor, estableciendo que **en el algoritmo docente P3 precede a P2 y P1** (`CONFIRMADO_POR_DOCENTE`).

En [DP-01](docs/DECISIONES_PENDIENTES.md#dp-01--política-prioritaria-y-compatibilidad-de-escalas) queda pendiente si el equipo adopta la escala docente 1–3 o define una adaptación formal a la escala contractual 1–10. Mientras no exista ratificación del equipo, se mantiene la escala 1–10 vigente. [DP-02](docs/DECISIONES_PENDIENTES.md#dp-02--conjunto-usado-por-assignpriority-y-momentos-de-recálculo) y [DP-10](docs/DECISIONES_PENDIENTES.md#dp-10--aging-y-prioridad-dinámica-por-tamaño) diferencian el aging por espera de la reclasificación por tamaño: operan con fines distintos y el aging no se suprime.

## Superficie de métodos implementada

Las declaraciones residen en [`scheduler/Scheduler.h`](scheduler/Scheduler.h) y la configuración en [`scheduler/SchedulerConfig.h`](scheduler/SchedulerConfig.h):

- `Scheduler(SchedulerConfig)` y `configuracion()` / `configurar(...)`: lectura y actualización dinámica de parámetros.
- `seleccionarSiguiente(const std::vector<Proceso>&) const noexcept`: devuelve PID ganador o `-1` si no hay READY.
- `ordenarColaPrioridad(const std::vector<Proceso>&) const`: devuelve vector de PIDs READY ordenados por prioridad descendente, `ordenLlegada` ascendente y PID ascendente.
- `aplicarAging(std::vector<Proceso>&, std::int64_t deltaTiempo) const noexcept`: actualiza espera y prioridad READY según configuración.
- `alAsignarCPU(Proceso&) const noexcept`: restaura prioridad base y pone espera a cero.

`SchedulerConfig` contiene `prioridadMinima` (1), `prioridadMaxima` (10), `agingIntervaloMs` (2,000) y `agingIncremento` (1), inicializados desde [`contracts/Configuracion.h`](contracts/Configuracion.h).

## FIFO, Round Robin y trabajo del motor

El Scheduler resuelve empates favoreciendo el menor orden de llegada. La rotación Round Robin no la realiza el Scheduler: cuando un proceso consume su turno de CPU y reingresa a READY, el **motor de ejecución** debe actualizar su `ordenLlegada` (o ticket de turno equivalente) para colocarlo al final de sus pares ([DP-05](docs/DECISIONES_PENDIENTES.md#dp-05--cola-desempate-fifo-y-rotación-round-robin)).

Las pruebas del Scheduler validan este comportamiento asignando el nuevo orden y confirmando la alternancia determinista.

## Entregables y pruebas verificadas

### Archivos del módulo
- [`scheduler/Scheduler.h`](scheduler/Scheduler.h)
- [`scheduler/Scheduler.cpp`](scheduler/Scheduler.cpp)
- [`scheduler/SchedulerConfig.h`](scheduler/SchedulerConfig.h)

### Suites de prueba
1. [`tests/test_scheduler_prioridad.cpp`](tests/test_scheduler_prioridad.cpp):
   - Selección por mayor prioridad (A=8, B=5, C=3 → elige A).
   - Caso de referencia P5 ([docs/CASOS_REFERENCIA.md](docs/CASOS_REFERENCIA.md)).
   - Filtrado estricto de estados no-READY (`WAIT`, `RUN`, `FINISH`, `CANCEL`).
   - Retorno `-1` y vector vacío ante ausencia de procesos `READY`.
   - Coherencia exacta entre `ordenarColaPrioridad()` y `seleccionarSiguiente()`.
2. [`tests/test_scheduler_empate.cpp`](tests/test_scheduler_empate.cpp):
   - Desempate FIFO por `ordenLlegada`.
   - Invarianza ante orden físico de almacenamiento en el vector.
   - Simulación de Round Robin tras actualización de `ordenLlegada` por el motor.
   - Desempate estricto final por PID menor ante misma prioridad y misma llegada.
   - Desempate múltiple determinista en colas de 3 o más procesos empatados.
3. [`tests/test_scheduler_aging.cpp`](tests/test_scheduler_aging.cpp):
   - Incremento de prioridad tras 2,000 ms acumulados en `READY`.
   - Salto múltiple ante deltas grandes en una sola llamada (6,000 ms → +3).
   - Acumulación de deltas fraccionarios que cruzan umbrales de intervalo.
   - Cota superior estricta en `prioridadMaxima` (10).
   - Inmunidad de procesos no-READY ante aging.
   - Restablecimiento de prioridad base y espera tras `alAsignarCPU()`.
   - Alternancia dinámica de selección y reordenamiento de cola por aging (Caso P7).
   - Protección e invariancia ante deltas no positivos ($\le 0$).
4. [`tests/test_scheduler_config.cpp`](tests/test_scheduler_config.cpp):
   - Validación de configuración predeterminada de contratos.
   - Actualización dinámica en caliente (`configurar()`) con nuevos rangos e intervalos.
   - Desactivación de aging mediante incremento 0.
   - Validación de invariantes con `SchedulerConfig::esValida()`.

## Criterios de aceptación (Gate Fase 2)

- [x] Selección determinista considerando únicamente procesos en `READY`.
- [x] Desempates deterministas por `ordenLlegada` ascendente y `PID` ascendente.
- [x] Cola ordenada coherente con el proceso seleccionado para despacho.
- [x] Aging determinista acumulativo por intervalos configurables sin rebasar límites.
- [x] Restablecimiento de prioridad y espera al asignar CPU.
- [x] Cero advertencias con `-Wall -Wextra -Wpedantic` y `/W4`.
- [x] 100% de aserciones superadas (92 comprobaciones en pruebas de scheduler, 252 comprobaciones totales con la suite integrada).
- [x] Sin dependencias de GUI, Qt, threads, mutex ni contratos fuera de v1.0.
