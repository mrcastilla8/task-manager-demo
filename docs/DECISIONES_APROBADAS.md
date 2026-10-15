# Decisiones aprobadas DP-01–DP-11

**Procedencia:** acuerdos aprobados por coordinación del equipo y provistos para Fase 0; no se atribuyen al docente. **Fecha registrada:** 2026-10-15 (formalización documental; la fecha efectiva de aprobación original no fue suministrada). **Responsables de aprobación:** coordinación del proyecto. Las filas son normativas para el diseño v2 candidato y no migran código v1.0.

| ID | Decisión y subdecisiones aprobadas | Procedencia | Aprobación | Responsables de migración | Efectos | Criterios de aceptación |
|---|---|---|---|---|---|---|
| DP-01 | D01 prioridades solo P1/P2/P3; D02 tamaño determina automáticamente la clase; D03 pequeño=P1, mediano=P2, grande=P3; D04 P3 gana y no hay prioridad manual ni escala activa 1–10. | DECISION_EQUIPO; coordinación | Aprobado antes de 2026-10-15; formalizado 2026-10-15 | Brickzon, Castilla, Poma | Alta sin prioridad; selector máximo P3 | Pruebas verifican mapeo y P3>P2>P1. |
| DP-02 | D05 TL contiene READY/RUN/WAIT; FINISH/CANCEL se excluyen de min/max; D06 recalcular después de alta admitida, FINISH y CANCEL y publicar antes del próximo despacho; D07 alta no interrumpe turno ni cambia su cuota; rechazo de memoria no cambia TL/clases. | DECISION_EQUIPO; coordinación | Aprobado antes de 2026-10-15; formalizado 2026-10-15 | Brickzon, Aldana, Ángel | Reclasificación dinámica con turno en curso fijado | Casos cubren entrada, terminalidad, WAIT y alta rechazada. |
| DP-03 | D08 fórmula estricta P1 si S<A+H; P2 si no P1 y S<A+2H; P3 en otro caso; forma exacta D=B-A, X=3(S-A), comparar X<D y X<2D; D09 A=B resulta P3 y TL vacía no calcula extremos. | DECISION_EQUIPO aprobando literal de notas pp. 24–25 | Aprobado antes de 2026-10-15; formalizado 2026-10-15 | Brickzon, Castilla | Aritmética exacta entera | Fronteras estrictas, A=B y tamaño máximo sin overflow; usar evaluación segura equivalente. |
| DP-04 | D10 cuota por despacho = prioridad×10 ms (10/20/30); D11 es máximo de ráfaga y puede terminar antes por CPU agotada, WAIT o CANCEL; sustituye quantum global 500 ms y controles globales contradictorios; cuota fijada al despacho no cambia retroactivamente. | DECISION_EQUIPO; coordinación | Aprobado antes de 2026-10-15; formalizado 2026-10-15 | Castilla, Aldana, Ángel, Poma | Quitar API global quantum del v2; snapshot expone cuota del turno | Pruebas de 10/20/30 ms, fin temprano y cambio de TL durante RUN. |
| DP-05 | D12 Scheduler elige prioridad READY mayor, luego ticket menor, luego PID ascendente; D13 ticket estrictamente creciente nuevo al entrar READY por alta, retorno WAIT y agotamiento de cuota; no duplicar regla en motores; lista enlazada no define selección. | DECISION_EQUIPO; coordinación | Aprobado antes de 2026-10-15; formalizado 2026-10-15 | Brickzon, Aldana, Ángel | Scheduler compartido y Round Robin coherente | Igualdad da FIFO por ticket y reingreso obtiene ticket nuevo. |
| DP-06 | D14 cancelar READY/RUN/WAIT; FINISH y CANCEL terminales; RUN se cancela en punto seguro reteniendo CPU simulada efectivamente ejecutada; registrar CPU/timeline solo si intervalo positivo; no usar reloj físico; especificar transiciones, eventos y memoria. | DECISION_EQUIPO; coordinación | Aprobado antes de 2026-10-15; formalizado 2026-10-15 | Aldana, Ángel, Poma | Contrato explícito para cancelación e historial | Matriz de transición, memoria liberada una vez, cancelación en t=0 no crea segmento. |
| DP-07 | D15 paso PAUSADO con READY ejecuta un turno hasta cuota/WAIT/FINISH/CANCEL; sin READY y WAIT con despertar adelanta reloj hasta el próximo; ese despertar no despacha en el mismo paso; solo terminales finaliza sistema. | DECISION_EQUIPO; coordinación | Aprobado antes de 2026-10-15; formalizado 2026-10-15 | Aldana, Ángel | Unidad lógica común; continuo itera unidades | Comparación ambos motores por paso: reloj, eventos, estado y timeline. |
| DP-08 | D16 timeline tiene huecos idle sin PID, historial total de sesión, scroll/zoom, memoria proporcional por PID/espacio libre y reproducción eventos/snapshots para RUN transitorio; GUI no modifica reloj y animación no equivale a CPU; atender memoria/rendimiento sin descartar datos. | DECISION_EQUIPO; coordinación | Aprobado antes de 2026-10-15; formalizado 2026-10-15 | Poma, Aldana, Ángel | Snapshot/eventos con reproducción; almacenamiento eficiente | La reproducción no altera estado lógico; datos completos recuperables durante sesión. |
| DP-09 | D17 Castilla implementa lista simplemente enlazada dinámica (nodo, primero, último, siguiente), conserva API funcional compatible; definir propiedad, copia/movimiento y destrucción; no cambiar `std::list`/core en Fase 0. | DECISION_EQUIPO; coordinación, atendiendo intención docente pp. 25 | Aprobado antes de 2026-10-15; formalizado 2026-10-15 | Castilla | Migración futura de estructura y garantía de valor | Pruebas de vacío, inserción, borrado, copia/move, autoasignación y destructor sin fugas/doble liberación. |
| DP-10 | D18 no hay aging activo; no sumar prioridad por espera. Espera sigue métrica; aging previo es solo antecedente/extensión futura. | DECISION_EQUIPO; coordinación | Aprobado antes de 2026-10-15; formalizado 2026-10-15 | Brickzon, Poma | Quitar configuración y expectativas de aging en v2 | Aumentar espera no cambia P1/P2/P3; referencias a aging etiquetadas históricas. |
| DP-11 | D19 GUI Qt 6 + C++17 + CMake; motores lógico Aldana y concurrente Ángel comparten Scheduler y contrato v2; Poma consume interfaz común sin planificar en widgets; un solo CPU lógico aunque haya threads. | DECISION_EQUIPO; coordinación | Aprobado antes de 2026-10-15; formalizado 2026-10-15 | Aldana, Ángel, Poma, Brickzon | Framework y límites de arquitectura | Mock y ambos motores cumplen la misma interfaz y casos observables. |

