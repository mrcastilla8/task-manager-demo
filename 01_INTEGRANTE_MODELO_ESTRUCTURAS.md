# Integrante 1 — Modelo, Estructuras de Datos y Memoria

## Objetivo

Construir las estructuras fundamentales que representan los procesos y los recursos simulados del sistema.

Este integrante **no implementa el algoritmo de planificación**, **no crea threads** y **no desarrolla la GUI**.

---

## Responsabilidades

### 1. Modelo `Proceso`

Implementar la representación de un proceso con, como mínimo:

```cpp
struct Proceso {
    int pid;
    std::string nombre;
    int tamanioBytes;

    int prioridadBase;
    int prioridadActual;

    int tiempoTotal;
    int tiempoRestante;
    int tiempoEspera;

    int quantumConsumido;

    Estado estado;

    long ordenLlegada;
};
```

Puede ampliarse si otros módulos necesitan información adicional, pero no debe introducir lógica propia del scheduler.

---

## 2. Estados

Definir:

```cpp
enum class Estado {
    READY,
    RUN,
    WAIT,
    FINISH,
    CANCEL
};
```

Debe existir una función auxiliar para convertir el estado a texto.

Ejemplo:

```cpp
std::string estadoToString(Estado estado);
```

---

## 3. Estructura dinámica de procesos

Implementar una estructura encargada de almacenar procesos.

Funciones mínimas:

```cpp
class ColaProcesos {
public:
    void agregar(const Proceso& proceso);
    bool eliminar(int pid);

    Proceso* buscar(int pid);
    const Proceso* buscar(int pid) const;

    std::vector<Proceso> obtenerTodos() const;
    std::vector<Proceso> obtenerPorEstado(Estado estado) const;

    bool vacia() const;
    size_t cantidad() const;
};
```

La estructura no decide qué proceso tiene mayor prioridad. Esa responsabilidad corresponde al Scheduler.

---

## 4. Memoria simulada

Implementar un modelo simple de memoria.

Ejemplo:

```cpp
class Memoria {
private:
    int capacidadTotal;
    int memoriaUtilizada;

public:
    bool puedeCargar(const Proceso& proceso) const;
    bool cargar(const Proceso& proceso);
    void liberar(int pid);

    int obtenerTotal() const;
    int obtenerUtilizada() const;
    int obtenerDisponible() const;
};
```

No implementar:

- paginación;
- segmentación;
- memoria virtual;
- reemplazo de páginas.

El objetivo es únicamente representar visualmente cuánto espacio ocupan los procesos.

---

## 5. Eventos y estructuras comunes

Colaborar en la definición de:

```text
contracts/
    Proceso.h
    Estado.h
    Evento.h
    EstadoSistema.h
```

Ejemplo de evento:

```cpp
enum class TipoEvento {
    PROCESS_CREATED,
    PROCESS_READY,
    PROCESS_RUNNING,
    QUANTUM_FINISHED,
    PROCESS_WAITING,
    PROCESS_FINISHED,
    PROCESS_CANCELLED,
    PRIORITY_CHANGED
};
```

---

## Entregables

```text
contracts/
    Proceso.h
    Estado.h
    Evento.h
    EstadoSistema.h

core/
    ColaProcesos.h
    ColaProcesos.cpp
    Memoria.h
    Memoria.cpp
```

Además:

```text
tests/
    test_proceso.cpp
    test_cola.cpp
    test_memoria.cpp
```

---

## Pruebas mínimas

### Agregar procesos

```text
Agregar A
Agregar B
Agregar C

cantidad = 3
```

### Buscar proceso

```text
buscar(pid=2)
→ B
```

### Eliminar proceso

```text
A → B → C

eliminar(B)

A → C
```

### Memoria

```text
Memoria total = 2048 MB

A = 500 MB
B = 1000 MB

utilizada = 1500 MB
disponible = 548 MB
```

Debe rechazarse un proceso que exceda la capacidad disponible.

---

## No es responsabilidad de este integrante

No implementar:

- cálculo de prioridades;
- aging;
- Round Robin;
- selección del siguiente proceso;
- ejecución del quantum;
- threads;
- mutex;
- condition variables;
- animaciones;
- widgets;
- timeline gráfico.

---

## Dependencias

Puede trabajar desde el inicio.

Los otros integrantes dependen del contrato de `Proceso`, `Estado` y `EstadoSistema`, por lo que estos archivos deben estabilizarse temprano.

Una vez acordados los campos comunes, evitar cambios incompatibles.

---

## Criterios de aceptación

El módulo se considera terminado cuando:

- pueden crearse procesos válidos;
- pueden buscarse y eliminarse por PID;
- pueden filtrarse por estado;
- la estructura admite altas y bajas dinámicas;
- la memoria simulada carga y libera procesos;
- los tests pasan;
- el módulo no contiene lógica gráfica, de scheduling ni concurrencia.
