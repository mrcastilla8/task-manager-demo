# ConcurrentSimulator

ConcurrentSimulator es una propuesta C++17 para simular admisión y planificación de procesos, memoria y un CPU lógico con time-sharing.

## Estado de contratos y decisiones

Los encabezados [v1.0](contracts/README.md) permanecen congelados y vigentes para el código existente en `master`. La API [2.0](docs/CONTRATOS_V2.md) bajo `contracts/v2/` sigue **candidata a adopción**; esta rama la integra en código sin declararla ratificada ni estable. Las decisiones del equipo aprobadas para ese destino se registran en [DP-01–DP-11](docs/DECISIONES_APROBADAS.md); el [plan de migración](docs/MIGRACION_CONTRATOS_V2.md) no ejecuta esa migración en master.

DP-01–DP-11 fijan prioridad automática P1/P2/P3 por tamaño, TL formada por READY/RUN/WAIT, fórmula estricta exacta, cuotas fijas de 10/20/30 ms por despacho, orden READY prioridad-ticket-PID, cancelación y estados terminales, unidad del paso lógico, timeline/historial, lista simplemente enlazada futura, ausencia de aging activo y Qt 6/C++17/CMake con dos motores sobre una interfaz común. Son decisiones de equipo, no requisitos docentes adicionales.

La selección del driver continuo, política de errores, protocolo reproducible de cancelación RUN y secuencia/replay de eventos siguen como propuestas técnicas por ratificar en [docs/PROPUESTAS_TECNICAS_V2.md](docs/PROPUESTAS_TECNICAS_V2.md); los contratos candidatos no están congelados ni operativos.

## Integración v2

La rama `integracion/v2-completa` integra Core v2, Scheduler v2, motor lógico,
motor concurrente y frontend Qt 6. El frontend selecciona motores reales, admite
procesos y presenta snapshots; el avance disponible es por pasos. El inicio
continuo y la cancelación parcial durante RUN siguen deshabilitados mientras se
ratifican sus detalles técnicos. Los contratos v2 permanecen candidatos y no se
modificaron sus firmas.

La comparación determinista Aldana–Ángel se ejecuta en
`test_concurrent_v2_equivalencia`; la integración Core–Scheduler en
`test_core_scheduler_integracion`. La guía de construcción y uso está en
[`gui/README.md`](gui/README.md).

## Documentación

- [Especificación funcional](docs/ESPECIFICACION_FUNCIONAL.md)
- [Requisitos y fuentes docentes](docs/REQUISITOS_DOCENTE.md)
- [Decisiones pendientes: historial y cierre](docs/DECISIONES_PENDIENTES.md)
- [Decisiones aprobadas](docs/DECISIONES_APROBADAS.md)
- [Contratos v1.0 y v2.0 candidata](contracts/README.md), [detalle v2](docs/CONTRATOS_V2.md)
- [Casos de referencia](docs/CASOS_REFERENCIA.md) y [matriz de trazabilidad](docs/MATRIZ_TRAZABILIDAD.md)
- [Workflow](docs/WORKFLOW_DESARROLLO.md)
- Asignaciones [Castilla](01_INTEGRANTE_MODELO_ESTRUCTURAS.md), [Brickzon](02_INTEGRANTE_SCHEDULER.md), [Aldana](03_INTEGRANTE_SIMULACION_LOGICA.md), [Ángel](04_INTEGRANTE_MOTOR_CONCURRENTE.md), [Poma](05_INTEGRANTE_GUI_INTEGRACION.md)
- [Guía de agentes](AGENTS.md)

Las notas fuente del curso se mantienen fuera del repositorio; sus localizadores figuran en [REQUISITOS_DOCENTE.md](docs/REQUISITOS_DOCENTE.md).

## Build base sin Qt

La rama base requiere compilador C++17, CMake 3.20 o posterior y CTest:

```powershell
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

El build base incluye v1 y los módulos v2 integrados, sin requerir Qt.
