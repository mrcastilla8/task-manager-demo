# GUI Qt 6 integrada

La ventana consume snapshots de los motores v2 reales. La selección inicial es
una vista previa identificada con fixtures; el selector **Motor lógico v2** o
**Motor concurrente v2** crea una sesión nueva con memoria de 1024 bytes. La GUI
no calcula prioridades, ordena READY ni modifica memoria o reloj.

## Compilar y abrir

Requiere CMake 3.20+, C++17, Qt 6.2+ Widgets y Qt Test para las pruebas.
Desde la raíz del repositorio:

```powershell
cmake -S . -B build-gui -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="<kit-mingw>/bin/g++.exe" -DBUILD_TESTING=ON -DSIMULATOR_WARNINGS_AS_ERRORS=ON -DSIMULATOR_BUILD_GUI_PREVIEW=ON -DCMAKE_PREFIX_PATH="<kit-qt>"
cmake --build build-gui --config Debug --parallel 2
ctest --test-dir build-gui -C Debug -R "^(test_gui|test_gui_models)$" --output-on-failure -j 2
ctest --test-dir build-gui -C Debug --output-on-failure -j 2
    .\build-gui\gui\Debug\simulator_gui.exe
```

Sustituye los dos marcadores por las rutas reales del compilador y del kit Qt
MinGW de la misma instalación. No combines Qt MinGW con MSVC ni Qt MSVC con
MinGW. La configuración base sin Qt se verifica aparte:

```powershell
cmake -S . -B build-integracion -DBUILD_TESTING=ON -DSIMULATOR_WARNINGS_AS_ERRORS=ON -DSIMULATOR_BUILD_GUI_PREVIEW=OFF
cmake --build build-integracion --config Debug --parallel 2
ctest --test-dir build-integracion -C Debug --output-on-failure -j 2
```

## Ejecutar una simulación real

1. Abre `simulator_gui.exe` y elige **Motor lógico v2** o
   **Motor concurrente v2** en el selector superior.
2. Pulsa **Agregar proceso** e indica PID, nombre, bytes y CPU total. La
   prioridad y cuota se derivan del motor; E/S es opcional.
3. Pulsa **Siguiente paso** para ejecutar la cuota o transición siguiente.
   Procesos, cola READY, CPU, memoria, eventos y timeline se actualizan desde el
   snapshot real.
4. Selecciona un proceso activo y pulsa **Cancelar PID seleccionado** para
   cancelar READY o WAIT. El motor concurrente completa el paso en curso antes
   de atender una cancelación encolada.
5. **Reiniciar** vacía la sesión y la deja pausada, lista para admitir procesos.
   Cambiar el selector destruye la sesión anterior, invalida sus callbacks
   pendientes y crea el motor elegido.

La vista previa sirve para revisar widgets, pero no admite procesos. El inicio
continuo permanece deshabilitado porque su driver todavía requiere ratificación.
La cancelación parcial durante RUN también está pendiente; no se usa tiempo
físico ni checkpoints artificiales para aproximarla. El motor concurrente
actual conserva el coordinador y workers, pero todavía delega la transición al
motor lógico después de calcular el turno. La revisión de Ángel debe hacer que
el resultado validado del worker autorice la transición antes de afirmar que
participa efectivamente en la ejecución.

`EngineSession` procesa todas las operaciones y snapshots en un hilo dedicado;
los callbacks regresan al hilo Qt. Al cambiar de sesión o cerrar la ventana,
destruye el motor y hace `join` de sus workers. Los refrescos solo consultan el
estado y no avanzan tiempo simulado.

## Pruebas

El build base ejecuta `test_core_scheduler_integracion`,
`test_v2_motor_integracion`, `test_simulation_atomicidad` y
`test_concurrent_v2_equivalencia`. La opción Qt añade pruebas de modelos y
ventana en modo `offscreen`. En Windows, Qt 6.8.3 MinGW x64 con MinGW 13.1.0 y
CMake 4.4.3 configuró y compiló con warnings-as-errors; `test_gui` y
`test_gui_models` pasaron 2/2, y la suite completa pasó 29/29. El build base
pasó 27/27. `test_gui` opera la ventana real con ambos motores, incluida E/S,
WAIT/READY, cambio de motor durante un paso y snapshots; el ejecutable también
inició y mantuvo su loop con `QT_QPA_PLATFORM=offscreen`. El motor concurrente
sigue pendiente de la revisión de autoridad del worker por Ángel.
