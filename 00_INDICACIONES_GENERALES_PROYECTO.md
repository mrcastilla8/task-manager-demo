# Simulador de Concurrencia con Cola de Prioridad

## 1. Objetivo general

Desarrollar en **C++** una aplicación con **interfaz gráfica obligatoria** que simule la gestión y planificación de procesos mediante una **cola/lista dinámica con prioridades**, **cuota de tiempo (quantum)** y **time-sharing**.

El proyecto tendrá dos modos de ejecución:

1. **Modo simulación lógica:** sin hilos reales.
2. **Modo concurrente:** con `std::thread`, `std::mutex` y `std::condition_variable`.

Ambos modos deben usar la misma política de planificación y mostrar el mismo tipo de información en la interfaz.

---

## 2. Base conceptual del proyecto

Cada proceso debe manejar, como mínimo:

- `id`
- `nombre`
- `tamaño`
- `estado`
- `cuota de tiempo`
- `prioridad`
- `tiempo total`
- `tiempo restante`
- `tiempo de espera`

Estados mínimos:

- `READY`
- `RUN`
- `WAIT`
- `FINISH`
- `CANCEL`

La estructura de procesos debe ser dinámica: pueden ingresar procesos nuevos mientras la simulación está ejecutándose y otros pueden finalizar o cancelarse.

---

## 3. Política de planificación acordada

Se utilizará:

**Priority Scheduling + Round Robin entre empates + Aging**

Reglas:

- Prioridades entre `1` y `10`.
- `10` representa la prioridad más alta.
- Quantum por defecto: `500 ms` simulados.
- El quantum debe poder configurarse desde la GUI.
- Siempre se selecciona el proceso `READY` con mayor `prioridadActual`.
- Si dos o más procesos tienen la misma prioridad, se aplica FIFO/Round Robin.
- Un proceso que consume su quantum y no termina pasa de `RUN` a `READY`.
- Un proceso que termina pasa de `RUN` a `FINISH`.
- Un proceso que realiza E/S pasa de `RUN` a `WAIT`, y luego de `WAIT` a `READY`.
- Un proceso puede pasar a `CANCEL` por acción del usuario.
- Aging:
  - Cada `2000 ms` simulados esperando en `READY`, la prioridad actual aumenta en `+1`.
  - La prioridad máxima es `10`.
  - Al recibir CPU, la prioridad actual vuelve a su prioridad base.

Estos valores deben quedar centralizados en configuración para poder modificarlos sin cambiar la lógica principal.

---

## 4. Comportamiento del quantum

Para cada proceso seleccionado:

```text
READY
  ↓
RUN
  ↓
Ejecuta min(quantum, tiempoRestante)
  ↓
¿terminó?
 ├─ Sí → FINISH
 └─ No → READY
```

El simulador no debe actualizar la GUI cada milisegundo. La interfaz se actualiza únicamente cuando ocurre un evento significativo.

Eventos mínimos:

- proceso creado
- proceso admitido
- `READY → RUN`
- quantum finalizado
- `RUN → READY`
- `RUN → WAIT`
- `WAIT → READY`
- `RUN → FINISH`
- aging aplicado
- proceso cancelado

---

## 5. Velocidad lógica y velocidad visual

La velocidad del algoritmo y la velocidad con la que se muestra en pantalla son conceptos diferentes.

Ejemplo:

- Quantum lógico: `500 ms`
- Duración visual del paso: `800 ms reales`

La interfaz debe permitir:

- Pausar
- Continuar
- Reiniciar
- Ejecutar paso a paso
- Cambiar velocidad visual
- Cambiar quantum

Velocidades visuales sugeridas:

- `0.25x`
- `0.5x`
- `1x`
- `2x`
- `5x`

---

## 6. Interfaz gráfica

La interfaz debe contener, como mínimo:

### 6.1 Controles

- Selector de modo:
  - Simulación lógica
  - Threads reales
- Quantum
- Velocidad
- Agregar proceso
- Iniciar
- Pausar
- Paso siguiente
- Reiniciar

### 6.2 Tabla de procesos

Columnas sugeridas:

- PID
- Nombre
- Tamaño
- Prioridad base
- Prioridad actual
- Estado
- Tiempo restante
- Tiempo de espera
- Acción de cancelar

### 6.3 Cola de prioridad

Representación gráfica ordenada de los procesos `READY`.

Ejemplo:

```text
[B P8] → [A P6] → [C P4]
```

La cola debe actualizarse visualmente cuando cambie el orden.

### 6.4 Procesador

Debe mostrar:

- proceso actualmente ejecutándose
- estado `RUN`
- prioridad
- quantum consumido
- tiempo restante

### 6.5 Memoria simulada

Mostrar:

- capacidad total
- memoria utilizada
- procesos cargados

No se requiere implementar administración avanzada de memoria.

### 6.6 Timeline / Time-Sharing

Debe conservar el historial de ejecución.

Ejemplo:

```text
| A | A | B | C | A | B | C |
0  0.5 1  1.5 2  2.5 3  3.5
```

### 6.7 Log de eventos

Ejemplo:

```text
00:02.000  B prioridad 6 → 7 por aging
00:02.500  A READY → RUN
00:03.000  A agotó quantum
00:03.000  A RUN → READY
```

