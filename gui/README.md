# Avance de Poma: prototipo Qt 6

**DECISION_EQUIPO:** Qt 6/C++17/CMake y reglas de presentación DP-08/11,
fuente: [decisiones aprobadas](../docs/DECISIONES_APROBADAS.md#dp-08--gui-y-timeline).
Esta aplicación es una preparación de vistas en `poma`, no la integración final
ni la adopción de la API candidata. No modifica `core/` ni `contracts/`.

La entrega del avance se publica desde la rama `poma` para revisión en el
[PR #6](https://github.com/mrcastilla8/task-manager-demo/pull/6), dirigido a
`master` y mantenido en borrador mientras falten los gates de integración.

## Construcción y ejecución

Requiere CMake 3.20+, Qt 6.2+ Widgets/Test y un compilador compatible con ese SDK.
En Windows, seleccionar el kit completo de Qt: no mezclar MinGW UCRT con un Qt
compilado para MinGW MSVCRT, ni un SDK MSVC con GCC.

```powershell
cmake -S . -B build-gui -DBUILD_TESTING=ON -DSIMULATOR_BUILD_GUI_PREVIEW=ON -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/mingw_64"
cmake --build build-gui --config Debug
# Qt/bin y el bin del compilador deben estar en PATH.
ctest --test-dir build-gui -C Debug --output-on-failure
./build-gui/gui/simulator_gui_preview.exe
# En generadores multiconfiguración: ./build-gui/gui/Debug/simulator_gui_preview.exe
```

La opción GUI es OFF por defecto. El build base sigue sin requerir Qt. La prueba
`test_gui_presentation` tampoco requiere Qt; `test_gui` usa plataforma offscreen.
Para conservar una captura durante esa prueba, definir
`SIMULATOR_GUI_SCREENSHOT` con una ruta absoluta PNG.

En el entorno donde se preparó este avance hay herramientas portables ignoradas
en `build-tools/` y build en `build-gui-qt/`. Se puede abrir la aplicación con:

```powershell
./scripts/run-gui-preview.ps1
```

Ese script solo configura el entorno del proceso y abre la GUI; no descarga nada.
Las dependencias y ejecutables no se publican en Git.

## Qué se puede revisar

- Tabla de procesos activos y terminales con PID/nombre/bytes/estado/prioridad,
  cuota vigente, CPU y ticket. Cola READY respeta exactamente el vector recibido.
- CPU lógico único, RUN explícito, CPU restante y progreso sobre cuota fijada.
- Memoria proporcional por PID activo y espacio libre; WAIT conserva ocupación.
- Timeline proporcional, scroll horizontal, arrastre y zoom; rayado para huecos
  antes/entre/después de CPU, sin insertar PID especial en `BloqueTimeline`.
- Log completo del snapshot, conservando orden recibido y scroll. No se deduce
  precedencia de eventos simultáneos ni semántica de duración no ratificada.
- Formulario de alta con PID, nombre, bytes y ms enteros de 64 bits, E/S opcional,
  validación y mensajes. Sin prioridad manual, quantum global ni aging.
- Anterior/siguiente/reproducción/pausa/inicio de vistas y cadencia de presentación.
  **Los botones recorren fixtures, no ejecutan una simulación.** El último escenario
  es una fixture independiente de gaps. El formulario de preview no admite procesos.

`PreviewFrames` contiene snapshots preparados; no implementa `IExecutionEngine`,
no clasifica ni planifica. RUN transitorio se muestra mediante snapshots explícitos,
sin inventar replay contractual. Las vistas reciben copias y se actualizan en el
hilo Qt. El timer de refresco de motor solamente llama `obtenerEstado()`.

## Conectar las entregas del equipo

**PENDIENTE_CONFIRMACION técnica:** las firmas, códigos y llamadas de
`contracts/v2/IExecutionEngine.h` siguen como borrador. Fuente:
[propuestas técnicas, secciones 1–4](../docs/PROPUESTAS_TECNICAS_V2.md).
`EngineSession` y `MainWindow::attachEngine` son un adaptador experimental aislado,
no una declaración de API estable. El doble usado en tests registra llamadas y
respuestas; no es un motor alternativo ni evidencia de conformidad de Aldana/Ángel.

1. Ratificar G1 (API, errores, driver, cancelación y eventos). Revisar el adaptador
   contra las firmas adoptadas, conservando los widgets como consumidores.
2. Esperar G2/G3 y las implementaciones G4. Integrar primero Aldana y luego Ángel.
   Añadir sus targets/fábricas en el ensamblaje de `main.cpp`; no copiar Scheduler.
3. Entregar propiedad exclusiva de un motor a
   `window.attachEngine(std::move(engine), "Motor lógico")`. El adaptador ya delega
   alta, cancelación, pausa, paso, reinicio y lectura. El selector muestra motores
   pendientes deshabilitados; su conmutación real requiere las dos fábricas y cierre
   seguro de la sesión anterior. No se simula disponibilidad.
4. Antes de conectar un motor, garantizar llamadas rápidas/no bloqueantes para el
   hilo UI. Si cancelación espera workers, introducir transporte asíncrono acordado:
   no llamar una cancelación bloqueante desde el slot Qt. El motor debe publicar
   snapshots coherentes y cerrar/join de workers al destruirse. No se acredita
   seguridad de los motores con el doble de prueba.
5. Inicio continuo real permanece deshabilitado: falta una unidad invocable en
   EJECUTANDO. No reutilizar `siguientePaso()` incumpliendo PAUSADO ni agregar una
   virtual unilateralmente. Con el protocolo aprobado, conectar Inicio y su driver;
   el timer de presentación seguirá separado del avance lógico.
6. Ratificar secuencia/timestamps para reproducir RUN desde eventos cuando falten
   snapshots intermedios. Hoy solo se muestra el RUN que el snapshot publica.
7. Ejecutar los casos de [workflow, fase 4](../docs/WORKFLOW_DESARROLLO.md#fase-4--gui-y-visualizacion)
   contra ambos motores y comparar trazas antes de declarar G5/G6.

## Límites del avance

No hay motores reales enlazados, continuidad real ni replay normativo de eventos.
El mapa de memoria es conceptual; no representa fragmentación ni direcciones.
El log y la escena conservan todo el snapshot sin truncarlo; la reconstrucción es
lineal y debe perfilarse/virtualizarse para sesiones grandes. La validación local
del formulario no sustituye validación/admisión autoritativa del motor.

El cierre de integrante 5 sigue pendiente de fábricas, protocolo asíncrono/driver,
conmutación segura, pruebas de equivalencia y aceptación del equipo. Véase la
[matriz del avance](../docs/MATRIZ_TRAZABILIDAD.md#avance-gui-de-poma).

## Validación local del avance (2026-10-15)

Windows, Qt 6.8.3, MinGW 13.1.0 del kit oficial, CMake 4.4.4, Ninja 1.13.2,
Debug. Se ejecutaron configuración/build y CTest en dos directorios:

| Configuración | Resultado observado |
|---|---|
| `build-base`, GUI OFF, BUILD_TESTING ON | 5/5 tests PASS; no se busca Qt |
| `build-gui-qt`, GUI ON, BUILD_TESTING ON | 6/6 tests PASS; interacción Qt offscreen |

La suite Qt comprueba lectura desde hilo UI, refresco sin pasos, zoom sin avance,
RUN explícito, cuota fijada frente a vigente, validación E/S y overflow de entrada,
delegación de paso/alta/cancelación/reinicio, rechazo por memoria y errores de snapshot.
Se inspeccionaron capturas Qt de READY, RUN parcial e idle, generadas por la prueba;
no son capturas de una ejecución real de los motores.

Para reproducir con las herramientas portables locales, desde la raíz:

```powershell
$env:PATH = "$PWD/build-tools/Qt/6.8.3/mingw_64/bin;$PWD/build-tools/Qt/Tools/mingw1310_64/bin;$env:PATH"
$cmake = "$PWD/build-tools/python/cmake/data/bin/cmake.exe"
$ctest = "$PWD/build-tools/python/cmake/data/bin/ctest.exe"
& $cmake -S . -B build-gui-qt -G Ninja "-DCMAKE_MAKE_PROGRAM=$PWD/build-tools/ninja/bin/ninja.exe" "-DCMAKE_CXX_COMPILER=$PWD/build-tools/Qt/Tools/mingw1310_64/bin/g++.exe" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DSIMULATOR_BUILD_GUI_PREVIEW=ON "-DCMAKE_PREFIX_PATH=$PWD/build-tools/Qt/6.8.3/mingw_64"
& $cmake --build build-gui-qt --parallel 4
$env:SIMULATOR_GUI_SCREENSHOT = "$PWD/build-gui-qt/preview.png"
& $ctest --test-dir build-gui-qt --output-on-failure
```

Para la base, configurar un directorio separado con GUI OFF y sin
`CMAKE_PREFIX_PATH`, conservando compilador y Ninja. Capturas esperadas:
`build-gui-qt/preview.png`, `preview-run.png`, `preview-idle.png`.
Se verifican además enlaces de la documentación nueva y `git diff --check`.
No se afirma prueba de concurrencia, equivalencia de motores ni CI remoto.
