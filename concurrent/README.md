# Módulo de Concurrencia — Integrante 4

Este módulo implementa el motor de ejecución concurrente (`ConcurrentEngine`) con hilos reales (`std::thread`), variables de condición (`std::condition_variable`) y exclusión mutua (`std::mutex`), cumpliendo con el contrato `IExecutionEngine`.

---

## 1. Arquitectura y Principio de Funcionamiento

Los threads reales del sistema operativo **no reemplazan la política de planificación**:
1. El `Scheduler` (Integrante 2) analiza la cola de procesos listos (`READY`) y selecciona qué PID debe ejecutarse.
2. El `ConcurrentEngine` administra la ejecución concediendo permisos lógicos de CPU al hilo worker del proceso elegido.
3. Cada proceso admitido tiene su propio hilo de ejecución (`ProcessThread`).
4. Los procesos que no tienen turno se suspenden mediante `std::condition_variable::wait`, garantizando **0% busy waiting**.

---

## 2. Prevención de Condiciones de Carrera (Race Conditions) y Sincronización

Conforme a la especificación de diseño del proyecto, se detallan las garantías de concurrencia:

### 2.1 Datos compartidos y mecanismos de protección

| Recurso compartido | Mecanismo de protección | Propósito |
|---|---|---|
| `ColaProcesos colaProcesos_` | `mutexEstado_` (`std::mutex`) | Almacenamiento dinámico de procesos, estados, contadores y prioridades. |
| `Memoria memoria_` | `mutexEstado_` | Control de espacio disponible y asignado en bytes. |
| `PlanificadorMotor planificador_` | `mutexEstado_` | Adaptador sin estado propio hacia el Scheduler compartido: selección y orden de la cola READY. |
| `std::unordered_map workers_` | `mutexEstado_` | Registro de hilos trabajadores asociados a cada PID. |
| `std::int64_t tiempoSimulado_` | `mutexEstado_` | Reloj lógico global del sistema. |
| `EstadoEjecucion estadoEjecucion_` | `mutexEstado_` | Control de estados (`DETENIDO`, `EJECUTANDO`, `PAUSADO`, `FINALIZADO`). |
| `std::vector<Evento> eventos_` | `mutexEstado_` | Historial cronológico de eventos para la GUI. |
| `std::vector<BloqueTimeline> timeline_` | `mutexEstado_` | Historial de asignaciones de CPU `[inicio, fin)`. |
| Estado interno de `ProcessThread` | `ProcessThread::mutex_` | Control atómico de turnos, ráfagas y señales de terminación del worker. |

### 2.2 Variables de condición y condiciones de despertar

1. **`cvTurno_` (en `ProcessThread`)**:
   - Condición: `permisoEjecucion_ || detener_`.
   - Propósito: Suspende el hilo del proceso en `std::condition_variable::wait` mientras está en `READY` o `WAIT`. Se despierta únicamente cuando el motor le otorga el turno o cuando se solicita la terminación del hilo.
2. **`cvFinTurno_` (en `ProcessThread`)**:
   - Condición: `turnoCompletado_ || detener_`.
   - Propósito: Permite al despachador del motor esperar a que el worker complete su quantum o ráfaga de CPU sin busy waiting.
3. **`cvCoordinador_` (en `ConcurrentEngine`)**:
   - Condición: `(estadoEjecucion_ == EstadoEjecucion::EJECUTANDO) || destruirCoordinador_`.
   - Propósito: Suspende el hilo coordinador del motor cuando se llama a `pausar()` o cuando no hay trabajo activo.

### 2.3 Garantía de CPU exclusivo (Un único proceso en RUN)

- El scheduler selecciona **un solo PID** a la vez.
- Se invoca `worker->concederTurno(solicitud)` únicamente para el `ProcessThread` seleccionado.
- El despachador aguarda mediante `worker->esperarFinTurno()` antes de considerar la siguiente asignación de CPU.
- Ningún otro worker puede avanzar porque sus variables `permisoEjecucion_` permanecen en `false` y están bloqueados en `cvTurno_`.

---

## 3. Seguridad de Parada y Liberación de Hilos (Safe Shutdown)

- En el destructor `~ConcurrentEngine()` y en `reiniciar()`:
  1. Se actualiza la bandera de detención bajo mutex.
  2. Se envía notificación broadcast a todas las variables de condición (`notify_all`).
  3. Se ejecuta `join()` sobre cada hilo activo (`worker_.joinable()`).
  4. Se garantiza ausencia total de hilos huérfanos (*orphan threads*), deadlocks o fugas de memoria.

