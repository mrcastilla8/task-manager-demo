# Decisiones pendientes para ConcurrentSimulator

Las alternativas y propuestas de esta lista no están aprobadas. Resolverlas requiere acuerdo del equipo y, cuando corresponda, consulta al docente. El orden refleja impacto sobre contratos, módulos e interoperabilidad.

## DP-01 — Política prioritaria y compatibilidad de escalas

**Alternativas:** conservar solo la política de equipo 1–10 con prioridad manual; adoptar clases docentes 1–3 calculadas por tamaño; o definir una transformación aprobada entre las clases 1–3 y la escala contractual 1–10.

**Evidencia:** notas, pág. 24, `AssignPriority()` asigna 1–3; transcripción 00:11:02–00:14:22 explica el rango dinámico. El README y los contratos 1.0 fijan 1–10 y el mayor número como mayor prioridad.

**Impacto:** campos base/actual, validación, scheduler, formulario GUI, ejemplos y contrato. La regla docente asocia tamaño pequeño con 1; con la semántica actual del scheduler esa clase tendría menos prioridad que 3.

**Propuesta para deliberar:** aclarar primero qué escala y sentido quería el docente para el simulador y si el equipo reemplaza o adapta su política. No definir una conversión unilateral.

## DP-02 — Conjunto usado por `AssignPriority()` y momentos de recálculo

**Alternativas:** calcular sobre todos los procesos creados; solo READY; procesos admitidos en memoria; o un conjunto de trabajo definido por el equipo. Elegir recálculo en cada llegada, admisión, cambio de estado o lote explícito.

**Evidencia:** el pseudocódigo recorre `TL[1..N]` (notas, pág. 24), pero no define la inclusión de WAIT, FINISH, CANCEL ni RUN. La transcripción dice que la llegada de otro programa puede cambiar las bandas (00:12:07–00:14:22), sin enumerar otras causas.

**Impacto:** prioridad de todos los procesos, orden de READY, fairness, eventos y costo de actualización. Recalcular el rango puede cambiar prioridades de procesos ya esperando o RUN.

**Propuesta para deliberar:** pedir al docente que precise si TL es la lista completa o solo la cola activa y cuándo propagar una reclasificación. Hasta entonces no excluir estados por intuición.

## DP-03 — Igual tamaño y precisión de los umbrales

**Alternativas:** conservar literalmente la consecuencia `A=B => H=0 => prioridad 3 para todo elemento`; definir un caso especial; o aclarar precisión/representación si tamaños admiten fracciones (el modelo actual usa bytes enteros).

**Evidencia:** `H=(B-A)/3` y límites estrictos `<` (notas, pág. 24); no hay ejemplo de tamaños iguales.

**Impacto:** clasificación de casos degenerados y reglas de frontera. El pseudocódigo con aritmética de enteros puede truncar H si `B-A` no es divisible por 3; el tipo numérico de cálculo no está indicado.

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

**Alternativas:** adoptar las transiciones del diseño de equipo (READY→RUN, RUN→READY/WAIT/FINISH/CANCEL, WAIT→READY/CANCEL); limitar o permitir cancelación por estado; definir terminalidad de FINISH/CANCEL.

**Evidencia:** estados en notas, pág. 23; diagrama de pág. 27; time-sharing exige turnos (pág. 28), pero el diagrama no muestra todas las flechas necesarias. El motor concurrente actual permite cancelar READY, RUN y WAIT e ignora PID inexistentes, FINISH o ya CANCEL; es comportamiento implementado, no mandato docente.

**Impacto:** validación de operaciones, emisión de eventos, memoria, hilos activos, representación y tests de motores.

**Propuesta para deliberar:** aprobar una matriz completa de estados-transiciones y el efecto de cancelar durante RUN; conservar diferenciada la semántica implementada de la evidencia académica.

## DP-07 — Significado de `siguientePaso()` y avance temporal

**Alternativas:** una transición estado-a-estado; un evento significativo (posiblemente varios por operación); o una asignación de CPU/ráfaga completa hasta quantum, WAIT o FINISH.

**Evidencia:** contrato 1.0 define “una transición significativa”; guía de motor lógico habla de pasos/eventos y ciclo de quantum; documento GUI dice “exactamente una transición/evento”; `ConcurrentEngine::siguientePaso()` llama a `ejecutarPasoInterno`, que espera una ráfaga de worker hasta un motivo de fin de turno.

**Impacto:** API observable, pausa, timeline, snapshots, número de clics, reloj, eventos y equivalencia entre motores.

**Propuesta para deliberar:** definir con un ejemplo reproducible cuánto avanza el reloj y qué eventos pueden ocurrir en una llamada; después alinear ambos motores en esa unidad.

## DP-08 — Detalles de timeline y mapa gráfico de memoria

**Alternativas:** segmento sin actividad o banda de CPU inactivo; eje con escala fija o adaptativa; historial ilimitado, ventana desplazable o límite configurable; memoria conceptual con bloques proporcionales en orden de admisión u otro orden.

**Evidencia:** notas, págs. 28–29, time-sharing temporal y diagrama de procesos/cola/memoria/CPU; transcripción 00:08:25–00:09:28. Contrato 1.0 tiene bloques `[inicio, fin)` solo para PID de proceso y la implementación de memoria no conserva dirección ni orden de colocación.

**Impacto:** tipo de timeline, consistencia de duraciones, rendimiento de GUI, interpretación de CPU ocioso y diseño del widget de memoria. Una región física/dirección real sería una ampliación no respaldada.

**Propuesta para deliberar:** acordar escala, idle, retención/desplazamiento y orden conceptual de bloques. Mantener el requerimiento visual; no prometer exportación PNG/HTML/PDF.

## DP-09 — Lista enlazada simple frente a `std::list`

**Alternativas:** conservar la implementación actual y confirmar que la recomendación académica es conceptual; adoptar lista enlazada simple solo si la estructura específica es obligatoria; o coordinar otra representación tras evaluar requisito y contrato.

**Evidencia:** notas, pág. 25, dicen lista enlazada simple con referencias a inicio/fin; `core/ColaProcesos.h` en master usa `std::list<Proceso>`, normalmente doblemente enlazada, preserva inserción pero no agenda.

**Impacto:** implementación, complejidad, pruebas y posible expectativa académica. El pseudocódigo de inserción/búsqueda de prioridad en esa página está incompleto/inconsistente.

**Propuesta para deliberar:** confirmar obligatoriedad con docente antes de reemplazar estructura. No inferir incumplimiento del proyecto completo solo por diferencia de representación.

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
