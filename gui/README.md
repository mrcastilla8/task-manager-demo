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
`test_gui_presentation` tampoco requiere Qt. `test_gui` y `test_gui_models` usan
plataforma offscreen, sin mostrar ventanas, generar capturas ni esperar animaciones.

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
  **Los botones recorren fixtures, no ejecutan una simulación.** El selector separa
  la sesión principal, rotación por cuota y gaps. Cada escenario empieza en su propio
  contexto; llegar al final detiene la reproducción, sin saltar a otra sesión.
  Cambiar escenario o volver al inicio borra selección/scroll del contexto anterior;
  el zoom elegido se conserva. El formulario de preview no admite procesos.

`PreviewFrames` contiene snapshots preparados; no implementa `IExecutionEngine`,
no clasifica ni planifica. RUN transitorio se muestra mediante snapshots explícitos,
sin inventar replay contractual. Las vistas reciben copias y se actualizan en el
hilo Qt. El timer de refresco de motor solamente llama `obtenerEstado()`. Una
publicación idéntica no vuelve a actualizar las vistas.

## Recuperación de lectura y selección

Si `obtenerEstado()` falla, se conserva la última vista válida, se muestra el error
y se deshabilitan paso/pausa/alta/reinicio/cancelación hasta recuperar una lectura.
Si falla la primera lectura de una sesión, las vistas quedan vacías, sin fixtures
de otra sesión. El timer de un solo disparo programa reintentos a 300, 600, 1200,
2400, 4800 y como máximo 5000 ms; el botón **Reintentar lectura del snapshot** permite
solicitar una lectura inmediata. El éxito restablece la consulta normal a 150 ms,
incluso si el snapshot recuperado es idéntico al anterior. No se reinicia el motor,
no se envían pasos, no se altera su tiempo y no se oculta el error mientras persista.

Cancelar exige selección de fila efectiva, PID positivo y estado READY/RUN/WAIT
publicado. FINISH/CANCEL, selección vacía y snapshots no disponibles deshabilitan
el botón; el slot vuelve a comprobar estas condiciones antes de delegar. El motor
sigue siendo autoritativo ante una transición ocurrida después del último snapshot.

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
El log conserva todos los eventos mediante un modelo de datos; Qt solicita las
celdas visibles, sin crear un objeto por celda ni medir todas las filas al refrescar.
Altas/bajas de filas y correcciones interiores emiten cambios del modelo sin reset.
La selección de proceso se conserva por PID y el scroll se restaura al actualizar;
el log sigue el final solo si ya se estaba allí.

El timeline conserva sus bloques y gaps en un solo item de escena; al pintar busca
por tiempo y dibuja únicamente los intervalos expuestos. Una publicación sin cambios
no reconstruye intervalos; zoom/scroll no reconstruyen el historial. Al cambiar
el timeline se validan/recalculan sus spans conservando zoom y desplazamiento.
El widget RAM no copia eventos ni timeline.

La interfaz candidata entrega snapshots completos sin revisión/deltas: obtenerlos
y compararlos exactamente sigue teniendo coste O(n) respecto del historial. El cache
evita copias/reconstrucciones UI en publicaciones idénticas; las tablas actualizan
solo los registros cambiados o añadidos. No se inventa un campo de versión ni se
deduce inmutabilidad del tamaño del vector. La validación local del formulario
no sustituye validación/admisión autoritativa del motor.

