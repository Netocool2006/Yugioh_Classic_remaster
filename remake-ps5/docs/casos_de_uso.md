# Mapa de casos de uso (Yugioh Classic Remaster, PPSA77002)

Rama `classic-ciclos`. Este es el árbol de lo que el juego tiene que hacer, del front (lo que el jugador ve y toca) al
back (las reglas, la IA, el guardado) y carta por carta. Tiene tres niveles: **caso** (`##`), **subcaso** (`###`) y
**sub-subcaso**. Las hojas son las líneas que empiezan con un ID entre comillas invertidas (`` `CU-...` ``).

Cada hoja tiene al menos una prueba automática:

- `src/pruebas/prueba_casos_uso.cpp`, con `cu_juego.h` (el juego real sin GPU: las pantallas de verdad y maquetas
  solo de Vulkan, sonido y figuras), `cu_front.h`, `cu_back.h` y `cu_cartas.h`;
- o una prueba de siempre (`src/pruebas/casos_suites.txt` dice cuál: la hoja vale si esa prueba terminó sin fallas
  en el mismo ciclo).

`src/pruebas/ciclo_completo.sh` corre todo desde cero y revisa que cada hoja de este archivo tenga su resultado. Si
una hoja no tiene prueba, cuenta como FALLA. Los resultados de cada ciclo están en `docs/ciclos_pruebas.md`. Las
fallas que necesitan una decisión del usuario están en `docs/decisiones_pendientes.md`.

---

## CU-FE: front end

### CU-FE-ARRANQUE: el juego arranca

- `CU-FE-COMPILA-PS5`: compila para la PS5 y la vista previa de PC (`probar_rama.sh`)
- `CU-FE-ARRANQUE`: carga las cartas, los rivales, la historia y la colección
- `CU-FE-GUARDADO-RUTAS`: progreso y opciones van en la carpeta de datos del jugador
- `CU-FE-CONS-MAZOS-INICIO`: la primera vez se arman los mazos de inicio y uno queda activo
- `CU-FE-AUDIO-MEZCLA`: el mezclador toca la música y los efectos

### CU-FE-PORTADA: la portada

- `CU-FE-PORTADA-ARRANQUE`: sale al arrancar, desde negro y con la música del menú
- `CU-FE-PORTADA-ESPERA`: no acepta botones durante el fundido de entrada
- `CU-FE-PORTADA-X`: X suena, funde a negro y va al menú principal
- `CU-FE-PORTADA-OPTIONS`: OPTIONS también va al menú
- `CU-FE-PORTADA-SIN-PAUSA`: en la portada OPTIONS no abre la pausa

### CU-FE-MENU: el menú principal

- `CU-FE-MENU-BOTONES-5`: los 5 botones en orden (MODO DUELO, CONSTRUCCIÓN DE DECK, LISTA DE CARTAS, OPCIONES, SALIR)
- `CU-FE-MENU-MUSICA`: suena la música del menú
- `CU-FE-MENU-NAVEGAR`: arriba y abajo dan la vuelta; la cruz mantenida repite
- `CU-FE-MENU-DUELO`: MODO DUELO va a elegir rival
- `CU-FE-MENU-CONSTRUIR`: CONSTRUCCIÓN DE DECK abre el constructor con su música
- `CU-FE-MENU-LISTA`: LISTA DE CARTAS abre la colección en modo solo ver
- `CU-FE-MENU-OPCIONES`: OPCIONES abre las 11 opciones y círculo vuelve
- `CU-FE-MENU-SALIR`: SALIR pide confirmar; X otra vez cierra el juego
- `CU-FE-MENU-SALIR-CANCELA`: otro botón cancela la salida
- `CU-FE-MENU-SIN-PAUSA`: en el menú OPTIONS no abre la pausa

### CU-FE-OPC: Opciones (cada opción, su efecto, guardado y carga)

