# Integrante 1 — Modelo, estructuras de datos y memoria

Este módulo sostiene los datos que usan Scheduler, motores y GUI. Esta versión consolida el documento de `master`, también presente sin diferencias en `castilla`, con los requisitos académicos y la especificación compartida.

## Responsabilidad

Mantener la representación de procesos y el almacenamiento dinámico, y modelar la capacidad y ocupación de memoria. No decide prioridades, no ejecuta procesos y no implementa GUI o threads.

## Contratos v1.0

Los encabezados vigentes son [Proceso](contracts/Proceso.h), [Estado](contracts/Estado.h), [Configuración](contracts/Configuracion.h), [Estado del sistema](contracts/EstadoSistema.h), [Evento](contracts/Evento.h) e [IExecutionEngine](contracts/IExecutionEngine.h). La referencia normativa de unidades, errores, orden y precedencia es [contracts/README.md](contracts/README.md).

Las notas del docente enumeran ID, nombre, tamaño en bytes, estado, cuota de tiempo en milisegundos y prioridad (pág. 23). El contrato actual contempla tiempos y estado, pero no una cuota individual; mantiene quantum global y prioridades de equipo 1–10. La diferencia está en DP-01 y DP-04. No agregues campos ni cambies validaciones mientras la versión 1.0 esté congelada.

## Estructura dinámica y semántica de cola

La lista de procesos permite altas, búsqueda, eliminación y filtros por estado según la API implementada en `ColaProcesos`. No selecciona el proceso que obtiene CPU. La estructura actual usa `std::list<Proceso>` y conserva orden de inserción.

Las notas mencionan lista enlazada simple con referencias al inicio y al final (pág. 25). Esto se registra como propuesta estructural del material cuya obligatoriedad para el proyecto completo está pendiente (DP-09). No reemplazar unilateralmente `std::list`. Tampoco usar “FIFO” para afirmar que el contenedor es un scheduler: FIFO es una regla de orden de llegada; prioridad y reingreso RR corresponden al Scheduler y motor. Los pseudocódigos académicos de cola y prioridad de las págs. 25–26 son incompletos/inconsistentes, por lo que no deben copiarse como API.

## Memoria simulada

La implementación actual rechaza la carga si el proceso es inválido, ya está cargado o supera el espacio disponible; una carga fallida no altera el estado. Libera bytes por PID y expone capacidad/uso disponible. No representa direcciones ni algoritmos de asignación.

La GUI necesita un mapa conceptual de ocupación por proceso, además de capacidad total y utilizada (notas, págs. 23 y 29; DP-08). La figura debe reflejar los procesos cargados y su tamaño relativo sin inventar direcciones físicas, fragmentación, paginación ni reemplazo.

## Casos y criterios

Utiliza [casos P1, P3 y P9](docs/CASOS_REFERENCIA.md) y la matriz [RD-01–RD-03, RD-12, DP-09](docs/MATRIZ_TRAZABILIDAD.md). El módulo se considera documentalmente alineado cuando:

- la estructura dinámica y la memoria se describen con el comportamiento que realmente exponen;
- se distingue almacenamiento FIFO de planificación por prioridad y rotación Round Robin;
- la diferencia entre lista simple académica y `std::list` implementada se mantiene explícita;
- cuota individual, escala 1–3 y cualquier cambio de memoria quedan como decisión hasta aprobación;
- no se alteran los contratos congelados.
