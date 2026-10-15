# Integrante 4 — Motor concurrente con threads reales

Documento recuperado de `origin/angel`. Se conservan el alcance concurrente, la sincronización segura y la atribución por integrante, junto con las aclaraciones canónicas sobre CPU lógico y decisiones abiertas.

## Principio

El motor usa threads reales de C++ para coordinar workers, pero el Scheduler concede un turno a la vez sobre un único CPU lógico simulado. Los threads de implementación no representan varios procesadores ni convierten time-sharing en ejecución paralela simultánea. El profesor describe la ilusión de concurrencia en un procesador (notas, pág. 28; transcripción 00:05:19–00:07:45), sin exigir este motor.

El motor concurrente es actualmente una decisión de arquitectura del equipo. La interfaz común y el contrato vigente están descritos en [contracts/README.md](contracts/README.md); no modificar encabezados v1.0 para anticipar requisitos pendientes.

## Responsabilidades

- Usar una política Scheduler compartida, no duplicada.
- Sincronizar estado, procesos, memoria, eventos, timeline y proceso atendido.
- Suspender workers que no tienen turno mediante mecanismos de espera, sin busy waiting.
- Evitar que dos workers actualicen simultáneamente la ejecución del CPU lógico.
- Asegurar que pausa, reinicio, cancelación y destrucción tengan una terminación coordinada.
- Entregar snapshots con el mismo significado que el motor lógico aprobado.
- No actualizar widgets desde threads reales; la GUI consume estado/eventos por un mecanismo seguro para su hilo.

## Discrepancias observadas

En `origin/angel`, `Scheduler` desempata por `ordenLlegada`; su prueba crea el comportamiento Round Robin cambiando manualmente ese valor antes de seleccionar. `ConcurrentEngine::ejecutarPasoInterno` retorna a READY al concluir `QUANTUM_AGOTADO`, pero no asigna nuevo `ordenLlegada`. Así, los tests del selector y la política integrada pueden divergir. La propuesta de arreglo queda pendiente DP-05; no se modifica código bajo esta documentación.

El contrato define `siguientePaso()` como una transición significativa; el método concurrente ejecuta una ráfaga coordinada hasta quantum, WAIT, FINISH o cancelación. El movimiento de reloj y la correspondencia con el motor lógico requieren cerrar DP-07.

## Casos y criterios

Consulta [P5–P8, P10–P13](docs/CASOS_REFERENCIA.md), matriz [DP-05–DP-07, RD-10](docs/MATRIZ_TRAZABILIDAD.md) y [especificación](docs/ESPECIFICACION_FUNCIONAL.md).

- Un solo PID ocupa el CPU lógico durante cada intervalo.
- El timeline, reloj, estado y motivo de fin describen la misma ráfaga.
- Igual entrada produce el mismo resultado observable que el motor lógico tras aprobar semántica de paso.
- La prueba de empate integrada incorpora el evento real de reingreso, una vez aprobado su criterio.
- Cancelación durante RUN documenta si el intervalo parcial cuenta; no asumirlo.
- No afirmar que la explicación docente requiere threads ni que los workers son CPUs simulados.