- `CU-FE-OPC-LISTA`: las 11 opciones en orden
- `CU-FE-OPC-RUTA`: `opciones.txt` está junto a `progreso.txt`
- `CU-FE-OPC-MUSICA`: volumen de la música de 0 a 10, al instante y guardado
- `CU-FE-OPC-EFECTOS`: volumen de los efectos, suena de prueba y se guarda
- `CU-FE-OPC-PROHIBIDAS`: permitir cartas prohibidas cambia el límite de la colección (3 o banlist Goat)
- `CU-FE-OPC-PREGUNTAR`: preguntar antes de activar recorre Auto, Siempre y Nunca y se guarda
- `CU-FE-OPC-PREGUNTAR-EFECTO`: el duelo obedece (Nunca: no se detiene; Auto: pregunta si hay algo que activar)
- `CU-BE-PREGUNTAR-SIEMPRE`: Siempre: el motor pregunta al jugador en cada ventana, aunque no tenga nada
- `CU-FE-OPC-VELOCIDAD`: Normal, Rápida y Muy rápida; el duelo la lee y el factor sube
- `CU-FE-OPC-ADAPTABLE`: dificultad adaptable en `variedad.txt`; apagada, la dificultad es 0
- `CU-FE-OPC-IA`: IA Clásica o Humana en `variedad.txt`
- `CU-FE-OPC-FIGURAS`: figuras 3D sí o no, guardado
- `CU-FE-OPC-IDIOMA`: español o inglés al instante y en `idioma.txt`
- `CU-FE-OPC-PERSONALIZACION`: abre Personalización
- `CU-FE-OPC-VOLVER`: Volver (X) o círculo guardan y vuelven
- `CU-FE-OPC-GUARDAR-CARGAR`: todo lo guardado vuelve igual al leer `opciones.txt`
- `CU-FE-OPC-CLAVES-AJENAS`: guardar no borra las claves de otras partes del juego
- `CU-FE-OPC-GUARDADO-ATOMICO`: no queda el `.tmp`

### CU-FE-PERSO: Personalización

- `CU-FE-PERSO-MENU`: su menú (Funda, Tapete, Controles, Estadísticas, Volver); círculo vuelve
- `CU-FE-PERSO-FUNDA`: elegir una funda libre la equipa y la guarda; la rejilla se recorre con la cruz
- `CU-FE-PERSO-FUNDA-BLOQUEADA`: una bloqueada no se equipa y avisa a quién hay que vencer
- `CU-FE-PERSO-TAPETE`: elegir un tapete libre; cada tapete tiene el escenario de su carta de campo
- `CU-FE-PERSO-TAPETE-BLOQUEADO`: uno bloqueado no se equipa
- `CU-FE-PERSO-CONTROLES`: las 3 pestañas (duelo, constructor, menús) con R1/L1 y sus filas
- `CU-FE-PERSO-ESTADISTICAS`: los duelos por rival, las cartas más jugadas y las firma desbloqueadas
- `CU-FE-PERSO-CAMARA`: la cámara elegida se guarda y se usa al arrancar
- `CU-FE-PERSO-GUARDAR-CARGAR`: funda y tapete vuelven al leer `opciones.txt`

### CU-FE-CONS: constructor de mazos y lista de cartas

- `CU-FE-CONS-MIS-MAZOS`: abre en MIS MAZOS
- `CU-FE-CONS-CERRAR`: círculo en MIS MAZOS vuelve al menú con su música
- `CU-FE-CONS-CREAR`: + Nuevo mazo con el teclado en pantalla
- `CU-FE-CONS-TECLADO`: el teclado escribe, borra, mayúsculas y acepta
- `CU-FE-CONS-BUSQUEDA`: buscar por nombre
- `CU-FE-CONS-FILTROS`: clase, subtipo, atributo, nivel, tipo, ATK, DEF, orden y quitar filtros
- `CU-FE-CONS-AGREGAR`: X agrega una copia
- `CU-FE-CONS-QUITAR`: cuadrado quita una copia
- `CU-FE-CONS-LIMITE-3`: no más de 3 copias y explica por qué
- `CU-FE-CONS-FUSION-EXTRA`: las fusiones van al extra
- `CU-FE-CONS-CLASE-RAPIDA`: L2/R2 cambian la clase rápida
- `CU-FE-CONS-STICK-DERECHO`: el stick derecho salta entre el mazo y la colección
- `CU-FE-CONS-VALIDAR`: un mazo inválido no se guarda
- `CU-FE-CONS-GUARDAR-SALIR`: círculo con cambios pregunta; Guardar y salir vuelve a la lista
- `CU-FE-CONS-USAR`: cuadrado lo hace el mazo de los duelos
- `CU-FE-CONS-COPIAR`: Copiar
- `CU-FE-CONS-RENOMBRAR`: Renombrar con el teclado
- `CU-FE-CONS-BORRAR`: Borrar pregunta antes
- `CU-FE-CONS-PROHIBIDAS`: la banlist Goat (prohibida, limitada, semilimitada) y la opción de permitirlas
- `CU-FE-CONS-FIRMA-BLOQUEADA`: una carta firma bloqueada no se agrega y dice a quién vencer
- `CU-FE-CONS-INICIO-VALIDOS`: los mazos de inicio son válidos y traen su carta de campo
- `CU-FE-CONS-LISTA-SOLO-VER`: la lista de cartas no agrega ni guarda
- `CU-FE-CONS-CARGAR`: los mazos vuelven igual al leerlos
- `CU-FE-CONS-MAZO-EN-DUELO`: el duelo roba del mazo activo, barajado
- `CU-FE-LISTA-CARTAS`: la lista de cartas muestra la colección y círculo la cierra

