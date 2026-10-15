# Migración de contratos v1.0 a v2.0 candidata

**Estado:** plan propuesto para una fase posterior. v1.0 sigue vigente en master; los encabezados `contracts/v2/` son candidatos aislados y ningún módulo actual depende de ellos. Migrar cada integrante en su rama tras revisión/adopción, portar con cambios compatibles, e integrar únicamente vía PRs. Mantener `contracts/*.h` y el build master compilables durante la convivencia; no renombrar/eliminar tipos v1 hasta que todas las ramas dependientes se hayan integrado y una versión posterior autorice retirada.

## Base auditada

Auditoría de `origin/master` en `c4613e5` y ramas remotas examinadas el 2026-10-15: master contiene `core/ColaProcesos` sobre `std::list`, búsqueda lineal por PID y copia a vector para vistas/filtrado; `Memoria` lleva total/usado y mapa PID→tamaño. Castilla mantiene esos mismos archivos de v1. `brickzon` contiene Scheduler que filtra READY y compara `prioridadActual` descendente, `ordenLlegada` ascendente y PID ascendente; además expone aging y restablece prioridad al asignar CPU. `angel` contiene ConcurrentEngine con quantum global y workers `ProcessThread`; reingreso tras quantum no actualiza `ordenLlegada`. El Scheduler duplica también en esa rama. `aldana` no contiene `simulation/`; `poma` no contiene `gui/`; no hay implementación nueva integrada en master. Estos hechos justifican migración por compatibilidad, no son afirmaciones de conformidad v2 ni resultados de pruebas.

| Elemento v1 | Problema | Equivalente v2 | Impacto | Responsable | Validación |
|---|---|---|---|---|---|
| Constructor/validación `Proceso` | Alta exige campos/prioridad 1–10; permite fuentes duplicadas | `AltaProceso` sin prioridad; validación de PID/nombre/tamaño/CPU/IO en motor | API de altas y fixtures | Castilla | Casos inválido, PID repetido, límites y alta aceptada/rechazada |
| `prioridadBase`, `prioridadActual` | Prioridad manual/aging en conflicto con clasificación dinámica | `Prioridad` P1–P3 derivada de TL activa | Modelo, scheduler, GUI | Castilla + Brickzon | exactitud, fronteras, A=B, TL vacía y reclasificación |
| `quantumConsumido` | No define cuota por proceso ni total CPU | CPU del turno y total/restante por separado | modelo, eventos, timeline | Castilla + Aldana/Ángel | contabilidad por ráfaga y fin temprano |
| Configuración quantum 500 ms/`configurarQuantum`/`obtenerQuantum` | Global contradice cuota fija por clase | Eliminar controles; cuota 10/20/30 del despacho | Scheduler, motor, GUI | Castilla, Brickzon, Aldana, Ángel, Poma | interfaz no expone quantum global; cuota fijada por despacho |
| READY y `ordenLlegada` | Campo original se reutiliza; test puede ocultar falta de RR integrado | Ticket creciente al cada entrada READY | Scheduler y motores | Brickzon + Aldana/Ángel | alta/WAIT/cuota dan ticket nuevo; comparador único |
| Scheduler config/aging | Aging modifica escala y compite con prioridad por tamaño | Sin aging; Scheduler compartido P desc, ticket asc, PID asc | política activa | Brickzon | espera no cambia prioridad; selección determinista |
| `std::list` en ColaProcesos | No satisface estructura enlazada acordada para futura migración | Lista simple Node/primero/último/siguiente | propiedad/copia/move/memoria | Castilla | API pública, orden, borrado, copia y destrucción seguros |
| Memoria | `int` bytes; asignaciones se conservan hasta fin/cancel | Capacidad y tamaños `int64_t`, alta reserva; WAIT conserva; FINISH/CANCEL liberan | conversión de tipos y contabilidad | Castilla + motores | límites, insuficiencia no modifica TL/memoria |
| `IExecutionEngine` v1 | paso ambiguo, excepciones y quantum global | interfaz v2 con resultados y paso normativo | rompe implementadores/mocks | Castilla, Aldana, Ángel, Poma | fake, motores satisfacen interfaz común |
| Eventos | No representa idle, tiempo CPU parcial ni secuencia de reconstrucción completa | eventos con ms simulados, duración/tipo, secuencia propuesta | GUI, ambos motores | Aldana + Ángel + Poma | reconstrucción determinista, no usar wall clock |
| `EstadoSistema`/snapshot | `quantumMs`, timeline sin idle y procesos no distinguen cuota de turno | Snapshot con cola ordenada, cuota fijada, CPU en turno, memoria/timeline | GUI y pruebas | Castilla + Poma | snapshot consistente y v1 sigue compilando |
| Timeline | PID obligatorio, no hueco idle, retención incierta | intervalos CPU `[a,b)` con PID=0 para idle | GUI y storage | Poma + motores | hueco no atribuye CPU/PID y reloj coherente |
| WAIT y cancelación | transiciones/disparadores/duración poco formalizados | estado terminal y efectos aprobados DP-06/07 | motores | Aldana + Ángel | igualdad de eventos, memoria, CPU parcial en ambos motores |
| Mocks/GUI | Formulario permite prioridad, widgets podrían planificar | alta sin prioridad; mock `IExecutionEngine` v2; solo presenta snapshots/eventos | sustitución de bindings | Poma | tests de mock; widget no contiene selector |
| Pruebas antiguas | Esperan 1–10, aging, quantum 500, `ordenLlegada`/excepciones v1 | suites v2 con frontera exacta, paso, cancelación y equivalencia | pruebas API y escenarios | Cada integrante; integración por CI | conservar tests v1 para v1; nuevas suites compilan contra namespace v2 |

