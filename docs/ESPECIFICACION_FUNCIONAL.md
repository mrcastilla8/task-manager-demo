# Especificación funcional de ConcurrentSimulator

**Estado canónico:** las decisiones aprobadas DP-01–DP-11 rigen el destino de contratos v2, aún candidato a adopción y no implementado. Los contratos v1.0 siguen vigentes para el código actual. Las políticas v1 y auditorías anteriores se identifican expresamente como históricas. Consulte [decisiones aprobadas](DECISIONES_APROBADAS.md), [contratos candidatos](CONTRATOS_V2.md) y [plan de migración](MIGRACION_CONTRATOS_V2.md).

## Objetivo y fuente

El simulador representa admisión de procesos, prioridad, memoria, un CPU lógico por turnos, estados y una vista temporal. Las notas del docente describen procesos, clases de prioridad por tamaño y time-sharing; DP-01–DP-11 son decisiones del equipo y no se atribuyen como requisitos docentes. Los documentos fuente permanecen fuera del repositorio; sus localizadores constan en [REQUISITOS_DOCENTE.md](REQUISITOS_DOCENTE.md).

La ruta de trabajo v2 es: validar alta sin prioridad manual; admitir solo si PID/datos/memoria son válidos; colocar en READY con ticket nuevo; seleccionar con el Scheduler común; ejecutar una ráfaga limitada por cuota; registrar tiempo, eventos y CPU realmente ejecutada; resolver WAIT/FINISH/CANCEL; recalcular cuando DP-02 lo indica; repetir.

## Proceso, prioridad y cuota

`AltaProceso` contiene PID, nombre, tamaño, CPU total y operación de E/S, nunca prioridad, cuota o ticket. `Proceso.prioridad` se clasifica automáticamente usando TL={READY,RUN,WAIT}. FINISH/CANCEL no participan en mínimo/máximo. TL vacía no calcula extremos. La regla exacta con tamaños enteros: D=B−A y X=3×(S−A); P1 si X<D, P2 si no se cumplió y X<2D, de otro modo P3. Límites estrictos. A=B produce P3. La evaluación debe evitar overflow para todo tamaño admitido.

La cuota vigente es una sola fuente derivada: `cuotaVigenteMs(proceso)=cuotaMs(proceso.prioridad)`; P1=10 ms, P2=20 ms, P3=30 ms. No se almacena una cuota vigente mutable en `Proceso`. Al despacho, el motor copia el resultado a `EstadoSistema.cuotaTurnoFijadaMs`; esa cuota no cambia durante el turno aunque se reclasifique el conjunto. Es una cuota máxima: CPU restante agotada, WAIT o cancelación pueden terminar la ráfaga antes.

Después de una admisión exitosa, finalización o cancelación se solicita reclasificación y se aplica antes del siguiente despacho. Alta exitosa no interrumpe RUN ni cambia su cuota ya fijada. Alta rechazada por memoria no altera TL ni prioridades. No hay prioridad manual ni política activa 1–10 en v2.

## Scheduler y estados

Una implementación compartida selecciona entre READY por prioridad descendente, ticket ascendente y PID ascendente como desempate defensivo. El ticket estrictamente creciente se renueva al ingresar a READY por admisión, retorno de WAIT o cuota agotada. La estructura de cola no elige el próximo proceso. No hay aging; el tiempo de espera puede seguir como métrica.

| Desde | Hacia | Disparador/efecto v2 |
|---|---|---|
| Alta válida | READY | Reserva memoria y ticket; admisión fallida no muta estado activo. |
| READY | RUN | Despacho y fijación de prioridad/cuota del turno. |
| RUN | READY | Cuota agotada; ticket nuevo. |
| RUN | WAIT | Bloqueo por E/S; conserva memoria. |
| WAIT | READY | Despertar previsto; ticket nuevo. |
| RUN | FINISH | CPU restante llega a cero; libera memoria; estado terminal. |
| READY/RUN/WAIT | CANCEL | Cancelación; libera memoria una vez; estado terminal. |

