# Integrante 1 — Modelo, estructuras de datos y memoria

Esta asignación conserva el alcance de `master`/`castilla` y devuelve las rutas, las operaciones esperadas y los escenarios originales. Para interfaces exactas, los encabezados actuales de `contracts/` y `core/` son normativos; ejemplos antiguos que no coinciden con ellos no deben copiarse.

## Objetivo y responsabilidades

Representar procesos, almacenarlos dinámicamente y administrar la capacidad y ocupación de memoria simulada. El módulo no implementa algoritmo de planificación, aging, Round Robin, ejecución de quantum, workers, animaciones ni widgets.

### Modelo vigente

El contrato `Proceso` incluye: PID (`int`), nombre, tamaño en bytes (`int`), prioridad base/actual (`int`), tiempo total/restante/espera y quantum consumido (`std::int64_t` milisegundos), estado, orden de llegada (`std::int64_t`) y operación opcional de E/S. La validez exige PID positivo, nombre no vacío, tamaño positivo, prioridades entre 1 y 10, tiempos coherentes y operación de E/S válida si se habilita. La cuota individual de las notas del docente no forma parte de v1.0; consultar DP-04.

Los estados y `estadoToString` viven en [contracts/Estado.h](contracts/Estado.h). No dupliques el enum en `core/`.

## API de ColaProcesos

Las firmas vigentes están en [core/ColaProcesos.h](core/ColaProcesos.h):

| Método | Resultado y semántica vigente |
|---|---|
| `agregar(const Proceso&)` | Inserta al final; lanza `std::invalid_argument` para proceso inválido o PID duplicado |
| `eliminar(int pid)` | Devuelve `true` si quita el PID; `false` si no existe |
| `buscar(int pid)` y sobrecarga const | Devuelve puntero al proceso o `nullptr` |
| `obtenerTodos() const` | Devuelve copia de los procesos en orden de inserción |
| `obtenerPorEstado(Estado) const` | Devuelve copia de los procesos que coinciden, conservando orden de almacenamiento |
| `vacia() const noexcept` | Indica si no hay elementos |
| `cantidad() const noexcept` | Devuelve `std::size_t` con la cantidad |

La estructura actual es `std::list<Proceso>`. No elige el siguiente proceso ni ordena por prioridad. Los usuarios de la API reciben copias en los métodos de enumeración.

## API de Memoria

Las firmas vigentes están en [core/Memoria.h](core/Memoria.h):

| Método | Resultado y semántica vigente |
|---|---|
| `Memoria(int capacidadTotal)` | Construye capacidad en bytes; capacidad negativa lanza `std::invalid_argument` |
| `puedeCargar(const Proceso&) const noexcept` | `true` solo si el proceso es válido, no está cargado y cabe |
| `cargar(const Proceso&)` | Carga y devuelve `true`; si no es posible devuelve `false` sin mutar la asignación |
| `liberar(int pid) noexcept` | Libera los bytes si estaba cargado; PID ausente es no-op |
| `obtenerTotal() const noexcept` | Capacidad total en bytes |
| `obtenerUtilizada() const noexcept` | Bytes actualmente asignados |
| `obtenerDisponible() const noexcept` | Total menos utilizado |
| `estaCargado(int pid) const noexcept` | Indica si el PID tiene asignación |

El modelo no reserva direcciones ni implementa paginación, segmentación, memoria virtual o reemplazo. La GUI sí debe visualizar la ocupación por proceso de forma conceptual, según DP-08.

## Casos operativos y pruebas

Pruebas del módulo ya nombradas en la rama base:

- `tests/test_proceso.cpp`: construcción/validación de procesos y casos inválidos de acuerdo con `procesoValido`.
- `tests/test_cola.cpp`: altas, cantidad, búsqueda por PID, eliminación de elemento y PID ausente, orden de inserción y filtros por estado.
- `tests/test_memoria.cpp`: capacidad, carga, descarga, PID repetido, proceso inválido y rechazo por falta de espacio sin alterar la memoria.

Escenarios de referencia conservados:

1. **Altas:** agregar A, B y C; la cantidad es 3 y la enumeración conserva el orden de inserción.
2. **Búsqueda:** PID 2 localiza B; un PID que no existe produce `nullptr`.
3. **Baja:** A→B→C; eliminar B deja A→C. Eliminar PID inexistente devuelve `false`.
4. **Carga y descarga:** memoria total 2,048 bytes; A=500 y B=1,000 bytes dan 1,500 utilizados y 548 disponibles; al liberar A, quedan 1,000 utilizados.
5. **Rechazo:** en capacidad 1,000 bytes, cargar A=600 tiene éxito y deja 600 utilizados; B=500 no cabe, `cargar` devuelve `false` y el uso sigue en 600.
6. **Validación al agregar:** proceso inválido o PID duplicado lanza `std::invalid_argument`; no se agrega un segundo elemento.

Los ejemplos usan bytes para coincidir con los contratos; no usar los MB del borrador antiguo con funciones cuyo tamaño se expresa en bytes.

## Entregables y dependencias

- Contratos compartidos vigentes: `contracts/Proceso.h`, `Estado.h`, `Configuracion.h`, `EstadoSistema.h`, `Evento.h` e `IExecutionEngine.h`. La congelación corresponde al equipo y la historia de cambios, no a una decisión del profesor.
- Modelo y almacenamiento: `core/ColaProcesos.h`, `core/ColaProcesos.cpp`.
- Memoria: `core/Memoria.h`, `core/Memoria.cpp`.
- Pruebas: `tests/test_proceso.cpp`, `tests/test_cola.cpp`, `tests/test_memoria.cpp`.

Este módulo puede comenzar primero. Scheduler, motores y GUI dependen de los contratos de proceso/estado/snapshot; mantenerlos estables. La página 25 dice que se implementará un modelo dinámico de lista enlazada simple con dos punteros, inicio y final. Se conserva esa intención explícita; queda pendiente confirmar si esa estructura concreta es obligatoria para la entrega (DP-09). El código actual usa `std::list` doblemente enlazada; no sustituir unilateralmente el contenedor. El pseudocódigo de cola/prioridad de las págs. 25–26 es incompleto o inconsistente y no constituye una API.

## Criterios de aceptación

- Procesos válidos se agregan una sola vez y se encuentran por PID.
- Se pueden enumerar todos y filtrar por estado en orden de inserción.
- Eliminación informa si quitó el elemento y preserva los demás.
- Memoria acepta una carga que cabe, rechaza una que no cabe sin alterar sus contadores y libera por PID.
- Las firmas, tipos, unidades y excepciones coinciden con los encabezados vigentes.
- No hay política de Scheduler, GUI ni concurrencia dentro de modelo/cola/memoria.
- La diferencia de estructura enlazada se conserva como pendiente, sin presentar la elección actual como requisito docente.
