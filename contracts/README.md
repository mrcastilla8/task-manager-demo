# Contratos compartidos — versión 1.0

> **Actualización:** `contracts/*.h` sigue siendo v1.0 congelada y vigente para código existente. `contracts/v2/*.h` es una candidata namespaced a adopción, sin implementación consumidora. Prioridad 1–10 y quantum 500 ms describen únicamente v1; no son política operativa de la candidata v2. Consulte [contratos candidatos 2.0](../docs/CONTRATOS_V2.md), [decisiones aprobadas](../docs/DECISIONES_APROBADAS.md) y [migración](../docs/MIGRACION_CONTRATOS_V2.md).

Estos encabezados son la API estable entre modelo, scheduler, motores y GUI.
Todos los módulos deben depender de ellos y no de detalles internos de otro
módulo.

## Decisiones congeladas

- Los tiempos se expresan en milisegundos simulados con `std::int64_t`.
- Los tamaños de proceso y memoria se expresan en bytes con `int`.
- Los PID válidos son positivos; `-1` representa ausencia de proceso.
- Las prioridades válidas están entre 1 y 10, inclusive; 10 es la máxima.
- `ordenLlegada` permite desempates FIFO/Round Robin deterministas.
- La E/S se activa con `OperacionIO::habilitada`; cuando está activa indica tras
  cuántos milisegundos de CPU ocurre el bloqueo y cuánto dura el estado `WAIT`.
- Un bloque de timeline representa el intervalo semiabierto `[inicio, fin)`.
- `EstadoSistema` es una fotografía por valor. Modificarla no modifica el motor.
- `colaPrioridad` contiene PID en el orden en que deben representarse.
- `pidEjecutando == -1` significa que el CPU simulado está libre.
- El quantum pertenece al motor, se expresa en milisegundos y se configura a
  través de `IExecutionEngine`.
- La velocidad visual no forma parte del motor ni de estos contratos.
- Los valores predeterminados compartidos viven en `Configuracion.h`; el
  scheduler puede copiarlos en su configuración modificable.

## Semántica de `IExecutionEngine`

- `agregarProceso` lanza `std::invalid_argument` ante datos inválidos o PID
  duplicado, y `std::runtime_error` cuando un proceso válido no puede admitirse.
- `cancelarProceso` es idempotente para PID inexistentes o ya cancelados.
- `iniciar` reanuda desde pausa y es idempotente si el motor ya está ejecutando.
- `pausar` es idempotente si el motor ya está pausado.
- `reiniciar` detiene y libera workers, borra la ejecución y conserva la
  configuración de quantum y capacidad de memoria.
- `siguientePaso` ejecuta exactamente una transición significativa y requiere
  estado pausado; en otro estado lanza `std::logic_error`.
- `configurarQuantum` exige un valor positivo, lanza `std::invalid_argument` en
  caso contrario y aplica el cambio desde la siguiente asignación de CPU.
- `obtenerEstado` devuelve un snapshot por valor y debe ser thread-safe en el
  motor concurrente.

## Políticas de error relacionadas

- `procesoValido` permite validar antes de enviar un proceso a un módulo.
- `ColaProcesos::agregar` lanza `std::invalid_argument` para procesos inválidos
  o PID duplicados.
- `Memoria::cargar` devuelve `false` y no cambia el estado si la carga falla.
- `Memoria::liberar` es idempotente para PID inexistentes.

## Compatibilidad

La versión 1.0 queda congelada. Cambiar campos públicos, valores de enums,
unidades, orden de agregados, excepciones o métodos virtuales requiere publicar
una nueva versión del contrato y coordinar la migración de todos los módulos.
Correcciones internas y funciones auxiliares que no cambien el comportamiento
observable pueden incorporarse sin romper la versión.

## Precedencia documental y fuentes académicas

Este archivo y los encabezados `contracts/*.h` describen la API vigente del
equipo; no son una transcripción de todos los requisitos académicos. La
especificación funcional y la matriz de trazabilidad registran por separado
las reglas del docente, decisiones del equipo y preguntas pendientes. En
particular, la prioridad docente 1–3 y la cuota de proceso no modifican esta
versión 1.0: su compatibilidad con prioridades 1–10 y el quantum global de
500 ms requiere aprobación y una migración coordinada. Consulte
[`docs/DECISIONES_PENDIENTES.md`](../docs/DECISIONES_PENDIENTES.md) (DP-01 y
DP-04).

La guía docente de lista enlazada simple no cambia la implementación de
`ColaProcesos` ni congela esa estructura dentro de la API; consulte DP-09.
`BloqueTimeline` expresa intervalos temporales con PID según la API actual.
Los detalles gráficos pendientes de CPU inactivo y memoria conceptual no
deben resolverse mediante cambios implícitos al contrato.

Una decisión futura que necesite campos de cuota, otra escala de prioridad,
semántica de recálculo o representación de intervalos sin PID deberá proponer
una nueva versión contractual. Ninguna modificación de encabezados se incluye
en esta consolidación documental.
