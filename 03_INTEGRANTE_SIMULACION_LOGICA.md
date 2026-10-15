# Integrante 3 — Motor de simulación lógica

Asignación recuperada de `origin/aldana`. Este módulo es el motor de referencia sin threads reales: mantiene un reloj simulado y avanza procesos como objetos. La carpeta `simulation/` y su implementación aún no aparecen en las ramas revisadas; las rutas abajo son entregables previstos, no archivos ya existentes.

## Contrato y responsabilidades

Implementa `IExecutionEngine` desde [contracts/IExecutionEngine.h](contracts/IExecutionEngine.h). Métodos contractuales exactos:

- `agregarProceso(const Proceso&)`, `cancelarProceso(int)`.
- `iniciar()`, `pausar()`, `reiniciar()`, `siguientePaso()`.
- `configurarQuantum(std::int64_t)`, `obtenerQuantum() const`.
- `obtenerEstado() const`, que devuelve snapshot `EstadoSistema` por valor.

Respeta errores, idempotencia, memoria y restricciones de cada método descritas en [contracts/README.md](contracts/README.md). No publiques una interfaz alternativa basada en el snippet original: este contrato vigente incluye configuración de quantum y describe `siguientePaso()` como una transición significativa. Sin embargo, su unidad exacta frente a una ráfaga sigue pendiente DP-07 y debe resolverse antes de la equivalencia de motores.

## Reloj y ciclo operativo

Mantener `tiempoSimulado` en milisegundos, independiente del reloj físico y de la velocidad visual. El ciclo funcional que debe coordinar el motor es:

1. Procesar expiraciones de procesos en WAIT y retornos a READY.
2. Actualizar espera READY y ejecutar aging según Scheduler aprobado.
3. Obtener snapshot/colección actual y pedir al Scheduler el PID READY siguiente.
4. Si no hay READY, determinar si existen esperas que puedan despertar procesos o si la ejecución finalizó; política de tiempo idle está pendiente.
5. Registrar READY→RUN y entregar el intervalo permitido al proceso elegido.
6. Avanzar tiempo simulado y descontar tiempo restante por el intervalo de CPU realmente ejecutado.
7. Resolver motivo de fin: agotamiento de turno→READY; operación de E/S→WAIT; tiempo restante cero→FINISH.
8. Actualizar memoria según el ciclo de vida acordado, orden de reingreso READY según DP-05 y cola de prioridad.
9. Registrar eventos y el bloque de timeline del intervalo de CPU.
10. Publicar una fotografía consistente mediante `EstadoSistema`.

Este orden es el ciclo lógico esperado por la asignación, no una definición normativa del límite de `siguientePaso()`. Debe marcarse qué suboperaciones son una transición/evento una vez DP-07 se decida.

## Quantum, cuotas y ejecución

**Decisión vigente del equipo:** motor usa quantum global configurable, por defecto 500 ms. En un recorrido completo con A `tiempoRestante=1300` ms y quantum 500 ms, las ráfagas son 500, 500 y 300 ms; la última termina el proceso. Esta ilustración no afirma cuántas ráfagas procesa una llamada a `siguientePaso()`.

**Fuente docente:** cuota es atributo de proceso (notas, pág. 23) y `prioridad × 10 ms` es ejemplo de la transcripción. Cuota individual frente a quantum global sigue pendiente DP-04. No agregar campo al contrato v1.0 ni imponer una fórmula.

## E/S, eventos y timeline

El contrato `OperacionIO` actual usa `habilitada`, `despuesDeCpuMs` y `duracionEsperaMs`; el evento de I/O ocurre tras CPU consumida y dura el valor configurado. Ejemplo del documento original del integrante: proceso A con 5,000 ms totales bloquea tras 1,500 ms de CPU y espera 1,000 ms. Al transcurrir la espera, vuelve a READY. El caso sirve para probar operación configurada; no atribuir esas cifras al docente ni generalizar qué estados entran en TL.

Registrar eventos con milisegundos simulados, tipo, PID y descripción, usando los tipos ya definidos (`PROCESS_CREATED`, `PROCESS_ADMITTED`, `PROCESS_READY`, `PROCESS_RUNNING`, `QUANTUM_FINISHED`, `PROCESS_WAITING`, `PROCESS_FINISHED`, `PROCESS_CANCELLED`, `PRIORITY_CHANGED`). No inventar tipos nuevos en v1.0.

`BloqueTimeline` vigente usa PID y extremos `std::int64_t`; representa intervalo semiabierto `[inicio, fin)` con `fin > inicio`. Ejemplo de recorrido completo: A `[0,500)`, B `[500,1000)`, A `[1000,1500)`. Cada bloque corresponde a CPU realmente ejecutado, no a WAIT/idle.

## Archivos y pruebas previstas

Entregables del módulo:

- `simulation/SimulationEngine.h`
- `simulation/SimulationEngine.cpp`
- `simulation/SimulationClock.h`
- `simulation/SimulationClock.cpp`
- `tests/test_simulation_quantum.cpp`
- `tests/test_simulation_wait.cpp`
- `tests/test_simulation_finish.cpp`
- `tests/test_simulation_cancel.cpp`
- `tests/test_simulation_dynamic_process.cpp`

Cinco escenarios de prueba:

1. **Quantum/tiempo restante:** A 1,300 ms con quantum 500 completa en intervalos 500/500/300 a través de la simulación total; verificar reloj restante y timeline.
2. **WAIT:** A bloquea tras 1,500 ms de CPU, WAIT dura 1,000 ms y luego regresa READY; verificar que tiempo CPU y tiempo de espera no se confundan.
3. **Finalización:** proceso completa cuando restante llega a cero, pasa RUN→FINISH, deja de estar en READY y libera memoria según decisión vigente.
4. **Cancelación:** PID inexistente/ya cancelado respeta idempotencia contractual; proceso activo se cancela según reglas y registra estado/evento apropiados.
5. **Alta dinámica:** agregar proceso durante ejecución; validar, admitir o devolver el error contractual por memoria, actualizar snapshot y cola.

La granularidad de cada “paso” no se fija hasta DP-07. Las pruebas de paso deben ser parametrizadas por la decisión aprobada.

## Dependencias y límites

Depende de contratos/modelo (Integrante 1) y Scheduler (Integrante 2). Puede desarrollarse con un doble de Scheduler durante el trabajo en paralelo, sin introducir otra interfaz pública. No implementa GUI, widgets, animaciones, threads, mutex, variables de condición, algoritmo interno de prioridad ni cola visual.

## Criterios de aceptación

- Simulación completa reproducible sin GUI ni threads reales.
- Reloj, CPU restante, espera WAIT, eventos y timeline se corresponden.
- Altas dinámicas, cancelación, espera, retorno READY y finalización respetan contrato y decisiones aprobadas.
- `EstadoSistema` se publica por valor y con cola/memoria/eventos coherentes.
- Comparte Scheduler y semántica temporal aprobada con ConcurrentEngine.
- No afirma que un `siguientePaso()` procese transición o quantum hasta resolver DP-07.
