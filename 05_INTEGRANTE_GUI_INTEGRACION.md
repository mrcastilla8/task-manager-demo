# Integrante 5 — GUI, visualización e integración

**Responsable:** Poma · **Rama:** `poma` · **Estado:** implementación futura, no presente en ramas auditadas. La plataforma aprobada para el destino v2 es Qt 6 + C++17 + CMake.

## Objetivo y límites

Construir la GUI e integrar el motor lógico de Aldana y el concurrente de Ángel mediante la misma interfaz v2. La GUI envía acciones y presenta snapshots/eventos; no ordena procesos, asigna prioridad, calcula cuota, aplica aging ni incluye planificación en widgets. La regla de v2 es prioridad automática P1–P3, cuota máxima 10/20/30 ms, sin quantum global configurable y sin aging activo. La asignación antigua de controles v1 no es instrucción v2.

El destino de contrato sigue siendo **candidato a adopción, no operativo**. Revisar [contratos v2](docs/CONTRATOS_V2.md), [decisiones](docs/DECISIONES_APROBADAS.md) y [migración](docs/MIGRACION_CONTRATOS_V2.md). La semántica de códigos de resultado y cancelación idempotente está propuesta, pendiente de ratificación; no convertirla en requisito de DP-06.

El modo continuo aún necesita ratificación de un driver no bloqueante para el motor lógico. La propuesta preferida usa timer cooperativo del loop Qt; revisar alternativas e impactos antes de cambiar la interfaz o los mocks en [propuestas técnicas](docs/PROPUESTAS_TECNICAS_V2.md).

## Dependencias y trabajo en paralelo

Poma puede empezar con mock/fixtures cuando se congele la interfaz, mientras Aldana y Ángel trabajan en paralelo tras contrato y Scheduler estables. Integrar primero motor lógico y después concurrente; ambos deben producir resultados observables equivalentes. Los threads concurrentes siguen representando un único CPU lógico. El refresco Qt no incrementa reloj ni cuenta como tiempo de CPU.

## Componentes y entregables

- `gui/MainWindow.h/.cpp`: ventana, controles y selección de motor detrás de interfaz común.
- `gui/ProcessTable.h/.cpp`: procesos, prioridad P1–P3, estado/tiempos y cancelar.
- `gui/QueueWidget.h/.cpp`: orden READY publicado por Scheduler.
- `gui/CPUWidget.h/.cpp`: PID en RUN, prioridad, tiempo restante, cuota de turno fijada/progreso.
- `gui/MemoryWidget.h/.cpp`: capacidad y mapa proporcional por PID y espacio libre; no direcciones ficticias.
- `gui/TimelineWidget.h/.cpp`: intervalos reales de CPU, escala, zoom y desplazamiento.
- `gui/EventLogWidget.h/.cpp`: eventos en milisegundos simulados e historial de sesión.
- `gui/NewProcessDialog.h/.cpp`: alta sin prioridad manual; nombre, tamaño, CPU y E/S opcional.
- `main.cpp`: ensamblaje de aplicación.

Son entregables previstos; su mención no declara archivos implementados. Conservar las pruebas de integración existentes y verificar los escenarios listados abajo; no se impone aquí un nombre de archivo de test GUI que la asignación fuente no fijó.

## Reglas de presentación

### Controles

Selector de motor lógico/concurrente, agregar, iniciar, pausar, paso siguiente y reiniciar. No exponer quantum global. La velocidad/animación visual, si se ofrece, solo cambia presentación; no equivale al tiempo lógico ni modifica el reloj.

### Proceso y cola

Alta no muestra campo de prioridad manual. La tabla usa la prioridad publicada por el motor y muestra la cuota vigente derivada como prioridad×10 si se desea; no guarda una segunda cuota editable. Durante RUN se muestra la cuota del turno fijada aunque el snapshot publique una clasificación vigente nueva para el próximo turno. La cola incluye solo READY y refleja exactamente orden prioridad descendente, ticket ascendente, PID ascendente.

### CPU, memoria y timeline

CPU muestra un solo PID lógico en ejecución o vacío. RAM muestra ocupación proporcional por PID y espacio libre. Cada bloque del timeline representa CPU ejecutado con PID positivo y rango semiabierto `[inicio,fin)`. Idle no tiene bloque ni PID contractual: la vista infiere huecos entre origen de sesión 0, intervalos CPU y reloj simulado del snapshot; eventos idle pueden dar contexto sin fabricar un segmento.

Ejemplo: A ejecuta `[0,8)`, entra WAIT y el CPU queda sin ejecución hasta despertar en 18; B ejecuta `[18,22)`. Si el snapshot marca tiempo 27, almacenar solo A `[0,8)` y B `[18,22)`; mostrar como huecos idle `[8,18)` y `[22,27)`. No crear bloques con PID 0/-1. Pausar la aplicación sin avanzar el reloj no produce idle simulado.

Retener la historia completa de sesión, permitir scroll/zoom y representar eventos/snapshots para RUN transitorio. Usar representación eficiente si se necesita, sin descartar el registro fuente. El replay no escribe al motor ni avanza reloj.

### Eventos

Presentar alta/admisión, prioridades publicadas, despacho, ejecución CPU, cuota agotada, WAIT/retorno, fin, cancelación, idle y finalización del sistema, según contrato. Formatear timestamps simulados sin sustituirlos por tiempo de pared. El orden y snapshotting exactos se etiquetan propuesta hasta ratificación.

## Mock y escenarios de integración

El mock implementa `IExecutionEngine` v2 cuando se ratifique la API y permite preparar vistas de:

1. READY con prioridades distintas y empate por ticket.
2. RUN transitorio con cuota fijada, ejecución parcial y CPU restante.
3. Reingreso a READY con ticket nuevo.
4. WAIT con otro READY y retorno en un paso posterior.
5. CPU idle inferido antes, entre y después de bloques de proceso.
6. FINISH/CANCEL conservados en historial y memoria liberada en snapshot.
7. RAM con varios procesos y hueco libre.
8. Error/rechazo de alta sin mutar TL/prioridades.
9. Misma traza de eventos/timeline para ambos motores.

Usar señales/slots o consulta desde el hilo UI; workers no actualizan widgets. QTimer puede repintar snapshots, pero nunca define paso/cuota ni reloj. Pruebas de integración deben cubrir que timeline solo contiene PID positivo y que los gaps se reconstruyen desde reloj snapshot.

## Criterios de cierre GUI

- App Qt 6/C++17/CMake opera contra una interfaz común, mock y ambos motores.
- Sin campo de prioridad manual, control de quantum global, aging o regla de Scheduler en widgets.
- CPU lógico único; espera idle no se atribuye a proceso; RUN transitorio puede reproducirse.
- Timeline conserva información completa, renderiza huecos sin PID y ofrece scroll/zoom.
- La GUI no altera reloj simulado ni trata duración de animación como CPU.
- Los escenarios y pruebas de integración se ejecutan después de implementar; la documentación actual no afirma PASS de módulos ausentes.
