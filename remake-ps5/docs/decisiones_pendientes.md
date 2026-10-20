# Decisiones pendientes (rama classic-ciclos)

Estas fallas no se arreglaron solas porque necesitan que decidas una regla o un diseño. En los ciclos de pruebas
cuentan como **DECISIÓN** (`cu::decision` en `src/pruebas/cu_back.h`) y no como FALLA. El juego hoy hace lo que dice
"Hoy"; la prueba comprueba ese comportamiento, que está documentado.

## 1. Repetición del ataque: elegir el objetivo (`CU-BE-REPETICION-ATAQUE-ELEGIR`)

**Contexto.** Cuando, después de declarar un ataque, cambia el número de monstruos del rival (por ejemplo, el rival
activa Scapegoat), la regla dice que hay "repetición del ataque": el atacante puede elegir otro objetivo, el mismo o no
atacar.

**Hoy.** Si el objetivo sigue en el campo, el ataque sigue solo contra él. Si el objetivo ya no está (o era un ataque
directo y aparecieron monstruos), el ataque se cancela y el monstruo puede volver a atacar (`repetirAtaque`). Es un
atajo anotado en `docs/efectos.md` y en `certificacion_notas.md`.

**Opciones.**
- A. Dejarlo así. Casi siempre el jugador volvería a atacar al mismo monstruo, así que el resultado suele ser igual.
- B. Implementar la regla completa: al cambiar el número de monstruos, el duelo le pregunta al atacante (con una
  pregunta del motor, `P_ELEGIR`: los objetivos posibles y "No atacar"). La IA elige con su política de ataque.

**Recomendación.** B, en una rama aparte, porque toca la máquina de estados de la batalla (`combate`, `PEND_BATALLA`)
y conviene correr la batería completa después. Si prefieres no arriesgar la batalla antes de validar en la PS5, A.

## 2. Inferno Tempest: el momento de la activación (`CU-BE-ATAJO-INFERNO-TEMPEST-MOMENTO`)

**Contexto.** Inferno Tempest dice "actívala solo cuando recibas 3000 o más de daño de batalla". Es uno de los 4
atajos que dejó `control_final.md`.

**Hoy.** Se puede activar en cualquier ventana del mismo turno después de ese daño (por ejemplo, en la fase final del
rival). Funciona igual en Attack and Receive y Numinous Healer ("cuando recibes daño"). El motor no abre ninguna
ventana justo después del daño de batalla: el combate sigue hasta el siguiente momento en que alguien puede responder.

**Opciones.**
- A. Dejarlo así: el efecto es el correcto y solo el momento es más amplio.
- B. Agregar al motor una ventana de respuesta justo después del daño (de batalla y de efecto), solo para el que lo
  recibió y solo si tiene una de estas cartas, y que estas tres cartas solo se puedan activar en esa ventana.

**Recomendación.** B, en una rama aparte, porque la ventana nueva está en medio del cálculo de daño (`calculoDanno`,
`PEND_CALCULO`) y afecta a las tres cartas. Los otros 3 atajos de `control_final.md` (Fairy Guardian, Pitch-Black
Power Stone y Level Conversion Lab) tenían un arreglo claro y se arreglaron en esta rama (ver `ciclos_pruebas.md`).
