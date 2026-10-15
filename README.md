# ConcurrentSimulator

ConcurrentSimulator es un proyecto C++17 para simular gráficamente la admisión y planificación de procesos, la ocupación de memoria y el uso de CPU mediante time-sharing. El repositorio reúne trabajo de varias ramas; la documentación distingue la implementación actual de los requisitos académicos y de las decisiones aún pendientes.

## Lectura inicial

1. [Indicaciones generales](00_INDICACIONES_GENERALES_PROYECTO.md)
2. [Especificación funcional canónica](docs/ESPECIFICACION_FUNCIONAL.md)
3. [Requisitos del docente y trazabilidad de fuentes](docs/REQUISITOS_DOCENTE.md)
4. [Decisiones pendientes](docs/DECISIONES_PENDIENTES.md)
5. [Contratos versión 1.0](contracts/README.md)

La [matriz de trazabilidad](docs/MATRIZ_TRAZABILIDAD.md) y los [casos de referencia](docs/CASOS_REFERENCIA.md) concentran la relación entre reglas, documentación, código y criterios verificables. Cada integrante debe leer además su asignación: [modelo y memoria](01_INTEGRANTE_MODELO_ESTRUCTURAS.md), [scheduler](02_INTEGRANTE_SCHEDULER.md), [motor lógico](03_INTEGRANTE_SIMULACION_LOGICA.md), [motor concurrente](04_INTEGRANTE_MOTOR_CONCURRENTE.md) o [GUI e integración](05_INTEGRANTE_GUI_INTEGRACION.md). Las reglas para agentes están en [AGENTS.md](AGENTS.md).

## Etiquetas de evidencia

- **CONFIRMADO_POR_DOCENTE:** aparece explícitamente en las notas o la transcripción; los detalles que la fuente omite pueden seguir pendientes.
- **PROPUESTA_DOCENTE:** ejemplo o alternativa sin evidencia suficiente de obligatoriedad.
- **DECISION_EQUIPO:** comportamiento documentado por el equipo y no atribuido al docente.
- **PENDIENTE_CONFIRMACION:** contradicción, detalle insuficiente o precedencia aún sin resolver.

Los archivos académicos se mantienen fuera del repositorio. Sus páginas y marcas de tiempo se citan como localizadores; no son enlaces a material publicado en GitHub.

## Estado del repositorio

En `origin/master`, los contratos compartidos están congelados en versión 1.0, el almacenamiento usa `std::list<Proceso>` y la memoria registra capacidad y tamaños cargados. El Scheduler y el motor concurrente están en ramas de integrantes; el motor lógico y la GUI no aparecen en las seis ramas auditadas. Qt 6 es una recomendación de la asignación de GUI, no una dependencia aprobada.

La política actual del equipo documenta prioridades 1–10 (10 máxima), quantum global configurable de 500 ms y aging cada 2,000 ms simulados. No son requisitos del docente. `AssignPriority()` calcula prioridades 1–3 según el tamaño mínimo y máximo de la lista de tareas; su compatibilidad y recalculado están pendientes de aprobación.

## Construcción y pruebas

La rama base requiere compilador C++17, CMake 3.20 o posterior y CTest:

```powershell
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Estos comandos describen el flujo disponible; no afirman que se haya compilado o probado esta rama documental.
