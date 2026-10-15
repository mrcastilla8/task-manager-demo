# Integrante 1 — Modelo, estructuras de datos y memoria

> **Instrucción operativa para migración v2:** Castilla mantiene el alcance de modelo, lista y memoria y migra solo después de ratificar/adoptar los contratos candidatos. v2 clasifica automáticamente P1–P3; la cuota vigente deriva de prioridad (10/20/30 ms), sin cuota mutable duplicada. No editar v1 durante la fase contractual. La descripción de la API v1 al final es histórica y no una lista de trabajo v2.

**Responsable:** Castilla · **Rama:** `castilla` · **Workflow:** [fase 1](docs/WORKFLOW_DESARROLLO.md#fase-1--modelo-estructuras-de-datos-y-memoria)

## Objetivo y entregables

Modelar procesos activos e históricos, mantener cola/lista y capacidad de memoria. El módulo no implementa Scheduler, reloj, ejecución de cuota, workers, animaciones ni widgets.

- `contracts/v2/Proceso.h` y los contratos asociados son candidatos; no migrar el código consumidor hasta ratificación.
- `core/ColaProcesos.h/.cpp`, `core/Memoria.h/.cpp` son código v1 existente; mantenerlos y sus pruebas intactos durante preparación documental.
- Pruebas base: `tests/test_proceso.cpp`, `tests/test_cola.cpp`, `tests/test_memoria.cpp` y `tests/test_integracion.cpp`.

## Modelo destino v2

`AltaProceso` no acepta prioridad, ticket ni cuota. La prioridad se deriva del tamaño relativo entre procesos activos READY/RUN/WAIT conforme al algoritmo documentado. FINISH/CANCEL salen del conjunto activo. Cuota vigente no es un campo mutable: `cuotaMs(P1)=10`, P2=20, P3=30 ms. El snapshot puede llevar separada la cuota fijada del turno RUN. Una reclasificación nunca deja estado público como prioridad P1 con cuota vigente de 30 ms.

Validar PID positivo, nombre no vacío, tamaño positivo, CPU válida y E/S opcional consistente. La lista v2 propuesta es simple enlazada con nodos primero/último/siguiente según el acuerdo de estructura; gestionar ownership, copia/move, borrado y destrucción. Memoria usa tamaños y contadores compatibles con el rango v2, reserva al admitir y libera una sola vez en FINISH/CANCEL; WAIT conserva asignación. No modelar direcciones físicas, paginación ni memoria virtual.

## Escenarios y pruebas

- `test_proceso.cpp`: conservar cobertura existente v1; la futura suite v2 prueba construcción/validación con API ratificada.
- `test_cola.cpp`: altas, búsqueda PID, borrado existente/ausente, orden y filtros; migrar aserciones al tipo de lista aprobado.
- `test_memoria.cpp`: capacidad, carga, descarga, PID repetido, proceso inválido y rechazo sin mutación.
- `test_integracion.cpp`: coherencia modelo/cola/memoria y admisión/rechazo.

Escenarios numéricos conservados: capacidad 2,048 bytes con A=500/B=1,000 deja 1,500 usados y 548 disponibles; liberar A deja 1,000. Con capacidad 1,000, admitir A=600 y rechazar B=500 deja uso en 600. FIFO describe orden de almacenamiento; Scheduler determina READY aparte.

## Contexto histórico v1

`contracts/*.h`/`core/` siguen vigentes para el código integrado: `std::list`, prioridad base/actual 1–10, quantum consumido, orden de llegada y validación/excepciones actuales. Sus firmas, ejemplos y pruebas existentes se conservan; no se modifican en esta fase. La documentación v1 no gobierna diseño ni tareas v2.

## Criterios de aceptación futura

- API ratificada consistente, cuota vigente derivada exclusivamente de prioridad y turno fijado separado.
- Inserción, búsqueda, filtros y borrado correctos; memoria rechaza sin cambiar contadores y libera por PID una sola vez.
- Ownership/copia/move de la lista sin fugas ni referencias colgantes.
- Unidades y rangos claros; sin planificación dentro del modelo.
- Registrar compilación/pruebas realmente ejecutadas; no presentar presencia de archivos como resultado PASS.