---

### 3.1 Garantías de construcción y de snapshot

- `ProcessThread` declara el `std::thread` como **último miembro**: el hilo arranca al construirlo y necesita que el mutex, las variables de condición y las banderas ya existan.
- `intervaloCoordinadorMs_` es `std::atomic<int>`: se configura desde cualquier hilo mientras el coordinador lo lee.
- Al asignar CPU, el motor recalcula `colaPrioridad_` **antes** de liberar `mutexEstado_` para la ráfaga. Durante el turno, un lector ve `pidEjecutando` con el proceso en RUN y una cola que contiene solo procesos READY.
- Cada snapshot de `obtenerEstado()` cumple: a lo sumo un proceso RUN y coincide con `pidEjecutando`; `memoriaUsada` es la suma de tamaños de procesos no terminados; el timeline está ordenado, sin solapes y no supera el reloj; los eventos no retroceden en el tiempo.
- Una única unidad lógica a la vez: `siguientePaso()`, el coordinador (`iniciar()`) y `reiniciar()` se serializan con `pasoEnCurso_` y `reiniciando_`. El coordinador solo inicia un paso si no hay otro en curso; `siguientePaso()` espera a que cierre el paso en curso y vuelve a validar que el motor siga PAUSADO; `pausar()` no retorna hasta cerrar la ráfaga.
- `reiniciar()` limpia todo el estado sin soltar el mutex, retira los workers a una variable local y hace el `join` fuera del mutex. Ningún paso puede usar un worker que se está retirando.
- Los hilos (`ProcessThread::worker_` y `ConcurrentEngine::hiloCoordinador_`) son el **último miembro** de su clase: arrancan al construirse y leen los demás miembros.
- Cancelar un proceso en RUN no descarta la ráfaga: queda como cancelación pendiente y se resuelve al cerrarla, tras contabilizar el CPU completado (bloque de timeline, reloj y tiempo restante). Si el proceso termina en ese mismo límite, FINISH gana.
- Cada ingreso a READY (alta, retorno de WAIT, fin de cuota) recibe un ticket estrictamente creciente en `Proceso::ordenLlegada`. El Scheduler desempata por ese campo, lo que produce Round Robin entre prioridades iguales. Los procesos que despiertan en el mismo turno reingresan por instante de despertar, ticket de bloqueo y PID.
- `configurarRetardoRafagaPruebasMs()` existe **solo para pruebas**: duerme de forma interrumpible dentro del worker para ensanchar la ventana de la ráfaga. Está desactivado por defecto y no altera el tiempo simulado.
- El alta es **transaccional**: el worker, la reserva de memoria, la inserción, el cálculo de la cola y los eventos se preparan antes de confirmar y se deshacen en orden inverso si algo lanza. Un fallo no deja memoria reservada, procesos sin worker, workers sin proceso ni tickets consumidos. Además, `ejecutarPasoInterno` localiza el worker **antes** de marcar RUN, de modo que un proceso nunca queda en RUN sin hilo.
- Todo acceso al Scheduler compartido pasa por `PlanificadorMotor` (`concurrent/PlanificadorMotor.*`), el único archivo del módulo que incluye `scheduler/`. No contiene política propia.
- **Sin aging** (DP-10): la espera se acumula en `tiempoEspera` como métrica y nunca modifica la prioridad. Consecuencia aceptada (propuesta técnica 8): un proceso de baja prioridad puede esperar indefinidamente mientras existan READY de mayor prioridad.
- La cuota del turno se decide en un único punto (`cuotaParaDespachoInterno`) y se copia a la solicitud al despachar; cambiarla durante una ráfaga solo afecta al siguiente despacho.
- El contador de tickets no desborda: el valor máximo nunca se asigna (misma regla que `AsignadorTickets` de Brickzon). El alta y cada paso comprueban **antes de modificar nada** que hay tickets suficientes (1 por el proceso en ejecución más 1 por cada proceso en WAIT). Si no los hay, lanzan `std::overflow_error` sin cambiar el estado; en ejecución continua el coordinador pausa el motor en lugar de terminar, y `reiniciar()` restablece el contador.
- Solo para pruebas, desactivados por defecto: `configurarRetardoRafagaPruebasMs`, `configurarFalloAltaPruebas` y `fijarSiguienteTicketPruebas`.

---

## 4. Pruebas Automatizadas

El módulo incluye las 5 suites de pruebas requeridas y suites adicionales de escenarios, exclusión de pasos, Round Robin y cancelación en RUN:

