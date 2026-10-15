# Decisiones aprobadas DP-01–DP-11

**Procedencia:** acuerdos aprobados por coordinación del equipo y provistos para Fase 0; no se atribuyen al docente. **Fecha registrada:** 2026-10-15 (formalización documental; la fecha efectiva de aprobación original no fue suministrada). **Responsables de aprobación:** coordinación del proyecto. Las filas son normativas para el diseño v2 candidato y no migran código v1.0.

| ID | Decisión y subdecisiones aprobadas | Procedencia | Aprobación | Responsables de migración | Efectos | Criterios de aceptación |
|---|---|---|---|---|---|---|
| DP-01 | Prioridades solo P1/P2/P3, automáticas por tamaño; pequeño=P1, mediano=P2, grande=P3; P3 gana; sin prioridad manual ni escala activa 1–10. | DECISION_EQUIPO; coordinación | Formalizada 2026-10-15; fecha exacta de aprobación original no informada | Brickzon, Castilla, Poma | Alta sin prioridad; selector máximo P3 | Pruebas verifican mapeo y P3>P2>P1. |
| DP-02a, DP-02b, DP-02c | **DP-02a:** TL contiene READY/RUN/WAIT y excluye FINISH/CANCEL de min/max. **DP-02b:** recalcular después de alta admitida, FINISH y CANCEL y aplicar antes del despacho siguiente. **DP-02c:** alta no interrumpe turno ni cambia cuota fijada; rechazo de memoria no altera TL/clases. | DECISION_EQUIPO; coordinación | Formalizada 2026-10-15; fecha exacta de aprobación original no informada | Brickzon, Aldana, Ángel | Reclasificación dinámica con turno en curso fijado | Casos cubren población, cada disparador y efectos de admisión. |
| DP-03 | Fórmula estricta P1 si S<A+H; P2 si no P1 y S<A+2H; P3 en otro caso; D=B-A, X=3(S-A), comparar X<D y X<2D; A=B da P3 y TL vacía no calcula extremos. | DECISION_EQUIPO aprobando literal de notas pp. 24–25 | Formalizada 2026-10-15; fecha exacta de aprobación original no informada | Brickzon, Castilla | Aritmética exacta entera | Fronteras estrictas, A=B y tamaño máximo sin overflow; usar evaluación segura equivalente. |
| DP-04 | Cuota por despacho = prioridad×10 ms (10/20/30); máxima, puede terminar por CPU agotada/WAIT/CANCEL; sustituye quantum global 500 ms; cuota del turno se fija al despacho. | DECISION_EQUIPO; coordinación | Formalizada 2026-10-15; fecha exacta de aprobación original no informada | Castilla, Aldana, Ángel, Poma | Quitar API global quantum del v2; snapshot expone cuota del turno | Pruebas de 10/20/30 ms, fin temprano y cambio de TL durante RUN. |
| DP-05 | Scheduler elige READY de mayor prioridad, luego menor ticket, luego PID ascendente; ticket estrictamente creciente al entrar READY por alta/retorno WAIT/cuota agotada; un único Scheduler. | DECISION_EQUIPO; coordinación | Formalizada 2026-10-15; fecha exacta de aprobación original no informada | Brickzon, Aldana, Ángel | Scheduler compartido y Round Robin coherente | Igualdad da FIFO por ticket y reingreso obtiene ticket nuevo. |
| DP-06a, DP-06b | **DP-06a:** cancelar desde READY/RUN/WAIT; FINISH/CANCEL terminales; especificar transiciones, eventos y efectos de memoria. **DP-06b:** RUN se cancela en punto seguro conservando CPU simulado realmente completado; timeline/tiempos solo con intervalo positivo; no usar reloj físico. | DECISION_EQUIPO; coordinación | Formalizada 2026-10-15; fecha exacta de aprobación original no informada | Aldana, Ángel, Poma | Contrato explícito para cancelación e historial | Memoria liberada una vez; cancelación sin CPU no crea segmento. Protocolo de frontera por ratificar. |
| DP-07a, DP-07b | **DP-07a:** `siguientePaso()` en PAUSADO, con READY ejecuta un turno hasta cuota/WAIT/FINISH/CANCEL. **DP-07b:** sin READY y con WAIT con despertar avanza reloj al próximo; no despacha en ese paso; si todo es terminal el sistema finaliza. Modo continuo repite estas unidades. | DECISION_EQUIPO; coordinación | Formalizada 2026-10-15; fecha exacta de aprobación original no informada | Aldana, Ángel | Unidad lógica común | Comparación ambos motores por paso: reloj, eventos, estado y timeline. Driver de ejecución continua sigue por ratificar. |
| DP-08a, DP-08b, DP-08c, DP-08d | **DP-08a:** idle como huecos sin PID ni bloques idle. **DP-08b:** historial completo de sesión con scroll/zoom. **DP-08c:** memoria proporcional por PID/espacio libre. **DP-08d:** reproducir eventos/snapshots para RUN transitorio; GUI no toca reloj ni confunde animación con CPU. | DECISION_EQUIPO; coordinación | Formalizada 2026-10-15; fecha exacta de aprobación original no informada | Poma, Aldana, Ángel | Snapshot/events y rendering con gaps inferidos | Gaps antes/entre/después se derivan hasta reloj snapshot; cada bloque es ejecución CPU PID positivo. Representación eficiente conserva la fuente completa. |
| DP-09 | Lista simplemente enlazada dinámica (nodo, primero, último, siguiente), API funcional compatible; definir propiedad, copia/movimiento y destrucción; no cambiar `std::list`/core en Fase 0. | DECISION_EQUIPO; coordinación, atendiendo intención docente pp. 25 | Formalizada 2026-10-15; fecha exacta de aprobación original no informada | Castilla | Migración futura de estructura y garantía de valor | Pruebas de vacío, inserción, borrado, copia/move y destructor seguros. |
| DP-10 | No hay aging activo ni suma de prioridad por espera; espera es métrica; aging anterior solo antecedente/extensión no activa. | DECISION_EQUIPO; coordinación | Formalizada 2026-10-15; fecha exacta de aprobación original no informada | Brickzon, Poma | Quitar configuración y expectativas de aging en v2 | Espera no cambia P1/P2/P3. |
| DP-11a, DP-11b | **DP-11a:** Qt 6 + C++17 + CMake aprobado. **DP-11b:** motor lógico Aldana y concurrente Ángel comparten Scheduler/interfaz v2; Poma no planifica en widgets; un CPU lógico aunque haya threads reales. | DECISION_EQUIPO; coordinación | Formalizada 2026-10-15; fecha exacta de aprobación original no informada | Aldana, Ángel, Poma, Brickzon | Framework y límites de arquitectura | Mock y ambos motores cumplen la misma interfaz y casos observables. |

