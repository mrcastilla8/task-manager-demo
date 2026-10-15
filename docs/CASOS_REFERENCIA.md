# Casos de referencia del simulador

> **Destino v2 formalizado 2026-10-15 (API aún candidata):** los once escenarios numerados en “Escenarios v2 aprobados” son los criterios normativos de destino aún no implementado. P1–P13, en el archivo histórico posterior, conservan solo el estado de discusión y pruebas v1/ramas; no son tareas ni expectativas v2.

## Escenarios v2 aprobados

1. **Clasificación estricta:** A=12, B=96, D=84: S=39→P1; 40→P2; 67→P2; 68→P3; 96→P3. A=B da P3 para todos; TL vacía no calcula.
2. **Desborde:** evaluar X=3(S-A) exactamente para tamaños válidos sin overflow; comparar casos próximos al límite de int64 con una referencia matemática exacta.
3. **Reclasificación/cuota:** TL contiene READY/RUN/WAIT. Alta admitida, FINISH y CANCEL piden reclasificación antes del despacho siguiente. Alta no interrumpe RUN ni cambia cuota del turno fijado; alta rechazada no cambia TL/prioridades.
4. **Cuotas:** P1/P2/P3 reciben máximos 10/20/30 ms. CPU agotada, WAIT o CANCEL pueden terminar antes; el cambio de TL no muta cuota del turno iniciado.
5. **READY/RR:** prioridad descendente, ticket ascendente, PID ascendente. Reingreso tras cuota recibe ticket nuevo. No existe aging activo.
6. **WAIT simultáneo:** orden determinista exacto queda sujeto a ratificación de la propuesta listada en [decisiones aprobadas](DECISIONES_APROBADAS.md#propuestas-técnicas-no-ratificadas).
7. **Cancelación:** READY/RUN/WAIT→CANCEL; terminales siguen en historial; CPU/timeline incluyen solo CPU efectivamente ejecutada y duración positiva. Punto seguro determinista aún requiere ratificación técnica.
8. **Paso:** PAUSADO+READY ejecuta turno hasta cuota/WAIT/FINISH/CANCEL; sin READY pero WAIT, adelanta al despertar más próximo sin despacho en ese paso; sin trabajo activo finaliza.
9. **Idle/timeline:** `BloqueTimeline` solo representa CPU ejecutado y exige PID positivo. A ejecuta `[0,8)`, entra WAIT y despierta en 18; CPU idle `[8,18)` no produce bloque ni PID especial. B ejecuta `[18,22)`; snapshot a `27` contiene solo bloques A `[0,8)` y B `[18,22)` e infiere idle `[8,18)` y `[22,27)`. Un evento idle puede explicar el primer hueco sin asociarlo a PID. El widget no avanza el reloj.
10. **Memoria/historial:** representación proporcional por PID y espacio libre; procesos FINISH/CANCEL permanecen en historia aunque salgan de TL/liberen memoria.
11. **Motores:** mismos pasos/entradas deben dar mismo reloj, prioridad, estados, eventos y timeline en lógico/concurrente; tiempo de threads no equivale a tiempo simulado.

En los casos v2 iniciales, las reglas aprobadas son normativas para el destino aún no implementado; únicamente el orden de WAIT simultáneos y el punto seguro de cancelación RUN quedan pendientes de ratificación técnica. Los casos históricos de la sección siguiente conservan sus etiquetas ilustrativas/pendientes en el sentido que tenían antes del cierre. Cada motor deberá usar los mismos casos/configuración al comparar resultados.

## Archivo histórico v1 / deliberación anterior al cierre

Los escenarios P1–P13 de esta sección documentan expectativas, pruebas o preguntas de las ramas y documentos previos. Sus etiquetas “pendiente DP” indican el estado de aquella deliberación, no una tarea abierta ni una prueba normativa v2. Los criterios normativos v2 son los once escenarios aprobados de la sección inicial y las 19 filas de [trazabilidad](MATRIZ_TRAZABILIDAD.md#trazabilidad-de-las-19-respuestas-aprobadas-v2). En caso de conflicto, usar DP aprobadas y contratos candidatos rotulados.

## Historial P1 — Tercios de tamaño y fronteras

**Tipo:** ilustrativo del algoritmo docente; operadores y fórmula normativos respecto a las notas.

Tamaños enteros `[12, 39, 40, 67, 68, 96]`, con `A=12`, `B=96`, `H=(96-12)/3=28`.

| Tamaño | Evaluación | Prioridad |
|---:|---|---:|
| 12 | menor que 40 | 1 |
| 39 | menor que 40 | 1 |
| 40 | no menor que 40; menor que 68 | 2 |
| 67 | menor que 68 | 2 |
| 68 | no menor que 68 | 3 |
| 96 | caso ELSE | 3 |

Para una implementación de referencia, la condición debe evaluarse en este orden: `< A+H`; luego `< A+2H`; en otro caso 3. Fuente: notas, pág. 24. No determina escala de scheduler 1–10.

## Historial P2 — Rango cambia con llegada de proceso grande

**Tipo:** reclasificación dinámica confirmada como explicación; resultados condicionados a DP-02.

TL inicial tiene tamaños 12, 40, 68 y 96. Llega un proceso de tamaño 180. `A` se mantiene 12; `B` pasa a 180; `H` pasa de 28 a 56. Las bandas pasan de cortes 40/68 a cortes 68/124. Por el algoritmo, 40 cambia de clase 2 a 1 y 68 cambia de 3 a 2. Determinar si el proceso nuevo ya participa al ingresar, cuándo se actualizan los existentes y qué estados se recalculan queda pendiente. Fuente: notas, pág. 24; transcripción 00:12:07–00:14:22.

## Historial P3 — Todos los tamaños iguales

**Tipo:** resultado mecánico ilustrativo, sujeto a DP-03.

Tres elementos con tamaño 50 producen `A=B=50`, `H=0`; con las dos comparaciones estrictas, cada elemento alcanza ELSE y resulta prioridad 3. Este es el resultado literal del pseudocódigo, no una confirmación de política para rango degenerado. La alternativa de caso especial requiere aprobación.

## Historial P4 — Cuota docente y quantum de equipo

**Tipo:** ejemplo docente más política de equipo; precedencia pendiente DP-04.

Para clases docentes 1, 2 y 3, la fórmula ilustrativa de la transcripción `prioridad × 10 ms` produce cuotas de 10, 20 y 30 ms. El quantum global de equipo es 500 ms por defecto. No se debe concluir si el turno efectivo es cuota, quantum, mínimo u otro valor. Una vez aprobada la precedencia, verificar los tiempos en eventos, timeline y ambos motores. Fuente: notas, pág. 23; transcripción 00:11:50–00:12:07; configuración/contrato del equipo.

## Historial P5 — Mayor prioridad y desempate

**Tipo:** resultado de decisión del equipo, condicionado a que siga vigente prioridad 1–10.

Con A READY P8, B READY P5 y C READY P3, la política de equipo selecciona A. Si A y B tienen igual prioridad actual, el desempate registrado usa menor `ordenLlegada`, con PID menor como desempate final. El FIFO de llegada debe distinguirse de la rotación posterior a un turno.

En la escala académica, comparar un proceso P1, uno P2 y uno P3: `ObtenerMaximaPrioridad()` conserva el valor mayor, por lo que gana P3. Esto está respaldado por notas, págs. 24–25. La decisión pendiente no es el sentido del orden 1–3; es si el equipo adopta esa escala o la adapta a su escala 1–10 (DP-01).

## Historial P6 — Rotación Round Robin y discrepancia integrada

**Tipo:** test del scheduler reproducible; integración pendiente DP-05.

Primera llamada: A y B están READY, prioridad 8, orden llegada A=1 y B=2; se elige A. Para simular A reingresado al final, segunda llamada: A=3 y B=2; se elige B. Esto describe exactamente lo que comprueba el caso de la prueba de empate del Scheduler, que prepara el nuevo `ordenLlegada` antes de seleccionar. En la implementación concurrente revisada, al finalizar `QUANTUM_AGOTADO` A vuelve a READY pero su `ordenLlegada` no se actualiza. Por tanto, la secuencia de dos turnos no queda demostrada de extremo a extremo y puede volver a favorecer A. No corregir código bajo esta documentación.

## Historial P7 — Aging del equipo

**Tipo:** decisión de equipo existente; coexistencia pendiente DP-10.

Scheduler tiene `agingIntervaloMs=2000`, suma +1 por cada intervalo completo de espera READY y limita el valor a 10. Un proceso en P4 con 2,000 ms completos de espera pasa a P5; desde P10 no supera P10. Confirmar interacción con recálculo por tamaño y momento de reset de espera antes de combinar mecanismos. Esta regla no se atribuye al docente.

## Historial P8 — WAIT y retorno

**Tipo:** ejemplo de equipo; relación exacta con fuente pendiente DP-06.

Proceso A ejecuta hasta su operación opcional de E/S y pasa RUN→WAIT; tras la duración configurada, vuelve WAIT→READY. Los documentos de motor describen este escenario, pero las notas docentes pertinentes no precisan instante ni duración de I/O. Comprobar que memoria continúe ocupada durante WAIT o aprobar explícitamente otra regla; el comportamiento actual del motor la mantiene cargada hasta FINISH/CANCEL.

## Historial P9 — Memoria insuficiente

**Tipo:** contrato actual de equipo.

Capacidad 1000 bytes. A ocupa 600: carga exitosa, utilizada 600. B ocupa 500: no cabe, la carga falla sin cambiar estado de memoria. La API de cola rechaza proceso inválido/PID duplicado; el motor concurrente expresa falta de espacio mediante error de admisión. La visual debe mostrar a A en el mapa conceptual y el espacio restante, sin inventar direcciones. No hay algoritmo de paginación o reemplazo.

## Historial P10 — Time-sharing y proporcionalidad visual

**Tipo:** comportamiento visual confirmado; escala concreta ilustrativa.

Representar sobre eje temporal CPU lógico ejecutando A `[0,20)`, B `[20,50)` y C `[50,60)`. Los anchos relativos deben ser 20:30:10. Las notas muestran segmentos coloreados de duración distinta para time-sharing (pág. 28); estos números solo ilustran proporcionalidad. El segmento pertenece al timeline, no a una tabla, cola, log o archivo exportado.

## Historial P11 — CPU sin READY

**Tipo:** pendiente DP-08 y DP-07.

Cuando todos los procesos estén WAIT, el motor concurrente avanza el reloj hasta que una espera termina. La GUI debe mostrar que el CPU no está ejecutando un proceso durante ese intervalo; la representación de ese intervalo como hueco o banda idle, y si se agrega al timeline como bloque, siguen sin decisión. Confirmar que tiempo de espera y timeline no atribuyan ejecución a un PID durante el intervalo.

## Historial P12 — Paso a paso

**Tipo:** pendiente DP-07.

Con quantum 500 ms y A con 1,300 ms restantes, comparar tres interpretaciones: un clic provoca READY→RUN; un clic procesa un evento significativo; o un clic ejecuta una ráfaga de 500 ms y devuelve A a READY con 800 ms restantes. La implementación concurrente actual se aproxima a la tercera y puede terminar antes por WAIT/FINISH. El resultado normativo requiere decidir reloj, eventos y estados de una llamada.

## Historial P13 — Transiciones y cancelación

**Tipo:** flechas del diagrama confirmadas; semántica operativa ampliada pendiente DP-06.

El diagrama docente de la pág. 27 muestra READY→RUN, RUN→WAIT, RUN→FINISH, RUN→CANCEL y WAIT→READY. No indica los disparadores, duraciones ni el orden temporal completo de esas transiciones. La pág. 28 dice que al pasar el tiempo asignado entra otro proceso, pero no indica el estado del proceso anterior; RUN→READY no se atribuye a esa explicación. La implementación concurrente sí reingresa a READY tras agotar quantum y permite cancelar desde READY/RUN/WAIT, e ignora FINISH; eso no reemplaza la evidencia gráfica ni define por sí solo la política común del equipo. DP-06 debe precisar estados destino, terminalidad de CANCEL, efecto/intervalo al cancelar RUN, eventos y transiciones adicionales. Las flechas ausentes permanecen sin especificar, no prohibidas.
