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
devuelto por `buscar` queda invalidado al eliminar ese PID o destruir/mover la
lista. La lista tiene copia profunda y movimientos que vacían la fuente.

`Memoria` registra tamaños positivos por PID y usa `int64_t` para capacidad,
asignaciones y contadores. `reservar` rechaza valores inválidos, duplicados o
insuficientes sin cambiar contadores. Primero inserta el registro y solo luego
actualiza utilización, por lo que un fallo de reserva de contenedor propaga la
excepción sin dejar los contadores desalineados. `liberar` retorna `false` si
no existía PID y no interpreta estados de proceso; WAIT conserva la asignación
porque la liberación se hace explícitamente por PID.