### CU-FE-GALERIA: galería de figuras 3D

- `CU-FE-GALERIA-ABRIR`: una entrada por figura, abierta desde la lista
- `CU-FE-GALERIA-ORDEN`: ordenadas por nombre
- `CU-FE-GALERIA-NAVEGAR`: la cruz mueve el cursor por la rejilla
- `CU-FE-GALERIA-ATACAR`: X hace atacar a la figura
- `CU-FE-GALERIA-BLOQUEADA`: una figura de carta firma bloqueada avisa
- `CU-FE-GALERIA-DETALLE`: triángulo abre el detalle
- `CU-FE-GALERIA-CERRAR`: círculo cierra

### CU-FE-RIVAL: elegir rival

- `CU-FE-RIVAL-HISTORIA`: modo historia: la saga actual y el primero sin vencer, con su mazo
- `CU-FE-RIVAL-HISTORIA-NAVEGAR`: izquierda/derecha cambian de personaje; arriba/abajo, de saga
- `CU-FE-RIVAL-SAGA-CERRADA`: una saga cerrada no deja empezar y avisa
- `CU-FE-RIVAL-VARIANTE`: cuadrado arma otra variante del mazo
- `CU-FE-RIVAL-REGLAS`: R1 abre las reglas; la cruz pasa las páginas; círculo cierra
- `CU-FE-RIVAL-DESAFIO`: L1 enciende el Modo Desafío solo con la historia terminada
- `CU-FE-RIVAL-CAMBIO-MODO`: triángulo cambia entre Historia y Duelo libre
- `CU-FE-RIVAL-LIBRE-NIVELES`: duelo libre: arriba/abajo cambian el nivel hasta el desbloqueado
- `CU-FE-RIVAL-LIBRE-RIVAL`: izquierda/derecha cambian el rival del nivel
- `CU-FE-RIVAL-LIBRE-AZAR`: cuadrado elige un rival al azar
- `CU-FE-RIVAL-VOLVER`: círculo vuelve al menú

### CU-FE-DIALOGO: diálogos de la historia

- `CU-FE-DIALOGO-CONTENIDO`: cada personaje tiene intro, victoria y derrota en ES y EN; el final tiene texto y créditos
- `CU-FE-DIALOGO-INTRO`: X sobre el rival abre su diálogo; X completa la línea o pasa a la siguiente; al final empieza el duelo
- `CU-FE-DIALOGO-SALTAR`: círculo salta el diálogo
- `CU-FE-DIALOGO-CIERRE`: el personaje tiene su línea de cierre

### CU-FE-DUELO: el duelo

