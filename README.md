# ConcurrentSimulator

ConcurrentSimulator es una aplicación C++17 y Qt 6 que representa la admisión,
planificación, memoria y ejecución por turnos de procesos sobre **un único CPU
lógico**. Su objetivo es mostrar cómo se coordinan procesos, prioridades,
esperas de E/S, memoria e historial temporal. El motor concurrente utiliza
workers reales para calcular las ráfagas, pero esos threads no representan
varios CPU simulados ni ejecutan procesos simultáneamente en el modelo.

## Descarga rápida para Windows

La opción recomendada para un usuario final será descargar
`ConcurrentSimulator-Windows-x64.zip` desde la sección **Releases** del
repositorio, extraerlo y ejecutar `simulator_gui.exe`. Ese paquete incluye las
bibliotecas Qt y runtime que necesita la aplicación; el usuario no necesita
instalar Qt, MinGW, CMake ni Visual Studio.

La Release portable se publicará cuando exista una versión etiquetada después
de integrar el PR principal en `master` y de completar la revisión de licencia
del proyecto. Mientras no haya una Release publicada, no descargues el ZIP
fuente de GitHub esperando encontrar un ejecutable: **Code → Download ZIP**
contiene el código fuente. El workflow
`.github/workflows/distribucion-windows.yml` genera el ZIP y su SHA-256 al
validar un tag `v*` que apunte a un commit ya incorporado en `master`.

## Características

- Alta dinámica de procesos y reserva de memoria; la memoria se libera al
  terminar o cancelar.
- Clasificación automática P1/P2/P3 según tamaño. P3 tiene mayor prioridad;
  no se elige manualmente ni se aplica aging.
- Scheduler común con selección READY por prioridad, ticket FIFO y PID como
  desempate.
- Cuotas máximas de 10, 20 o 30 ms, fijadas al despachar cada proceso.
- Estados READY, RUN, WAIT, FINISH y CANCEL; E/S bloquea un proceso y lo
  devuelve a READY en su instante de despertar.
- CPU consumida/restante, memoria ocupada/disponible, eventos y timeline de CPU.
  Los huecos idle se infieren; no son segmentos de CPU.
- Dos motores reales sobre la misma interfaz: lógico y concurrente.
- GUI Qt 6 conectada al motor elegido. El refresco visual no avanza el reloj.

## Ejecutar desde el código fuente

Para compilar en Windows 64 bits, instala:

