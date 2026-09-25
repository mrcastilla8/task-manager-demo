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

## 4. Pruebas Automatizadas

El módulo incluye las 5 suites de pruebas requeridas:

1. `test_concurrent_basic.cpp`: Validación de admisibilidad, errores, inicialización y ejecución paso a paso con workers reales.
2. `test_concurrent_pause.cpp`: Idempotencia de `iniciar()` y `pausar()`, suspensión en condición variable y control de `siguientePaso()`.
3. `test_concurrent_cancel.cpp`: Cancelación idempotente en estados `READY`, `WAIT` y durante ejecución.
4. `test_concurrent_thread_shutdown.cpp`: Destrucción segura RAII, detención de workers y reinicio conservando configuración.
5. `test_concurrent_race_safety.cpp`: Prueba bajo estrés con múltiples hilos lectores llamando a `obtenerEstado()` concurrentemente mientras un hilo productor inserta procesos en tiempo real.