1. `test_concurrent_basic.cpp`: Validación de admisibilidad, errores, inicialización y ejecución paso a paso con workers reales.
2. `test_concurrent_pause.cpp`: Idempotencia de `iniciar()` y `pausar()`, suspensión en condición variable y control de `siguientePaso()`.
3. `test_concurrent_cancel.cpp`: Cancelación idempotente en estados `READY`, `WAIT` y `FINISH`, con liberación de memoria y worker.
4. `test_concurrent_thread_shutdown.cpp`: Destrucción segura RAII, detención de workers y reinicio conservando configuración.
5. `test_concurrent_race_safety.cpp`: Prueba bajo estrés con múltiples hilos lectores llamando a `obtenerEstado()` concurrentemente mientras un hilo productor inserta procesos en tiempo real. Verifica en cada snapshot las invariantes de la sección 3.1.
6. `test_concurrent_scenarios.cpp`: Ciclo de E/S `RUN→WAIT→READY→FINISH` con secuencia exacta de eventos y bloques de timeline, ausencia de aging (la espera no cambia la prioridad) y ejecución continua con `iniciar()` hasta `FINALIZADO`.
7. `test_concurrent_step_exclusion.cpp`: Varios hilos llaman a `siguientePaso()`, `iniciar()`, `pausar()` y `reiniciar()` a la vez mientras un monitor valida que nunca haya dos procesos en RUN. Usa el retardo de pruebas para que la carrera sea reproducible.
8. `test_concurrent_round_robin.cpp`: Alternancia entre prioridades iguales, ticket de alta asignado por el motor, reingreso por cuota y retorno de WAIT, y orden determinista de varios despertares en el mismo turno.
9. `test_concurrent_cancel_run.cpp`: Cancelación con la ráfaga en curso: conserva el CPU completado, un único evento, FINISH gana en el mismo límite, cancelación que llega con la ráfaga terminando en WAIT, reinicio durante la ráfaga y cancelación en ejecución continua.

Las pruebas de carrera se verifican también por mutación: sin la corrección correspondiente, `test_concurrent_step_exclusion`, `test_concurrent_round_robin` y `test_concurrent_cancel_run` fallan.
10. `test_concurrent_alta_rollback.cpp`: Fallo inyectado en cada etapa del alta (worker, inserción, registro, cola, eventos): sin memoria reservada, procesos ni workers a medias, sin consumir ticket, con el motor aún utilizable y en ejecución continua.
11. `test_concurrent_cuota.cpp`: La cuota se fija al despachar; cambiarla durante una ráfaga solo afecta al siguiente despacho.
12. `test_concurrent_tickets.cpp`: Agotamiento del contador en el alta y en el paso (incluido el conteo de despertares de WAIT), sin cambios de estado, pausa en ejecución continua y recuperación con `reiniciar()`.

Las pruebas de carrera y de rollback se verifican también por mutación: sin la corrección correspondiente, `test_concurrent_step_exclusion`, `test_concurrent_round_robin`, `test_concurrent_cancel_run`, `test_concurrent_alta_rollback` y `test_concurrent_tickets` fallan.

---

## 5. Decisiones aprobadas y estado en este módulo

DP-01 a DP-11 constan como **aprobadas** en [`docs/DECISIONES_APROBADAS.md`](../docs/DECISIONES_APROBADAS.md). Los contratos `contracts/v2/` son candidatos y los detalles técnicos señalados como "no ratificados" en [`docs/PROPUESTAS_TECNICAS_V2.md`](../docs/PROPUESTAS_TECNICAS_V2.md) siguen pendientes (gate G1). El motor sigue sobre el contrato **v1.0**.

