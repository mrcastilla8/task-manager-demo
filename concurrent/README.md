# Motor concurrente v2 (Integrante 4)

`ConcurrentSimulator::v2::ConcurrentEngineV2` implementa `IExecutionEngine` (contracts/v2) con workers reales (`std::thread`), un hilo coordinador, `std::mutex` y `std::condition_variable`. Simula **un solo CPU lógico**: los hilos del sistema no son núcleos simulados.

## Dependencias oficiales (no se duplican)

| Responsabilidad | Módulo |
|---|---|
| Lista de procesos, memoria y admisión (`ListaProcesos`, `Memoria`, `admitirProceso`) y almacenamiento de prioridades | `core/v2` (Castilla) |
| Clasificación P1/P2/P3, selección READY, cola visible y `AsignadorTickets` | `scheduler/SchedulerV2` (Brickzon) |
| Workers, coordinador, sincronización, reloj, turnos, eventos y timeline | `concurrent/` (este módulo) |

`concurrent/` **no** incluye nada de `simulation/`: el motor lógico de Aldana solo se usa como referencia en `tests/test_concurrent_v2_comparacion.cpp`.

## Comportamiento

- **Alta** transaccional: se admite en `core/v2`, se crea el worker, se recalcula P1/P2/P3 y se asigna el ticket. Si algo falla se deshace todo (proceso, memoria y worker) sin consumir ticket.
- **Recálculo P1/P2/P3 (DP-02):** tras alta admitida, FINISH y CANCEL, con `Scheduler::clasificarPrioridades` y `ListaProcesos::aplicarPrioridades`; un evento `PRIORIDAD_CAMBIADA` por cada cambio.
- **Cuotas (DP-04):** 10/20/30 ms según la prioridad, fijadas al despachar (`cuotaTurnoFijadaMs`); una alta durante RUN no cambia la cuota del turno.
- **Tickets (DP-05):** `AsignadorTickets`; ticket nuevo en alta, retorno de WAIT y fin de cuota. Antes de modificar nada, cada paso reserva exactamente los tickets que usará (un WAIT que despierta durante la ráfaga, y el propio proceso solo si vuelve a READY por cuota). Si no alcanzan, el paso se rechaza completo con `ESTADO_INVALIDO`, también el salto idle.
- **Sin aging (DP-10).** La espera es solo `tiempoEsperaMs`.
- **Paso (DP-07):** `siguientePaso()` exige PAUSADO. Con READY ejecuta un turno (cuota, E/S o fin); sin READY y con WAIT salta el reloj al próximo despertar sin despachar; sin trabajo finaliza.
- **Resultado del worker:** el coordinador valida PID, duración y motivo exacto (`PROCESO_TERMINADO`, `BLOQUEO_IO` o `QUANTUM_AGOTADO`) contra el despacho. `CANCELADO` no es un fin de ráfaga válido: RUN se cancela al cerrar la ráfaga, sin checkpoints G1.
- **Fallos al aplicar un turno:** después de recibir un resultado válido, el motor conserva una imagen previa de procesos, memoria, tickets, reloj, esperas, cancelaciones, eventos y timeline. Una excepción al aplicar restaura esa imagen y revierte el evento READY→RUN; el worker permanece activo y el coordinador puede procesar otro paso.
- **Ejecución continua:** hasta que G1 la ratifique, `iniciar()` solo fija EJECUTANDO, igual que el motor lógico. No hay retardos físicos para simular CPU.
- **Cancelación (DP-06):** READY y WAIT al instante. Con el proceso en RUN la cancelación se resuelve al cerrar la ráfaga, tras contabilizar el CPU completado; si el proceso termina en ese límite gana FINISH. Los checkpoints de cancelación parcial **no** están implementados (pendiente de G1).
- **Concurrencia:** el coordinador ejecuta los pasos de uno en uno; `pausar()` y `reiniciar()` esperan la ráfaga en curso; los snapshots (`obtenerEstado()`) son consistentes. Los hilos (`ProcessThread::worker_` y `coordinador_`) son el último miembro de su clase. Cierre: se interrumpen los workers y se hace `join` de todos.

## Herramientas solo para pruebas (desactivadas por defecto)

`activarPuertaRafagaPruebas` + `liberarRafagaPruebas` retienen la ráfaga en su worker (con límite de 5 s) para probar exclusión, pausa y cancelación en RUN sin depender de tiempos; `configurarFalloAltaPruebas`, `configurarFalloPasoPruebas`, `forzarMotivoWorkerPruebas` y `fijarSiguienteTicketPruebas`. Permanecen desactivadas por defecto y no cambian reloj ni CPU simulado.

## Pruebas

`tests/test_concurrent_v2_*`: `basico`, `cuotas` (cuotas y recálculo), `tickets`, `alta_rollback`, `cancelacion`, `pausa_reinicio`, `exclusion` y `comparacion` (mismas operaciones sobre este motor y `SimulationEngineV2`: escenarios aprobados más un fuzz determinista, comparando el snapshot completo tras cada operación). `test_concurrent_v2_equivalencia` es del integrador.

## Incompatibilidades y pendientes

- **G1 sin ratificar:** política de errores (códigos), driver de ejecución continua, protocolo de cancelación parcial (granularidad, precedencia CANCEL/FINISH/fin de cuota, si cancelar bloquea), y orden y secuencia de eventos.
- **Agotamiento de tickets:** ambos motores rechazan un paso (incluido el salto idle) *antes* de tocar reloj, eventos o estados. Este motor lo prueba con `fijarSiguienteTicketPruebas`; el motor lógico no expone cómo fijar el contador, así que esa igualdad en el límite no se comprueba en `test_concurrent_v2_comparacion`.
- **`CPU_EJECUTADA`:** ambos motores usan como marca de tiempo el fin del segmento; la propuesta técnica 4 propone el inicio.
- **`pausar()`** devuelve `ESTADO_INVALIDO` si el motor no está EJECUTANDO (igual que el motor lógico).
- `test_concurrent_v2_exclusion` inyecta un motivo de worker contradictorio y un fallo después de contabilizar un segmento; comprueba rechazo, rollback observable (CPU, reloj, eventos, timeline, tickets y memoria) y que el coordinador siga vivo.