Las once decisiones oficiales contienen diecinueve respuestas: DP-01, DP-02a, DP-02b, DP-02c, DP-03, DP-04, DP-05, DP-06a, DP-06b, DP-07a, DP-07b, DP-08a, DP-08b, DP-08c, DP-08d, DP-09, DP-10, DP-11a y DP-11b. Se conserva la agrupación DP-01–DP-11 y las opciones aprobadas sin cambios. Las decisiones describen el destino aprobado; hasta completar migración, v1.0 sigue vigente para el código ya compilado.

## DP-01 — Prioridades

DP-01: P1–P3 automáticas desde tamaño, P3 máxima, alta sin selección manual y sin escala activa 1–10 en v2.

<a id="dp-02--población-y-recalculo"></a>
## DP-02 — Población y recálculo

DP-02a, DP-02b y DP-02c: TL=READY/RUN/WAIT; excluir terminales; recalcular tras admisión exitosa, FINISH y CANCEL antes del próximo despacho; alta rechazada no cambia TL; alta admitida no modifica turno/cuota ya fijados.

## DP-03 — Algoritmo exacto

DP-03: usar límites estrictos y evaluación entera exacta de D/X; A=B da P3; TL vacía evita min/max.

## DP-04 — Cuotas individuales

DP-04: cuota máxima `prioridad×10 ms`; 10/20/30 ms; no mantener quantum global configurable en v2; fijar cuota al inicio de despacho.

## DP-05 — Cola y Round Robin

DP-05: seleccionar READY por prioridad descendente, ticket ascendente, PID ascendente; nuevo ticket estrictamente creciente en cada ingreso a READY.

## DP-06 — Estados y cancelación

DP-06a y DP-06b: cancelar desde READY/RUN/WAIT; terminales FINISH/CANCEL; conservar CPU simulado efectivo y no crear intervalo cero; registrar efectos/eventos/memoria.

## DP-07 — Paso y tiempo simulado

DP-07a y DP-07b: un paso PAUSADO procesa una ráfaga lógica hasta cuota/WAIT/FINISH/CANCEL, o avanza idle hasta despertar sin despachar hasta el paso siguiente; terminales finalizan sistema.

## DP-08 — GUI y timeline

DP-08a, DP-08b, DP-08c y DP-08d: CPU idle se representa como hueco inferido, sin PID ni bloque contractual; sesión completa con scroll/zoom, memoria proporcional por PID/libre, replay RUN; GUI no toca reloj simulado.

