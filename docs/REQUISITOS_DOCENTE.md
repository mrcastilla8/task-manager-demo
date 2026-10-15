# Requisitos del docente para ConcurrentSimulator

Este documento reúne solo las partes pertinentes de las notas de clase y la explicación del 1 de octubre de 2026. Los materiales originales no se incluyen en el repositorio. Las páginas y marcas de tiempo son localizadores de las copias proporcionadas para esta auditoría. La clasificación describe la evidencia disponible; una regla explícita puede dejar detalles de implementación pendientes.

## Fuentes consultadas

| Clave | Fuente y localizador |
|---|---|
| N | *NOTAS DE CLASE PROG; CONCURRENTE PARALELA.docx*, págs. 23–29, con foco en 23–28. |
| T | `transcripcion_explicaciontrabajo2_2026-10-01_22-00-18.md`, marcas originales 00:03:59–00:14:22. |

Los ejercicios de QR, multiplicación de matrices, speedup y grafos de dependencias quedan fuera del alcance de este proyecto y no se convierten en requisitos.

## Requisitos rastreados

### Modelo de proceso y representación

**RD-01 — Atributos visibles del proceso. CONFIRMADO_POR_DOCENTE.** Las notas enumeran identificador, nombre, tamaño en bytes, estado, cuota en milisegundos y prioridad (N, pág. 23, lista de atributos). La transcripción también menciona identificador, nombre, bytes, estado, prioridad y tiempo asignado (T, 00:09:00–00:09:28). No se especifican tipos C++, rangos ni si todos esos atributos deben persistirse en el contrato.

**RD-02 — Procesos dinámicos. CONFIRMADO_POR_DOCENTE.** Nuevos procesos ingresan para almacenarse en memoria y ejecutarse, y otros terminan (N, pág. 25; T, 00:08:25–00:08:52). El detalle de admisión cuando la memoria no alcanza no queda fijado por estas fuentes.

**RD-03 — Estructura de cola. CONFIRMADO_POR_DOCENTE como descripción académica; obligatoriedad técnica PENDIENTE_CONFIRMACION.** Las notas describen una lista enlazada simple con referencias al inicio y al final (N, pág. 25). La misma página contiene pseudocódigo incompleto o internamente inconsistente para insertar y buscar prioridades. Se documenta la sugerencia, pero no se la convierte en una orden de reemplazo de la estructura actual.

### Prioridad basada en tamaños

**RD-04 — `AssignPriority()` por tercios. CONFIRMADO_POR_DOCENTE.** En la página 24, el algoritmo inicializa `A` y `B` con el tamaño del primer elemento de `TL`, recorre `N` elementos y actualiza mínimo y máximo usando comparaciones estrictas; calcula `H = (B - A) / 3`. En la segunda pasada asigna prioridad 1 si `Size < A + H`; prioridad 2 si no se cumplió lo anterior y `Size < A + 2H`; en los demás casos asigna prioridad 3. Se respetan literalmente los operadores `<` del pseudocódigo (N, pág. 24).

Ejemplo verificable: con tamaños mínimo `A=12` y máximo `B=96`, `H=28`. Tamaños 12 y 39 reciben 1; 40 y 67 reciben 2; 68 y 96 reciben 3. El ejemplo solo valida los umbrales del algoritmo; no define qué procesos pertenecen a `TL`.

**RD-05 — Rangos dinámicos. CONFIRMADO_POR_DOCENTE como comportamiento explicado.** La transcripción explica que la llegada de un proceso mayor puede cambiar el rango y reclasificar como pequeño un proceso que antes estaba en una banda superior (T, 00:11:02–00:12:25 y 00:12:50–00:14:22). No precisa el conjunto de procesos incluido, el instante exacto de recálculo ni el tratamiento del proceso RUN. Esos detalles quedan pendientes.

