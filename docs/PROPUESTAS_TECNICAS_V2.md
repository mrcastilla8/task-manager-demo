# Propuestas técnicas v2 pendientes de ratificación

**Estado:** documento de análisis; ninguna opción de esta página está aprobada. No modifica DP-01–DP-11 ni vuelve operativos `contracts/v2/`. La recomendación preferida sirve para orientar revisión, no para congelar API ni iniciar migración.

## 1. Ejecución continua sin bloquear Qt

### Hallazgo contractual

La interfaz candidata expone `iniciar()`, `pausar()` y `siguientePaso()` (que exige PAUSADO), pero no una operación que avance una unidad mientras EJECUTANDO ni un mecanismo de continuación. Un motor lógico síncrono no puede ejecutar un bucle continuo dentro de `iniciar()` sin bloquear el hilo que lo llama. DP-07 solo aprueba que el modo continuo aplique sucesivamente las mismas unidades lógicas; no el mecanismo de conducción.

### Alternativa A — Driver cooperativo con timer de Qt (**preferida**)

Un controlador Qt mantiene un `QTimer` en el hilo UI. `iniciar()` cambia el estado a EJECUTANDO y activa el timer; cada tick pide exactamente una unidad lógica rápida al motor; `pausar()` detiene el timer en el siguiente límite seguro. Esa unidad puede ser una ráfaga acotada a cuota/WAIT/terminal o un salto idle, según DP-07. El motor lógico sigue síncrono y sin threads; solo devuelve control al loop de eventos entre unidades.

La API actual no da un método para ese tick mientras EJECUTANDO. Requeriría ratificar una de estas expresiones equivalentes de contrato: un `avanzarUnidad()` reservado al estado EJECUTANDO, o ampliar la precondición de `siguientePaso()` y precisar su semántica en ambos estados. No cambiar firmas hasta ratificación. No se requiere que `IExecutionEngine` conozca Qt; un driver GUI invoca la interfaz.

**Impacto:** pequeño cambio de API; motor lógico implementa unidad atómica existente; motor concurrente adapta su coordinador al mismo avance; Qt controla cadencia sin tocar reloj; mocks modelan inicio/tick/pausa. Una ráfaga tiene tope de 30 ms simulado, pero su coste de CPU real debe medirse para proteger la respuesta UI.

**Pruebas:** cada tick avanza una unidad y retorna al event loop; pausa impide el tick siguiente; reanudar continúa el mismo reloj; ticks repetidos equivalen a llamadas manuales a `siguientePaso()`; un timer lento/rápido cambia cadencia visual, no tiempo por unidad; prueba de latencia UI con CPU y WAIT; errores del tick no dejan timer activo.

### Alternativa B — Continuador cooperativo inyectado

El host entrega al motor o a un pequeño driver una función para encolar la continuación en su propio loop de eventos. `iniciar()` arma una unidad y encola la siguiente solo después de que la actual retorna; `pausar()` invalida cualquier continuación pendiente. El host Qt puede implementar el encolado con `QMetaObject::invokeMethod`/timer; otro host usa su loop sin dependencia de Qt. El motor sigue sin thread y no bloquea la UI.

Requiere una abstracción de scheduler/callback en construcción o configuración, o un driver propietario externo, además de una regla para cancelar continuaciones en reinicio/destrucción. Es más reutilizable que ligar Qt al motor, pero amplía puntos de vida útil y reentrada; callback duplicado o tardío podría avanzar una sesión nueva si no se invalida.

**Impacto:** mayor superficie de API y ownership; ambos motores deben honrar callback exactamente una vez por unidad; GUI y mocks prueban loop y cancelación de callback; la dependencia de Qt queda fuera del motor si el continuador es genérico.

**Pruebas:** callback diferido/no reentrante; un único callback pendiente; pausa/reinicio/destrucción invalidan callbacks viejos; sin avance si callback se ejecuta dos veces; equivalencia por unidad con `siguientePaso()`; respuesta UI y cancelación de timer.

### Recomendación

Ratificar Alternativa A con un driver fuera del motor y el método/semántica de avance correspondiente. Tiene menos infraestructura, conserva el motor lógico determinista y hace explícito el límite cooperativo que mantiene Qt disponible. Antes de ratificar, decidir si la interfaz común expresa el tick o si un adaptador común puede hacerlo sin añadir una virtual. La alternativa B queda como opción si se requiere desacoplar la cadencia del host.

## 2. Errores: excepciones frente a códigos

