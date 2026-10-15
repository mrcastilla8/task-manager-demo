# Contratos candidatos 2.0

**Estado: CANDIDATO A ADOPCIÓN. No integrado ni operativo.** `contracts/*.h` permanece en v1.0 y es el contrato de código existente. Los encabezados paralelos viven en `contracts/v2/`, namespace `ConcurrentSimulator::v2`, y pueden coexistir con los globales de v1 en una unidad de traducción.

## Archivos y tipos

- `Configuracion.h`: solo `memoriaTotalBytes`; no configurable quantum ni aging.
- `Estado.h`: estados de proceso READY/RUN/WAIT/FINISH/CANCEL y estado del motor.
- `Proceso.h`: `AltaProceso` no acepta prioridad/ticket/cuota. `Proceso` expone clase automática, cuota actual derivada, CPU total/restante/consumida, espera, operación IO y ticket READY. `cuotaMs(Prioridad)` es prioridad numérica × 10 ms.
- `Evento.h`: eventos simulados timestamped, PID opcional por convención y duración; nunca timestamp de pared.
- `EstadoSistema.h`: reloj, estados, procesos activos e históricos, READY en orden efectivo, RUN, cuota fijada al turno, CPU consumida en turno, memoria, eventos y timeline.
- `IExecutionEngine.h`: resultados explícitos; operaciones de alta/cancelar/iniciar/pausar/reiniciar/paso/snapshot.

## Semántica de prioridad y cuota

TL de reclasificación son procesos READY/RUN/WAIT admitidos. Sea A mínimo y B máximo de sus tamaños, D=B−A, y para cada tamaño S, X=3×(S−A): P1 si X<D, P2 si no y X<2D, P3 en otro caso. Las comparaciones equivalen a límites estrictos del algoritmo docente. A=B produce X=0,D=0 y P3. TL vacía no se clasifica. Implementación evita overflow para tamaños válidos mediante comparación exacta con guardas/cocientes; no depende de extensión de compilador. `uint64_t` ticket aumenta estrictamente al entrar READY; sobre flujo el límite debe manejarse sin envolver (propuesta precisa en decisiones aprobadas).

La cuota base es fija: P1 10 ms, P2 20 ms, P3 30 ms. En un despacho, motor copia la cuota actual a `cuotaTurnoFijadaMs`; al reclasificar mientras RUN, el campo vigente puede cambiar para el siguiente despacho, no el turno abierto. El turno concluye al agotar cuota fijada, completar CPU, entrar WAIT o cancelar.

La llamada que admite un proceso no interrumpe CPU. Solo admisión exitosa cambia TL y dispara clasificación pendiente previa al siguiente despacho. FINISH/CANCEL se excluyen de TL, conservándose en historial. Admisión rechazada no altera memoria, TL ni prioridades.

## Scheduler y estados

Una única implementación compartida ordena READY por prioridad descendente, ticket ascendente y PID ascendente. El ticket cambia en admisión, retorno WAIT y fin de cuota. La cola que almacena procesos es estructura de datos, no política de elección. No existe aging.

Transiciones operativas aprobadas: admisión→READY; READY→RUN al despacho; RUN→READY por fin de cuota; RUN→WAIT por E/S; WAIT→READY al despertar; RUN→FINISH al agotarse CPU; READY/RUN/WAIT→CANCEL por cancelación. FINISH/CANCEL son terminales. Admisión exige proceso válido, PID nuevo y memoria disponible. Memoria se reserva al admitir y libera una vez al FINISH/CANCEL; WAIT conserva memoria.

## Unidad de paso e interfaz común

`siguientePaso()` exige motor PAUSADO. Si READY existe, hace un despacho y ejecuta una ráfaga hasta cuota, WAIT, FINISH o CANCEL; devuelve avance lógico y si usó CPU. Si no hay READY y existe WAIT con despertar, avanza el reloj hasta el vencimiento más cercano sin CPU; WAIT se vuelve READY y ese despertar no despacha en el mismo paso. Sin READY/WAIT con trabajo activo, el sistema llega a estado finalizado. Motor continuo itera las mismas unidades. Logical/concurrent deben concordar en resultados, eventos, reloj, timeline y snapshots; los threads reales no representan CPUs adicionales.

Los eventos tienen orden reproducible; los intervalos se expresan `[inicio,fin)` en ms simulados. Timeline admite PID positivo o PID=0 únicamente como CPU idle. Snapshot copia estado coherente; debe poder observar RUN vía eventos/snapshots aunque el motor termine la ráfaga antes del repintado GUI. GUI nunca avanza reloj.

## Errores, pausa, reinicio y cancelación

`Resultado` representa argumento inválido, PID duplicado, memoria insuficiente, inexistente y estado inválido sin excepción de política. Fallos de recursos del sistema, como `std::bad_alloc`, pueden propagarse. Paso cuando no PAUSADO devuelve ESTADO_INVALIDO. `iniciar` reanuda PAUSADO; pausar e iniciar pueden ser idempotentes. Reiniciar lleva a estado inicial y conserva configuración. La política concreta sugerida para cancelar PID inexistente/terminal es devolver `PID_INEXISTENTE` sin efectos; ratificarla al adoptar API.

La política sugerida para cancelar terminal/inexistente devuelve `PID_INEXISTENTE` sin efectos; ratificarla al adoptar API. Cancelación RUN debe alcanzar punto seguro lógico común: se contabiliza únicamente el CPU simulado ya completado, se cierra timeline solo para duración >0 y se publica CANCEL/liberación de memoria una vez. Granularidad exacta de los subpasos de cancelación queda en propuestas técnicas, no fijada por DP-06.

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
