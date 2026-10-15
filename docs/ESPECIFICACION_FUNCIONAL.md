# Especificación funcional de ConcurrentSimulator

> **Estado canónico desde Fase 0 (2026-10-15):** decisiones aprobadas DP-01–DP-11 gobiernan el destino contractual v2; el texto histórico más abajo conserva procedencia/estado anterior, y no reabre alternativas. v1.0 sigue vigente para el código existente. Véase [decisiones aprobadas](DECISIONES_APROBADAS.md), [contratos v2 candidatos](CONTRATOS_V2.md) y [matriz de migración](MIGRACION_CONTRATOS_V2.md). Prioridad 1–10, quantum global 500 ms y aging describen solo v1 congelada/antecedentes, no política activa v2. Los detalles no aprobados son propuestas técnicas separadas.

Esta especificación consolida el comportamiento documentado del simulador y separa lo respaldado por las fuentes del docente de las decisiones existentes del equipo. Los contratos `contracts/*.h` y su [README](../contracts/README.md) describen la API vigente 1.0; esta especificación no los modifica ni convierte propuestas pendientes en una nueva API.

## Alcance y estado de la evidencia

ConcurrentSimulator representa la llegada y admisión de procesos, su prioridad, la ocupación de memoria, el uso de un CPU lógico por turnos, los estados y el historial visual. Las notas académicas fijan el concepto de procesos en competencia, prioridades derivadas del tamaño en tres clases, y time-sharing en un procesador lógico. La política 1–10, quantum global 500 ms, aging de 2,000 ms, dos motores y campos del contrato 1.0 son decisiones registradas del equipo. Su coexistencia con la propuesta docente está pendiente; consultar [decisiones pendientes](DECISIONES_PENDIENTES.md).

El recorrido funcional es: crear o recibir un proceso; validar y admitirlo según las reglas vigentes del equipo; incorporarlo a READY y a la vista de cola; elegir un proceso elegible según la política que se apruebe; concederle un intervalo de CPU; registrar tiempo, estado, evento y segmento de timeline; y repetir hasta completar, esperar, cancelar o no quedar trabajo ejecutable. Las reglas exactas de admisión, cuota, priorización dinámica y paso deben cerrarse antes de afirmar equivalencia entre motores.

## Entidades y conceptos

### Proceso

Las notas enumeran identificador, nombre, tamaño de memoria, estado, cuota en milisegundos y prioridad (pág. 23). El contrato 1.0 implementado además define prioridad base y actual, tiempo total/restante/espera, quantum consumido, orden de llegada y una operación opcional de E/S. Sus unidades y validaciones vigentes están en [contracts/README.md](../contracts/README.md) y [Proceso.h](../contracts/Proceso.h). Una cuota individual explícita no forma parte de esa estructura congelada.

### Estados y transiciones

La matriz cubre los cinco estados definidos. `D` indica una flecha dibujada en el diagrama docente de la pág. 27 o una explicación docente separada de time-sharing en la pág. 28, según se cita en cada celda; no acredita una semántica temporal completa. `E` indica decisión/documentación del equipo; `I`, comportamiento observado en la implementación concurrente; `?`, transición no especificada. `?` no significa prohibida. Una llamada cancelar sobre FINISH no produce transición en la implementación actual. Los estados repetidos no son transición.

| Desde ↓ / Hacia → | READY | RUN | WAIT | FINISH | CANCEL |
|---|---|---|---|---|---|
| READY | — | D: flecha en diagrama p. 27; condición temporal no indicada | ?: sin regla | ?: sin regla | ?: sin flecha en diagrama |
| RUN | I: el motor concurrente actual reingresa a READY al agotar quantum; política común y orden pendientes DP-05/06/07 | — | D: flecha en p. 27; causa/duración no indicadas | D: flecha en p. 27; condición temporal no indicada | D: flecha en p. 27; condición temporal no indicada |
| WAIT | D: flecha en p. 27; condición/duración de retorno no indicadas | ?: sin flecha en diagrama | — | ?: sin flecha en diagrama | ?: sin flecha en diagrama |
| FINISH | ?: sin regla | ?: sin regla | ?: sin regla | — | I: cancelar no cambia estado |
| CANCEL | ?: sin regla | ?: sin regla | ?: sin regla | ?: sin regla | — |

