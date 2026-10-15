# Integrante 3 — Motor lógico

> **Instrucción operativa v2 candidata:** Aldana implementa motor lógico sin threads reales, contra el contrato común v2 una vez ratificado. En esta fase se formalizaron P1–P3, cuota máxima derivada 10/20/30 ms, sin quantum global, sin aging y un paso por ráfaga o avance idle hasta despertar. Los encabezados v2 todavía no son operativos ni integrados. Este documento no asigna como tarea v2 ninguna regla v1.

**Responsable:** Aldana · **Rama:** `aldana` · **Workflow:** [fase 3](docs/WORKFLOW_DESARROLLO.md#fase-3--motores-de-ejecucion)

## Objetivo y archivos

Crear `simulation/SimulationEngine.h/.cpp` y `simulation/SimulationClock.h/.cpp`. Mantener un único CPU lógico, reloj determinista en milisegundos simulados y resultados observables equivalentes al motor concurrente. Compartir Scheduler y comparador; no implementarlos otra vez.

Pruebas previstas: `tests/test_simulation_quantum.cpp` (nombre legado; en v2 verifica cuota y turno), `test_simulation_wait.cpp`, `test_simulation_finish.cpp`, `test_simulation_cancel.cpp` y `test_simulation_dynamic_process.cpp`. Preservar responsabilidades de casos: duración y CPU restante; E/S y retorno; FINISH/liberación; cancelación y estado terminal; alta dinámica, snapshot y rechazo de memoria. La semántica detallada de códigos/cancelación es propuesta pendiente de ratificación.

## Comportamiento v2 aprobado como destino

- Clasificar automáticamente procesos activos READY/RUN/WAIT en P1–P3 según TL. FINISH/CANCEL no participan. Una alta admitida causa reclasificación antes del siguiente despacho; no interrumpe RUN ni modifica su turno fijado. Un alta rechazada no altera memoria/TL/prioridades.
- Derivar cuota vigente solo con `cuotaMs(prioridad)`: P1=10, P2=20, P3=30 ms. Al despacho copiar a `cuotaTurnoFijadaMs`; una reclasificación durante RUN solo afecta el próximo turno.
- Despachar según Scheduler compartido; renovar ticket al ingresar READY por alta, WAIT o cuota agotada. Sin quantum global configurable ni aging activo.
- `siguientePaso()` requiere PAUSADO. Con READY, ejecutar un turno hasta agotar cuota o hasta WAIT/FINISH/CANCEL. Sin READY pero con WAIT, adelantar reloj al despertar más cercano, pasar vencidos a READY y terminar ese paso sin despacho. Si no quedan procesos activos, finalizar.
- Bloquear CPU durante WAIT; conservar memoria. Liberarla una vez al FINISH/CANCEL.
- Eventos llevan tiempo simulado reproducible. `BloqueTimeline` solo registra CPU realmente ejecutado, con PID positivo y duración positiva. Idle se infiere de los gaps entre bloques desde t=0 hasta `tiempoSimuladoMs`; evento idle opcional no porta PID de proceso. Ejemplo: A `[0,8)`, WAIT/idle `[8,18)`, B `[18,22)` y snapshot t=27: bloques A/B únicamente; gaps `[8,18)` y `[22,27)`.
- Pausa del motor/UI sin avance no crea idle simulado. La GUI nunca modifica el reloj.

## API y estado de contrato

Usar `ConcurrentSimulator::v2` solo después de ratificación e integración de la API. `Resultado`/`CodigoResultado` frente a excepciones es una propuesta técnica; conservar en diseño la distinción PID inexistente, FINISH y CANCEL hasta ratificar semántica. DP-06 aprueba transiciones/efectos generales y no aprueba esos códigos ni idempotencia específica.

DP-07 aprueba unidades lógicas repetidas en modo continuo, pero `IExecutionEngine` aún no define el driver no bloqueante. Revisar alternativas de timer cooperativo/continuador en [propuestas técnicas](docs/PROPUESTAS_TECNICAS_V2.md) antes de añadir un método o callback; el motor lógico no ejecutará un bucle continuo bloqueante dentro de `iniciar()`.

## Contexto histórico de v1

La API raíz `contracts/*.h` sigue vigente para el código existente: prioridades 1–10, quantum global configurable 500 ms, aging y contabilidad `quantumConsumido`. Mantener v1 sin cambios en esta fase. Esas reglas son compatibilidad histórica y no deben copiarse al motor v2. La antigua ambigüedad de `siguientePaso()` quedó resuelta para el destino por DP-07.

## Criterios de aceptación futura

- Reloj, procesos, memoria, eventos y bloques timeline coherentes para cada paso.
- Cuota vigente derivada de prioridad y cuota del turno inmutable durante RUN.
- CPU solo contabilizada como segmento para ejecución efectiva PID>0; gaps inferidos cubren idle hasta reloj del snapshot.
- Casos WAIT/despertar, fin temprano, cancelación y alta dinámica reproducibles.
- Sin Scheduler duplicado ni diferencias observables con el motor concurrente en las mismas entradas.
- Compilación y pruebas reportadas con comandos y resultados observados; existencia de test no significa PASS.
