# Integrante 2 — Scheduler y política de planificación

Documento recuperado de `origin/brickzon` (la versión de `origin/angel` contiene la misma asignación funcional). Se conserva su responsabilidad y atribución por integrante y se sincroniza con la especificación canónica.

## Objetivo

Decidir qué proceso `READY` obtiene el CPU lógico y exponer el orden visible de la cola. El Scheduler es independiente de GUI, temporizadores visuales y threads. Los motores lo usan; no deben duplicar su política.

## Comportamiento registrado del equipo

En ramas de Scheduler, la decisión actual selecciona el proceso READY con mayor `prioridadActual`; empata por `ordenLlegada` menor y, si persiste el empate, por PID menor. Las prioridades vigentes son 1–10, con 10 como máxima. El aging documentado añade prioridad a procesos READY cada 2,000 ms de espera, hasta 10; el valor actual se restablece a base cuando el proceso obtiene CPU. Estas reglas son `DECISION_EQUIPO`, no exigencias atribuidas al profesor.

La prueba de empate crea la rotación entre turnos modificando `ordenLlegada` para A antes de volver a invocar el Scheduler. Eso prueba el selector con un orden de reingreso ya actualizado, no que el motor integrado haga la actualización. `ConcurrentEngine` de `origin/angel` devuelve el proceso a READY tras `QUANTUM_AGOTADO` sin cambiar su `ordenLlegada` (DP-05). La política de actualización está pendiente y el código no se corrige aquí.

## Regla docente aún no integrada

Las notas, pág. 24, definen `AssignPriority()` por tercios del tamaño, con `A=min(TL.Size)`, `B=max(TL.Size)`, `H=(B-A)/3`, prioridad 1 cuando `Size < A+H`, prioridad 2 cuando no se cumplió lo anterior y `Size < A+2H`, y 3 en los demás casos. La transcripción explica el cambio de bandas si llega un proceso mayor (00:11:02–00:14:22). Esta clasificación 1–3 difiere de la escala de equipo 1–10; el sentido de mayor prioridad, los miembros de TL y los momentos de recálculo están pendientes (DP-01, DP-02, DP-03).

No reemplazar la política actual ni introducir una escala híbrida por intuición. No tratar aging y recálculo dinámico como el mismo mecanismo; su relación queda en DP-10.

## Evidencia y criterios de aceptación

Consulta [requisitos RD-04–RD-07](docs/REQUISITOS_DOCENTE.md), [P1–P7](docs/CASOS_REFERENCIA.md) y [DP-01–DP-05, DP-10](docs/DECISIONES_PENDIENTES.md).

- Las prioridades docentes producen exactamente los resultados de frontera del caso P1 si se implementan por decisión aprobada.
- READY se distingue de WAIT, RUN, FINISH y CANCEL al seleccionar según política actual.
- FIFO de llegada, selección por mayor prioridad y Round Robin de reingreso están descritos separadamente.
- Las pruebas unitarias se interpretan según lo que configuran; una prueba que cambia `ordenLlegada` a mano no prueba la actualización del motor.
- Aging no se elimina ni se confunde con reclasificación por tamaño.
- Ningún cambio de prioridad/contrato se presenta como aprobado antes de cerrar las decisiones correspondientes.