Las notas enumeran READY, RUN, WAIT, CANCEL y FINISH (pág. 23). El diagrama de la página 27 dibuja READY→RUN, RUN→WAIT, RUN→FINISH, RUN→CANCEL y WAIT→READY. Cada flecha confirma que esa transición está representada en el diseño del docente; por sí sola no establece disparador, duración, orden temporal, política de admisión ni comportamiento de cada motor. La explicación de la página 28 dice que al pasar el tiempo asignado a un proceso entra otro, pero no especifica el estado del proceso anterior; no se atribuye RUN→READY a esa fuente. La implementación concurrente actual reingresa a READY al agotar quantum; su política común sigue pendiente. Las flechas ausentes quedan sin especificar, no prohibidas.

La [matriz de transiciones](#estados-y-transiciones) conserva por separado las flechas dibujadas, la explicación temporal de la página 28, el diseño del equipo y el comportamiento implementado. El alcance más amplio de transiciones, condiciones, eventos y cancelación debe acordarse en [DP-06](DECISIONES_PENDIENTES.md).

## Prioridad y cola

La regla docente `AssignPriority()` calcula mínimo `A`, máximo `B` y `H=(B-A)/3`; usa límites estrictos `< A+H` y `< A+2H`, asignando clases 1, 2 y 3 (notas, pág. 24). Por ejemplo, con `A=12`, `B=96`, `H=28`: `[12,40)` recibe 1, `[40,68)` recibe 2 y `[68,96]` recibe 3 para tamaños enteros incluidos en el conjunto. La página 25 presenta `ObtenerMaximaPrioridad()`; al comparar valores de prioridad, P3 tiene precedencia sobre P2 y P1. Está pendiente decidir si el equipo adopta la escala docente o la adapta a la política 1–10 (DP-01). El conjunto TL, los momentos de recálculo y el caso degenerado siguen en DP-02 y DP-03.

En el repositorio, `ColaProcesos` conserva orden de inserción y no planifica; `Scheduler` en ramas `brickzon` y `angel` selecciona la mayor prioridad actual y desempata por `ordenLlegada`, luego PID. FIFO describe el orden relativo de llegada dentro de una cola/clase; Round Robin describe el turno rotativo que surge cuando un proceso agotó su intervalo y se reencola detrás de sus pares. No son sinónimos. El Scheduler por sí solo no puede producir esa rotación si el motor no actualiza el orden de reingreso. La prueba de scheduler modela el cambio a mano; `ConcurrentEngine` no actualiza `ordenLlegada` al agotar quantum. No se corrige código en esta tarea.

El aging existente suma prioridad a procesos READY cada 2,000 ms de espera hasta 10 y restablece la prioridad base al asignar CPU. La reclasificación docente por cambios del rango de tamaños es otro mecanismo. Su coexistencia o sustitución requiere decisión explícita; no se elimina el aging.

## Tiempo, cuotas y eventos

El contrato actual usa milisegundos simulados y configura un quantum global positivo, predeterminado en 500 ms por la configuración del equipo. Las notas incluyen cuota como atributo del proceso (pág. 23), y la transcripción ofrece `prioridad × 10 ms` como ejemplo (00:11:50–00:12:07). Cuota individual y quantum global son conceptos diferentes: el primero sería el tiempo concedido a un proceso; el segundo es el límite común vigente del motor. No hay compatibilidad acordada para migrar entre ambos.

El contrato define `siguientePaso()` como una transición significativa; los documentos de integrantes alternan entre transición/evento y una ráfaga/turno completo. El motor concurrente implementado ejecuta una asignación/ráfaga (hasta quantum, fin o E/S) por llamada. El motor lógico no está presente en las ramas auditadas. El paso debe tener semántica única antes de exigir que ambos motores produzcan tiempos, estados, eventos y bloques equivalentes. Mientras tanto, resultados que dependan del tamaño de paso quedan condicionados a DP-07.

Los eventos deben permitir explicar admisión, cambio de estado, agotamiento de quantum, fin, espera, cancelación y cambio de prioridad, según el contrato y la decisión de equipo. El orden, granularidad y timestamp exactos se rigen por la decisión DP-07.

## Memoria

El comportamiento actual admite un proceso solo si es válido, no está ya cargado y cabe en la capacidad disponible. La implementación registra bytes ocupados por PID y libera la asignación al finalizar o cancelar. No asigna direcciones ni describe regiones contiguas. La vista funcional debe ir más allá de totales: presentar un mapa gráfico conceptual de memoria ocupada por procesos, sus tamaños relativos y disponibilidad, conectado visualmente con la lista/cola y CPU como en las notas, págs. 23 y 29 y la transcripción 00:08:25–00:09:28. El orden de bloques, orientación, escala y representación cuando hay falta de espacio están pendientes. No se requieren paginación, segmentación, memoria virtual ni reemplazo.

## Interfaz gráfica

La GUI se define como una vista coordinada de:

1. Lista dinámica de procesos con identificador, nombre, tamaño, prioridad, estado y tiempos acordados.
2. Cola READY con prioridad y orden de selección visibles.
3. Mapa conceptual de memoria que distingue procesos cargados y espacio disponible.
4. CPU lógico que muestra el proceso atendido y su estado.
5. Temporizador del intervalo de atención.
6. Timeline gráfico de time-sharing.
7. Estados y transiciones observables.
8. Registro textual de eventos y controles acordados por el equipo (iniciar, pausar, paso, reiniciar, agregar/cancelar y modo cuando exista integración).

El timeline es una visualización gráfica de segmentos sobre un eje temporal. Cada segmento representa un intervalo de CPU de un proceso y su anchura es proporcional a la duración del intervalo. Es distinto de la tabla, la cola, el registro textual y cualquier archivo exportado. La guía visual docente es la comparación serial/time-sharing de la pág. 28. Las notas no exigen PNG, HTML, PDF ni exportación.

La GUI debe comunicar el CPU lógico único sin sugerir que los hilos de implementación son varios CPUs simulados. La escala exacta, segmentos de CPU inactivo, retención del historial y desplazamiento son decisiones pendientes (DP-08). Qt 6 es recomendado en el documento de Poma, no está confirmado como framework aprobado y no aparece en el código de las ramas auditadas.

## Arquitectura y contratos

El equipo documenta dos estrategias: motor lógico sin threads y motor concurrente con `std::thread`, mutex y variables de condición. Los threads reales son un detalle de implementación para coordinar workers; el Scheduler concede un único turno de CPU lógico a la vez. La explicación del profesor sobre time-sharing no obliga a tener ambas variantes. Mantenerlas forma parte de la decisión de arquitectura del equipo hasta nueva aprobación.

Los encabezados compartidos v1.0 permanecen congelados. Cualquier aprobación de cuota individual, prioridades 1–3, política de recálculo, unidad de paso o nuevo tipo de segmento de timeline exige evaluar y versionar una migración coordinada; no se anticipa el cambio en esta especificación.

## Criterios de aceptación funcionales

- La implementación identifica claramente qué comportamiento corresponde al contrato 1.0, qué regla docente implementa y qué política fue acordada por el equipo.
- El algoritmo de prioridad 1–3 produce los resultados de frontera de [los casos de referencia](CASOS_REFERENCIA.md), sin reemplazar silenciosamente el modelo 1–10.
- El reingreso tras turno respeta la rotación aprobada entre pares de prioridad y se distingue de FIFO de llegada inicial.
- Memoria muestra procesos ocupantes y no presenta direcciones físicas inexistentes.
- Timeline codifica intervalos y duración proporcional sobre tiempo; no se confunde con el registro de eventos.
- Motor lógico y concurrente, si ambos se conservan, publican resultados equivalentes para los escenarios acordados.
- El mapa de transiciones y la unidad de `siguientePaso()` están aprobados y documentados antes de declarar equivalencia.
