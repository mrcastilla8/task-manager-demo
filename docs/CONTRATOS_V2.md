# Contratos candidatos 2.0

**Estado: CANDIDATO A ADOPCIÓN. No integrado ni operativo.** `contracts/*.h` permanece en v1.0 y es el contrato de código existente. Los encabezados paralelos viven en `contracts/v2/`, namespace `ConcurrentSimulator::v2`, y pueden coexistir con los globales de v1 en una unidad de traducción.

## Archivos y tipos

- `Configuracion.h`: solo `memoriaTotalBytes`; no configurable quantum ni aging.
- `Estado.h`: estados de proceso READY/RUN/WAIT/FINISH/CANCEL y estado del motor.
- `Proceso.h`: `AltaProceso` no acepta prioridad/ticket/cuota. `Proceso` expone clase automática, CPU total/restante/consumida, espera, operación IO y ticket READY. La única fuente de verdad de la cuota vigente es `cuotaMs(prioridad)`; no se almacena una cuota vigente redundante. Snapshot puede guardar aparte `cuotaTurnoFijadaMs`, copiada al despacho y estable durante ese turno.
- `Evento.h`: eventos simulados timestamped, PID opcional por convención y duración; nunca timestamp de pared.
- `EstadoSistema.h`: reloj, estados, procesos activos e históricos, READY en orden efectivo, RUN, cuota fijada al turno, CPU consumida en turno, memoria, eventos y timeline.
- `IExecutionEngine.h`: superficie candidata de alta/cancelar/iniciar/pausar/reiniciar/paso/snapshot. `Resultado`, `CodigoResultado` y `ResultadoPaso` siguen siendo propuestas, no tipos ratificados.

## Semántica de prioridad y cuota

TL de reclasificación son procesos READY/RUN/WAIT admitidos. Sea A mínimo y B máximo de sus tamaños, D=B−A, y para cada tamaño S, X=3×(S−A): P1 si X<D, P2 si no y X<2D, P3 en otro caso. Las comparaciones equivalen a límites estrictos del algoritmo docente. A=B produce X=0,D=0 y P3. TL vacía no se clasifica. Implementación evita overflow para tamaños válidos mediante comparación exacta con guardas/cocientes; no depende de extensión de compilador. `uint64_t` ticket aumenta estrictamente al entrar READY; sobre flujo el límite debe manejarse sin envolver (propuesta precisa en decisiones aprobadas).

La cuota se deriva exclusivamente de la prioridad vigente: `cuotaMs(P1)=10`, `cuotaMs(P2)=20`, `cuotaMs(P3)=30`. En un despacho, el motor copia ese valor a `cuotaTurnoFijadaMs`; la copia es la cuota máxima de ese RUN y no cambia si la prioridad se reclasifica durante el turno. No hay campo público `cuotaActualMs` que pueda contradecir `prioridad`.

La llamada que admite un proceso no interrumpe CPU. Solo admisión exitosa cambia TL y dispara clasificación pendiente previa al siguiente despacho. FINISH/CANCEL se excluyen de TL, conservándose en historial. Admisión rechazada no altera memoria, TL ni prioridades.

## Scheduler y estados

Una única implementación compartida ordena READY por prioridad descendente, ticket ascendente y PID ascendente. El ticket cambia en admisión, retorno WAIT y fin de cuota. La cola que almacena procesos es estructura de datos, no política de elección. No existe aging.

Transiciones operativas aprobadas: admisión→READY; READY→RUN al despacho; RUN→READY por fin de cuota; RUN→WAIT por E/S; WAIT→READY al despertar; RUN→FINISH al agotarse CPU; READY/RUN/WAIT→CANCEL por cancelación. FINISH/CANCEL son terminales. Admisión exige proceso válido, PID nuevo y memoria disponible. Memoria se reserva al admitir y libera una vez al FINISH/CANCEL; WAIT conserva memoria.

## Unidad de paso e interfaz común