Las diecinueve subdecisiones se identifican D01–D19 para revisión sin cambiar la agrupación oficial DP-01–DP-11. Las decisiones describen el destino aprobado; hasta completar migración, v1.0 sigue vigente para el código ya compilado.

## DP-01 — Prioridades

D01–D04: P1–P3 automáticas desde tamaño, P3 máxima, alta sin selección manual y sin escala activa 1–10 en v2.

<a id="dp-02--población-y-recalculo"></a>
## DP-02 — Población y recálculo

D05–D07: TL=READY/RUN/WAIT; excluir terminales; recalcular tras admisión exitosa, FINISH y CANCEL antes del próximo despacho; alta rechazada no cambia TL; alta admitida no modifica turno/cuota ya fijados.

## DP-03 — Algoritmo exacto

D08–D09: usar límites estrictos y evaluación entera exacta de D/X; A=B da P3; TL vacía evita min/max.

## DP-04 — Cuotas individuales

D10–D11: cuota máxima `prioridad×10 ms`; 10/20/30 ms; no mantener quantum global configurable en v2; fijar cuota al inicio de despacho.

## DP-05 — Cola y Round Robin

D12–D13: seleccionar READY por prioridad descendente, ticket ascendente, PID ascendente; nuevo ticket estrictamente creciente en cada ingreso a READY.

## DP-06 — Estados y cancelación

D14: cancelar desde READY/RUN/WAIT; terminales FINISH/CANCEL; conservar CPU simulado efectivo y no crear intervalo cero; registrar efectos/eventos/memoria.

## DP-07 — Paso y tiempo simulado