Las reglas anteriores son decisiones funcionales del equipo para v2. La granularidad segura reproducible, la carrera con FINISH y el orden de publicación siguen como propuestas técnicas, no completan DP-06 por inferencia. El protocolo de checkpoints lógicos propuesto está en [Propuestas técnicas v2](PROPUESTAS_TECNICAS_V2.md#3-cancelación-run-parcial).

## Reloj, paso, eventos y timeline

Todo tiempo del modelo usa milisegundos simulados. Threads reales, demoras de coordinación, animación y refresco Qt no avanzan por sí mismos ese reloj.

`siguientePaso()` requiere PAUSADO. Con READY despacha y ejecuta una ráfaga hasta cuota, WAIT, FINISH o CANCEL. Sin READY y con WAIT, avanza el reloj hasta el despertar más próximo sin atribuir CPU y sin despachar en esa misma llamada; el próximo despacho pertenece al paso siguiente. Sin procesos activos ni WAIT pendiente, publica FINALIZADO. DP-07 aprueba que modo continuo repita esas unidades, pero el contrato no define todavía cómo impulsar el motor lógico sin bloquear Qt. No llamar a un bucle continuo dentro de `iniciar()`. La [propuesta técnica](PROPUESTAS_TECNICAS_V2.md#1-ejecución-continua-sin-bloquear-qt) compara timer cooperativo del host con continuador inyectado; no se ratificó cambio de firma ni mecanismo. Ambos motores deben dar el mismo resultado observable por unidad.

Cada `BloqueTimeline` es exclusivamente un intervalo de CPU realmente ejecutado, semiabierto `[inicioMs,finMs)`, con PID positivo. Los intervalos van en orden cronológico, son positivos y no se solapan. CPU idle no es un bloque, no tiene PID de timeline y no introduce tipo contractual de segmento idle. La vista infiere huecos dentro de `[0, EstadoSistema.tiempoSimuladoMs)`: antes del primer intervalo, entre intervalos y después del último hasta el reloj del snapshot. Un `Evento` de CPU idle puede señalar inicio/fin con `pid=-1` como ausencia de proceso asociado, pero no cambia la regla de los bloques.

Ejemplo de ejecución, espera e idle inferido: A ejecuta `[0,8)`; luego A está WAIT y el CPU no ejecuta durante `[8,18)`; al despertar, B ejecuta `[18,22)`. Con snapshot en tiempo 27, el timeline contractual contiene solamente bloques `(A,[0,8))` y `(B,[18,22))`. La vista deduce idle `[8,18)` y `[22,27)`. No crea segmentos para esos huecos. Una pausa de interfaz sin avance del reloj no crea intervalo idle simulado.

Eventos/snapshots reproducibles deben permitir mostrar RUN transitorio incluso si el motor completa una ráfaga antes del repintado. `Evento` hoy no define orden total de timestamps iguales ni inicio/fin de CPU. La propuesta de secuencia monótona, representación `[inicio,inicio+duración)` y replay por frontera está en [Propuestas técnicas v2](PROPUESTAS_TECNICAS_V2.md#4-orden-y-representación-de-eventos); no se ha añadido campo a los headers. El GUI no avanza el reloj.

## Memoria e interfaz

La admisión reserva tamaño por PID; WAIT conserva memoria; FINISH/CANCEL la libera una vez. La vista muestra proporciones por PID y espacio libre sin inventar direcciones. Procesos terminales permanecen en el historial de sesión aunque se excluyan de TL/memoria activa.

La interfaz destino está aprobada con Qt 6, C++17 y CMake. Poma consume la interfaz común y presenta datos; los widgets no planifican. Aldana mantiene motor lógico; Ángel motor concurrente. Ambos reutilizan el mismo Scheduler y representan un CPU lógico, incluso si uno utiliza varios threads.

El historial completo de sesión no descarta información. Indexación, eventos compactos, snapshots y render por ventana/zoom son propuestas de almacenamiento; el usuario puede desplazarse/usar zoom a través de la sesión.

## Política de errores candidata

`Resultado`, `CodigoResultado` y `ResultadoPaso` son propuestas técnicas; DP-01–DP-11 no eligieron el mecanismo de error. La [comparación por método y capa](PROPUESTAS_TECNICAS_V2.md#2-errores-excepciones-frente-a-códigos) muestra efectos sobre motores, GUI, mocks y pruebas. La distinción PID inexistente/FINISH/CANCEL también requiere ratificación; un terminal conocido no equivale a PID inexistente.

## Contexto histórico v1.0

El contrato raíz `contracts/*.h` mantiene tipos globales, escala 1–10, prioridad base/actual, quantum configurable 500 ms, aging, `quantumConsumido`, `ordenLlegada` y semántica previa de excepciones. Son vigentes para el código v1 que aún depende de ellos, no instrucciones de implementación v2. No modificar v1 durante la migración. La auditoría observó Scheduler en `brickzon`/`angel`, motor concurrente en `angel`, y ausencia de motor lógico/GUI en ramas revisadas; existencia en rama no significa integración.

## Criterios de aceptación v2

- Clasificación exacta, fronteras estrictas, A=B, TL vacía y extremos enteros sin overflow.
- Cuota vigente derivada solo de prioridad y cuota de turno fijada; sin doble fuente de verdad.
- Selección READY prioridad/ticket/PID, tickets nuevos en cada ingreso y sin aging.
- Paso, WAIT/idle, transiciones, CPU consumida y timeline iguales en ambos motores.
- Todo bloque timeline usa PID>0 y representa CPU ejecutado; los huecos se infieren al renderizar.
- Snapshot y replay no alteran el reloj simulado; interfaz Qt 6 no contiene política de planificación.
- Documentación separa políticas de equipo de evidencia docente y v1 de candidata v2.