`siguientePaso()` exige motor PAUSADO. Si READY existe, hace un despacho y ejecuta una ráfaga hasta cuota, WAIT, FINISH o CANCEL. Si no hay READY y existe WAIT con despertar, avanza el reloj hasta el vencimiento más cercano sin CPU; WAIT se vuelve READY y ese despertar no despacha en el mismo paso. Sin READY/WAIT con trabajo activo, el sistema llega a estado finalizado. DP-07 aprueba que modo continuo aplique sucesivamente estas unidades, pero la superficie actual no define cómo pedir cada unidad sin bloquear Qt. Las alternativas A/B y sus impactos constan como propuestas en [Propuestas técnicas v2](PROPUESTAS_TECNICAS_V2.md#1-ejecución-continua-sin-bloquear-qt). No se ha ratificado una operación de avance continuo ni se cambian aquí firmas. Ambos motores deberán concordar en resultados observables; los threads reales no representan CPUs adicionales.

El esquema actual de `Evento` no define todavía orden total de timestamps iguales ni si timestamp en CPU es inicio o fin. La propuesta es secuencia creciente por sesión, timestamp de transición en la frontera y CPU como `[tiempoSimuladoMs, tiempoSimuladoMs+duracionMs)`. Un snapshot conserva eventos en orden de secuencia para reconstruir RUN transitorio; el nuevo campo `secuencia` se propone, pero aún no se añade a headers. Especificación, auditoría e impactos están en [Propuestas técnicas v2](PROPUESTAS_TECNICAS_V2.md#4-orden-y-representación-de-eventos).

Cada `BloqueTimeline` es exclusivamente CPU ejecutado por un proceso con PID positivo. Idle no tiene bloque: se infiere de las discontinuidades entre bloques en `[0, tiempoSimuladoMs)` —antes del primero, entre bloques y después del último—. Un evento idle puede explicar el intervalo sin asociarlo a un proceso; ningún PID especial representa idle en el timeline. Por ejemplo, A ejecuta `[0,8)`, espera en WAIT y queda idle `[8,18)`; B ejecuta `[18,22)`. Un snapshot en 27 ms contiene solo esos dos bloques y permite inferir idle `[8,18)` y `[22,27)`. Snapshot copia estado coherente. GUI nunca avanza reloj.

## Errores, pausa, reinicio y cancelación

El uso de `Resultado`/`CodigoResultado`/`ResultadoPaso` en lugar de excepciones es PROPUESTA TÉCNICA, no decisión explícita del cuestionario ni parte de DP-06. La comparación concreta de efectos por método y consumidor está en [Propuestas técnicas v2](PROPUESTAS_TECNICAS_V2.md#2-errores-excepciones-frente-a-códigos). Las firmas de retorno de `IExecutionEngine` dependen de ratificación; no están congeladas. Los fallos de recursos del sistema, como `std::bad_alloc`, pueden propagarse bajo cualquiera de las opciones.

**PROPUESTA TÉCNICA pendiente de ratificación — cancelación e idempotencia:** cancelar un PID desconocido devuelve `PID_INEXISTENTE`; cancelar un proceso FINISH devuelve `PID_FINALIZADO`; cancelar uno ya CANCEL devuelve `PID_YA_CANCELADO`; cancelar READY/RUN/WAIT activos devuelve `OK` y lo lleva a CANCEL. Una repetición sobre terminal informa su estado, sin cambiar memoria, eventos ni historial, por lo que la operación es idempotente respecto al estado. Así, un terminal conocido no se confunde con un PID inexistente. Esta propuesta no se atribuye a DP-06. El punto seguro para RUN contabiliza solo CPU simulada ya completada, añade bloque timeline solo con duración positiva y publica CANCEL/liberación una vez; el orden concurrente exacto también requiere ratificación.

## Ejemplo válido

```cpp
ConcurrentSimulator::v2::Configuracion cfg{64 * 1024};
ConcurrentSimulator::v2::AltaProceso p{1, "corto", 4096, 25, {}};
// auto engine = crearMotor(cfg); // factoría/producto concreto fuera de este contrato candidato
// engine->agregarProceso(p); engine->iniciar(); engine->pausar();
// engine->siguientePaso(); // cuota depende de tamaño frente al TL; máximo 30 ms
// const auto vista = engine->obtenerEstado();
```

Una alta con prioridad manual, configurarQuantum, edadación configurable o un CPU por worker es inválida respecto del diseño v2. El ejemplo es conceptual, no compilable sin una factoría concreta.

## Compatibilidad y v1.0

v1 mantiene `prioridadBase/prioridadActual` escala 1–10, `quantumConsumido`, `ordenLlegada`, quantum global, aging y nombres globales. v2 evita doble fuente de verdad: prioridad manual desaparece; prioridad es enum derivado; cuota deriva de prioridad y se fija por despacho; `quantumConsumido` pasa a CPU consumida en turno/total; `ordenLlegada` pasa a ticket READY no reutilizable. No editar v1 hasta migración coordinada. Vea [matriz de migración](MIGRACION_CONTRATOS_V2.md).

## Precisión a ratificar

Se requiere ratificación sobre subpasos deterministas de cancelación concurrente, simultaneidad WAIT, orden publicación de eventos/snapshots, desborde de tickets, y tratamiento detallado del error de cancelación. Las propuestas están separadas en [DECISIONES_APROBADAS.md](DECISIONES_APROBADAS.md); añadir v2 no constituye adopción.
