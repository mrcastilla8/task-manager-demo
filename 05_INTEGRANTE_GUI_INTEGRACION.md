<<<<<<< HEAD
# Integrante 5 — GUI, Visualización e Integración

## Objetivo

Construir la interfaz gráfica completa y conectar los dos motores de ejecución.

Framework recomendado:

**Qt 6 + C++**

La GUI debe funcionar inicialmente con datos mock para no depender del avance de otros integrantes.

---

## Principio fundamental

La GUI **no decide cómo funciona el scheduler**.

Solo:

1. envía acciones del usuario;
2. solicita `EstadoSistema`;
3. representa visualmente ese estado.

---

## Responsabilidades

### 1. Pantalla principal

Debe contener:

```text
Controles
Tabla de procesos
Cola de prioridad
CPU
Memoria
Timeline
Log de eventos
```

---

## 2. Controles

Implementar:

- modo:
  - Simulación lógica
  - Threads reales
- quantum
- velocidad visual
- Agregar proceso
- Iniciar
- Pausar
- Paso siguiente
- Reiniciar

Velocidades sugeridas:

```text
0.25x
0.5x
1x
2x
5x
```

---

## 3. Tabla tipo Task Manager

Columnas:

```text
PID
Nombre
Tamaño
Prioridad base
Prioridad actual
Estado
Tiempo restante
Tiempo de espera
Acción
```

Ejemplo:

```text
01 Chrome   220MB  8  8  RUN    3200ms   0ms
02 VSCode   350MB  5  7  READY  4100ms   4200ms
03 Spotify  150MB  3  5  READY  2800ms   4100ms
```

---

## 4. Cola visual

Mostrar procesos READY en el orden decidido por el scheduler.

Ejemplo:

```text
[B P8] → [A P6] → [C P4]
```

Cuando cambie la prioridad o el orden:

- animar el desplazamiento;
- evitar cambios instantáneos difíciles de seguir;
- destacar brevemente el proceso afectado.

La GUI debe actualizar la cola solamente cuando ocurre un evento significativo.

---

## 5. CPU

Mostrar:

- PID;
- nombre;
- estado;
- prioridad;
- quantum actual;
- progreso del quantum;
- tiempo restante.

Ejemplo:

```text
CPU

Chrome
RUN
Prioridad: 8

██████████░░░
340 / 500 ms

Restante: 3200 ms
```

---

## 6. Memoria

Mostrar:

```text
Memoria total
Memoria utilizada
Memoria disponible
Procesos cargados
```

No implementar algoritmos complejos de memoria.

---

## 7. Timeline

Debe conservar el historial.

Ejemplo:

```text
| A | A | B | C | A | B |
0  0.5 1  1.5 2  2.5 3
```

Debe permitir visualizar claramente el time-sharing.

---

## 8. Log de eventos

Ejemplo:

```text
00:02.000  B prioridad 6 → 7
00:02.500  A READY → RUN
00:03.000  A agotó quantum
00:03.000  A RUN → READY
```

Mantener historial, no únicamente el último evento.

---

## 9. Formulario Agregar proceso

Campos mínimos:

```text
Nombre
Tamaño
Tiempo total CPU
Prioridad
```

Opcionalmente:

```text
Simular E/S
Instante de bloqueo
Duración WAIT
```

Validar:

- prioridad 1-10;
- tiempos positivos;
- tamaño positivo;
- nombre no vacío.

---

## 10. Cancelar proceso

Permitir cancelar desde la tabla.

Confirmación opcional:

```text
¿Cancelar proceso VSCode?
```

Después:

```text
estado → CANCEL
```

---

## 11. Velocidad visual

La velocidad de representación no debe estar acoplada al quantum.

Ejemplo:

```text
Quantum lógico:
500 ms

Duración visual:
800 ms
```

La GUI puede usar:

```cpp
QTimer
```

para controlar las actualizaciones visuales.

---

## 12. Paso a paso

Cuando la aplicación está pausada:

```text
[ Paso siguiente ]
```

debe ejecutar exactamente una transición/evento importante.

Esto es esencial para demostraciones.

---

## 13. Integración de motores

La GUI trabajará únicamente contra:

```cpp
IExecutionEngine* engine;
```

Ejemplo:

```cpp
engine = new SimulationEngine();
```

o:

```cpp
engine = new ConcurrentEngine();
```

Al cambiar de modo, la GUI cambia la implementación, no su propia lógica visual.

---

## 14. Mocks

Mientras los motores no estén listos, crear:

```cpp
class MockExecutionEngine : public IExecutionEngine
```

que devuelva estados predeterminados.

Ejemplo:

```text
Paso 1:
A READY
B READY

Paso 2:
A RUN
B READY

Paso 3:
A READY
B RUN
```

Así la GUI puede desarrollarse completamente en paralelo.

---

## 15. Thread safety de Qt

La GUI debe actualizarse únicamente desde el hilo principal de Qt.

El motor concurrente puede producir estados/eventos, pero no tocar widgets.

Usar signals/slots o un mecanismo equivalente para transferir información de forma segura.

---

## Entregables

```text
gui/
    MainWindow.h
    MainWindow.cpp

    ProcessTable.h
    ProcessTable.cpp

    QueueWidget.h
    QueueWidget.cpp

    CPUWidget.h
    CPUWidget.cpp

    MemoryWidget.h
    MemoryWidget.cpp

    TimelineWidget.h
    TimelineWidget.cpp

    EventLogWidget.h
    EventLogWidget.cpp

    NewProcessDialog.h
    NewProcessDialog.cpp
```

Más integración en:

```text
main.cpp
CMakeLists.txt
```

---

## No es responsabilidad de este integrante

No implementar:

- algoritmo de prioridad;
- aging;
- Round Robin;
- estructura interna de los threads;
- mutex internos del motor;
- cálculo de selección del proceso.

---

## Dependencias

Puede trabajar desde el primer día con:

- `IExecutionEngine`;
- `EstadoSistema`;
- `MockExecutionEngine`.

La integración real se hace después.

---

## Criterios de aceptación

La GUI está terminada cuando:

- muestra procesos;
- muestra estados;
- muestra cola dinámica;
- muestra CPU;
- muestra memoria;
- muestra timeline;
- muestra log;
- permite agregar procesos;
- permite cancelar;
- permite pausa;
- permite paso a paso;
- permite modificar velocidad;
- permite modificar quantum;
- permite alternar entre los dos motores;
- no contiene lógica propia de scheduling.
=======
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
>>>>>>> 952c1f39b1aca418ef5616b3f230a9d780c89911
