# Decisiones pendientes

Las fallas que necesitan que decidas una regla o un diseño cuentan como **DECISIÓN** en los ciclos de pruebas
(`cu::decision` en `src/pruebas/cu_back.h`) y no como FALLA. Hoy no queda ninguna abierta.

## Resueltas

### 1. Repetición del ataque: elegir el objetivo (`CU-BE-REPETICION-ATAQUE-ELEGIR`) — RESUELTA (opción B, regla oficial)

**Decisión del usuario:** respetar la regla oficial (opción B). Rama `classic-reglas-oficiales`.

**Ahora.** Si después de declarar un ataque cambia el número de monstruos del rival (por ejemplo, Scapegoat), aparece
un monstruo ante un ataque directo o el objetivo ya no está, el duelo le pregunta al atacante (pregunta del motor
`P_ELEGIR`, texto `ef.elegir.repeticion`): otro objetivo válido, el mismo (va primero) o "No atacar". Si el rival se
quedó sin monstruos, se ofrece "Ataque directo". Elegir un objetivo es una declaración nueva (el rival puede volver a
responder). "No atacar" deja al monstruo volver a atacar en esa fase. Si el número no cambia (Waboku), el ataque sigue.
El jugador lo ve con el foco (círculo: no atacar) o con una lista ("Ataque directo", "No atacar"); la IA clásica y la
humana (también en sus simulaciones) responden con su política de ataque. Casos: `CU-BE-REPETICION-ATAQUE`,
`-ELEGIR`, `-DIRECTO`, `-IA` y `CU-FE-DUELO-REPETICION`.

### 2. Inferno Tempest: el momento de la activación (`CU-BE-ATAJO-INFERNO-TEMPEST-MOMENTO`) — RESUELTA (opción B, regla oficial)

**Decisión del usuario:** respetar la regla oficial (opción B). Rama `classic-reglas-oficiales`.

**Ahora.** Justo después de aplicar daño (de batalla o de efecto) se abre una ventana (`V_DANNO`) solo para quien lo
recibió y solo si tiene una carta "cuando recibes daño" que pueda activar. Inferno Tempest (3000 o más de daño de
batalla de un ataque), Attack and Receive y Numinous Healer solo se pueden activar en esa ventana; después, ya no. Se
revisó el catálogo: no hay más cartas con ese momento (Trap of Board Eraser responde a la activación, Nutrient Z y
Kuriboh van en el cálculo, Dark Room of Nightmare es continua). "Preguntar antes de activar" se respeta (Nunca: pasa
solo; Auto y Siempre: pregunta, porque la ventana solo se abre si hay una carta para ella). Casos:
`CU-BE-ATAJO-INFERNO-TEMPEST`, `CU-BE-ATAJO-INFERNO-TEMPEST-MOMENTO`, `CU-BE-VENTANA-DANNO*` y
`CU-FE-DUELO-VENTANA-DANNO`.
