# Contratos compartidos — versión 1.0

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
