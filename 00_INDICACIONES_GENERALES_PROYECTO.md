# Indicaciones generales de ConcurrentSimulator

## Propósito y documentación canónica

ConcurrentSimulator es un simulador gráfico de procesos, memoria y uso de CPU mediante turnos de time-sharing. Para evitar que los integrantes implementen reglas diferentes, utiliza este orden de lectura: [README](README.md), [especificación funcional](docs/ESPECIFICACION_FUNCIONAL.md), [requisitos del docente](docs/REQUISITOS_DOCENTE.md), [decisiones pendientes](docs/DECISIONES_PENDIENTES.md), [matriz](docs/MATRIZ_TRAZABILIDAD.md), [casos de referencia](docs/CASOS_REFERENCIA.md) y [contratos v1.0](contracts/README.md).

Los materiales del profesor se conservan fuera del repositorio. Las fuentes canónicas para sus reglas son las notas, págs. 23–29, y la transcripción del 1 de octubre de 2026, 00:03:59–00:14:22. Los ejemplos de QR, matrices, speedup y grafos no pertenecen al simulador.

Cada regla importante utiliza una etiqueta: `CONFIRMADO_POR_DOCENTE`, `PROPUESTA_DOCENTE`, `DECISION_EQUIPO` o `PENDIENTE_CONFIRMACION`, con página/minuto y documentación afectada. Una fuente explícita puede dejar detalles sin resolver; no completar vacíos por intuición.

## Modelo funcional

Las notas identifican procesos por ID y nombre, tamaño en bytes, estado, cuota en milisegundos y prioridad. El equipo define además prioridad base/actual, tiempo total/restante/espera, quantum consumido, orden de llegada y operación de E/S opcional en el contrato vigente. Consulte [contratos](contracts/README.md); los encabezados v1.0 no deben modificarse sin nueva versión aprobada.

Estados observados: `READY`, `RUN`, `WAIT`, `FINISH` y `CANCEL`. Las transiciones de diseño, su evidencia y estado de aprobación se detallan en la [especificación](docs/ESPECIFICACION_FUNCIONAL.md) y DP-06. El time-sharing requiere alternar procesos al agotarse el turno, pero el diagrama docente no muestra todas las flechas.

## Prioridades, FIFO y tiempo

La página 24 documenta `AssignPriority()` con mínimo `A`, máximo `B`, `H=(B-A)/3`, y límites estrictos `< A+H` y `< A+2H`, que asignan prioridad 1, 2 o 3. El repositorio conserva una política de equipo 1–10 (10 máxima), prioridad manual, quantum global predeterminado de 500 ms y aging cada 2,000 ms. No tratar esta política como exigencia docente ni reemplazarla sin decisión. La cuota individual descrita en la fuente y la fórmula ilustrativa `prioridad × 10 ms` tampoco se deben confundir con el quantum global. Consulte DP-01, DP-04 y DP-10.

FIFO conserva orden de llegada; prioridad decide qué clase READY gana; FIFO resuelve empates iniciales; Round Robin describe la rotación de reingreso tras agotar turno. El código concurrente actualmente no actualiza `ordenLlegada` al reencolar tras quantum, aunque la prueba del Scheduler prepara manualmente un nuevo orden. La discrepancia está registrada en DP-05; no corregir código en una tarea documental.

## Vista gráfica

La vista debe reunir procesos dinámicos, cola FIFO con prioridades, mapa conceptual de memoria ocupada por procesos, CPU lógico y proceso atendido, temporizador, estados/transiciones, log de eventos, controles del equipo y timeline.

El timeline es una vista gráfica de segmentos sobre un eje temporal; cada segmento representa un intervalo de ejecución y su anchura es proporcional a su duración. No es la tabla de procesos, la cola, el log ni un archivo PNG/HTML/PDF. Las notas sitúan la comparación de procesamiento serial y time-sharing en la pág. 28, y el flujo proceso/cola/memoria/CPU en la pág. 29. CPU idle, escala, retención y desplazamiento están pendientes. Qt 6 es recomendado en la asignación de GUI, no un requisito docente ni una dependencia aprobada.

## Arquitectura y responsabilidades

El equipo mantiene como decisión dos motores: uno lógico sin threads reales y otro concurrente con `std::thread`, mutex y variables de condición. Ambos representan un único CPU lógico: los hilos reales no significan que varios procesos reciban CPU simulado simultáneamente. La explicación docente sobre concurrencia no obliga por sí misma a implementar ambos motores.

| Documento | Área | Alcance |
|---|---|---|
| [01 Modelo y memoria](01_INTEGRANTE_MODELO_ESTRUCTURAS.md) | Contratos vigentes, almacenamiento y memoria | No decide planificación, threads ni GUI |
| [02 Scheduler](02_INTEGRANTE_SCHEDULER.md) | Selección READY, empates y política temporal aprobada | No implementa GUI ni threads |
| [03 Motor lógico](03_INTEGRANTE_SIMULACION_LOGICA.md) | Reloj, eventos y transición sin threads reales | No duplica política ni GUI |
| [04 Motor concurrente](04_INTEGRANTE_MOTOR_CONCURRENTE.md) | Coordinación de workers reales | Un solo proceso puede recibir CPU lógico a la vez |
| [05 GUI e integración](05_INTEGRANTE_GUI_INTEGRACION.md) | Visualización, controles e integración de motores | No decide política del Scheduler |

Los documentos 02–05 se recuperaron de sus ramas originales: `brickzon` (Scheduler), `aldana` (simulación lógica), `angel` (motor concurrente) y `poma` (GUI). Sus detalles se armonizaron con esta guía y la especificación canónica; la atribución por rol y procedencia se conserva en los encabezados. La guía de agentes está en [AGENTS.md](AGENTS.md).

## Construcción

Requisitos observados en la rama base: compilador C++17, CMake 3.20 o posterior y CTest. Los comandos de build y test están en el README. Los documentos describen el flujo de validación, no un resultado de ejecución de esta rama documental.
