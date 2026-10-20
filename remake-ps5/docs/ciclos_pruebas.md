# Ciclos de pruebas (rama classic-ciclos)

Cada ciclo es `src/pruebas/ciclo_completo.sh` corrido completo desde cero dentro del contenedor `arch-ps5`. Compila
el juego, borra y recompila cada prueba, corre la batería, la certificación y los casos de uso (`docs/casos_de_uso.md`).
Los logs de cada ciclo quedan en `D:\Docker\compartido\wt\ciclos_logs\cicloN\` (`resumen.txt` y `resultados.tsv`).
Los casos marcados DECISIÓN no cuentan como falla (`docs/decisiones_pendientes.md`).

## Ciclo 1 (20-10-2026, 07:00 a 07:21)

**7008 casos: OK 6956, FALLA 50, DECISIÓN 2.**

Fallas del juego (6) y lo que se hizo:

| caso | causa | arreglo |
|---|---|---|
| `CU-FE-OPC-PREGUNTAR-EFECTO` | "Preguntar antes de activar" se guardaba, pero el duelo no la leía en ningún lado (Siempre y Nunca hacían lo mismo que Auto) | Nunca: el duelo pasa solo cuando le pregunta al jugador (`duelo_juego.h`). Siempre: el motor le ofrece al jugador cada ventana de respuesta aunque no tenga nada que activar (`Duelo::preguntarSiempre`, `ofrecer` en `ia.h`; la IA humana no lo usa al imaginar) |
| `CU-CARTA-cave_dragon-INVOCAR`, `CU-CARTA-terrorking_archfiend-INVOCAR` | estaban en la lista "no invocables" de `rivales.txt` (línea N), pero su texto solo pone una condición (Cave Dragon: sin otros monstruos; Terrorking: con un Archfiend). Nunca se podían invocar de modo normal | se sacaron de la lista N; la condición ya la revisaba `motorPuedeInvocar` |
| `CU-BE-ATAJO-FAIRY-GUARDIAN` | devolvía la última magia del cementerio, no la que mandó el rival | el motor anota qué magias mandó el rival al cementerio en el turno (`ef.magiasPerdidas`); Fairy Guardian elige entre esas |
| `CU-BE-ATAJO-PITCH-BLACK` | solo pasaba contadores a monstruos propios y al primero | pasa a cualquier monstruo boca arriba que use contadores de magia, de los dos lados, y elige quien la activa (la IA, uno suyo) |
| `CU-BE-ATAJO-LEVEL-LAB` | elegía sola el monstruo de más nivel | lo elige quien la activa (pregunta del motor); la IA sigue eligiendo el de más nivel |

Otras fallas:

- 43 por el runner: a las pruebas se les pasaba la carpeta de cartas sin la barra final (`/tmp/ciclo/ui`) y no leían
  nada (prueba_constructor, prueba_duelo, prueba_variedad, prueba_ia_humana y prueba_audio, más las hojas que cubren).
  La prueba estaba mal: se arregló `ciclo_completo.sh`.
- 1 hoja sin prueba: `CU-BE-PREGUNTAR-SIEMPRE`. Se escribió su prueba junto con el arreglo.

Antes del ciclo 1, al armar las pruebas, se corrigieron pruebas mal planteadas (no el juego): el id de Blue-Eyes es
`blue_eyes`; la fase final pasa sola al turno siguiente si no hay nada que hacer; Toon Alligator es un monstruo normal;
Tsukuyomi se pone boca abajo con su propio efecto; la cadena se resuelve dentro de la misma jugada si nadie más
responde; `estadisticas::leer` reinicia los datos antes de abrir (la prueba le pasaba una referencia a su propia ruta).
También se corrigió el id de Blue-Eyes en la prueba de capturas del resultado (`resultado_juego.h`, solo PC).

## Ciclo 2 (20-10-2026, 07:32 a 07:54)

**7008 casos: OK 7006, FALLA 0, DECISIÓN 2.** Se terminó ahí.

- Compilación del juego (PS5 y PC), las 9 pruebas de siempre, la certificación y los casos de uso: todo con rc 0.
- Certificación: 1385 cartas con efecto, OK 1385, FALLA 0, SIN PRUEBA 0.
- Carta por carta: 1774 DATOS, 1774 CARAS, 1094 INVOCAR, 680 COLOCAR, 1385 EFECTO y 25 CAMPO, todas OK.
- Las 2 DECISIÓN (repetición del ataque e Inferno Tempest) están en `decisiones_pendientes.md`.
- No hizo falta arreglar nada más.