El cierre de integrante 5 sigue pendiente de fábricas, protocolo asíncrono/driver,
conmutación segura, pruebas de equivalencia y aceptación del equipo. Véase la
[matriz del avance](../docs/MATRIZ_TRAZABILIDAD.md#avance-gui-de-poma).

## Validación local del avance (2026-10-15)

Windows, Qt 6.8.3, MinGW 13.1.0 del kit oficial, CMake 4.4.4, Ninja 1.13.2,
Debug. Se ejecutaron configuración/build y CTest en dos directorios:

| Configuración | Resultado observado |
|---|---|
| `build-base`, GUI OFF, BUILD_TESTING ON | 5/5 tests PASS; no se busca Qt |
| `build-gui-qt`, GUI ON, BUILD_TESTING ON | 7/7 tests PASS; modelos/controladores Qt offscreen |

La suite Qt comprueba lectura desde hilo UI, refresco sin pasos, zoom sin avance,
RUN explícito, cuota fijada frente a vigente, validación E/S y overflow de entrada,
delegación de paso/cancelación/reinicio, fallo inicial/posterior de snapshot y
recuperación automática/manual. Se sustituyeron las automatizaciones gráficas de
la primera entrega por llamadas directas a modelos/controles: no hay `show()`,
capturas ni esperas `qWait` en las pruebas.

| Corrección | Evidencia automatizada |
|---|---|
| POMA-01 | `test_gui_presentation` recorre cada escenario con reloj monótono y prueba que `next()` no cruza sesiones; `test_gui` comprueba selector/final/reinicio |
| POMA-02 | `test_gui_models`: 50.000 eventos, 50.000 bloques, 5.000 procesos; 20 snapshots repetidos no emiten cambios/reset ni cambian revisiones; append y edición interior sin reset; escena de un solo item |
| POMA-03 | `test_gui_presentation` prueba backoff/tope; `test_gui` prueba fallo inicial, vistas vacías, último snapshot conservado, timer rearmado y recuperación automática/manual, incluso sin cambios de datos |
| POMA-04 | `test_gui`: sin selección, FINISH y CANCEL no envían cancelación; WAIT habilita; fallo de snapshot bloquea acciones |
| POMA-05 | `test_gui_models`: selección por PID tras cambios/reordenamiento, scroll horizontal/vertical, seguimiento del final, zoom y scroll de timeline conservados |
| POMA-06 | Pruebas sin Qt de navegación/comparación/reintento; pruebas Qt de modelos/controladores; refrescos, errores y zoom no cambian reloj ni piden pasos |

Para reproducir con las herramientas portables locales, desde la raíz:

```powershell
$env:PATH = "$PWD/build-tools/Qt/6.8.3/mingw_64/bin;$PWD/build-tools/Qt/Tools/mingw1310_64/bin;$env:PATH"
$cmake = "$PWD/build-tools/python/cmake/data/bin/cmake.exe"
$ctest = "$PWD/build-tools/python/cmake/data/bin/ctest.exe"
& $cmake -S . -B build-gui-qt -G Ninja "-DCMAKE_MAKE_PROGRAM=$PWD/build-tools/ninja/bin/ninja.exe" "-DCMAKE_CXX_COMPILER=$PWD/build-tools/Qt/Tools/mingw1310_64/bin/g++.exe" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DSIMULATOR_BUILD_GUI_PREVIEW=ON "-DCMAKE_PREFIX_PATH=$PWD/build-tools/Qt/6.8.3/mingw_64"
& $cmake --build build-gui-qt --parallel 4
& $ctest --test-dir build-gui-qt --output-on-failure
& $cmake -S . -B build-base -G Ninja "-DCMAKE_MAKE_PROGRAM=$PWD/build-tools/ninja/bin/ninja.exe" "-DCMAKE_CXX_COMPILER=$PWD/build-tools/Qt/Tools/mingw1310_64/bin/g++.exe" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DSIMULATOR_BUILD_GUI_PREVIEW=OFF
& $cmake --build build-base --parallel 3
& $ctest --test-dir build-base --output-on-failure
```

Para la base, configurar un directorio separado con GUI OFF y sin
`CMAKE_PREFIX_PATH`, conservando compilador y Ninja. Las capturas locales de la
primera entrega son históricas; la suite actual ya no las genera.
Se verifican además enlaces de la documentación nueva y `git diff --check`.
No se afirma prueba de concurrencia, equivalencia de motores ni CI remoto.
