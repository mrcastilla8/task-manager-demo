# Casos de referencia del simulador

Los resultados se marcan como **normativos** solo cuando la evidencia o el contrato actual los fija. Los casos **ilustrativos** demuestran una regla sin convertir todos sus parámetros en requisitos. Los casos **pendientes** no tienen un único resultado hasta que se resuelva la decisión enlazada. Cada motor debe usar el mismo caso y configuración al comparar resultados.

## P1 — Tercios de tamaño y fronteras

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

## P2 — Rango cambia con llegada de proceso grande

**Tipo:** reclasificación dinámica confirmada como explicación; resultados condicionados a DP-02.

TL inicial tiene tamaños 12, 40, 68 y 96. Llega un proceso de tamaño 180. `A` se mantiene 12; `B` pasa a 180; `H` pasa de 28 a 56. Las bandas pasan de cortes 40/68 a cortes 68/124. Por el algoritmo, 40 cambia de clase 2 a 1 y 68 cambia de 3 a 2. Determinar si el proceso nuevo ya participa al ingresar, cuándo se actualizan los existentes y qué estados se recalculan queda pendiente. Fuente: notas, pág. 24; transcripción 00:12:07–00:14:22.

## P3 — Todos los tamaños iguales

**Tipo:** resultado mecánico ilustrativo, sujeto a DP-03.

Tres elementos con tamaño 50 producen `A=B=50`, `H=0`; con las dos comparaciones estrictas, cada elemento alcanza ELSE y resulta prioridad 3. Este es el resultado literal del pseudocódigo, no una confirmación de política para rango degenerado. La alternativa de caso especial requiere aprobación.

## P4 — Cuota docente y quantum de equipo

**Tipo:** ejemplo docente más política de equipo; precedencia pendiente DP-04.

Para clases docentes 1, 2 y 3, la fórmula ilustrativa de la transcripción `prioridad × 10 ms` produce cuotas de 10, 20 y 30 ms. El quantum global de equipo es 500 ms por defecto. No se debe concluir si el turno efectivo es cuota, quantum, mínimo u otro valor. Una vez aprobada la precedencia, verificar los tiempos en eventos, timeline y ambos motores. Fuente: notas, pág. 23; transcripción 00:11:50–00:12:07; configuración/contrato del equipo.

## P5 — Mayor prioridad y desempate

**Tipo:** resultado de decisión del equipo, condicionado a que siga vigente prioridad 1–10.

Con A READY P8, B READY P5 y C READY P3, la política de equipo selecciona A. Si A y B tienen igual prioridad actual, el desempate registrado usa menor `ordenLlegada`, con PID menor como desempate final. El FIFO de llegada debe distinguirse de la rotación posterior a un turno.

## P6 — Rotación Round Robin y discrepancia integrada

**Tipo:** test del scheduler reproducible; integración pendiente DP-05.

Primera llamada: A y B están READY, prioridad 8, orden llegada A=1 y B=2; se elige A. Para simular A reingresado al final, segunda llamada: A=3 y B=2; se elige B. Esto describe exactamente lo que comprueba el caso de la prueba de empate del Scheduler, que prepara el nuevo `ordenLlegada` antes de seleccionar. En la implementación concurrente revisada, al finalizar `QUANTUM_AGOTADO` A vuelve a READY pero su `ordenLlegada` no se actualiza. Por tanto, la secuencia de dos turnos no queda demostrada de extremo a extremo y puede volver a favorecer A. No corregir código bajo esta documentación.

## P7 — Aging del equipo

**Tipo:** decisión de equipo existente; coexistencia pendiente DP-10.

Scheduler tiene `agingIntervaloMs=2000`, suma +1 por cada intervalo completo de espera READY y limita el valor a 10. Un proceso en P4 con 2,000 ms completos de espera pasa a P5; desde P10 no supera P10. Confirmar interacción con recálculo por tamaño y momento de reset de espera antes de combinar mecanismos. Esta regla no se atribuye al docente.

## P8 — WAIT y retorno

**Tipo:** ejemplo de equipo; relación exacta con fuente pendiente DP-06.

Proceso A ejecuta hasta su operación opcional de E/S y pasa RUN→WAIT; tras la duración configurada, vuelve WAIT→READY. Los documentos de motor describen este escenario, pero las notas docentes pertinentes no precisan instante ni duración de I/O. Comprobar que memoria continúe ocupada durante WAIT o aprobar explícitamente otra regla; el comportamiento actual del motor la mantiene cargada hasta FINISH/CANCEL.

## P9 — Memoria insuficiente

**Tipo:** contrato actual de equipo.

Capacidad 1000 bytes. A ocupa 600: carga exitosa, utilizada 600. B ocupa 500: no cabe, la carga falla sin cambiar estado de memoria. La API de cola rechaza proceso inválido/PID duplicado; el motor concurrente expresa falta de espacio mediante error de admisión. La visual debe mostrar a A en el mapa conceptual y el espacio restante, sin inventar direcciones. No hay algoritmo de paginación o reemplazo.

## P10 — Time-sharing y proporcionalidad visual

**Tipo:** comportamiento visual confirmado; escala concreta ilustrativa.

Representar sobre eje temporal CPU lógico ejecutando A `[0,20)`, B `[20,50)` y C `[50,60)`. Los anchos relativos deben ser 20:30:10. Las notas muestran segmentos coloreados de duración distinta para time-sharing (pág. 28); estos números solo ilustran proporcionalidad. El segmento pertenece al timeline, no a una tabla, cola, log o archivo exportado.

## P11 — CPU sin READY

**Tipo:** pendiente DP-08 y DP-07.

Cuando todos los procesos estén WAIT, el motor concurrente avanza el reloj hasta que una espera termina. La GUI debe mostrar que el CPU no está ejecutando un proceso durante ese intervalo; la representación de ese intervalo como hueco o banda idle, y si se agrega al timeline como bloque, siguen sin decisión. Confirmar que tiempo de espera y timeline no atribuyan ejecución a un PID durante el intervalo.

## P12 — Paso a paso

**Tipo:** pendiente DP-07.

Con quantum 500 ms y A con 1,300 ms restantes, comparar tres interpretaciones: un clic provoca READY→RUN; un clic procesa un evento significativo; o un clic ejecuta una ráfaga de 500 ms y devuelve A a READY con 800 ms restantes. La implementación concurrente actual se aproxima a la tercera y puede terminar antes por WAIT/FINISH. El resultado normativo requiere decidir reloj, eventos y estados de una llamada.

## P13 — Transiciones y cancelación

**Tipo:** estados confirmados; matriz ampliada pendiente DP-06.

READY→RUN y el turno RUN→READY describen planificación/time-sharing. RUN→FINISH representa completar CPU; RUN→WAIT y WAIT→READY están en el diseño de equipo con E/S. Cancelación desde READY/RUN/WAIT está soportada en la implementación concurrente actual; desde FINISH se ignora. Aprobar explícitamente si CANCEL es terminal, qué intervalo se registra al interrumpir RUN y qué eventos se emiten. El diagrama docente (pág. 27) no basta para inferir transiciones prohibidas.
