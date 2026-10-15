# Integrante 4 — Motor concurrente con threads reales

> **Instrucción operativa v2 candidata:** Ángel migra el motor a la interfaz común ratificada, comparte Scheduler con el motor lógico y conserva un único CPU lógico. El destino usa cuota derivada 10/20/30 ms, sin quantum global ni aging, y paso por ráfaga o avance idle hasta despertar. El contrato candidato aún no está integrado/operativo. Las instrucciones v1 siguientes se limitan a contexto histórico y compatibilidad.

**Responsable:** Ángel · **Rama:** `angel` · **Workflow:** [fase 3](docs/WORKFLOW_DESARROLLO.md#fase-3--motores-de-ejecucion)

## Objetivo y entregables

Implementar los mismos resultados observables que el motor lógico mediante `std::thread`, mutex y variables de condición. Los threads son workers del único CPU simulado: como máximo un proceso ejecuta CPU lógica a la vez. Un coordinador consulta el Scheduler compartido, autoriza un turno y reúne el resultado.

- `concurrent/ConcurrentEngine.h/.cpp`
- `concurrent/ProcessThread.h/.cpp`
- `concurrent/Synchronization.h`
- `tests/test_concurrent_basic.cpp`, `test_concurrent_pause.cpp`, `test_concurrent_cancel.cpp`, `test_concurrent_thread_shutdown.cpp`, `test_concurrent_race_safety.cpp`.

## Reglas de ejecución

- La prioridad automática P1–P3 determina cuota máxima vigente con `cuotaMs(prioridad)` (10/20/30 ms). El coordinador fija una copia al iniciar RUN; reclasificación no cambia la cuota del turno ya abierto.
- Consultar la implementación común de Scheduler; renovar ticket al retornar a READY por WAIT o cuota. No implementar un comparador propio, quantum global configurable ni aging activo.
- `siguientePaso()` en PAUSADO completa un turno hasta cuota/WAIT/FINISH/CANCEL; si no hay READY pero sí WAIT, avanza reloj al vencimiento más próximo y termina ese paso sin despachar. Continuo itera esas unidades.
- CPU lógico simulado y reloj no dependen de duración real de locks, sleeps o coordinación de threads.
- Cada bloque `BloqueTimeline` representa únicamente CPU ejecutado por PID positivo, duración positiva. Inferir idle por gaps desde origen de sesión hasta reloj snapshot. Ejemplo: A ejecuta `[0,8)`, WAIT produce idle `[8,18)`, B ejecuta `[18,22)`; a snapshot 27 los gaps idle son `[8,18)` y `[22,27)` y no hay segmentos con PID especial.
- Sin busy waiting. Workers sin turno duermen en CV. Proteger procesos, estado, memoria, eventos, timeline y snapshots; no tomar mutex compartido mientras se espera a que worker/cancelación pueda completar. Cierre despierta y une todos los threads.
- La cancelación terminal/error sigue pendiente: distinguir PID desconocido, FINISH y CANCEL es una propuesta técnica que debe ratificarse; no atribuirla a DP-06.

## Pruebas necesarias

1. `test_concurrent_basic.cpp`: alta/validación/PID duplicado/memoria, paso pausado, cuotas, reloj y timeline `[inicio,fin)`.
2. `test_concurrent_pause.cpp`: pausa/reanudación y paso único; fuera de un avance lógico el reloj no cambia.
3. `test_concurrent_cancel.cpp`: cancelación READY/RUN/WAIT, PID desconocido y terminales según la semántica finalmente ratificada; liberar memoria exactamente una vez.
4. `test_concurrent_thread_shutdown.cpp`: destrucción y reinicio despiertan y unen workers; después se puede iniciar otro ciclo. No conservar una opción de quantum global v1.
5. `test_concurrent_race_safety.cpp`: consultas de snapshot y altas concurrentes mientras opera coordinador; coherencia, ausencia de carreras/deadlocks y cierre completo.

Los nombres y alcances de estas pruebas se preservan como entregables, aunque se actualicen aserciones v1 incompatibles para migración. Pruebas antiguas `quantum` verifican ahora cuota/turno; la temporización real no es resultado lógico.

## Contexto histórico v1

La implementación heredada usaba quantum global, workers `ProcessThread`, una pausa coordinada y contratos raíz v1.0. Es contexto de compatibilidad, no requisito para nueva implementación. No cambiar `contracts/*.h` ni módulos fuera del motor durante la fase documental. El motor concurrente no obliga al docente; los dos motores sí son el destino aprobado por decisión de equipo.

## Criterios de aceptación futura

- Ningún estado observable permite dos procesos RUN simultáneos.
- Sin busy waiting; cierre/reinicio sin deadlocks ni threads huérfanos.
- Pausa, paso, cancelación y snapshot coherentes con el motor lógico.
- Eventos, reloj y timeline describen la misma ejecución simulada; idle inferido y PID de bloque siempre positivo.
- Validación concurrente reporta los comandos y resultados realmente ejecutados.