## Secuencia para no romper master

1. Mantener cada encabezado y consumidor v1 aislados. Los nuevos tipos están en `ConcurrentSimulator::v2`; no hacer alias globales ni editar configuración/build raíz durante preparación documental.
2. Revisar/adoptar contratos y ratificar propuestas antes de portar.
3. Castilla crea tipos v2/core enlazados en su rama; un PR integra primero modelo sin borrar v1.
4. Brickzon integra Scheduler v2 contra tipos v2. Motores solo portan después de que esos contratos compartidos estén disponibles.
5. Aldana y Ángel pueden portar en paralelo sobre el mismo punto contractual congelado y comparar semántica/casos.
6. Poma desarrolla mock UI v2 en paralelo una vez congelada interfaz, luego integra ambos motores.
7. CMake añade fuentes sin dejar de construir suites v1. Retirar v1 es fase separada, solo cuando `rg` no encuentre dependientes activos y después de aceptación explícita.

## Responsables, dependencias y gates

Castilla (modelo → estructura/memoria) es prerequisito de Brickzon (Scheduler). Con contratos/Scheduler fijados, Aldana y Ángel migran motores en paralelo; Poma avanza mock Qt 6 contra interfaz congelada en paralelo. Gates: G1 ratificación de propuestas y namespace/API; G2 modelo+validación+memoria; G3 Scheduler y criterios READY; G4 ambos motores pasan mismos vectores deterministas; G5 mock/GUI reproduce eventos y timeline de ambos; G6 revisión de memoria, concurrency, aceptación de equipo y PR. No marcar módulos integrados antes de merge de sus ramas.

## Pruebas y límites conocidos

Mantener por separado C++17 compilando ambos namespaces en una TU, tests v1 sin cambios, suites v2 para fórmula con extremos, tickets, cuotas, WAIT/idle, cancelación parcial y reinicio/pausa; golden traces comparan reloj, estados, prioridades, eventos y timeline por paso. El commit documental solo prueba sintaxis aislada de headers si se dispone de compilador; no certifica el scheduler/motores. Casos de equivalencia concurrente requieren pruebas repetidas y puntos de sincronización controlados, no sleeps físicos como tiempo simulado.
