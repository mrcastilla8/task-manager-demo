# Integrante 4 — Motor Concurrente con Threads Reales

## Objetivo

Implementar una segunda versión del motor utilizando concurrencia real en C++:

- `std::thread`
- `std::mutex`
- `std::condition_variable`
- sincronización segura

Debe implementar el mismo contrato `IExecutionEngine` que el motor lógico.

---

## Principio fundamental

Los threads reales **no reemplazan la política de planificación**.

El Scheduler continúa decidiendo qué proceso simulado obtiene el CPU.

El sistema operativo seguirá administrando físicamente los threads, pero el proyecto controla de forma lógica qué thread puede avanzar.

---

## Arquitectura esperada

```text
                 Scheduler
                     │
             selecciona PID
                     │
                     ▼
          ConcurrentEngine
                     │
         concede permiso lógico
                     │
       ┌─────────────┼─────────────┐
       ▼             ▼             ▼
   Thread A       Thread B      Thread C
     RUN            WAIT          WAIT
```

---

## Responsabilidades

### 1. `ConcurrentEngine`

Ejemplo:

```cpp
class ConcurrentEngine : public IExecutionEngine {
public:
    void agregarProceso(const Proceso& proceso) override;
    void cancelarProceso(int pid) override;

    void iniciar() override;
    void pausar() override;
    void reiniciar() override;
    void siguientePaso() override;

    EstadoSistema obtenerEstado() const override;
};
```

---

## 2. Thread por proceso

Cada proceso puede asociarse con una estructura:

```cpp
class ProcessThread {
private:
    std::thread worker;
    std::mutex mutex;
    std::condition_variable cv;

    bool permisoEjecucion;
    bool terminado;
};
```

Conceptualmente:

```cpp
while (!terminado) {
    esperarPermisoDelScheduler();
    ejecutarQuantumLogico();
    notificarFinDeQuantum();
}
```

---

## 3. Sin busy waiting

Está prohibido implementar:

```cpp
while (!miTurno) {
}
```

Debe utilizarse:

```cpp
std::condition_variable
```

para suspender los threads que no tienen permiso.

---

## 4. Exclusión mutua

Proteger los datos compartidos:

- lista/cola de procesos;
- estados;
- prioridad;
- tiempo restante;
- historial;
- eventos;
- proceso ejecutándose.

Utilizar:

```cpp
std::mutex
std::lock_guard
std::unique_lock
```

según corresponda.

---

## 5. Scheduler

No duplicar la lógica del integrante 2.

Debe usar:

```cpp
scheduler.seleccionarSiguiente(...)
```

para determinar qué proceso obtiene permiso.

---

## 6. GUI

Los threads **nunca actualizan widgets directamente**.

El flujo esperado es:

```text
threads
   ↓
estado/eventos protegidos
   ↓
ConcurrentEngine
   ↓
EstadoSistema
   ↓
GUI
```

Si Qt requiere signals/slots, la GUI se encarga de recibir la información en su propio hilo.

---

## 7. Pausa y paso a paso

Debe ser posible:

```text
Pausar
Paso siguiente
Continuar
```

Aunque existan threads reales.

Al pausar, los workers deben quedar esperando mediante `condition_variable`.

---

## 8. Terminación segura

Al cerrar/reiniciar:

- notificar a todos los threads;
- marcar terminación;
- ejecutar `join()` sobre threads joinable;
- liberar recursos;
- evitar threads huérfanos.

Ejemplo:

```cpp
if (worker.joinable()) {
    worker.join();
}
```

---

## 9. Race conditions

El integrante debe identificar y documentar explícitamente al menos:

- qué datos son compartidos;
- qué mutex protege cada dato;
- qué condiciones despiertan threads;
- cómo se evita que dos procesos ejecuten el CPU lógico simultáneamente.

Esto formará parte de la explicación del curso.

---

## Entregables

```text
concurrent/
    ConcurrentEngine.h
    ConcurrentEngine.cpp
    ProcessThread.h
    ProcessThread.cpp
    Synchronization.h
```

Tests:

```text
tests/
    test_concurrent_basic.cpp
    test_concurrent_pause.cpp
    test_concurrent_cancel.cpp
    test_concurrent_thread_shutdown.cpp
    test_concurrent_race_safety.cpp
```

---

## No es responsabilidad de este integrante

No implementar:

- GUI;
- animaciones;
- widgets;
- política de prioridad;
- cálculo de aging;
- componentes visuales;
- un scheduler distinto.

---

## Dependencias

Depende de:

- contratos;
- modelo;
- Scheduler.

Puede desarrollar primero con procesos sintéticos.

La GUI no debe ser necesaria para probar este módulo.

---

## Criterios de aceptación

El módulo se considera terminado cuando:

- cada proceso concurrente utiliza un thread real;
- no hay busy waiting;
- existe sincronización con mutex/condition_variable;
- un único proceso obtiene el CPU lógico a la vez;
- pausa y reanudación funcionan;
- cancelar procesos funciona;
- todos los threads terminan correctamente;
- produce el mismo formato `EstadoSistema` que `SimulationEngine`;
- no actualiza Qt directamente;
- no duplica la política del Scheduler.