`Resultado`, `CodigoResultado` y `ResultadoPaso` en `contracts/v2/IExecutionEngine.h` son únicamente una propuesta técnica. DP-01–DP-11 no aprueban el mecanismo. Afecta todas las operaciones que pueden fallar y por tanto la interfaz completa, no solo cancelación.

| Tema | Excepciones | `Resultado`/códigos | Consecuencia a decidir |
|---|---|---|---|
| `agregarProceso` | Puede preservar retorno `void` v1 y lanzar para argumento/PID/memoria inválidos. | Retorna éxito o códigos distintos para argumento, PID duplicado y memoria. | Tipo de retorno cambia y cada caller debe comprobarlo. |
| `cancelarProceso` | Lanza si PID desconocido/estado terminal, o define no-op/idempotencia aparte. Una excepción sola no distingue terminales salvo tipos/categorías propios. | Códigos pueden distinguir desconocido, FINISH y CANCEL (`PID_INEXISTENTE`, `PID_FINALIZADO`, `PID_YA_CANCELADO`). | Ratificar la semántica y si repetición retorna éxito o código terminal; no es DP-06. |
| `iniciar`, `pausar`, `reiniciar` | Precondiciones y recursos fallan por excepciones; idempotencia puede ser no-op. | `ESTADO_INVALIDO`/código de recursos devuelto en cada método. | Firma/flujo de control distinto en ambos motores. |
| `siguientePaso` | Devuelve datos del paso o lanza por no estar PAUSADO. | `ResultadoPaso` mezcla código con avance/CPU/tiempo. | Debe definirse qué campos valen en error y cómo evitar resultados parciales. |
| Motores | Excepciones cruzan fronteras de llamadas; concurrente debe capturarlas en el coordinador y transportar causa sin terminar worker. | Mismo enum facilita equivalencia; todo fallo debe mapearse y el worker nunca debe dejar resultado sin notificar. | Frontera ABI/threads y propagación a UI. |
| GUI y mocks | GUI usa try/catch; mocks pueden lanzar el mismo tipo y probarlo. | GUI hace switch exhaustivo; mocks fabrican cada código y requieren actualizar expectativas. | Cambian clientes, mocks y harness de pruebas. |
| Recursos del sistema | `std::bad_alloc` y fallos no recuperables se propagan naturalmente. | Normalmente también se propagan, salvo que se prometa mapearlos. | No ocultar fallos de recursos como error funcional común. |

La opción de códigos hace más explícita la respuesta esperada y reduce excepciones de política en el coordinador; exige enum estable/exhaustivo, estados de salida inequívocos y cambios de todos los callers. Las excepciones preservan el estilo actual de v1 y propagación, pero requieren catálogo de excepciones/garantías para clientes concurrentes. **No hay decisión preferida ratificada.** Toda la sección de errores y firmas de retorno de `IExecutionEngine` depende de esa elección. Mantener los tipos actualmente escritos en el header solo como borrador, no congelados.

## 3. Cancelación RUN parcial

### Auditoría de `origin/angel` (solo lectura)

`ProcessThread::ejecutarLoopWorker()` calcula de una vez `min(cuotaMs, tiempoRestante)`, asigna el motivo y notifica el turno; no consulta un token de cancelación ni emite subpasos durante CPU. `ConcurrentEngine::ejecutarPasoInterno()` suelta `mutexEstado_` mientras espera el resultado y luego aplica toda la duración, construye un único bloque y procesa el motivo. `cancelarProceso()` puede adquirir ese mutex durante la espera, marcar CANCEL, liberar memoria y llamar `solicitarDetencion()`; el coordinador descarta después la ráfaga al encontrar CANCEL. Si el coordinador obtiene primero el mutex y procesa FINISH, la cancelación posterior ve terminal. `solicitarDetencion()` detiene el loop del worker, pero no interrumpe la ráfaga que ya se calculó. Los hilos no usan tiempo de pared para calcular CPU, pero la carrera de locks decide qué resultado gana y hoy no hay contabilidad parcial reproducible.

Esto describe la rama auditada, no afirma el comportamiento de pruebas ejecutadas ni cambia esa implementación. El tiempo de 10 ms del coordinador es pacing físico y no puede ser una frontera semántica de cancelación.

### Reglas ya aprobadas por DP-06

Se permite cancelar READY/RUN/WAIT; FINISH/CANCEL son terminales; desde RUN se conserva solo el CPU simulado realmente completado; registrar tiempos/timeline solo si hubo duración positiva; el reloj físico no mide CPU; efectos de memoria/eventos deben ocurrir una vez. DP-06 no define granularidad del punto seguro, ganador de carrera con FINISH ni códigos/idempotencia.

### Protocolo técnico propuesto (por ratificar)

