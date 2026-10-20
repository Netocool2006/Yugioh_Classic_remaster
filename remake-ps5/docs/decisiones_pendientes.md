# Decisiones pendientes

Las fallas que necesitan que decidas una regla o un diseño cuentan como **DECISIÓN** en los ciclos de pruebas
(`cu::decision` en `src/pruebas/cu_back.h`) y no como FALLA. Hoy no queda ninguna abierta.

## Resueltas el 20-10-2026 (cartas extra de los mazos de torneo)

El usuario confirmó lo que ya hace el juego; no hubo que cambiar nada:
1. **Foolish Burial**: se queda **limitada (1)**.
2. **Il Blud en el cementerio**: cuenta como monstruo de efecto (regla oficial).
3. **Yamato-no-Kami**: destruye una magia/trampa solo cuando destruye en batalla atacando (se deja así).
4. **Izanagi y Yamato-no-Kami** (2008): se quedan; son de mecánica clásica, sin Sincronía, Xyz, Péndulo ni Link.
5. **Assets**: las caras y listas de las 10 cartas ya están copiadas a `classic-assets` (respaldo de las listas
   anteriores en `D:\Docker\compartido\respaldo-assets\antes_torneo_extras_20261020`).

## Resueltas

### Mazos de torneo (rama classic-mazos-torneo) — RESUELTAS (rama classic-torneo-extras)

1. **Cuánto sale un mazo de torneo** (40 % genéricos de nivel 7-8, 30 % personajes, 40 % desde el nivel 7): se dejan.
2. **Personajes de nivel 5-6 con mazo de torneo**: se quedan así.
3. **Joey (nivel 4) con Gearfried y Equipos**: hecho (`A|joey|gearfried_equipo`; `CU-BE-TORNEO-JOEY`).
4. **Chaos Control sin Chaos Emperor Dragon**: se deja así (fiel a la banlist Goat del juego).
5. **Las 10 cartas que faltaban** (Command Knight, Cyber Dragon, Foolish Burial, Goblin Zombie, Il Blud, Magical
   Mallet, Izanagi, Yamato-no-Kami, Obnoxious Celtic Guard, Drillroid): agregadas como cartas extra con datos ES/EN,
   arte de Gemini, caras y efecto en el motor (regla Géminis nueva para Il Blud); las listas de torneo ya son exactas
   (`CU-BE-TORNEO-EXACTAS`, `CU-BE-EXTRA-*`). Marshmallon y Blue-Eyes Ultimate Dragon siguen siendo cartas firma.
6. **Mazos de torneo para el jugador**: en el constructor, Triángulo en MIS MAZOS abre **MAZOS DE TORNEO**; se copian
   a tus mazos para usarlos o editarlos (`CU-FE-CONS-TORNEO-*`).

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