| DP | Estado en el motor | Pendiente |
|---|---|---|
| DP-05 (tickets, Round Robin) | Implementado sobre v1: `ordenLlegada` actúa como ticket en alta, retorno de WAIT y fin de cuota. Contador con límite (propuesta técnica 11) y `std::overflow_error` sin cambios de estado. | Orden de varios despertares simultáneos (propuesta técnica 1, no ratificada); política final ante el agotamiento (hoy: el alta falla y el motor se pausa). |
| DP-06 (cancelación) | READY y WAIT cancelan al instante; RUN se cancela al cerrar la ráfaga conservando el CPU completado. Memoria y evento se emiten una vez. | Protocolo técnico: granularidad de checkpoint (1 ms propuesto), precedencia FINISH/CANCEL/cuota agotada en el mismo límite y si `cancelarProceso` es bloqueante o no. Hoy no bloquea y FINISH gana. **No se implementan checkpoints arbitrarios antes de ratificar.** |
| DP-07 (paso) | `siguientePaso()` ejecuta un turno completo (cuota, E/S o fin) o, sin READY y con WAIT, avanza el reloj hasta el próximo despertar sin despachar. | Driver de ejecución continua (G1). |
| DP-08 (timeline) | El tiempo ocioso avanza el reloj sin crear bloque. | Replay de RUN transitorio y secuencia de eventos (propuesta técnica 4); corresponde a la GUI. |
| DP-10 (sin aging) | Implementado: el motor no aplica aging ni emite `PRIORITY_CHANGED`; la espera es solo una métrica. | — |
| DP-04 (cuotas 10/20/30 ms) | **Preparado, no migrado.** La cuota sale de un único punto (`cuotaParaDespachoInterno`) y se fija al despachar; hoy devuelve el quantum global de v1. | Activar `cuotaMs(prioridad)` al adoptar `contracts/v2` (ver sección 6). |
| DP-01 (P1–P3 automáticas) | **No migrado.** Corresponde a Brickzon (clasificación) y Castilla (almacenamiento). | Ver sección 6. |
| DP-11 | Un CPU lógico con workers reales; todo acceso al Scheduler pasa por `PlanificadorMotor`. | Equivalencia con el motor lógico cuando exista. |

---

## 6. Dependencias y migración a v2

### 6.1 Quién hace qué

| Responsable | Aporta | Estado observado en su rama |
|---|---|---|
| Brickzon (`brickzon`) | Clasificación P1/P2/P3 (`clasificarPrioridades`), selección READY por prioridad, ticket y PID, cola visible, `AsignadorTickets` | `scheduler/SchedulerV2.*` y pruebas v2 |
| Castilla (`castilla`) | Lista de procesos, memoria, admisión transaccional y **almacenamiento** de las prioridades calculadas | `core/v2/*` (`ListaProcesos`, `Memoria`, `admitirProceso`) |
| Motor concurrente (`angel`) | Workers, sincronización, reloj, eventos, timeline, cuota fijada al despachar, cancelación | Este módulo |
| Aldana (`aldana`) | Motor lógico con la misma semántica observable | Sin código aún |

El motor **no** clasifica prioridades ni guarda la política: la selección va por `PlanificadorMotor` y las prioridades las recalcula el Scheduler sobre la lista de Castilla.

### 6.2 Sobre `scheduler/` en esta rama

`scheduler/` llegó a `angel` por el commit `b2e991e` de Brickzon (merge `fec95cf`); esta rama **no lo modifica**. Mientras la rama de Brickzon no esté en `master`, el motor necesita ese código para compilar. Orden de integración: (1) Brickzon y Castilla a `master`; (2) incorporar `master` en `angel`, con lo que `scheduler/`, sus pruebas y sus líneas de CMake dejan de formar parte del diff de este PR; (3) migrar el adaptador. **No fusionar este PR antes que el de Brickzon.**

### 6.3 Pasos de la migración

1. Cambiar `PlanificadorMotor` para delegar en `ConcurrentSimulator::v2::Scheduler` (selección, cola, clasificación).
2. Adoptar `contracts/v2`: alta sin prioridad (`AltaProceso`), `ticketReady` en lugar de `ordenLlegada`, sin `configurarQuantum`.
3. `cuotaParaDespachoInterno` devuelve `cuotaMs(prioridad)` (P1=10, P2=20, P3=30 ms).
4. Sustituir el contador propio por `AsignadorTickets` de Brickzon (mismo límite).
5. Reclasificar tras alta admitida, FINISH y CANCEL, antes del siguiente despacho (DP-02b).
6. Adaptar las suites a P1–P3 y comparar con el motor lógico sobre los casos de referencia.

El constructor de `ConcurrentEngine` ya no recibe `SchedulerConfig`: el Scheduler v2 no tiene configuración.

### 6.4 Preguntas abiertas para coordinación

1. **Lista v2 oficial:** existen dos, `core/ColaProcesosV2` (rama de Brickzon) y `core/v2/ListaProcesos` (rama de Castilla). El motor debe consumir una sola.
2. **Cancelación parcial (G1):** granularidad, precedencia entre CANCEL, FINISH y fin de cuota en el mismo límite, y si `cancelarProceso` bloquea o no.
3. **Driver de ejecución continua y política de errores (G1):** excepciones frente a `Resultado`.
4. **Tickets:** si el contador se reinicia con `reiniciar()` (hoy sí) y qué debe ocurrir al agotarse (hoy: el alta falla con `overflow_error` y el motor se pausa).