D15: un paso PAUSADO procesa una ráfaga lógica hasta cuota/WAIT/FINISH/CANCEL, o avanza idle hasta despertar sin despachar hasta el paso siguiente; terminales finalizan sistema.

## DP-08 — GUI y timeline

D16: timeline con idle sin PID, sesión completa con scroll/zoom, memoria proporcional por PID/libre, replay RUN; GUI no toca reloj simulado.

## DP-09 — Estructura enlazada

D17: lista simplemente enlazada dinámica con primero/último y ownership/copy/move/destructor seguros; API funcional compatible; migración futura, no en Fase 0.

## DP-10 — Aging

D18: no hay aging activo; espera solo métrica; aging anterior queda como antecedente/extensión futura.

## DP-11 — Interfaz y motores

D19: Qt 6/C++17/CMake; motores lógico/concurrente comparten Scheduler e interfaz v2; GUI no planifica; CPU lógico único.

## Propuestas técnicas no ratificadas

Estas recomendaciones resuelven detalles de diseño para revisión; no amplían D01–D19:

1. Para varios WAIT con el mismo vencimiento: procesar por (instante de despertar, ticket WAIT asignado al bloquear, PID); todos los retornos del mismo instante se convierten a READY en un lote antes de publicar snapshot. Prueba: invertir el orden de almacenamiento no cambia la secuencia de eventos/tickets.
2. Prioridades derivadas recalculadas se publican como lote atómico junto al evento/snapshot, antes de seleccionar el próximo READY. No se publica estado parcial.
3. `Proceso.cuotaActualMs` es cuota publicada vigente; al despacho, copiarla a `cuotaTurnoFijadaMs`. Cambios del TL actualizan el proceso RUN para su siguiente despacho, sin alterar la copia del turno.
4. Para cancelación reproducible, motores reconocen solicitud en los mismos límites de avance lógico (cada milisegundo simulado o subevento determinista); priorizar API de comandos de paso, evitando dependencia del instante de planificación de threads reales. Pruebas solicitan cancelación en t=0, antes de borde de cuota y en borde exacto.
5. Registrar eventos ordenados con intervalos CPU explícitos (inicio, fin, PID, duración) y snapshots por número de secuencia; esto reconstruye RUN transitorio tras terminar la ráfaga.
6. Pausa toma efecto en un límite lógico seguro; snapshot es copia consistente bajo el mismo límite. Reinicio detiene/une workers, borra procesos/eventos/timeline y reloj, conserva configuración.
7. FINISH/CANCEL quedan en `EstadoSistema.procesos` y la sesión hasta reinicio, aunque se excluyan de TL/memoria activa.
8. Sin aging puede haber inanición P1 si siempre hay READY P2/P3; medir espera máxima y documentar esta consecuencia. No añadir mitigación sin nueva aprobación.
9. Idle como tramo `[inicio,fin)` en timeline con `pid=0`, sin evento de proceso; el reloj avanza exactamente al siguiente despertar.
10. Historial completo puede crecer sin límite. Propuesta: eventos compactos inmutables, snapshots/checkpoints indexados y renderizado por ventana/LOD sin borrar el registro fuente; prueba de memoria con sesiones largas y equivalencia de reconstrucción.
11. El contador ticket `uint64_t` no debe envolver. Antes de agotarse, devolver error de límite o renumerar READY de forma estable en punto pausado; elegir regla antes de implementar y probar el límite por inyección del contador.
12. La fórmula D/X es matemática; implementación C++17 portable debe evitar producto fuera de rango. Para tamaños positivos `int64_t`, evaluar producto mediante cociente/guardas o una rutina de comparación triple exacta, sin depender de `__int128` no portable. Probar límites de `int64_t`.

## Criterios de ratificación y precedencia

Coordinación revisará estas propuestas antes de Fase 1. Una propuesta aceptada se incorpora aquí como decisión identificada y actualiza contratos/casos; hasta entonces se etiqueta **PROPUESTA TÉCNICA**. [Contratos v2](CONTRATOS_V2.md), [migración](MIGRACION_CONTRATOS_V2.md), [especificación](ESPECIFICACION_FUNCIONAL.md) y [workflow](WORKFLOW_DESARROLLO.md) detallan materialización y gates.