## DP-09 — Estructura enlazada

DP-09: lista simplemente enlazada dinámica con primero/último y ownership/copy/move/destructor seguros; API funcional compatible; migración futura, no en Fase 0.

## DP-10 — Aging

DP-10: no hay aging activo; espera solo métrica; aging anterior queda como antecedente/extensión futura.

## DP-11 — Interfaz y motores

DP-11a y DP-11b: Qt 6/C++17/CMake; motores lógico/concurrente comparten Scheduler e interfaz v2; GUI no planifica; CPU lógico único.

## Propuestas técnicas no ratificadas

Estas recomendaciones resuelven detalles de diseño para revisión; no amplían las diecinueve respuestas aprobadas:

1. Para varios WAIT con el mismo vencimiento: procesar por (instante de despertar, ticket WAIT asignado al bloquear, PID); todos los retornos del mismo instante se convierten a READY en un lote antes de publicar snapshot. Prueba: invertir el orden de almacenamiento no cambia la secuencia de eventos/tickets.
2. Prioridades derivadas recalculadas se publican como lote atómico junto al evento/snapshot, antes de seleccionar el próximo READY. No se publica estado parcial.
3. La cuota vigente se deriva exclusivamente con `cuotaMs(prioridad)`; no almacenar `cuotaActualMs` redundante. Al despacho, copiar el valor a `cuotaTurnoFijadaMs`. Reclasificar RUN puede cambiar la cuota derivada para un despacho futuro, sin alterar la copia del turno. Esta precisión aplica el principio de DP-04; el layout de la API v2 sigue candidato a adopción.
4. La granularidad y precedencia de cancelación se proponen por separado en [protocolo de cancelación RUN](PROPUESTAS_TECNICAS_V2.md#3-cancelación-run-parcial). Cualquier checkpoint se define en tiempo lógico y no con reloj físico; DP-06 no aprueba el punto exacto.
5. Protocolo de eventos y replay por secuencia creciente propuestos en [orden de eventos](PROPUESTAS_TECNICAS_V2.md#4-orden-y-representación-de-eventos). Timestamp de CPU + duración representa `[inicio,inicio+duración)` si se ratifica; RUN puede reconstruirse desde dispatch hasta evento de fin.
6. Pausa toma efecto en un límite lógico seguro; snapshot es copia consistente bajo el mismo límite. Reinicio detiene/une workers, borra procesos/eventos/timeline y reloj, conserva configuración.
7. FINISH/CANCEL quedan en `EstadoSistema.procesos` y la sesión hasta reinicio, aunque se excluyan de TL/memoria activa.
8. Sin aging puede haber inanición P1 si siempre hay READY P2/P3; medir espera máxima y documentar esta consecuencia. No añadir mitigación sin nueva aprobación.
9. Idle no crea bloque contractual. GUI infiere cada hueco del timeline dentro de `[0, tiempoSimuladoMs)` antes del primer bloque, entre bloques y después del último hasta el reloj del snapshot. Eventos de idle sin PID asociado pueden describir el inicio/fin del hueco. Caso: CPU A `[0,8)`, WAIT deja el CPU idle `[8,18)`, CPU B `[18,22)`; con snapshot en 27, también se infiere idle `[22,27)`. `BloqueTimeline` siempre lleva PID positivo y sólo CPU realmente ejecutado.
10. Historial completo puede crecer sin límite. Propuesta: eventos compactos inmutables, snapshots/checkpoints indexados y renderizado por ventana/LOD sin borrar el registro fuente; prueba de memoria con sesiones largas y equivalencia de reconstrucción.
11. El contador ticket `uint64_t` no debe envolver. Antes de agotarse, devolver error de límite o renumerar READY de forma estable en punto pausado; elegir regla antes de implementar y probar el límite por inyección del contador.
12. La fórmula D/X es matemática; implementación C++17 portable debe evitar producto fuera de rango. Para tamaños positivos `int64_t`, evaluar producto mediante cociente/guardas o una rutina de comparación triple exacta, sin depender de `__int128` no portable. Probar límites de `int64_t`.

## Criterios de ratificación y precedencia

Coordinación revisará estas propuestas antes de Fase 1. Una propuesta aceptada se incorpora aquí como decisión identificada y actualiza contratos/casos; hasta entonces se etiqueta **PROPUESTA TÉCNICA**. [Contratos v2](CONTRATOS_V2.md), [migración](MIGRACION_CONTRATOS_V2.md), [especificación](ESPECIFICACION_FUNCIONAL.md) y [workflow](WORKFLOW_DESARROLLO.md) detallan materialización y gates.
