# Decisiones pendientes para ConcurrentSimulator

> **Cierre Fase 0 (2026-10-15):** DP-01–DP-11 fueron aprobadas por coordinación antes de esta fecha y se formalizan en [DECISIONES_APROBADAS.md](DECISIONES_APROBADAS.md). Las alternativas de los apartados siguientes son historial de deliberación; no representan opciones actualmente abiertas ni política v2. v1.0 sigue vigente para el código presente. Precisiones de implementación no ratificadas están separadas como propuestas técnicas en el registro aprobado.

| Decisión histórica | Cerrada | Registro normativo |
|---|---|---|
| DP-01 | 2026-10-15 (formalización; aprobación previa por coordinación) | [Decisión aprobada](DECISIONES_APROBADAS.md#dp-01--prioridades) |
| DP-02 | 2026-10-15 (formalización; aprobación previa por coordinación) | [Decisión aprobada](DECISIONES_APROBADAS.md#dp-02--población-y-recalculo) |
| DP-03 | 2026-10-15 (formalización; aprobación previa por coordinación) | [Decisión aprobada](DECISIONES_APROBADAS.md#dp-03--algoritmo-exacto) |
| DP-04 | 2026-10-15 (formalización; aprobación previa por coordinación) | [Decisión aprobada](DECISIONES_APROBADAS.md#dp-04--cuotas-individuales) |
| DP-05 | 2026-10-15 (formalización; aprobación previa por coordinación) | [Decisión aprobada](DECISIONES_APROBADAS.md#dp-05--cola-y-round-robin) |
| DP-06 | 2026-10-15 (formalización; aprobación previa por coordinación) | [Decisión aprobada](DECISIONES_APROBADAS.md#dp-06--estados-y-cancelación) |
| DP-07 | 2026-10-15 (formalización; aprobación previa por coordinación) | [Decisión aprobada](DECISIONES_APROBADAS.md#dp-07--paso-y-tiempo-simulado) |
| DP-08 | 2026-10-15 (formalización; aprobación previa por coordinación) | [Decisión aprobada](DECISIONES_APROBADAS.md#dp-08--gui-y-timeline) |
| DP-09 | 2026-10-15 (formalización; aprobación previa por coordinación) | [Decisión aprobada](DECISIONES_APROBADAS.md#dp-09--estructura-enlazada) |
| DP-10 | 2026-10-15 (formalización; aprobación previa por coordinación) | [Decisión aprobada](DECISIONES_APROBADAS.md#dp-10--aging) |
| DP-11 | 2026-10-15 (formalización; aprobación previa por coordinación) | [Decisión aprobada](DECISIONES_APROBADAS.md#dp-11--interfaz-y-motores) |

Las alternativas y propuestas de esta lista no están aprobadas. Resolverlas requiere acuerdo del equipo y, cuando corresponda, consulta al docente. El orden refleja impacto sobre contratos, módulos e interoperabilidad.

## DP-01 — Política prioritaria y compatibilidad de escalas

**Alternativas:** conservar la política de equipo 1–10 con prioridad manual; adoptar las clases docentes 1–3 calculadas por tamaño; o aprobar una transformación entre clases docentes y escala contractual 1–10.

**Evidencia:** notas, pág. 24, `AssignPriority()` asigna 1 a pequeños, 2 a medianos y 3 a grandes; `ObtenerMaximaPrioridad()` en pág. 25 conserva el valor mayor, así que P3 precede a P2 y P1. La transcripción 00:11:02–00:14:22 explica el rango dinámico. README y contratos 1.0 fijan 1–10 y el mayor número como mayor prioridad.

**Impacto:** validación, Scheduler, formulario GUI y ejemplos; una adaptación o sustitución puede exigir migración contractual. La precedencia interna de P1/P2/P3 no está pendiente; falta decidir únicamente si adoptar esa escala o adaptarla al modelo 1–10.

**Propuesta para deliberar:** decidir con el equipo si se reemplaza la política vigente por las clases docentes o si se aprueba una adaptación explícita. No definir una conversión unilateral.

## DP-02 — Conjunto usado por `AssignPriority()` y momentos de recálculo

**Alternativas:** calcular sobre todos los procesos creados; solo READY; procesos admitidos en memoria; o un conjunto de trabajo definido por el equipo. Elegir recálculo en cada llegada, admisión, cambio de estado o lote explícito.

**Evidencia:** el pseudocódigo recorre `TL[1..N]` (notas, pág. 24), pero no define la inclusión de WAIT, FINISH, CANCEL ni RUN. La transcripción dice que la llegada de otro programa puede cambiar las bandas (00:12:07–00:14:22), sin enumerar otras causas.

**Impacto:** prioridad de todos los procesos, orden de READY, fairness, eventos y costo de actualización. Recalcular el rango puede cambiar prioridades de procesos ya esperando o RUN.

**Propuesta para deliberar:** pedir al docente que precise si TL es la lista completa o solo la cola activa y cuándo propagar una reclasificación. Hasta entonces no excluir estados por intuición.

## DP-03 — Igual tamaño y precisión de los umbrales

**Alternativas:** conservar literalmente la consecuencia `A=B => H=0 => prioridad 3 para todo elemento`; definir un caso especial; o precisar cómo se calcula `H` y se tratan los límites cuando `(B-A)` no es divisible por 3. El contrato actual representa tamaños en bytes enteros, no fracciones.

**Evidencia:** `H=(B-A)/3` y límites estrictos `<` (notas, pág. 24); no hay ejemplo de tamaños iguales.

**Impacto:** clasificación de casos degenerados y reglas de frontera. Con tamaños enteros, una implementación puede truncar `H` por división entera si `B-A` no es divisible por 3; la fuente no indica el tipo numérico ni una regla de redondeo.

**Propuesta para deliberar:** mantener los operadores `<` y confirmar con docente el caso A=B y la precisión antes de fijar una definición normativa completa.

## DP-04 — Cuota individual frente a quantum global

**Alternativas:** conservar solo el quantum global configurable de 500 ms; adoptar una cuota por proceso; o mantener ambos, precisando si el intervalo efectivo es la cuota propia, el quantum global, su mínimo u otra regla.

**Evidencia:** las notas listan cuota como atributo (pág. 23); la transcripción ilustra `prioridad × 10 ms` (00:11:50–00:12:07) y también muestra turnos de duraciones distintas (00:06:29–00:07:37). README/configuración de equipo y contrato 1.0 definen quantum global 500 ms.

**Impacto potencial:** `Proceso` necesitaría el campo cuota; `EstadoSistema` requeriría exponerla si la GUI debe mostrarla; `IExecutionEngine` tendría que definir configuración/precedencia; Scheduler, motores y GUI deben aplicar y representar la misma regla. Podría requerir contrato 2.0 y migración coordinada. No editar la API v1.0 mientras no se apruebe.

**Propuesta para deliberar:** confirmar con docente si la cuota por prioridad es requisito o ejemplo, y definir explícitamente cómo convive con el quantum global. Mantener ambas decisiones actuales como alternativas registradas.

## DP-05 — Cola, desempate FIFO y rotación Round Robin

**Alternativas:** representar READY en orden de llegada y rotar al reingresar tras turno; actualizar un ticket/orden de reingreso en cada agotamiento; o definir otra estructura de cola estable por prioridad.

**Evidencia:** notas, pág. 23, nombran una organización FIFO; el equipo define selección por prioridad y FIFO/RR entre empates. La prueba `test_scheduler_empate.cpp` crea el resultado rotatorio cambiando `ordenLlegada` a mano. El `ConcurrentEngine` de `origin/angel` no cambia `ordenLlegada` al terminar `QUANTUM_AGOTADO`, y `Scheduler` sigue desempantando con ese campo.

**Impacto:** testabilidad, starvation, orden mostrado y equivalencia del motor integrado. La prueba unitaria por sí sola no acredita la política completa del motor.

**Propuesta para deliberar:** ratificar si el empate es FIFO de llegada al READY o Round Robin de turnos, y definir el evento exacto que actualiza el orden. No corregir el código bajo este encargo documental.

## DP-06 — Matriz de transiciones y cancelación

**Alternativas:** acordar los estados de destino y eventos del proceso que deja CPU al acabar su turno; establecer si el diseño del equipo añade transiciones no dibujadas; especificar condiciones y eventos de las transiciones del diagrama; definir cancelación por estado y terminalidad de FINISH/CANCEL. Las flechas READY→RUN, RUN→WAIT/FINISH/CANCEL y WAIT→READY sí aparecen en el diagrama docente.

**Evidencia:** estados en notas, pág. 23; flechas indicadas en el diagrama, pág. 27; la pág. 28 dice que, una vez pasa el tiempo asignado a un proceso, entra otro, pero no nombra el estado del proceso anterior. El diagrama no define disparadores, duración ni orden temporal completo, y las flechas que no dibuja quedan sin especificar, no prohibidas. El motor concurrente actual reingresa a READY al agotar quantum y permite cancelar READY, RUN y WAIT; ignora PID inexistentes, FINISH o ya CANCEL. Es comportamiento implementado, no mandato docente.

**Impacto:** validación de operaciones, emisión de eventos, memoria, hilos activos, representación y pruebas de motores; distinguir las flechas dibujadas de la transición RUN→READY de implementación, y RUN→CANCEL dibujada de los detalles sobre cuándo y cómo cancelar.

**Propuesta para deliberar:** aprobar una matriz completa de estados-transiciones y el efecto de cancelar durante RUN; conservar diferenciada la semántica implementada de la evidencia académica.

## DP-07 — Significado de `siguientePaso()` y avance temporal

**Alternativas:** una transición estado-a-estado; un evento significativo (posiblemente varios por operación); o una asignación de CPU/ráfaga completa hasta quantum, WAIT o FINISH.

**Evidencia:** contrato 1.0 define “una transición significativa”; guía de motor lógico habla de pasos/eventos y ciclo de quantum; documento GUI dice “exactamente una transición/evento”; `ConcurrentEngine::siguientePaso()` llama a `ejecutarPasoInterno`, que espera una ráfaga de worker hasta un motivo de fin de turno.

**Impacto:** API observable, pausa, timeline, snapshots, número de clics, reloj, eventos y equivalencia entre motores.

**Propuesta para deliberar:** definir con un ejemplo reproducible cuánto avanza el reloj y qué eventos pueden ocurrir en una llamada; después alinear ambos motores en esa unidad.

## DP-08 — Detalles de timeline y mapa gráfico de memoria

**Alternativas:** segmento sin actividad o banda de CPU inactivo; eje con escala fija o adaptativa; historial ilimitado, ventana desplazable o límite configurable; memoria conceptual con bloques proporcionales en orden de admisión u otro orden; y definir la cadencia/fuente de actualización que permita percibir el estado RUN aunque sea transitorio.

**Evidencia:** notas, págs. 28–29, time-sharing temporal y diagrama de procesos/cola/memoria/CPU; transcripción 00:08:25–00:09:28. Contrato 1.0 tiene bloques `[inicio, fin)` solo para PID de proceso y la implementación de memoria no conserva dirección ni orden de colocación.

**Impacto:** tipo y retención del timeline, consistencia de duraciones, rendimiento de GUI, visibilidad de RUN transitorio, interpretación de CPU ocioso y diseño del widget de memoria. La actualización visual debe presentar el estado observado sin avanzar ni alterar el reloj simulado; su cadencia no define el paso del motor (DP-07). Una región física/dirección real sería una ampliación no respaldada.

**Propuesta para deliberar:** acordar escala, idle, retención/desplazamiento, orden conceptual de bloques y cómo hacer visible RUN transitorio sin modificar tiempo simulado. Mantener el requerimiento visual; no prometer exportación PNG/HTML/PDF.

## DP-09 — Lista enlazada simple frente a `std::list`

**Alternativas:** cumplir literalmente la intención docente con una lista enlazada simple dinámica y punteros de inicio/final; o confirmar con el docente/equipo que `std::list` satisface la entrega pese a que no es una lista simple. La fuente expresa que se implementará la primera estructura; queda pendiente si esa forma concreta es criterio obligatorio de aceptación.

**Evidencia:** notas, pág. 25: “Se va implementar un modelo dinámico basado en una estructura de lista enlazada simple que posee 2 punteros, al inicio y al final.” `core/ColaProcesos.h` en master usa `std::list<Proceso>`, normalmente doblemente enlazada; preserva inserción, pero no agenda.

**Impacto:** implementación, complejidad, pruebas y posible expectativa académica. El pseudocódigo de inserción/búsqueda de prioridad en esa página está incompleto/inconsistente.

**Propuesta para deliberar:** conservar la intención literal en el plan y confirmar si la estructura específica será evaluada como requisito de entrega antes de cambiar la implementación actual. No rebajar la intención a una mera recomendación ni declarar incumplimiento hasta resolver la obligatoriedad.

## DP-10 — Aging y prioridad dinámica por tamaño

**Alternativas:** mantener ambos; mantener solo aging; o sustituir aging por recálculo de bandas de tamaño.

**Evidencia:** aging 2,000 ms, +1 y máximo 10 aparece en política del equipo, no en las secciones académicas pertinentes. La transcripción describe recalcular rangos por llegadas de nuevos tamaños (00:12:07–00:14:22).

**Impacto:** prioridades pueden subir por espera y bajar/cambiar al recalcular bandas. Debe definirse prioridad base, cuándo aging opera respecto al recálculo, máximo de escala, acumulación de espera, y eventos/tie-breaks.

**Propuesta para deliberar:** comparar ambos mecanismos como objetivos distintos (justicia por espera frente a clasificación relativa por tamaño) y aprobar orden/precedencia sin eliminar aging por omisión.

## DP-11 — Toolkit de GUI y dos motores

**Alternativas:** confirmar Qt 6; elegir otra biblioteca; mantener ambos motores de equipo; o simplificar a un motor si el equipo lo acuerda.

**Evidencia:** documento de Poma recomienda Qt 6; el código revisado no contiene GUI Qt ni motor lógico. La explicación docente describe CPU lógico por turnos y no obliga a usar hilos reales ni dos arquitecturas.

**Impacto:** build, integración, plataforma, seguridad de GUI y costo de mantener equivalencia.

**Propuesta para deliberar:** ratificar toolkit y alcance de dos motores en decisión explícita del equipo. Hasta entonces Qt es recomendación y los dos motores, arquitectura acordada de equipo.
