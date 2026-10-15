# Integrante 3 — Motor de simulación lógica

Documento recuperado de `origin/aldana`. Se conserva su alcance de motor sin threads, y se armoniza el vocabulario de tiempo, transiciones y decisiones con la documentación canónica.

## Objetivo y estado de rama

El motor lógico modela el reloj y el avance de los procesos sin crear threads reales y publica snapshots/eventos mediante `IExecutionEngine`. No contiene widgets ni sustituye al Scheduler. No aparece una implementación de este módulo en las seis ramas revisadas para la auditoría; la asignación documenta trabajo esperado, no una capacidad ya integrada.

## Reglas que deben compartir los motores

El tiempo es milisegundo simulado en el contrato vigente. El equipo mantiene quantum global configurable de 500 ms y E/S opcional. Las notas del profesor describen cuota como atributo y time-sharing en un CPU lógico; la fórmula `prioridad × 10 ms` es un ejemplo, no regla obligatoria confirmada. No mezclar la cuota individual con el quantum común (DP-04).

El motor debe informar transiciones y eventos, tiempo restante, memoria, cola y timeline con la misma semántica que el motor concurrente una vez definida. La unidad de `siguientePaso()` no está resuelta: el contrato 1.0 dice una transición significativa, mientras los documentos de motor permiten entender una ráfaga completa y el motor concurrente actual espera el fin de una ráfaga/turno. Cerrar DP-07 antes de definir pruebas de equivalencia.

Estados docentes: READY, RUN, WAIT, FINISH y CANCEL (notas, págs. 23 y 27). El retorno RUN→READY por agotamiento de turno se desprende de time-sharing; las reglas de E/S y cancelación actuales son decisiones del equipo. No considerar ausente una transición solo porque no esté dibujada. Ver DP-06 y la matriz de la especificación.

## Casos y criterios

Usa [P4, P8, P10–P13](docs/CASOS_REFERENCIA.md) y trazabilidad [RD-08–RD-14, DP-04, DP-06–DP-08](docs/MATRIZ_TRAZABILIDAD.md).

- Define el reloj simulado y el momento de registrar eventos tras aprobar DP-07.
- Un turno crea un bloque timeline `[inicio, fin)` consistente con duración ejecutada.
- WAIT y retorno READY se coordinan con el reloj común acordado.
- Altas dinámicas y cancelaciones siguen transiciones aprobadas, no reglas inventadas.
- En el escenario CPU sin READY no se atribuye ejecución a un proceso; idle requiere decisión visual.
- No implementes reglas o interfaces que requieran modificar contratos v1.0 antes de aprobar una nueva versión.