#### Inicio
- `CU-FE-DUELO-SORTEO`: piedra, papel o tijera; la cruz elige la mano
- `CU-FE-DUELO-SORTEO-REGLA`: el resultado sigue la regla del juego
- `CU-FE-DUELO-SORTEO-EMPATE`: el empate vuelve a elegir
- `CU-FE-DUELO-SORTEO-GANA`: si gana el jugador, elige ir primero o segundo y se respeta
- `CU-FE-DUELO-SORTEO-PIERDE`: si gana el rival, decide él y se respeta
- `CU-FE-DUELO-INICIO`: 8000 LP y el mazo activo del jugador
- `CU-FE-DUELO-REPARTO`: el reparto anima 10 u 11 cartas y la mano queda igual a la del duelo
- `CU-FE-DUELO-MUSICA`: suena la música del duelo

#### Turno y fases
- `CU-FE-DUELO-FASE-ESPERA`: la fase de espera se ve antes de la principal 1
- `CU-FE-DUELO-TURNO-JUGADOR`: el jugador queda libre en su principal 1 y la fase en pantalla es esa
- `CU-FE-DUELO-FASES-CUADRADO`: cuadrado pasa de fase y termina el turno
- `CU-FE-DUELO-ROBO-ANIMADO`: cada robo se anima
- `CU-FE-DUELO-MESA-IGUAL-DUELO`: la mesa siempre muestra lo mismo que el duelo
- `CU-FE-DUELO-CONTADORES`: los contadores (mazo, extra, cementerio, desterradas) son los del duelo

#### La mano y la mesa
- `CU-FE-DUELO-MANO-MENU`: X sobre una carta abre su menú
- `CU-FE-DUELO-INVOCAR`: invocar con el mando pone la carta en el duelo y en la mesa
- `CU-FE-DUELO-COLOCAR-MONSTRUO`: colocar boca abajo
- `CU-FE-DUELO-VOLTEAR`: voltear
- `CU-FE-DUELO-POSICION`: cambiar de posición
- `CU-FE-DUELO-ATACAR`: atacar eligiendo el objetivo
- `CU-FE-DUELO-ACTIVAR-MAGIA`: activar una magia con sus costes y objetivos
- `CU-FE-DUELO-ACTIVAR-TRAMPA`: activar una carta colocada
- `CU-FE-DUELO-CAMPO-ACTIVAR`: activar la magia de campo
- `CU-FE-DUELO-ELEGIR-OBJETIVO`: elegir objetivo o carta con el foco o la lista
- `CU-FE-DUELO-PREGUNTAS`: las preguntas del duelo (responder con una trampa o no; elegir al resolver)
- `CU-FE-DUELO-REPETICION`: la repetición del ataque llega al jugador: elegir con el foco (círculo: no atacar) o con la
  lista ("Ataque directo", "No atacar")
- `CU-FE-DUELO-VENTANA-DANNO`: tras recibir daño, la pregunta "Recibiste daño" ofrece Attack and Receive; con
  "Preguntar antes de activar" = Nunca el duelo no se detiene
- `CU-FE-DUELO-JUGABLES`: lo que se puede jugar brilla, y nada más
- `CU-FE-DUELO-COMPLETO-MANDO`: duelos enteros solo con el mando, sin cuelgues

#### Lo que se ve
- `CU-FE-DUELO-DETALLE`: triángulo abre el detalle de la carta; círculo vuelve
- `CU-FE-DUELO-DETALLE-SCROLL`: el stick derecho desplaza el texto
- `CU-FE-DUELO-CADENA-VISIBLE`: la pila de la cadena: entran, se marcan negados, salen al resolverse
- `CU-FE-DUELO-REGISTRO`: L3 abre el registro del duelo y anota las jugadas
- `CU-FE-DUELO-VISORES`: los visores de cementerio, desterradas y extra coinciden con el duelo
- `CU-FE-DUELO-VISORES-ZONAS`: X sobre el cementerio o las desterradas abre su visor
- `CU-FE-DUELO-CEMENTERIO-VER`: ver el cementerio desde la mesa
- `CU-FE-DUELO-ENTRADA-CLAVE`: las cartas clave (firma o con figura) hacen su entrada con su clase y duración
- `CU-FE-DUELO-FIGURAS`: cada carta de `figuras.txt` existe y tiene su modelo
- `CU-CARTA-<id>-CAMPO` (por carta): cada magia de campo cambia el escenario (campos.h)

### CU-FE-PAUSA: la pausa