**RD-06 — Precedencia de las clases 1–3. CONFIRMADO_POR_DOCENTE.** El algoritmo asigna 1 a tamaños pequeños, 2 a medianos y 3 a grandes (N, pág. 24); el procedimiento `ObtenerMaximaPrioridad()` recorre la lista y conserva el valor mayor (N, pág. 25). Por tanto, en el algoritmo presentado P3 tiene precedencia sobre P2 y P1. Lo único pendiente de esta escala es la decisión de adoptarla o adaptarla a la política de equipo 1–10 (DP-01); no está pendiente cuál de P1/P2/P3 es mayor en el algoritmo docente.

**RD-07 — Caso de tamaños iguales. PENDIENTE_CONFIRMACION.** Aplicando literalmente `A=B`, `H=0`; ningún tamaño cumple `Size < A`, así que el pseudocódigo asigna 3 a todos. La fuente no comenta explícitamente este caso ni confirma que ese resultado sea el deseado. El caso se registra como consecuencia mecánica ilustrativa, no como decisión de política.

### Cuota, CPU y time-sharing

**RD-08 — Cuota como atributo. CONFIRMADO_POR_DOCENTE.** La cuota de tiempo en milisegundos figura como atributo del proceso (N, pág. 23); la explicación relaciona el tamaño/prioridad con cuánto tiempo se entrega a cada proceso (T, 00:06:29–00:07:45 y 00:09:00–00:09:28).

**RD-09 — Cuota de ejemplo `prioridad × 10 ms`. PROPUESTA_DOCENTE.** La transcripción ilustra multiplicar las clases 1, 2 y 3 por diez milisegundos, lo que produce 10, 20 y 30 ms en ese ejemplo (T, 00:11:50–00:12:07). También menciona duraciones ilustrativas distintas durante la explicación del diagrama (T, 00:06:29–00:07:37). No se afirma que exista una fórmula obligatoria ni una unidad distinta de milisegundos.

**RD-10 — CPU lógico único y turnos. CONFIRMADO_POR_DOCENTE.** La explicación toma un procesador y varios procesos; el gestor divide el uso de CPU en porciones, retira el proceso al acabar su tiempo y atiende otro. El profesor aclara que esto es concurrencia percibida, no ejecución paralela simultánea en el CPU lógico descrito (N, pág. 28; T, 00:05:19–00:07:45). No exige hilos reales en el programa.

**RD-11 — Timeline temporal. CONFIRMADO_POR_DOCENTE para mostrar turnos y duración relativa.** La página 28 contrasta el procesamiento serial con el time-sharing mediante segmentos coloreados sobre eje de tiempo; segmentos de distinta anchura representan turnos de distinta duración (N, pág. 28; T, 00:07:14–00:07:50). El formato concreto de ejes, escala, persistencia, CPU ocioso y desplazamiento no está definido.

**RD-12 — Interfaz conceptual. CONFIRMADO_POR_DOCENTE como orientación visual.** La transcripción describe procesos que llegan, se cargan en memoria, se ejecutan en procesador y muestran un temporizador según el tiempo asignado (T, 00:08:25–00:09:28). El diagrama de aplicación conecta lista/cola, memoria y procesador (N, pág. 29). La fuente no especifica toolkit gráfico ni disposición final.

### Estados y flujo

**RD-13 — Estados nombrados. CONFIRMADO_POR_DOCENTE.** Las notas enumeran `READY`, `WAIT`, `RUN`, `CANCEL` y `FINISH` (N, pág. 23). El diagrama de estados de la página 27 es una referencia visual y no define por sí solo todas las transiciones del time-sharing.

**RD-14 — Bloqueo y retorno. CONFIRMADO_POR_DOCENTE a nivel conceptual; regla temporal exacta PENDIENTE_CONFIRMACION.** El estado `WAIT` aparece en las notas y el diagrama de estados (N, págs. 23 y 27), pero las fuentes suministradas no establecen el origen del bloqueo, duración ni todas las flechas necesarias. No se debe inferir que una flecha ausente prohíba una transición.

## Fuera de alcance

La documentación del proyecto no debe derivar requisitos de los ejercicios ajenos de descomposición QR, multiplicación de matrices, speedup ni grafos de dependencia que aparecen en otras secciones del material. No se copiaron sus ejemplos ni código.