---

## 7. Arquitectura común

Los cinco integrantes deben trabajar contra contratos comunes.

```text
contracts/
    Proceso.h
    Estado.h
    Evento.h
    EstadoSistema.h
    IExecutionEngine.h
```

### Contrato principal

```cpp
class IExecutionEngine {
public:
    virtual ~IExecutionEngine() = default;

    virtual void agregarProceso(const Proceso& proceso) = 0;
    virtual void cancelarProceso(int pid) = 0;

    virtual void iniciar() = 0;
    virtual void pausar() = 0;
    virtual void reiniciar() = 0;
    virtual void siguientePaso() = 0;

    virtual EstadoSistema obtenerEstado() const = 0;
};
```

### Estado compartido

```cpp
struct EstadoSistema {
    long tiempoSimulado;

    std::vector<Proceso> procesos;
    std::vector<int> colaPrioridad;

    int pidEjecutando;

    int memoriaUsada;
    int memoriaTotal;

    std::vector<Evento> eventos;
    std::vector<BloqueTimeline> timeline;
};
```

La GUI solo debe depender de `IExecutionEngine` y `EstadoSistema`.

---

## 8. División del equipo

| Integrante | Módulo |
|---|---|
| 1 | Modelo, estructuras de datos y memoria simulada |
| 2 | Scheduler y política de planificación |
| 3 | Motor de simulación lógica sin threads |
| 4 | Motor concurrente con threads |
| 5 | GUI, visualización e integración |

Cada integrante tiene un archivo independiente con sus responsabilidades y criterios de aceptación.

---

## 9. Estructura sugerida del repositorio

```text
ConcurrentSimulator/
│
├── contracts/
│   ├── Proceso.h
│   ├── Estado.h
│   ├── Evento.h
│   ├── EstadoSistema.h
│   └── IExecutionEngine.h
│
├── core/
│   ├── ColaProcesos.h
│   ├── ColaProcesos.cpp
│   ├── Memoria.h
│   └── Memoria.cpp
│
├── scheduler/
│   ├── Scheduler.h
│   └── Scheduler.cpp
│
├── simulation/
│   ├── SimulationEngine.h
│   └── SimulationEngine.cpp
│
├── concurrent/
│   ├── ConcurrentEngine.h
│   ├── ConcurrentEngine.cpp
│   ├── ProcessThread.h
│   └── ProcessThread.cpp
│
├── gui/
│   ├── MainWindow.h
│   ├── MainWindow.cpp
│   ├── ProcessTable.*
│   ├── QueueWidget.*
│   ├── CPUWidget.*
│   ├── MemoryWidget.*
│   └── TimelineWidget.*
│
├── tests/
├── CMakeLists.txt
└── README.md
```

---

## 10. Regla de integración

Ningún módulo debe modificar directamente la lógica interna de otro.

- La GUI no decide prioridades.
- El scheduler no dibuja componentes.
- El modelo no ejecuta threads.
- El motor sin threads no contiene código gráfico.
- El motor concurrente no modifica widgets directamente.
- Los threads nunca actualizan la GUI directamente.
- La comunicación se hace mediante contratos, estados y eventos.

---

## 11. Orden recomendado de trabajo

1. Acordar y congelar los archivos de `contracts/`.
2. Cada integrante implementa su módulo.
3. Integrante 5 construye la GUI inicialmente con datos simulados/mock.
4. Integrar primero `SimulationEngine`.
5. Validar comportamiento.
6. Integrar `ConcurrentEngine`.
7. Comparar ambos modos con los mismos escenarios.
8. Ejecutar pruebas de integración.
9. Preparar escenarios de demostración.

---

## 12. Escenarios mínimos para demostrar

### Escenario A — Prioridad

```text
A: prioridad 8
B: prioridad 5
C: prioridad 3
```

Debe ejecutar primero `A`.

### Escenario B — Empate

```text
A: prioridad 8
B: prioridad 8
```

Se resuelve por orden FIFO/Round Robin.

### Escenario C — Aging

Un proceso de baja prioridad debe aumentar progresivamente su prioridad mientras espera.

### Escenario D — WAIT

Un proceso entra a `WAIT` por una operación de E/S y posteriormente regresa a `READY`.

### Escenario E — Proceso dinámico

Se agrega un nuevo proceso mientras el simulador ya está ejecutándose.

### Escenario F — Cancelación

El usuario cancela un proceso desde la GUI.

### Escenario G — Comparación de motores

El mismo conjunto de procesos se ejecuta en:

- modo simulación lógica
- modo threads reales

---

## 13. Criterio general de finalización

El proyecto se considera terminado cuando:

- compila correctamente;
- los dos motores pueden seleccionarse desde la misma GUI;
- la cola cambia visualmente;
- el CPU muestra el proceso en ejecución;
- el timeline conserva el historial;
- la simulación puede pausarse y ejecutarse paso a paso;
- se pueden agregar y cancelar procesos;
- las prioridades y el aging funcionan;
- los estados cambian correctamente;
- el modo con threads utiliza sincronización segura;
- ambos modos siguen la misma política de planificación.
