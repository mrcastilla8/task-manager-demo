# Integrante 2 — Scheduler

> **Instrucción operativa v2 candidata:** Brickzon migrará el Scheduler tras ratificar e integrar el contrato v2. El destino usa prioridad automática P1–P3, cuota máxima derivada 10/20/30 ms, orden READY por prioridad descendente/ticket ascendente/PID ascendente y ningún aging activo. Estos contratos candidatos aún no son operativos para código integrado. Las reglas v1 al final son antecedentes de migración; no tareas v2 pendientes.

**Responsable:** Brickzon · **Rama:** `brickzon` · **Workflow:** [fase 2](docs/WORKFLOW_DESARROLLO.md#fase-2--scheduler)

## Objetivo y entregables

Implementar una sola regla de selección compartida por ambos motores. El Scheduler inspecciona READY, compara prioridades/tickets/PID de forma determinista y prepara la cola visible; no crea threads, memoria, GUI ni reloj.

- `scheduler/Scheduler.h`, `scheduler/Scheduler.cpp`, `scheduler/SchedulerConfig.h` (si la interfaz ratificada requiere configuración; no aging).
- Pruebas: `tests/test_scheduler_prioridad.cpp`, `tests/test_scheduler_empate.cpp`.
- `tests/test_scheduler_aging.cpp` es una prueba heredada de v1. Conservarla durante compatibilidad y migrarla/eliminar su expectativa solo en coordinación con la suite v2; aging no es requisito v2.

## Reglas destino aprobadas

- La prioridad es una clase automática P1–P3 calculada desde tamaños de los procesos activos según `DECISIONES_APROBADAS.md`.
- La cuota máxima vigente se deriva como `cuotaMs(prioridad)` (10/20/30 ms). El Scheduler elige el proceso; el motor copia la cuota al inicio de RUN en un valor de turno fijado. No existe cuota pública independiente ni quantum global configurable.
- Seleccionar solo READY. Orden: prioridad descendente, ticket ascendente y PID ascendente como desempate defensivo.
- Asignar ticket estrictamente creciente al entrar a READY por admisión, retorno de WAIT o cuota agotada. No reutilizar `ordenLlegada` v1 como fuente de verdad v2.
- La espera puede medirse, pero no cambia prioridad por aging.
- Ambos motores consultan esta misma implementación; ninguno duplica el comparador.

## Validaciones y pruebas conservadas

Cubrir lista vacía, estados mezclados, prioridad máxima, empate y PID, tickets renovados en cada entrada READY, límite/overflow de ticket conforme a la decisión técnica que se ratifique y estabilidad determinista. Probar las fronteras y el caso A=B en el cálculo de prioridad. La cuota debe derivarse desde prioridad y no ser escrita por Scheduler.

Los casos de `test_scheduler_aging.cpp` permanecen identificados como historial v1 hasta que la migración de tests sea autorizada; no se deben implementar aging ni incorporar aging a `SchedulerConfig` v2 para hacerlos pasar.

## Contexto histórico v1

La rama heredada ordenaba escala 1–10 por `prioridadActual`, `ordenLlegada` y PID, y exponía `agingIntervaloMs`/`agingIncremento`. Esos campos explican compatibilidad y pruebas existentes, pero fueron reemplazados para el destino v2 por P1–P3 y sin aging. Mantener `contracts/*.h` y el código v1 intactos durante la fase documental.

## Criterios de aceptación al migrar

- Selección READY determinista con la regla común; motores no incluyen comparadores alternativos.
- Ticket nuevo al reingresar READY; alternancia demostrable entre iguales tras los turnos.
- Ningún aumento de prioridad por tiempo de espera ni quantum global configurable.
- Pruebas de prioridad/empate ejecutadas junto con casos compartidos; no reportar prueba pasada por el mero hecho de que el archivo exista.
- Migración precedida por ratificación de API v2 y revisión del contrato común.