1. El coordinador es dueño exclusivo del estado lógico. GUI/productores encolan una orden de cancelación con PID y ordinal monotónico de comando; no mutan `Proceso`, memoria o timeline desde el hilo llamante. La llegada física decide a qué checkpoint se encola, pero no se convierte en milisegundos simulados; una vez aceptada, la secuencia de comandos con ordinal puede reproducirse exactamente.
2. Representar el trabajo de CPU como unidades enteras de 1 ms simulado, o como checkpoints equivalentes al límite de la resolución contractual. Tras cada unidad, el coordinador confirma contadores y revisa órdenes pendientes. El worker puede calcular/esperar trabajo, pero no publica estado compartido ni usa duración física como avance.
3. Antes del primer ms, drenar órdenes ya encoladas: cancelar un READY/RUN aún sin CPU lleva a CANCEL con delta cero, sin bloque timeline y sin evento CPU con duración positiva. Publicar CANCEL y liberar memoria una vez.
4. Si la orden llega después de `n` unidades confirmadas, cerrar CPU como `[inicio,inicio+n)` con PID positivo, actualizar restante/consumido/reloj en `n`, y luego CANCEL al timestamp final. Para `n=0` no añadir bloque. No interpolar a partir de wall clock.
5. Resolver cada frontera con orden fijo: primero confirmar el CPU ejecutado hasta `t`; luego resolver condición intrínseca alcanzada (CPU agotada→FINISH; umbral E/S→WAIT; cuota agotada→READY y ticket nuevo); después procesar cancelaciones ordenadas por ordinal contra el estado ya resultante; luego agrupar/publicar reclasificación, eventos y snapshot antes del siguiente despacho. Así, FINISH exactamente en `t` gana y la cancelación devuelve ya finalizado; cuota agotada en `t` produce READY y la orden cancela ese estado antes de otro despacho. Si el equipo prefiere CANCEL antes que READY al mismo límite de cuota, debe ratificar otra precedencia explícita.
6. Al decidir un terminal bajo el lock lógico único, marcar terminal una vez, retirar de activos, liberar memoria una sola vez y emitir un único evento terminal. Solicitudes posteriores no duplican evento/liberación; respuesta funcional (excepción/código/no-op) depende de la decisión de errores.
7. Ambos motores consumen la misma secuencia de unidades/checkpoints y el mismo orden de frontera. El motor concurrente serializa los resultados de workers por ordinal de turno/unidad; nunca deja que la planificación del SO defina la precedencia lógica.

**Coste/precisión:** el checkpoint de 1 ms da cancelación exacta a esa resolución y cuadra con los milisegundos enteros del contrato, pero aumenta sincronización; la prueba debe medir coste de unidad y respetar determinismo. Una granularidad mayor reduce costo y también resolución, lo cual requiere decisión funcional adicional. No incorporar un campo/token nuevo en headers antes de elegir si el ordinal de comando vive en el controlador, engine o snapshot.

**Interacción de API también por ratificar:** la propuesta encola el comando, pero el `cancelarProceso(int)` candidato luce síncrono. Esperar hasta el checkpoint y devolver estado final puede bloquear al hilo Qt; llamar solo a un hilo de coordinación evita el bloqueo, aunque exige que el GUI reciba el desenlace después. Alternativa preferida para GUI: separar solicitud no bloqueante (aceptación/ID de orden) del resultado terminal comunicado por evento/snapshot. Alternativa compatible con firma síncrona: documentar que solo puede llamarse fuera del hilo UI y esperar el próximo checkpoint. La primera cambia método/tipos y mocks; la segunda limita a los clientes cuándo pueden llamar. Ambas deben compartir la misma semántica de checkpoint y errores.

**Pruebas de propuesta:** cancel antes de CPU (delta 0, no bloque); tras 3 ms de cuota (un bloque de 3 ms); exactamente en límite de cuota (READY/ticket y CANCEL sin siguiente turno); CPU restante cero contra cancel al mismo tiempo (FINISH una vez); repetición de órdenes; memoria/eventos una vez; cancelación desde WAIT/READY; repetir secuencia de comandos en ambos motores y comparar reloj, CPU, estado, eventos y timeline byte a byte.

## 4. Orden y representación de eventos

### Auditoría de los tipos actuales

`Evento` tiene `tiempoSimuladoMs`, `tipo`, `pid`, `duracionMs` y texto `detalle`, pero carece de secuencia y no declara si el timestamp de `CPU_EJECUTADA` es inicio o fin. `EstadoSistema` conserva un vector de eventos y reloj final, pero no define orden para timestamps iguales ni watermark incremental. Los tipos incluyen `READY_A_RUN`, `CPU_EJECUTADA`, transiciones de fin de turno y marcadores idle, suficientes como vocabulario inicial pero no como protocolo de replay completo.

