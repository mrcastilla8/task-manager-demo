# Integrante 3 — Motor de Simulación Lógica

## Objetivo

Implementar el funcionamiento completo del simulador **sin utilizar threads reales**.

Los procesos son objetos y el motor avanza mediante pasos/eventos.

Debe implementar `IExecutionEngine`.

---

## Responsabilidades

### 1. Motor principal

Ejemplo:

```cpp
class SimulationEngine : public IExecutionEngine {
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

## 2. Reloj lógico

Mantener:

```text
tiempoSimulado
```

El simulador no depende directamente del tiempo real.

Ejemplo:

```text
t=0
t=500
t=1000
t=1500
```

---

## 3. Ciclo lógico

Cada paso debe ejecutar aproximadamente:

```text
1. Actualizar procesos WAIT
2. Actualizar tiempos de espera
3. Aplicar aging
4. Obtener procesos READY
5. Solicitar al Scheduler el siguiente PID
6. READY → RUN
7. Ejecutar quantum lógico
8. Actualizar tiempo restante
9. Resolver transición final
10. Registrar evento
11. Registrar timeline
12. Publicar EstadoSistema
```

---

## 4. Transiciones

Debe implementar:

```text
READY → RUN
RUN → READY
RUN → WAIT
WAIT → READY
RUN → FINISH
* → CANCEL
```

---

## 5. Quantum

Quantum por defecto:

```text
500 ms
```

Debe ser configurable.

Ejemplo:

```text
Proceso A
restante = 1300 ms
quantum = 500 ms

ejecuciones:
500
500
300
→ FINISH
```

---

## 6. WAIT / E/S simulada

Cada proceso puede tener opcionalmente un evento de E/S.

Ejemplo:

```text
A
tiempoTotal = 5000 ms

después de consumir 1500 ms CPU:
RUN → WAIT

duración WAIT:
1000 ms

luego:
WAIT → READY
```

El motor debe controlar este comportamiento sin threads.

---

## 7. Eventos

Registrar eventos para que la GUI pueda mostrarlos.

Ejemplo:

```text
00:00.000  Proceso A creado
00:00.000  A → READY
00:00.500  A READY → RUN
00:01.000  A agotó quantum
00:01.000  A RUN → READY
```

---

## 8. Timeline

Registrar cada intervalo ejecutado.

Ejemplo de estructura:

```cpp
struct BloqueTimeline {
    int pid;
    long inicio;
    long fin;
};
```

Ejemplo:

```text
A  0    500
B  500  1000
A  1000 1500
```

---

## 9. EstadoSistema

Después de cada paso, el motor debe poder devolver una fotografía completa del sistema.

```cpp
EstadoSistema estado = obtenerEstado();
```

La GUI no necesita conocer la lógica interna.

---

## Entregables

```text
simulation/
    SimulationEngine.h
    SimulationEngine.cpp
    SimulationClock.h
    SimulationClock.cpp
```

Tests:

```text
tests/
    test_simulation_quantum.cpp
    test_simulation_wait.cpp
    test_simulation_finish.cpp
    test_simulation_cancel.cpp
    test_simulation_dynamic_process.cpp
```

---

## No es responsabilidad de este integrante

No implementar:

- GUI;
- widgets;
- animaciones;
- `std::thread`;
- mutex;
- condition variables;
- política interna del scheduler;
- estructura visual de la cola.

Debe utilizar el Scheduler del integrante 2.

---

## Dependencias

Depende de:

- contratos del integrante 1;
- Scheduler del integrante 2.

Mientras estos módulos no estén listos, puede trabajar con mocks.

Ejemplo:

```cpp
class MockScheduler {
public:
    int seleccionarSiguiente(...) {
        return primerPidReady;
    }
};
```

---

## Criterios de aceptación

El módulo está terminado cuando:

- puede ejecutar una simulación completa sin GUI;
- respeta quantum;
- actualiza tiempo restante;
- genera transiciones correctas;
- admite procesos nuevos durante ejecución;
- permite cancelar procesos;
- implementa WAIT;
- registra timeline;
- registra eventos;
- produce `EstadoSistema`;
- no contiene threads reales.