- `CU-FE-PAUSA-ABRE`: OPTIONS corto la abre con sus 5 opciones
- `CU-FE-PAUSA-DETIENE`: con la pausa abierta el duelo no avanza
- `CU-FE-PAUSA-CONTINUAR`: Continuar vuelve al duelo
- `CU-FE-PAUSA-OPCIONES`: Opciones abre las Opciones y círculo vuelve a la pausa
- `CU-FE-PAUSA-CERRAR`: OPTIONS corto la cierra
- `CU-FE-PAUSA-CONSTRUIR`: Construir mazo abre el constructor y vuelve al mismo duelo
- `CU-FE-PAUSA-LARGO`: mantener OPTIONS 2 s no abre la pausa
- `CU-FE-PAUSA-ABANDONAR`: Abandonar cuenta la derrota y muestra el resultado
- `CU-FE-PAUSA-MENU`: Salir al menú principal no cuenta derrota
- `CU-FE-PAUSA-ELEGIR-RIVAL`: en elegir rival, la pausa cambia de modo

### CU-FE-RESULTADO: el resultado del duelo

- `CU-FE-RESULTADO-PANTALLA`: ganar muestra la pantalla de resultado
- `CU-FE-RESULTADO-MOTIVO`: el motivo (LP a 0, Exodia, rendición...)
- `CU-FE-RESULTADO-DERROTA`: perder muestra su resultado
- `CU-FE-RESULTADO-MUSICA`: después del sfx, la música de victoria o de derrota
- `CU-FE-RESULTADO-NAVEGAR`: la cruz recorre las opciones
- `CU-FE-RESULTADO-REVANCHA`: Revancha repite el rival con otro sorteo
- `CU-FE-RESULTADO-OTRO-RIVAL`: Elegir otro rival
- `CU-FE-RESULTADO-MENU`: Menú principal
- `CU-FE-RESULTADO-CIRCULO`: círculo vuelve al menú
- `CU-FE-RESULTADO-SUBE-NIVEL`: ganar en el nivel máximo desbloquea el siguiente
- `CU-FE-RESULTADO-ESTADISTICAS`: cada duelo se cuenta en las estadísticas
- `CU-FE-RESULTADO-HISTORIA`: vencer a un personaje lo marca vencido
- `CU-FE-RESULTADO-FIRMA`: y desbloquea sus cartas firma
- `CU-FE-RESULTADO-SIGUIENTE`: dice quién es el siguiente rival de la historia
- `CU-FE-FINAL`: vencer al último abre el final de la historia
- `CU-FE-FINAL-CREDITOS`: el final pasa por los créditos
- `CU-FE-FINAL-DESAFIO`: el final desbloquea el Modo Desafío
- `CU-FE-DESAFIO-DUELO`: el duelo de desafío empieza con 4000 LP

### CU-FE-IDIOMA: español e inglés en todo

- `CU-FE-IDIOMA-MENU`: los botones del menú
- `CU-FE-IDIOMA-OPCIONES`: las opciones y sus valores
- `CU-FE-IDIOMA-PAUSA`: la pausa
- `CU-FE-IDIOMA-AVISOS`: los avisos del duelo
- `CU-FE-IDIOMA-FASES`: los nombres de las fases
- `CU-FE-IDIOMA-MENU-CARTA`: el menú de cada carta y sus motivos
- `CU-FE-IDIOMA-RESULTADO`: los motivos y las opciones del resultado
- `CU-FE-IDIOMA-REGLAS`: las reglas y la ayuda de botones
- `CU-FE-IDIOMA-CONTROLES`: los controles de Personalización
- `CU-FE-IDIOMA-PERSO`: fundas y tapetes
- `CU-FE-IDIOMA-SAGAS`: las sagas
- `CU-FE-IDIOMA-PERSONAJES`: nombre y lema de cada personaje
- `CU-FE-IDIOMA-RIVALES`: nombre y lema de cada rival
- `CU-FE-IDIOMA-CARTAS`: la colección en inglés (los filtros siguen igual)
- `CU-FE-IDIOMA-CONSTRUCTOR`: los avisos del constructor
- `CU-CARTA-<id>-DATOS` (por carta): nombre, texto, atributo y tipo en ES y EN

### CU-FE-MANDO: el mando

