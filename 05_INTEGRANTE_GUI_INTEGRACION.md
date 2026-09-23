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
