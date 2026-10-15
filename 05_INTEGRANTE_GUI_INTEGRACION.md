# Integrante 5 — GUI, visualización e integración

**Responsable:** Poma · **Rama:** `poma` · **Workflow:** [fase 4](docs/WORKFLOW_DESARROLLO.md#fase-4--gui-y-visualizacion)

Asignación recuperada de `origin/poma`. Qt 6 + C++ se conserva como **recomendación del equipo**, no requisito del docente ni toolkit aprobado. La GUI debe poder empezar con un mock mientras los motores avanzan.

## Principio de diseño y dependencias

La GUI solamente envía acciones, consume snapshots/eventos y los presenta. No ordena procesos, asigna prioridades, ejecuta aging ni implementa quantum. Debe depender de la API real de `IExecutionEngine` (`agregarProceso`, `cancelarProceso`, `iniciar`, `pausar`, `reiniciar`, `siguientePaso`, `configurarQuantum`, `obtenerQuantum`, `obtenerEstado`) y de `EstadoSistema`; no copiar interfaces anteriores a v1.0.

Comenzar con mock y fixtures de `EstadoSistema`. Integrar primero `SimulationEngine` y después `ConcurrentEngine`. La selección de modo cambia la instancia detrás de la misma interfaz, no la lógica visual. La regla de dos motores es decisión de equipo y no se deriva de que el profesor explique concurrencia.

## Componentes y rutas propuestas

- `gui/MainWindow.h/.cpp`: ventana principal, coordinación de controles y selección de motor.
- `gui/ProcessTable.h/.cpp`: tabla de procesos y acción de cancelar.
- `gui/QueueWidget.h/.cpp`: cola READY en orden publicado por Scheduler.
- `gui/CPUWidget.h/.cpp`: proceso atendido, estado y progreso temporal disponible.
- `gui/MemoryWidget.h/.cpp`: mapa conceptual y resúmenes de capacidad.
- `gui/TimelineWidget.h/.cpp`: segmentos ordenados sobre eje temporal.
- `gui/EventLogWidget.h/.cpp`: historial textual de eventos.
- `gui/NewProcessDialog.h/.cpp`: formulario y validación de alta.
- `main.cpp`: ensamblaje de ventana/motor.

Son rutas propuestas en la asignación de GUI, no archivos que existan en las ramas auditadas. Build/CMake se integra coordinadamente; esta tarea documental no los modifica. La asignación inicial no fija nombres de archivos de pruebas GUI; los escenarios abajo son criterios de integración, no una suite inventada como obligación.

## Pantalla y datos

### Controles

- Selector de modo “Simulación lógica” / “Threads reales” cuando ambas variantes estén integradas.
- Quantum global, velocidad visual, agregar proceso, iniciar, pausar, paso siguiente y reiniciar.
- Velocidades sugeridas por el equipo: `0.25x`, `0.5x`, `1x`, `2x`, `5x`. No son requisito del docente.
- Desacoplar velocidad visual de quantum y tiempo simulado.

### Tabla de procesos

Presentar identificador, nombre, tamaño, prioridad base y actual si aplica, estado, tiempo restante, espera y acción. No inventar una columna de cuota aprobada: en contrato v1.0 no existe; mostrarla solo después de una decisión y migración contractual.

### Cola

Representar solo procesos READY, con prioridad y orden que devuelve `EstadoSistema.colaPrioridad`. La GUI no reconstruye la selección con un orden propio. Resaltar o animar desplazamiento es sugerencia visual, no regla funcional. Debe quedar distinguible el FIFO de llegada inicial y la rotación tras quantum; el motor debe actualizar ticket/orden al reingresar según DP-05.

### CPU y temporizador

Mostrar PID/nombre del proceso atendido, estado RUN, prioridad vigente, tiempo restante y tiempo de intervalo disponible desde el snapshot. **Necesidad de GUI del equipo:** dar feedback visual al estado RUN aunque sea transitorio. Acordar la cadencia y el mecanismo de actualización de snapshots para que ese estado llegue a la vista. El refresco solo representa el reloj/estado publicado: no incrementa ni altera el tiempo simulado. Su cadencia de presentación queda en DP-08, separada de la semántica de avance del motor en DP-07. El equipo propone visualizar quantum actual/progreso; el contrato contiene quantum global y `Proceso.quantumConsumido`. No mostrar varios workers como CPUs simulados: el modelo es un CPU lógico. La velocidad visual solo controla la presentación.

### Memoria

Mostrar capacidad total, utilizada, disponible y procesos cargados; además, un mapa gráfico conceptual con porciones proporcionales identificadas por proceso. El código conserva tamaño por PID, no direcciones ni colocación física. No presentar un layout de direcciones real ni agregar paginación, segmentación, memoria virtual o reemplazo.

### Timeline

Debe ser una vista gráfica de segmentos de CPU sobre eje temporal; cada segmento se corresponde con `[inicio, fin)` y su anchura es proporcional a la duración. La retención/ventana del historial, escala, CPU idle, desplazamiento y zoom quedan pendientes DP-08. No se fija cuánto tiempo permanecen los segmentos visibles ni se garantiza conservar toda la sesión. No confundir con tabla, cola, log o exportación. No se pide PNG/HTML/PDF.

### Registro de eventos

Presentar historial, no solo último evento: altas/admisión, cambios de estado, quantum, WAIT/retorno, finalización, prioridad y cancelación según tipos de `Evento`. Mostrar timestamps en milisegundos simulados con formato legible, sin alterar el valor del snapshot.

## Diálogo de alta y cancelación

El diálogo propone campos Nombre, Tamaño, Tiempo total de CPU y Prioridad. Campos opcionales de E/S: habilitar, instante de bloqueo tras CPU y duración WAIT. Validar nombre no vacío, tamaño/tiempo positivos y prioridad conforme a la política seleccionada; mientras DP-01 siga abierta, no presentar una sola escala como docente y equipo indistintamente. La operación E/S debe ajustarse a validación de `procesoValido`.

La acción de cancelar vive junto a cada proceso; puede pedir confirmación. Refrescar estado/eventos después de la llamada y mostrar estado CANCEL cuando lo reporte el motor. La cancelación durante RUN y la política sobre intervalo parcial permanecen pendientes DP-06.

## Mock e integración

El mock implementa todos los métodos de `IExecutionEngine` vigente y devuelve snapshots controlados. Preparar vistas estáticas de:

1. Dos procesos READY en la cola.
2. Un proceso RUN con intervalo parcial reflejado en CPU/timeline.
   - La vista muestra el estado RUN transitorio a partir del snapshot; el refresco visual no adelanta el reloj simulado.
3. Proceso que agota quantum y retorna a READY con orden actualizado después de que el motor aplique la política aprobada.
4. Proceso WAIT con otro READY y retorno posterior.
5. FINISH y CANCEL, incluida memoria liberada como indique snapshot.
6. Memoria con más de un proceso y espacio disponible.
7. CPU sin READY, sin atribuir ejecución a un proceso.

El motor no notifica directamente widgets. En Qt, el equipo propone actualizar desde el hilo principal mediante signals/slots o consulta periódica con `QTimer`; el timer solo refresca/presenta snapshots y no define quantum. `QTimer` y Qt son detalles propuestos mientras no se ratifique DP-11.

## Pruebas de integración visuales

La asignación fuente no nombró archivos de tests GUI. Mantener los siguientes escenarios reproducibles dentro del mecanismo de pruebas/integración que el equipo elija:

- Mock y ambos motores producen las mismas columnas, cola, CPU, memoria, eventos y bloques para snapshots equivalentes.
- `agregarProceso` valida campos, refleja admisión y muestra error de memoria insuficiente sin inventar estado.
- `cancelarProceso` actualiza fila, memoria/log y cola del snapshot; cubre PID inexistente según contrato.
- Pausa, paso y reinicio activan los controles y actualizan vistas sin congelar ventana; semántica de paso condicionada a DP-07.
- El estado RUN transitorio puede observarse mientras ocurre; el temporizador/refresco GUI no cambia el tiempo simulado.
- Actualizaciones desde ConcurrentEngine llegan al hilo UI principal; carga concurrente de snapshots no toca widgets desde workers.
- Timeline conserva orden, marcas de tiempo y proporciones; espera/idle no aparece como ejecución de PID.
- Mapa refleja individualmente tamaños cargados, total y disponible sin direcciones físicas.
- Velocidad visual cambia el ritmo de refresco sin cambiar el reloj/resultado simulado.

## Criterios de aceptación

- Componentes de pantalla y diálogo existen en las rutas propuestas y se integran sin lógica de scheduling.
- GUI opera contra `IExecutionEngine`/`EstadoSistema`, no contra miembros internos del motor.
- Cola, CPU, memoria, timeline y eventos no se confunden y reflejan un mismo snapshot.
- Timeline tiene intervalos de anchura proporcional; memoria incluye ocupantes visuales.
- Un solo CPU lógico es visible aunque haya threads reales internos.
- Controles, pausa y actualización segura respetan decisiones DP-04, DP-06, DP-07 y DP-08.
- Velocidades visuales se presentan como sugerencias del equipo, no requisitos académicos.