- `CU-FE-MANDO-CRUZ-TOQUE`: un toque es un paso
- `CU-FE-MANDO-CRUZ-REPETIR`: mantener la cruz repite (0,40 s y luego cada 0,09 s)
- `CU-FE-MANDO-STICK`: el stick izquierdo hace de cruz
- `CU-FE-MANDO-R2-ACELERA`: mantener R2 acelera las animaciones (sin animación es zoom)
- `CU-FE-MANDO-R2-SALTA`: un toque de R2 salta la animación
- `CU-FE-MANDO-VELOCIDAD`: los factores de velocidad suben

### CU-FE-GUARDADO: guardado atómico y avisos

- `CU-FE-GUARDADO-ATOMICO`: se escribe entero (temporal, fsync y renombrar) y reemplaza el viejo
- `CU-FE-GUARDADO-AVISO`: si no se puede guardar, avisa con el nombre del archivo
- `CU-FE-GUARDADO-SIN-CARPETA`: sin carpeta de datos, avisa una sola vez
- `CU-FE-GUARDADO-PROGRESO`: el nivel desbloqueado queda en `progreso.txt`
- `CU-FE-GUARDADO-HISTORIA`: los vencidos y el final quedan en `historia.txt`
- `CU-FE-GUARDADO-ESTADISTICAS`: `estadisticas.txt` vuelve igual al leerlo

---

## CU-BE: back end

### CU-BE-REGLAS: las reglas del duelo

#### Inicio, robo y fases
- `CU-BE-INICIO`: 8000 LP, 5 cartas, el extra son las fusiones, empieza en la principal 1
- `CU-BE-INICIO-QUIEN`: sin sorteo, empieza cualquiera de los dos
- `CU-BE-ROBO-PRIMER-TURNO`: el que empieza roba en su primer turno
- `CU-BE-ROBO`: cada turno se roba 1 (con su suceso)
- `CU-BE-FASES-ORDEN`: principal 1, batalla, principal 2, final
- `CU-BE-FASES-PRIMER-TURNO`: en el primer turno no hay batalla
- `CU-BE-FASES-PASA-TURNO`: la fase final pasa el turno
- `CU-BE-FASES-ESPERA`: robo, espera y principal 1 del turno nuevo

#### Invocaciones
- `CU-BE-INVOCAR-NORMAL`: invocación normal boca arriba en ataque
- `CU-BE-INVOCAR-UNA-POR-TURNO`: una sola normal por turno
- `CU-BE-INVOCAR-FASE`: solo en las fases principales
- `CU-BE-COLOCAR`: colocar boca abajo en defensa
- `CU-BE-TRIBUTO-1`: nivel 5 y 6 con 1 sacrificio, que va al cementerio
- `CU-BE-TRIBUTO-2`: nivel 7 o más con 2
- `CU-BE-TRIBUTO-COLOCAR`: colocar también pide sacrificios
- `CU-BE-TRIBUTO-DE-SOBRA`: no se sacrifica de más
- `CU-BE-TRIBUTO-DIOS`: los Dioses con 3 y no se colocan
- `CU-BE-VOLTEO`: voltear, no el turno en que se colocó
- `CU-BE-POSICION`: cambiar de posición una vez por turno, no el turno en que llegó
- `CU-BE-ESPECIAL`: invocación especial (Monster Reborn)
- `CU-BE-ESPECIAL-NO-GASTA-NORMAL`: la especial no gasta la normal
- `CU-BE-FUSION`: Polymerization con los materiales de la mano
- `CU-BE-FUSION-CAMPO`: o del campo
- `CU-BE-FUSION-SIN-MATERIALES`: sin los materiales, no
- `CU-BE-RITUAL`: magia ritual, monstruo ritual y sacrificios con nivel suficiente
- `CU-BE-RITUAL-NIVEL`: con nivel de menos, no
- `CU-BE-UNION`: la unión se equipa y da su bono
- `CU-BE-ESPIRITU`: el espíritu vuelve a la mano en la fase final
- `CU-BE-TOON`: el toon necesita Toon World y no se invoca normal

