# Integrante 2 — Scheduler y política de planificación

**Responsable:** Brickzon · **Rama:** `brickzon` · **Workflow:** [fase 2](docs/WORKFLOW_DESARROLLO.md#fase-2--scheduler)

Asignación recuperada de `origin/brickzon`; `origin/angel` contiene la misma responsabilidad funcional. Se restituye la superficie operativa original, ajustando métodos y unidades a la implementación real de `scheduler/`.

## Objetivo y límites

Decidir qué proceso `READY` recibe el CPU lógico y construir el orden de PID READY que la GUI representa. Scheduler no depende de GUI, workers, temporizadores gráficos ni del motor concreto. Los motores llaman al mismo Scheduler; no duplican su política.

## Decisión vigente del equipo

La política documentada actualmente es Priority Scheduling con desempate por FIFO/orden de llegada y aging:

- prioridad válida 1–10; 10 es la mayor en la escala actual del equipo;
- solo se seleccionan procesos en `READY`;
- gana mayor `prioridadActual`; empatan por `ordenLlegada` menor y luego por PID menor;
- cada intervalo completo de espera READY de 2,000 ms suma +1, sin superar 10;
- al asignar CPU, el Scheduler restaura `prioridadActual` a `prioridadBase` y pone `tiempoEspera` en cero.

Todo lo anterior es `DECISION_EQUIPO`, no requisito atribuido al docente. La implementación de aging cuenta intervalos cruzados según el delta recibido. Procesos que no están READY no acumulan aging.

## Algoritmo docente y precedencia

Las notas, pág. 24, dan prioridades 1, 2 y 3 por bandas de tamaño con cortes estrictos `A+H` y `A+2H`. En la pág. 25, `ObtenerMaximaPrioridad()` recorre la lista y conserva el proceso cuyo valor de prioridad es mayor. Por tanto, **en el algoritmo presentado P3 precede a P2 y P1**. Este sentido interno del algoritmo queda documentado como evidencia docente; no está pendiente.

Lo que sigue pendiente en DP-01 es si el equipo adopta la escala docente 1–3 o la adapta a la política contractual 1–10. DP-02 determina sobre qué estados se calcula TL y cuándo se recalcula. DP-03 registra el caso degenerado de tamaños iguales/precisión de H. No reemplazar la escala actual ni cambiar scheduler sin ratificación.

## Superficie de métodos implementada

Las firmas exactas están en `scheduler/Scheduler.h` y la configuración en `scheduler/SchedulerConfig.h` (rutas de la rama de Scheduler; no están presentes en esta rama documental):

- `Scheduler(SchedulerConfig)` y `configuracion()` / `configurar(...)` para leer y actualizar `SchedulerConfig`.
- `seleccionarSiguiente(const std::vector<Proceso>&) const` devuelve PID ganador o `-1` si no hay READY.
- `ordenarColaPrioridad(const std::vector<Proceso>&) const` devuelve PIDs READY en orden de prioridad descendente, `ordenLlegada` ascendente y PID ascendente.
- `aplicarAging(std::vector<Proceso>&, std::int64_t deltaTiempo) const` actualiza espera/prioridad READY con los valores configurados.
- `alAsignarCPU(Proceso&) const` restaura prioridad base y espera cero.

`SchedulerConfig` contiene prioridad mínima/máxima, `agingIntervaloMs` y `agingIncremento`, inicializados desde `contracts/Configuracion.h`. El quantum global pertenece al motor y su contrato; no es parámetro del Scheduler actual.

## FIFO, Round Robin y trabajo del motor

Una primera selección empatada favorece el menor orden de llegada. La rotación Round Robin no la crea el Scheduler: cuando un proceso consume turno y reingresa a READY, el **motor** debe actualizar su `ordenLlegada` (o el ticket equivalente aprobado) para colocarlo después de sus pares. La prueba de scheduler configura A con orden 3 y B con 2 de manera manual y entonces selecciona B; prueba la selección bajo el nuevo orden, no la actualización integrada.

El motor concurrente revisado vuelve de `QUANTUM_AGOTADO` a READY sin actualizar `ordenLlegada`. Esta discrepancia puede impedir alternancia y está pendiente DP-05; no corregirla en esta tarea documental. Aging tampoco es el recálculo dinámico por tamaño: tienen objetivos y precedencia independientes (DP-10).

## Entregables y pruebas

- Implementación: `scheduler/Scheduler.h`, `scheduler/Scheduler.cpp`, `scheduler/SchedulerConfig.h`.
- Pruebas: `tests/test_scheduler_prioridad.cpp`, `tests/test_scheduler_empate.cpp`, `tests/test_scheduler_aging.cpp`.

Escenarios obligatorios recuperados:

1. Prioridades READY A P8, B P5, C P3: elige A según la escala vigente del equipo.
2. Filtrado: A WAIT P10 y B READY P5: elige B; RUN, FINISH y CANCEL tampoco compiten.
3. Sin READY (WAIT/FINISH/CANCEL) y vector vacío: devuelve `-1`.
4. Empate inicial A P8 llegada 1 y B P8 llegada 2: elige A incluso si el vector está invertido.
5. Empate con misma prioridad y misma llegada: desempata con PID menor.
6. Reingreso: después de quantum, A recibe orden más nuevo que B; elige B. La asignación del nuevo orden pertenece al motor.
7. Aging: espera acumulada en READY cambia prioridad por intervalo; límite P10 no se supera; WAIT/RUN no acumulan aging; `alAsignarCPU` restaura prioridad base/espera.
8. Cambio de selección por aging: con deltas suficientes y la prioridad restablecida del proceso atendido, el orden nuevo debe reflejar los valores actuales. La expectativa de prueba existente debe ser revisada como regla de equipo antes de modificar código.

## Dependencias y criterios de aceptación

Depende del contrato `Proceso` y sus estados. Puede probarse con vectores de procesos sintéticos y fixtures; no requiere GUI ni motor. Scheduler queda alineado cuando selecciona únicamente READY, respeta prioridades y desempates deterministas, produce cola visible ordenada, aplica aging según configuración, y el diseño deja explícito que motor actualiza el orden de reingreso. Los tests deben probar el Scheduler y la integración en niveles distintos. No implementa memoria, altas/bajas físicas, animaciones, Qt, threads, mutex ni timeline.
