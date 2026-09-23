# Integrante 2 — Scheduler y Política de Planificación

## Objetivo

Implementar toda la lógica encargada de decidir **qué proceso READY debe utilizar el CPU**.

Este módulo debe ser independiente de:

- GUI;
- threads;
- temporizadores gráficos;
- implementación concreta del motor.

---

## Política oficial

Se utilizará:

**Priority Scheduling + Round Robin/FIFO en empates + Aging**

Reglas:

- prioridades de `1` a `10`;
- `10` es máxima prioridad;
- se consideran únicamente procesos en estado `READY`;
- gana el proceso con mayor `prioridadActual`;
- entre procesos con igual prioridad, gana el que lleva más tiempo esperando / llegó antes a esa prioridad;
- aging cada `2000 ms` simulados;
- aging incrementa `+1`;
- prioridad máxima `10`;
- al obtener CPU, `prioridadActual` vuelve a `prioridadBase`.

---

## Responsabilidades

### 1. Seleccionar siguiente proceso

Ejemplo:

```cpp
class Scheduler {
public:
    int seleccionarSiguiente(
        const std::vector<Proceso>& procesos
    ) const;
};
```

Debe devolver el PID seleccionado o un valor que represente ausencia de proceso.

---

## 2. Resolver empates

Ejemplo:

```text
A P8 llegada=1
B P8 llegada=2

seleccionado → A
```

Cuando A consume quantum y vuelve a READY:

```text
B P8
A P8

seleccionado → B
```

Esto produce comportamiento Round Robin entre procesos de igual prioridad.

---

## 3. Aging

Implementar:

```cpp
void aplicarAging(
    std::vector<Proceso>& procesos,
    int deltaTiempo
);
```

Ejemplo:

```text
B prioridadBase = 4
B prioridadActual = 4
B espera = 2000 ms

→ prioridadActual = 5
```

Nunca debe superar `10`.

---

## 4. Fin de quantum

El scheduler puede exponer una operación auxiliar para restablecer la prioridad de un proceso cuando obtiene CPU o al finalizar el quantum.

Ejemplo:

```cpp
void alAsignarCPU(Proceso& proceso);
```

Regla:

```text
prioridadActual = prioridadBase
tiempoEspera = 0
```

---

## 5. Configuración

Centralizar valores:

```cpp
struct SchedulerConfig {
    int prioridadMinima = 1;
    int prioridadMaxima = 10;

    int agingIntervalMs = 2000;
    int agingIncremento = 1;
};
```

No dispersar números mágicos por el código.

---

## Entregables

```text
scheduler/
    Scheduler.h
    Scheduler.cpp
    SchedulerConfig.h
```

Tests:

```text
tests/
    test_scheduler_prioridad.cpp
    test_scheduler_empate.cpp
    test_scheduler_aging.cpp
```

---

## Casos de prueba obligatorios

### Mayor prioridad

```text
A P8
B P5
C P3

resultado → A
```

### Filtrar por estado

```text
A P10 WAIT
B P5 READY

resultado → B
```

### Empate

```text
A P8 llegada=1
B P8 llegada=2

resultado → A
```

### Aging

```text
A P8
B P3 esperando durante suficiente tiempo

B debe aumentar:
3 → 4 → 5 → ...
```

### Límite

```text
B P10
aging

resultado → P10
```

No puede llegar a P11.

### Ningún READY

```text
A WAIT
B FINISH
C CANCEL

resultado → ninguno
```

---

## No es responsabilidad de este integrante

No implementar:

- GUI;
- timers gráficos;
- `std::thread`;
- `std::mutex`;
- `condition_variable`;
- memoria;
- creación/eliminación física de procesos;
- timeline gráfico;
- animaciones.

---

## Interfaces esperadas

El motor debe poder utilizar el scheduler sin conocer su implementación.

Ejemplo:

```cpp
int pid = scheduler.seleccionarSiguiente(procesos);
```

El scheduler nunca debe acceder directamente a Qt.

---

## Dependencias

Depende únicamente del contrato de `Proceso`.

Puede desarrollarse completamente con vectores de procesos creados manualmente en tests.

No necesita esperar al motor ni a la GUI.

---

## Criterios de aceptación

El módulo se considera terminado cuando:

- selecciona correctamente la mayor prioridad;
- ignora procesos no READY;
- resuelve empates de manera determinista;
- aging funciona;
- se respetan límites 1-10;
- no depende de Qt;
- no depende de threads;
- los tests son reproducibles.