#### Límites
- `CU-BE-LIMITE-MANO`: con más de 6 en la fase final hay que descartar
- `CU-BE-LIMITE-ZONAS`: 5 monstruos y 5 magias/trampas; la de campo va a su zona

#### Cadena y ventanas
- `CU-BE-VENTANA-INVOCACION`: tras una invocación el rival puede responder (Trap Hole)
- `CU-BE-TRAMPA-MISMO-TURNO`: una trampa no se activa el turno en que se colocó
- `CU-BE-CADENA-VENTANA`: tras una activación el rival puede responder
- `CU-BE-CADENA-VELOCIDAD`: solo responde lo de velocidad suficiente
- `CU-BE-CADENA-LIFO`: la cadena se resuelve de arriba hacia abajo
- `CU-BE-EFECTOS-CASOS`: los casos a mano del motor de efectos
- `CU-BE-EFECTOS-ELECCIONES`: las elecciones al resolver

#### Batalla y daño
- `CU-BE-BATALLA-ATK`: ATK contra ATK: se destruye el más débil y su dueño recibe la diferencia
- `CU-BE-BATALLA-EMPATE`: ATK iguales: se destruyen los dos sin daño
- `CU-BE-BATALLA-DEF`: ATK contra DEF: el defensor no recibe daño; con DEF mayor, el atacante sí
- `CU-BE-BATALLA-BOCA-ABAJO`: el atacado boca abajo se voltea
- `CU-BE-BATALLA-DIRECTO`: ataque directo solo con el campo rival vacío
- `CU-BE-BATALLA-DEFENSA-NO-ATACA`: en defensa no se ataca
- `CU-BE-BATALLA-UN-ATAQUE`: un ataque por monstruo
- `CU-BE-REPETICION-ATAQUE`: si cambia el número de monstruos del rival, la repetición del ataque: el atacante elige
  (pregunta del motor); "no atacar" deja al monstruo volver a atacar en esa fase; sin cambio (Waboku) el ataque sigue
- `CU-BE-REPETICION-ATAQUE-ELEGIR`: en la repetición se elige otro objetivo o el mismo (el de antes va primero)
- `CU-BE-REPETICION-ATAQUE-DIRECTO`: ante un ataque directo aparecen monstruos: hay repetición sin ataque directo; si
  el objetivo se va y el campo queda vacío, la repetición ofrece el ataque directo
- `CU-BE-REPETICION-ATAQUE-IA`: la IA clásica y la humana responden la repetición con su política de ataque (letal:
  ataca; suicida: no ataca)
- `CU-BE-VENTANA-DANNO`: justo después del daño se abre una ventana solo para el que lo recibió y solo si tiene una
  carta "cuando recibes daño"; sin esa carta no se abre nada
- `CU-BE-VENTANA-DANNO-ATTACK-RECEIVE`: Attack and Receive en esa ventana (daño de batalla); el rival recibe 700 y tiene
  su propia ventana; después ya no se puede activar
- `CU-BE-VENTANA-DANNO-NUMINOUS`: Numinous Healer en esa ventana (daño de efecto, Hinotama); después ya no
- `CU-BE-VENTANA-DANNO-IA`: la IA clásica y la humana usan su carta en esa ventana

#### Condiciones de victoria
- `CU-BE-VICTORIA-LP`: LP a 0
- `CU-BE-VICTORIA-SIN-MAZO`: no poder robar
- `CU-BE-VICTORIA-EXODIA`: las 5 piezas de Exodia
- `CU-BE-VICTORIA-FIN`: con el duelo terminado no se juega

#### Atajos conocidos (de `control_final.md`; todos resueltos con la regla oficial)
- `CU-BE-ATAJO-FAIRY-GUARDIAN`: devuelve la magia que mandó el rival al cementerio este turno
- `CU-BE-ATAJO-PITCH-BLACK`: el contador va a cualquier carta que pueda tenerlo (también del rival) y se elige
- `CU-BE-ATAJO-LEVEL-LAB`: el monstruo de la mano lo elige quien la activa
- `CU-BE-ATAJO-INFERNO-TEMPEST`: se activa en la ventana justo después de recibir 3000+ de daño de batalla (con 1700, no)
- `CU-BE-ATAJO-INFERNO-TEMPEST-MOMENTO`: solo en esa ventana: después (batalla, fase final) ya no se puede activar

