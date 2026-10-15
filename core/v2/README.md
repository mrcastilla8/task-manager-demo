# API provisional de Castilla para v2

Estos tipos y operaciones viven en `core/v2/` y sirven para desarrollar y
validar el modelo, la lista y la memoria sin activar la migración de motores.
No modifican ni ratifican `contracts/v2/`; sus nombres y firmas son una API
provisional que se debe coordinar con Brickzon antes de consumirse como
contrato compartido.

## Modelo y clasificación

`validarAltaProceso` exige PID, tamaño y CPU positivos, nombre no vacío. Si la
operación IO está habilitada, el umbral CPU debe estar en `(0, CPU total)` y la
duración WAIT debe ser positiva. Si IO está deshabilitada, sus campos no se
interpretan. `construirProcesoInicial` crea READY con CPU consumida cero,
restante igual al total, prioridad inicial P3 y sin decidir cuándo se
despachará.

`clasificarPrioridades` recibe un vector de procesos, toma los extremos solo
de READY/RUN/WAIT y actualiza esas prioridades en lote. FINISH/CANCEL se
ignoran; un conjunto sin procesos activos no calcula extremos ni cambia nada.
Un tamaño activo no positivo hace que devuelva `false` sin mutar prioridades.
`clasificarTamano` puede reutilizarse para un tamaño dado con extremos positivos
y `minimo <= tamanio <= maximo`; en caso `minimo == maximo` asigna P3. Aplica
P1 si `3*(S-A) < D`, P2 si no es P1 y `3*(S-A) < 2*D`, y P3 en los demás
casos. La implementación usa umbrales enteros por cociente/residuo, sin formar
los productos. Estas funciones calculan clases; no disparan recálculos, eligen
READY ni interrumpen RUN. Esos momentos pertenecen al Scheduler/motor.

La cuota vigente continúa derivándose con `cuotaMs(prioridad)` del header
candidato. No se almacena una cuota mutable en el proceso.

## Lista y memoria

`ListaProcesos` conserva el orden de inserción, rechaza PID no positivo o
duplicado y expone vistas por copia junto a búsqueda prestada. Un puntero
devuelto por `buscar` es const y queda invalidado al eliminar ese PID o
destruir/mover la lista; la identidad no se puede cambiar por esa vista. Los
cambios de estado, prioridad, CPU, espera y ticket se hacen por métodos
controlados. La lista valida los campos del proceso al insertarlo y tiene copia
profunda y movimientos que vacían la fuente. `reclasificarPrioridades()` crea
una copia antes de aplicar la clasificación, por lo que fallos de asignación o
datos inválidos no dejan una actualización parcial. La lista conserva PID,
estado, ticket, contadores y orden; esta operación solo cambia prioridades de
READY/RUN/WAIT.

`Memoria` registra tamaños positivos por PID y usa `int64_t` para capacidad,
asignaciones y contadores. `reservar` rechaza valores inválidos, duplicados o
insuficientes sin cambiar contadores. Primero inserta el registro y solo luego
actualiza utilización, por lo que un fallo de reserva de contenedor propaga la
excepción sin dejar los contadores desalineados. `liberar` retorna `false` si
no existía PID y no interpreta estados de proceso; WAIT conserva la asignación
porque la liberación se hace explícitamente por PID.

`admitirProceso` valida, rechaza PID ya presente en lista o memoria, reserva y
luego inserta. Un guardián libera solo la reserva creada por esa llamada si la
inserción falla o lanza; la inserción normal de `ListaProcesos` tiene garantía
fuerte y deja la lista intacta al fallar. La variante `detail` permite probar
fallos de inserción y no se ofrece como API para Scheduler/motores. Estas
operaciones no son thread-safe ni reemplazan la coordinación del motor.

`actualizarEstado` aplica las transiciones operativas de
[`CONTRATOS_V2.md`](../../docs/CONTRATOS_V2.md#scheduler-y-estados):
READY→RUN/CANCEL, RUN→READY/WAIT/CANCEL y WAIT→READY/CANCEL; RUN→FINISH solo
cuando la CPU restante es cero. FINISH y CANCEL son terminales. Repetir el
mismo estado, incluso uno terminal, devuelve `true` sin mutación; toda otra
transición no permitida devuelve `false` y conserva el proceso. Esto solo
protege el modelo y no define cuándo ni en qué frontera temporal se cancela.

`actualizarCpu` solo acepta avances efectivos mientras el proceso está en RUN.
Una llamada con los mismos contadores actuales es idempotente y se acepta en
cualquier estado, incluidos FINISH y CANCEL. Todo avance debe conservar el
total, mantener CPU consumida no decreciente y CPU restante no creciente, y
usar valores no negativos dentro del total; así no se acumula CPU en READY,
WAIT ni estados terminales. Una llamada rechazada no muta el proceso. La
construcción de una instancia inicia CPU consumida en cero; el módulo no mide
tiempo ni define un reinicio de sesión.

El Scheduler/motor es responsable de asignar tickets READY globales nuevos al
admitir o reingresar un proceso a READY, conforme a DP-05. El valor inicial
cero significa “aún sin ticket asignado”; `actualizarTicketReady` es la
operación local para conservar el valor o avanzar a uno mayor. Un retroceso se
rechaza sin mutación; repetir el mismo ticket es idempotente. Castilla no
garantiza unicidad ni orden global. El contador `uint64_t` no debe envolver:
la respuesta al agotamiento (error de límite o renumeración estable) sigue
pendiente de ratificación según la propuesta 11 de
`docs/DECISIONES_APROBADAS.md`; el generador debe detectar el límite antes de
incrementar.
