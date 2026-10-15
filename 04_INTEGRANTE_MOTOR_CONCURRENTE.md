# Integrante 4 — Motor concurrente con threads reales

Asignación recuperada de `origin/angel`. Se restituye el detalle de workers, coordinación, sincronización, cierre y pruebas, conservando que los hilos reales implementan un solo CPU lógico simulado.

## Objetivo y principio de operación

El motor concurrente implementa el mismo `IExecutionEngine` que el motor lógico, usando C++ `std::thread`, `std::mutex` y `std::condition_variable`. El Scheduler continúa decidiendo qué PID obtiene CPU lógico. Cada proceso tiene un worker que espera un permiso, ejecuta una ráfaga simulada y reporta resultado; un coordinador selecciona, concede permiso, integra resultados y actualiza el snapshot. Aunque existan varios threads de sistema, como máximo un proceso recibe CPU lógico a la vez.

La arquitectura concurrente es decisión de equipo. El docente describe time-sharing en un CPU lógico, pero no obliga a tener workers o dos motores.

## Archivos y dependencias

Entregables:

- `concurrent/ConcurrentEngine.h`
- `concurrent/ConcurrentEngine.cpp`
- `concurrent/ProcessThread.h`
- `concurrent/ProcessThread.cpp`
- `concurrent/Synchronization.h`
- Pruebas: `tests/test_concurrent_basic.cpp`, `test_concurrent_pause.cpp`, `test_concurrent_cancel.cpp`, `test_concurrent_thread_shutdown.cpp`, `test_concurrent_race_safety.cpp`.

Depende de contratos v1.0, modelo/cola/memoria y Scheduler. Se puede desarrollar con procesos sintéticos, sin GUI. No cambia la API de `IExecutionEngine`, no duplica Scheduler ni configura widgets.

## Coordinación y sincronización

- El coordinador espera la condición de ejecución, recopila procesos, consulta Scheduler y concede un `SolicitudTurno` al worker seleccionado.
- `ProcessThread` espera permiso de turno mediante condición; al acabar devuelve `ResultadoRafaga`, duración realmente ejecutada y motivo (`QUANTUM_AGOTADO`, `PROCESO_TERMINADO`, `BLOQUEO_IO` o `CANCELADO`).
- No usar busy waiting (por ejemplo, un ciclo que consulta una bandera continuamente). Los workers que no tienen turno deben dormir en una variable de condición.
- Proteger snapshot y estado compartido con mutex: procesos, estado, prioridad, tiempo restante, PID ejecutando, memoria, eventos y timeline.
- Usar `lock_guard`/`unique_lock` según duración y espera; nunca mantener el mutex de estado tomado mientras se bloquea esperando el resultado de un worker si eso impide que el worker/cancelación complete.
- Cada worker protege sus banderas/solicitud/resultado con su mutex; sus variables de condición separan espera de turno y notificación de fin.
- `ConcurrentEngine` usa mutex de estado y condiciones para coordinador y paso en curso; `ProcessThread` tiene mutex y CV propios. Documentar cualquier cambio de alcance de protección antes de considerarlo seguro.

## Pausa, paso, cancelación y cierre

- `iniciar()` reanuda una pausa y es idempotente si ya está ejecutando.
- `pausar()` detiene avances nuevos y espera que el paso en curso llegue a un punto consistente. Los workers quedan bloqueados hasta un turno posterior.
- `siguientePaso()` exige PAUSADO bajo el contrato actual. El motor concurrente existente ejecuta una ráfaga/turno mediante un worker; la definición contractual “una transición significativa” y la ráfaga completa no coinciden. Cerrar DP-07 antes de fijar el comportamiento compartido con motor lógico.
- `cancelarProceso()` debe ser seguro ante PID inexistente o ya cancelado; al cancelar worker activo, notificarlo y evitar deadlock. El motor actual libera memoria y permite cancelar READY/RUN/WAIT, ignora FINISH/CANCEL. El segmento parcial al cancelar RUN requiere decisión DP-06.
- `reiniciar()` detiene y une workers, elimina procesos/eventos/timeline del ciclo y conserva quantum/capacidad según el contrato.
- Destructor debe señalizar terminación, despertar coordinador/workers y hacer `join()` de todos los threads joinable; no dejar threads huérfanos.
- Ningún worker actualiza widgets. `ConcurrentEngine` entrega snapshot/eventos; GUI procesa ese estado desde su hilo principal mediante mecanismo seguro.

## Distribución de trabajo esperada

El motor usa `scheduler.seleccionarSiguiente(...)` para elegir; `ordenarColaPrioridad(...)` prepara presentación. Al retornar un proceso agotado de quantum a READY, el motor debe asignar el nuevo orden/ticket de reingreso según DP-05. El Scheduler no puede crear el Round Robin por sí solo. La prueba de empate configura el nuevo `ordenLlegada` manualmente, mientras la implementación concurrente actual no lo actualiza; mantener esta discrepancia explícita y corregirla solo en una tarea de código aprobada.

Velocidad visual y temporizador de refresco no deben cambiar la duración simulada. El intervalo de coordinador de 10 ms que existe en el motor concurrente es coordinación real, no quantum simulado.

## Cinco pruebas de seguridad y comportamiento

1. **`tests/test_concurrent_basic.cpp`:** creación, quantum válido/no válido, validación/PID duplicado/memoria insuficiente, paso pausado, tiempo y bloque de timeline con `[inicio, fin)`.
2. **`tests/test_concurrent_pause.cpp`:** pausa e inicio idempotentes; espera de `siguientePaso()` fuera de pausa; al pausar deja de avanzar el reloj mientras workers duermen.
3. **`tests/test_concurrent_cancel.cpp`:** cancelar PID inexistente, READY y ya cancelado; liberar memoria; ejecutar el proceso restante; cancelar último proceso. Añadir/coordinar casos RUN/WAIT según DP-06.
4. **`tests/test_concurrent_thread_shutdown.cpp`:** destruir engine con workers activos; reiniciar detiene workers, limpia snapshot y timeline/eventos, conserva capacidad/quantum y permite nuevas altas.
5. **`tests/test_concurrent_race_safety.cpp`:** lectores llaman a `obtenerEstado()` concurrentemente mientras coordinador avanza y productor añade procesos; snapshots mantienen contadores/timestamps coherentes y los threads se unen al cerrar.

Los escenarios son expectativas de equipo/pruebas del módulo, no requisitos del docente.

## Criterios de aceptación

- Sin busy waiting; workers sin turno esperan bloqueados.
- Mutex y condiciones protegen datos compartidos y la terminación despierta a todos los waiters.
- Nunca hay dos procesos en RUN/CPU lógico a la vez.
- Pausa, reanudación, turno único, cancelación, reinicio y destrucción terminan sin deadlock ni workers huérfanos.
- Snapshot, reloj, evento y timeline describen el mismo resultado de ráfaga.
- `ordenLlegada`/ticket se actualiza en el motor en el reingreso aprobado y la prueba integrada demuestra alternancia.
- No se toca GUI desde threads ni se duplica Scheduler.
- Los resultados observables equivalen al motor lógico con la semántica de paso acordada.