- [Qt 6 para MinGW x64](https://www.qt.io/download-qt-installer-oss), con
  **Qt Widgets**, **Qt Test** y el compilador MinGW compatible con ese kit. El
  entorno usado para validar esta rama es Qt 6.8.3 MinGW x64 y MinGW 13.1.0.
- [CMake 3.20 o posterior](https://cmake.org/download/).
- Windows PowerShell 5.1 o PowerShell 7; viene incluido en Windows o se obtiene
  desde [Microsoft](https://learn.microsoft.com/powershell/).

No mezcles un kit Qt compilado con MinGW con MSVC, ni un kit Qt MSVC con MinGW.
Los scripts buscan Qt en `QT_ROOT`, `QTDIR`, `Qt6_DIR`, la ubicación de `qmake`
y las ubicaciones habituales de Qt. También verifican la versión GCC esperada
por el kit. No instalan herramientas ni modifican el PATH permanente; si una
dependencia falta, indican qué instalar y permiten volver a ejecutar el
lanzador.

1. Descarga el código fuente como ZIP o clona el repositorio.
2. Extrae el ZIP, si corresponde.
3. Ejecuta `Ejecutar-desde-codigo.cmd`.
4. El script configura CMake, compila con hasta dos trabajos y abre la GUI.

También puedes ejecutarlo desde PowerShell:

```powershell
.\scripts\ejecutar-desde-codigo.ps1
```

Si Qt está en una ubicación no detectada, puedes pasar su raíz de kit sin
editar el script:

```powershell
.\scripts\ejecutar-desde-codigo.ps1 -QtRoot 'D:\Qt\6.8.3\mingw_64'
```

### Configuración CMake manual

Con Qt y el compilador del kit ya en `PATH`, desde la raíz del repositorio:

```powershell
cmake -S . -B build-windows-mingw `
  -G 'MinGW Makefiles' `
  -DCMAKE_CXX_COMPILER='D:/Qt/Tools/mingw1310_64/bin/g++.exe' `
  -DCMAKE_PREFIX_PATH='D:/Qt/6.8.3/mingw_64' `
  -DBUILD_TESTING=ON `
  -DSIMULATOR_WARNINGS_AS_ERRORS=ON `
  -DSIMULATOR_BUILD_GUI_PREVIEW=ON
cmake --build build-windows-mingw --config Debug --parallel 2
ctest --test-dir build-windows-mingw -C Debug --output-on-failure -j 2
```

Las rutas son ilustrativas para la configuración manual. Para detección
automática usa los scripts, que no requieren editar rutas.

## Guía de la GUI

Al abrir la ventana, el selector superior ofrece **Vista previa de snapshots**,
**Motor lógico v2** y **Motor concurrente v2**. La vista previa reproduce
fixtures para revisar la presentación; no es una simulación ejecutada y no
admite altas. Elige uno de los motores v2 para usar la aplicación real.

1. Elige el motor lógico o concurrente. Cambiar el selector crea una sesión
   nueva y descarta la anterior.
2. Pulsa **Agregar proceso** y completa PID, nombre, tamaño en bytes, CPU total
   y, opcionalmente, la configuración de E/S. La prioridad la calcula el
   Scheduler.
3. Pulsa **Siguiente paso**. Cada acción ejecuta como máximo una cuota, termina
   antes si se agota la CPU, alcanza E/S o se cancela, o avanza idle hasta el
   siguiente despertar si no hay READY.
4. Interpreta READY como listo para competir por CPU; RUN como turno actual;
   WAIT como bloqueado por E/S; FINISH y CANCEL como estados terminales.
5. La prioridad P1/P2/P3 y la cola READY reflejan el Scheduler. La CPU y el
   reloj muestran tiempo simulado. El panel de memoria muestra uso por proceso
   y memoria libre. El timeline muestra intervalos de CPU realmente ejecutados;
   los huecos son idle inferido. El registro presenta eventos del motor.
6. Selecciona un proceso activo y pulsa **Cancelar PID seleccionado**. Una
   cancelación concurrente durante RUN queda pendiente hasta cerrar la ráfaga
   completa; no introduce checkpoints parciales.
7. Pulsa **Reiniciar** para vaciar la sesión. Puedes cambiar de motor con el
   selector; la sesión anterior se detiene y sus workers se unen al cerrar.

El control de inicio continuo aparece deshabilitado mientras su driver no esté
ratificado. Usa **Siguiente paso** para avanzar el modelo.

### Ejemplo reproducible

Selecciona cualquiera de los motores reales y agrega estos procesos en orden;
los tres tienen el mismo tamaño, por lo que pertenecen a P3:

| PID | Tamaño | CPU total | E/S |
|---|---:|---:|---|
| 1 | 20 bytes | 25 ms | después de 10 ms, espera 8 ms |
| 2 | 20 bytes | 15 ms | ninguna |
| 3 | 20 bytes | 40 ms | ninguna |

Pulsa **Siguiente paso** repetidamente. El primer turno ejecuta PID 1 durante
10 ms y lo lleva a WAIT. PID 2 termina con 15 ms de CPU. Mientras otro turno
está en curso, el reloj alcanza el despertar de PID 1; su retorno obtiene un
ticket READY nuevo. PID 3 consume primero su ticket antiguo y vuelve a READY al
agotar 30 ms. PID 1 termina con sus 15 ms restantes y PID 3 termina después con
los 10 ms restantes. Observa el evento WAIT/retorno, la liberación de memoria
al terminar y los intervalos de timeline, que no incluyen CPU durante idle.

## Arquitectura

| Integrante | Responsabilidad integrada |
|---|---|
| Castilla | Core v2: lista única de procesos, validación de alta y memoria. |
| Basualdo | Scheduler v2: clasificación, prioridades y tickets READY. |
| Aldana | Motor lógico independiente, sin workers reales. |
| Ángel | Motor concurrente independiente con workers reales y un coordinador que serializa un solo CPU lógico. |
| Poma | GUI Qt 6: crea sesiones mediante `IExecutionEngine` y presenta snapshots/eventos. |

Los dos motores comparten Core, Scheduler e interfaz; no comparten una función
que ejecute toda la simulación. Sus resultados observables se comparan con
pruebas deterministas y fuzz reproducible. La GUI no calcula prioridades ni
ejecuta planificación dentro de widgets. Las decisiones aprobadas y los
contratos v2 permanecen documentados en el historial de esta rama; todavía no
se han retirado documentos internos.

## Pruebas

Las pruebas requieren `BUILD_TESTING=ON`; los tests GUI usan Qt offscreen en
automatización. Desde el directorio de build:

```powershell
ctest --test-dir build-windows-mingw -C Debug -R '^test_concurrent_v2_' --output-on-failure -j 2
ctest --test-dir build-windows-mingw -C Debug -R '^(test_gui|test_gui_models)$' --output-on-failure -j 2
ctest --test-dir build-windows-mingw -C Debug --output-on-failure -j 2
```

`test_concurrent_v2_comparacion` y `test_concurrent_v2_equivalencia` comparan
ambos motores. `test_concurrent_v2_exclusion` comprueba cancelación concurrente,
fallos controlados y consistencia del worker. En la validación de esta rama,
Qt 6.8.3 MinGW x64, MinGW 13.1.0, CMake 4.4.3 y generador MinGW Makefiles
pasaron el build Qt con warnings-as-errors, los tests concurrentes **9/9**, los
tests GUI offscreen **2/2** y CTest completo **37/37**. El build base sin Qt
pasó con MSVC 19.29.30159 y warnings-as-errors; CTest base **35/35**.

## Limitaciones vigentes

- Inicio continuo desde la GUI: pendiente de ratificación del driver G1.
- Checkpoints de cancelación parcial durante RUN: pendientes; hoy se espera el
  fin de la ráfaga completa.
- Política definitiva de overflow de tickets: pendiente; el motor rechaza de
  forma segura la operación cuando no puede asignar tickets.
- Precedencia normativa entre FINISH/CANCEL en una frontera concurrente y el
  orden total de eventos coincidentes: pendientes de ratificación. La
  implementación conserva el comportamiento provisional documentado.
- No se ha probado aún el ZIP en una instalación Windows limpia; iniciar la
  aplicación en el equipo de desarrollo no demuestra independencia de
  dependencias globales.

## Licencias y problemas frecuentes

El repositorio no tiene actualmente un archivo `LICENSE` para el código propio;
por ello no se atribuye una licencia ni se presenta la descarga como una
Release pública ya disponible. Los avisos consultados para el paquete se
describen en [`licenses/AVISOS_DEPENDENCIAS.md`](licenses/AVISOS_DEPENDENCIAS.md).
El repositorio incluye textos LGPLv3 y GPLv3 y el empaquetador agrega el SBOM
de los módulos Qt disponibles en el kit.
Qt y MinGW conservan sus licencias y avisos propios; consulta también la
[licencia Qt](https://www.qt.io/licensing/) y la
[licencia de libstdc++](https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html).
El workflow genera el ZIP y el SHA-256 tras un tag de versión en `master`, una
vez que se cierre la decisión de licencia y la validación final.

- **Qt 6 no encontrado:** instala el kit Qt MinGW x64 con Widgets y Qt Test;
  vuelve a ejecutar `Ejecutar-desde-codigo.cmd`.
- **Compilador incompatible:** usa el MinGW instalado para ese kit de Qt; no
  mezcles MinGW y MSVC.
- **Falta una DLL al abrir el ZIP:** comprueba que ejecutaste el ZIP extraído
  completo y que no moviste `platforms/qwindows.dll` ni las DLL junto al EXE.
- **No se encuentra el plugin `windows`:** restaura la carpeta `platforms` del
  paquete portable; no copies plugins de otra versión/kit Qt.
- **Reportar un error:** abre un issue en GitHub con Windows, arquitectura,
  motor elegido, pasos reproducibles, mensaje completo y SHA del artefacto.

## Distribución y validación

Para preparar localmente el paquete Release, con Qt MinGW x64 y el toolchain
compatible instalados:

```powershell
./scripts/crear-distribucion-windows.ps1
```

El script construye sin tests para distribución, ejecuta `windeployqt`,
comprueba el plugin de Windows y runtime MinGW, y genera
`dist/ConcurrentSimulator-Windows-x64.zip` más el hash en consola. La prueba
real en Windows limpio debe completarse antes de afirmar que ese ZIP es
portable para cualquier usuario final.