### CU-BE-IA: el rival

- `CU-BE-IA-MAZOS-RIVALES`: los 120 mazos de los rivales son válidos (40-60, copias, banlist)
- `CU-BE-IA-LEGAL`: la IA nunca intenta una jugada ilegal
- `CU-BE-IA-TERMINA`: los duelos IA contra IA terminan
- `CU-BE-IA-RESULTADOS-VARIAN`: los resultados varían
- `CU-BE-IA-NIVEL`: el nivel alto gana más que el bajo
- `CU-BE-IA-PLANES`: planes de juego que cambian entre duelos y a mitad de duelo
- `CU-BE-IA-MAZOS-DINAMICOS`: mazos dinámicos válidos y distintos entre duelos
- `CU-BE-IA-LECTURA-JUGADOR`: lee al jugador (trampero o farolero)
- `CU-BE-IA-ADAPTABLE`: dificultad adaptable tras rachas y se puede apagar
- `CU-BE-IA-MEMORIA`: la memoria del rival se guarda y se lee igual
- `CU-BE-IA-HUMANA-FUERZA`: la IA humana le gana a la clásica con los mismos mazos
- `CU-BE-IA-HUMANA-PRESUPUESTO`: piensa dentro de su presupuesto de tiempo
- `CU-BE-IA-HUMANA-DIVERSIDAD`: juega distinto entre duelos
- `CU-BE-IA-HUMANA-FAROLES`: faroles y trampas guardadas
- `CU-BE-IA-HUMANA-ERRORES`: errores creíbles por nivel

### CU-BE-MAZO: validación de mazos

- `CU-BE-MAZO-INICIAL`: el mazo inicial es válido
- `CU-BE-MAZO-MIN-40`: menos de 40, no
- `CU-BE-MAZO-MAX-60`: más de 60, no
- `CU-BE-MAZO-MAX-3`: más de 3 copias, no
- `CU-BE-MAZO-EXTRA-15`: más de 15 en el extra, no
- `CU-BE-MAZO-BANLIST`: la banlist Goat (prohibidas, limitadas, semilimitadas)

---

## CU-CARTA: carta por carta

Para cada una de las 1774 cartas (`<id>` es su id). Estas hojas no se listan una por una aquí: salen de los datos de
las cartas y están todas en `resultados.tsv` de cada ciclo.

- `CU-CARTA-<id>-DATOS`: la carta en ES y EN con nombre y texto, la misma clase, nivel, ATK y DEF, el atributo y el
  tipo traducidos como en el resto de la colección y su línea de `cartas_classic.txt` (passcode, límite Goat, subtipo
  que corresponde a la clase)
- `CU-CARTA-<id>-CARAS`: sus 6 caras (`cara_`, `cara_en_`, `mini_`, `mini_en_`, `cara_hd_`, `cara_hd_en_`)
- `CU-CARTA-<id>-INVOCAR` (monstruos): se invoca como dice su texto (normal con sus sacrificios y colocada, Dioses con
  3, fusión con Polymerization y su receta o por contacto, ritual con su magia, toon con Toon World, guardianes con su
  arma); las que el texto dice que no se invocan de modo normal no se invocan; las de condición (Cave Dragon,
  Terrorking Archfiend), solo cuando se cumple
- `CU-CARTA-<id>-COLOCAR` (magias y trampas): se coloca boca abajo (la de campo en su zona)
- `CU-CARTA-<id>-EFECTO` (las 1385 con efecto): la certificación del mismo ciclo (casos a mano y plantilla, en el motor
  y en el front: `prueba_certificacion.cpp`)
- `CU-CARTA-<id>-CAMPO` (las 25 de campo): tiene su escenario en `campos.h`

---

## CU-PRUEBA: las pruebas enteras

Cada prueba de `ciclo_completo.sh` también es un caso (`CU-PRUEBA-<nombre>`): COMPILAR-JUEGO, prueba_mando,
prueba_constructor, prueba_juego, prueba_duelo, prueba_efectos, prueba_ia, prueba_variedad, prueba_ia_humana,
prueba_audio, CERTIFICACION y CASOS-DE-USO.
