# Integrante 5 — GUI, visualización e integración

Documento recuperado de `origin/poma`. Se conservan su responsabilidad de GUI, integración y uso inicial de mocks, con una especificación visual más precisa y alineada a fuentes y decisiones canónicas.

## Objetivo y toolkit

La GUI solicita acciones, consume snapshots/eventos y representa el simulador; no decide prioridades ni ordena procesos. El documento original recomienda Qt 6 + C++, pero no existe confirmación del docente ni dependencia Qt en las ramas auditadas. Tratar Qt 6 como recomendación del equipo hasta que se ratifique (DP-11).

Las notas describen procesos que llegan, entran a memoria, pasan al procesador y muestran un temporizador de atención (págs. 23 y 29; transcripción 00:08:25–00:09:28). La pantalla debe permitir comprender visualmente ese flujo.

## Elementos visuales obligatorios

1. Lista dinámica de procesos: PID, nombre, tamaño, prioridad y estado, más tiempos aprobados.
2. Cola FIFO con prioridades: procesos READY y su prioridad/posición de selección. Hacer visible el criterio de desempate aprobado.
3. Mapa gráfico conceptual de memoria: capacidad, ocupación por proceso y espacio disponible; no reducirlo a dos totales, pero tampoco fingir direcciones o administración avanzada.
4. CPU lógico: proceso actualmente atendido y su estado.
5. Temporizador del turno de ejecución.
6. Timeline gráfico de time-sharing.
7. Estados y transiciones observables.
8. Log textual de eventos con historial.
9. Controles y acciones acordadas por el equipo: agregar/cancelar, iniciar/pausar, paso, reiniciar, velocidad visual y selección de motor donde esté integrada.

## Timeline

El timeline es una visualización gráfica de intervalos de CPU sobre un eje temporal. Cada segmento identifica el proceso y cubre su intervalo de ejecución; la anchura es proporcional a la duración. La página 28 compara el trabajo serial con muchos turnos de time-sharing y muestra segmentos de distintos anchos. Por tanto, el timeline no es la tabla de procesos, la cola de prioridad, el registro textual ni un archivo exportado. No se promete exportación a PNG, HTML o PDF.

La escala y el sentido de avance deben ser legibles. No están acordados el tratamiento gráfico de CPU inactivo, retención de historial, límite, desplazamiento ni zoom (DP-08). No etiquetar una decisión de diseño como exigencia docente.

## CPU, tiempo y arquitectura

Mostrar un único CPU lógico. El motor concurrente puede usar threads reales internamente, pero no mostrar workers como núcleos ni varios procesos simultáneamente ejecutándose en ese CPU lógico. El profesor explica turnos y sensación de concurrencia, no una obligación de implementar threads (notas, pág. 28).

El quantum global configurable de 500 ms es decisión del equipo. La cuota individual de las notas y el ejemplo `prioridad × 10 ms` de la transcripción son conceptos distintos y requieren resolución DP-04. La velocidad visual no debe cambiar el tiempo simulado. `siguientePaso()` necesita la decisión DP-07 antes de definir cuántas transiciones o cuánto tiempo debe avanzar un clic.

## Casos y aceptación

Consulta [P4, P9–P13](docs/CASOS_REFERENCIA.md), [RD-11–RD-12, DP-04, DP-07–DP-08, DP-11](docs/MATRIZ_TRAZABILIDAD.md) y [especificación](docs/ESPECIFICACION_FUNCIONAL.md).

- Tabla, cola, memoria, CPU, temporizador, estados, timeline y log se distinguen sin confundir conceptos.
- La cola sigue la selección publicada por Scheduler; GUI no inventa criterio.
- Mapa de memoria individualiza ocupantes y proporciones sin afirmar ubicación real.
- Timeline representa duraciones con anchos proporcionales y mantiene orden temporal.
- GUI no afirma que los bloques sean exportables ni que Qt esté aprobado si no se ratifica.
- Los dos motores usan la misma vista si la arquitectura de equipo se conserva; no se infiere su obligatoriedad a partir de la explicación de concurrencia.
