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
| `Scheduler scheduler_` | `mutexEstado_` | Ordenamiento de cola de prioridades y cálculo determinista de aging. |
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

---

## 4. Pruebas Automatizadas

El módulo incluye las 5 suites de pruebas requeridas y suites adicionales de escenarios, exclusión de pasos, Round Robin y cancelación en RUN:

1. `test_concurrent_basic.cpp`: Validación de admisibilidad, errores, inicialización y ejecución paso a paso con workers reales.
2. `test_concurrent_pause.cpp`: Idempotencia de `iniciar()` y `pausar()`, suspensión en condición variable y control de `siguientePaso()`.
3. `test_concurrent_cancel.cpp`: Cancelación idempotente en estados `READY`, `WAIT` y `FINISH`, con liberación de memoria y worker.
4. `test_concurrent_thread_shutdown.cpp`: Destrucción segura RAII, detención de workers y reinicio conservando configuración.
5. `test_concurrent_race_safety.cpp`: Prueba bajo estrés con múltiples hilos lectores llamando a `obtenerEstado()` concurrentemente mientras un hilo productor inserta procesos en tiempo real. Verifica en cada snapshot las invariantes de la sección 3.1.
6. `test_concurrent_scenarios.cpp`: Ciclo de E/S `RUN→WAIT→READY→FINISH` con secuencia exacta de eventos y bloques de timeline, aging integrado (3→9 en 12,000 ms) y ejecución continua con `iniciar()` hasta `FINALIZADO`.
7. `test_concurrent_step_exclusion.cpp`: Varios hilos llaman a `siguientePaso()`, `iniciar()`, `pausar()` y `reiniciar()` a la vez mientras un monitor valida que nunca haya dos procesos en RUN. Usa el retardo de pruebas para que la carrera sea reproducible.
8. `test_concurrent_round_robin.cpp`: Alternancia entre prioridades iguales, ticket de alta asignado por el motor, reingreso por cuota y retorno de WAIT, y orden determinista de varios despertares en el mismo turno.
9. `test_concurrent_cancel_run.cpp`: Cancelación con la ráfaga en curso: conserva el CPU completado, un único evento, FINISH gana en el mismo límite, cancelación que llega con la ráfaga terminando en WAIT, reinicio durante la ráfaga y cancelación en ejecución continua.

Las pruebas de carrera se verifican también por mutación: sin la corrección correspondiente, `test_concurrent_step_exclusion`, `test_concurrent_round_robin` y `test_concurrent_cancel_run` fallan.

---

## 5. Decisiones aprobadas y estado en este módulo

DP-01 a DP-11 constan como **aprobadas** en [`docs/DECISIONES_APROBADAS.md`](../docs/DECISIONES_APROBADAS.md). Los contratos `contracts/v2/` son candidatos y los detalles técnicos señalados como "no ratificados" en [`docs/PROPUESTAS_TECNICAS_V2.md`](../docs/PROPUESTAS_TECNICAS_V2.md) siguen pendientes (gate G1). El motor sigue sobre el contrato **v1.0**.

| DP | Estado en el motor | Pendiente |
|---|---|---|
| DP-05 (tickets, Round Robin) | Implementado sobre v1: `ordenLlegada` actúa como ticket en alta, retorno de WAIT y fin de cuota. | Orden de varios despertares simultáneos (propuesta técnica 1, no ratificada); límite del contador de tickets (propuesta 11). |
| DP-06 (cancelación) | READY y WAIT cancelan al instante; RUN se cancela al cerrar la ráfaga conservando el CPU completado. Memoria y evento se emiten una vez. | Protocolo técnico: granularidad de checkpoint (1 ms propuesto), precedencia FINISH/CANCEL/cuota agotada en el mismo límite y si `cancelarProceso` es bloqueante o no. Hoy no bloquea y FINISH gana. |
| DP-07 (paso) | `siguientePaso()` ejecuta un turno completo (cuota, E/S o fin) o, sin READY y con WAIT, avanza el reloj hasta el próximo despertar sin despachar. | Driver de ejecución continua (G1). |
| DP-08 (timeline) | El tiempo ocioso avanza el reloj sin crear bloque. | Replay de RUN transitorio y secuencia de eventos (propuesta técnica 4); corresponde a la GUI. |
| DP-04, DP-10, DP-01 (cuotas 10/20/30 ms, sin aging, P1–P3) | **No migrado.** Sigue el quantum global configurable y el aging heredado de v1. | Migración a v2 (ver sección 6). |
| DP-11 | Un CPU lógico con workers reales; mismo Scheduler compartido. | Equivalencia con el motor lógico cuando exista. |

---

## 6. Lista de verificación de la migración a v2

Bloqueada hasta ratificar G1 (errores, driver continuo, checkpoints de cancelación, eventos) y contar en `master` con el Scheduler v2 de Brickzon.

- Eliminar la copia de `scheduler/` en esta rama y usar el módulo compartido.
- Adoptar `contracts/v2`: alta sin prioridad, `cuotaMs(prioridad)` fijada al despachar, sin `configurarQuantum` ni aging, `ticketReady`.
- Sustituir `ordenLlegada` por `ticketReady`; los tickets de este motor ya siguen la misma regla.
- Retirar el caso de aging de `test_concurrent_scenarios.cpp` y adaptar las suites a P1–P3 con cuotas 10/20/30.
- Comparar con el motor lógico de Aldana sobre los casos de referencia.
