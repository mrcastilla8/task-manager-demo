# ConcurrentSimulator — módulo de modelo y estructuras

Esta rama contiene los contratos compartidos versión 1.0, el almacenamiento
dinámico de procesos y la memoria simulada del proyecto. Las decisiones de
compatibilidad están documentadas en `contracts/README.md`.

## Requisitos

- compilador compatible con C++17;
- CMake 3.20 o superior;
- CTest (incluido con CMake).

## Compilar y probar

```powershell
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Las advertencias se tratan como errores de forma predeterminada. Para una
compilación exploratoria puede usarse
`-DSIMULATOR_WARNINGS_AS_ERRORS=OFF`, pero no para validar una entrega.

## Reglas públicas del módulo

- Un proceso válido tiene PID positivo, nombre no vacío, tamaño y tiempo total
  positivos, prioridades entre 1 y 10 y contadores no negativos.
- Un proceso puede declarar una operación de E/S opcional con instante de
  bloqueo por CPU consumida y duración de `WAIT`.
- `ColaProcesos::agregar` lanza `std::invalid_argument` si el proceso es inválido
  o si su PID ya existe.
- `ColaProcesos` conserva el orden de inserción; no aplica prioridad ni otras
  reglas de scheduling.
- `Memoria::cargar` devuelve `false` cuando el proceso es inválido, ya está
  cargado o no cabe. Una carga fallida no cambia el estado.
- `Memoria::liberar` no hace nada si el PID no está cargado.
- Los tiempos están expresados en milisegundos simulados y los tamaños en bytes.
- Los motores lógico y concurrente, además de los mocks de GUI, implementan
  `IExecutionEngine`; el quantum se cambia únicamente mediante esa interfaz.