En `origin/angel`, los eventos se anexan a un vector bajo `mutexEstado_` con el reloj actual; RUN se registra antes de ceder lock, CPU y timeline se aplican después de retornar el worker, y la transición se publica a continuación. El vector conserva orden de append en una ejecución, pero no existe secuencia contractual, y cancelación concurrente puede insertar CANCEL antes de que el coordinador resuelva resultado. No se debe interpretar wall-clock del thread como orden simulado.

### Propuesta (por ratificar; no cambia headers)

- Total order por `secuencia` estrictamente creciente de sesión; almacenar vector ya ordenado por secuencia. El tiempo es un segundo eje, no desempate. Eventos con mismo timestamp se reproducen en secuencia.
- Para transiciones de estado, `tiempoSimuladoMs` denota el instante frontera. `duracionMs=0` salvo un evento que represente intervalo. `CPU_EJECUTADA` usa timestamp como **inicio** y duración positiva `d`; define intervalo semiabierto `[t,t+d)`. Reloj resultante es `t+d`. No emitir evento CPU de duración cero.
- Orden en frontera `t`: órdenes de CPU que cierran intervalo (inicio `t`, si aplica); causa/transition del proceso ejecutor; despertares WAIT con vencimiento `t` ordenados por ticket al bloquear y luego PID; admisiones/comandos pendientes por ordinal; lote de cambios de prioridad; actualización de READY/tickets; snapshot. Este orden se debe ajustar a la precedencia cancelación/FINISH propuesta en sección 3; ningún elemento se supone ya aprobado.
- Replay: fold de estados por PID desde `PROCESO_ADMITIDO`, `READY_A_RUN` y transiciones; `READY_A_RUN` abre RUN en `t`; `CPU_EJECUTADA` agrega el intervalo y mantiene RUN durante `[t,t+d)`; evento de fin cierra RUN en el límite `t+d`. `detalle` queda solo para presentación y no es fuente de estado. Un snapshot se captura atómicamente en una frontera, es autoritativo al `tiempoSimuladoMs` y su `eventos` es el prefijo completo de sesión ordenado por secuencia; la última secuencia del vector es su watermark sin añadir otro campo. Al cruzar huecos, la GUI infiere idle según DP-08a. Eventos y snapshot con mismo timestamp se alinean mediante secuencia, no por reloj solamente.
- Marcadores CPU idle son explicativos y no asignan proceso (`pid=-1` o PID opcional vacío, según representación final); no crean `BloqueTimeline`. La GUI puede reconstruir los huecos aun si esos marcadores faltan.

### ¿Hace falta un campo nuevo?

Se propone añadir `std::uint64_t secuencia` a `Evento`, asignada por el coordinador serial. Timestamp no ordena dos eventos simultáneos, y orden vectorial por sí solo se pierde al filtrar, combinar o entregar deltas a UI; el número estable resuelve ese caso y permite un replay incremental sin basarse en locks/wall-clock. La secuencia no debe envolver: al agotarse, detener publicación de nuevos eventos con fallo explícito según la política de errores ratificada; no reutilizar IDs dentro de una sesión. No se propone campo de secuencia adicional en `EstadoSistema`: como el contrato actual mantiene historial completo ordenado, el vector da el watermark del snapshot. Si se migra a entrega por deltas, revisar entonces un `ultimaSecuenciaEvento`; no añadirlo por adelantado. No agregar `inicioMs`/`finMs` a `Evento` porque la pareja existente tiempo-inicio + duración ya representa `[t,t+d)` si se ratifica esa semántica.

**Pruebas de propuesta:** eventos de igual timestamp reproducen mismo orden con almacenamiento distinto; secuencias estrictamente crecientes incluso tras cancelar/reiniciar (reinicio define si resetea sesión); reconstrucción de RUN transitorio antes/durante/después de ráfaga; CPU parcial, WAIT simultáneos, cancel/FINISH en la misma frontera; timeline es igual al replay y solo tiene PID>0/duración positiva; snapshot final y replay coinciden.

## Decisiones necesarias antes de congelar API v2

Ratificar continuidad A/B y forma de avance; errores por excepciones o resultado; protocolo/checkpoint y precedencia de cancelación; timestamp/secuencia de evento y orden del replay. Mientras tanto, headers y documentos de contrato son candidatos de diseño, no API aprobada ni adopción definitiva.
