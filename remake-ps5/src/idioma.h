/*
 * Yugioh Classic Remaster - idioma de la interfaz (rama classic-idioma): Espanol (por defecto) o Ingles.
 *
 * - tr("texto en espanol") devuelve el texto en el idioma actual (la clave ES el texto en espanol; si falta en la
 *   tabla, queda en espanol). interfaz.h la aplica sola en texto()/medir()/parrafo(): cualquier literal de la
 *   interfaz o de control.h (menus, avisos, ayuda del mando) que este en la tabla sale traducido.
 * - Las cartas en ingles salen de assets/ui/cartas_ui_en.txt y de las caras cara_en_<id>.png / mini_en_<id>.png
 *   (si falta la inglesa de una carta, se usa la espanola).
 * - La preferencia se guarda en idioma.txt ("es" o "en") junto a capturas.txt (consola: /data/homebrew/<TITULO>/,
 *   es decir getAssetPath() + "../"). En el PC, YUGI_IDIOMA=en|es manda sobre el archivo.
 * - Cambiar el idioma sube version(): la interfaz recarga las caras visibles en el siguiente cuadro (en caliente).
 * - Caras HD: cara_hd_<id>.jpg / cara_hd_en_<id>.jpg en el visor y el detalle (una sola cargada a la vez).
 *
 * COMO REGISTRAR TEXTOS NUEVOS (ramas classic-constructor, classic-ia, classic-base, las que vengan):
 *  1. Lo que se dibuja con interfaz.h texto()/medir()/parrafo() se traduce solo si la frase exacta en espanol esta en
 *     tabla() (abajo): agregar { "texto en espanol", "English text" }. Las partes variables (numeros, nombres de
 *     cartas) se unen DESPUES de traducir cada pieza: std::to_string(n) + idioma::tr(" cartas restantes").
 *  2. Un archivo propio puede tener su tabla (como constructor_textos.h) y caer a idioma::tr en lo que no tenga, o
 *     usar idioma::t("es", "en") en el mismo lugar del literal (sin tocar esta tabla).
 *  3. Datos de cartas: siempre por CartaUI::vNombre()/vAtributo()/vTipo()/vDesc() (nunca .nombre a pelo) o, fuera
 *     de interfaz.h, cartas_ui<idioma::sufijo()>.txt. Para refrescar algo propio al cambiar, comparar idioma::version().
 *  4. Ventana de Ajustes (ImGui): overlay->checkBox(idioma::tr("..."), ...).
 *  Ningun texto visible debe quedar fuera: con Espanol todo en espanol y con English todo en ingles.
 */
#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <unordered_map>

namespace idioma {

enum { ES = 0, EN = 1 };

inline int& actual() { static int v = ES; return v; }
inline int& version() { static int v = 0; return v; }   // sube con cada cambio (la interfaz recarga las caras)
inline bool ingles() { return actual() == EN; }
inline std::string& rutaPreferencia() { static std::string r; return r; }
// el sufijo de los archivos de cartas: cartas_ui<suf>.txt, cara<suf>_<id>.png ("" o "_en")
inline const char* sufijo() { return ingles() ? "_en" : ""; }

// ---------------------------------------------------------------------------------------------- tabla ES -> EN
inline const std::unordered_map<std::string, std::string>& tabla()
{
	static const std::unordered_map<std::string, std::string> t = {
		// fases (columna y cartel)
		{ "FASE", "PHASE" },
		{ "ROBO", "DRAW" }, { "ESPERA", "STANDBY" }, { "PRINCIPAL 1", "MAIN 1" }, { "BATALLA", "BATTLE" },
		{ "PRINCIPAL 2", "MAIN 2" }, { "FINAL", "END" },
		{ "FASE DE ROBO", "DRAW PHASE" }, { "FASE DE ESPERA", "STANDBY PHASE" }, { "FASE PRINCIPAL 1", "MAIN PHASE 1" },
		{ "FASE DE BATALLA", "BATTLE PHASE" }, { "FASE PRINCIPAL 2", "MAIN PHASE 2" }, { "FASE FINAL", "END PHASE" },
		// camaras
		{ "CÁMARA", "CAMERA" }, { "CINE", "CINEMA" }, { "TABLERO", "BOARD" }, { "CENITAL", "TOP-DOWN" },
		{ "LATERAL", "SIDE" }, { "LIBRE", "FREE" },
		// LP y zonas
		{ "RIVAL", "OPPONENT" }, { "JUGADOR", "PLAYER" },
		{ "MAZO", "DECK" }, { "FUSIÓN", "FUSION" }, { "CEMENTERIO", "GRAVEYARD" },
		{ "MANO DEL RIVAL", "OPPONENT'S HAND" }, { " cartas ocultas", " hidden cards" },
		{ "El rival guarda estas cartas boca abajo. Su contenido se revelara cuando las juegue.",
		  "Your opponent keeps these cards face-down. They will be revealed when played." },
		{ "MAZO DEL RIVAL", "OPPONENT'S DECK" }, { "TU MAZO", "YOUR DECK" }, { " cartas restantes", " cards left" },
		{ "Al comenzar cada turno se roba la carta de arriba. Si no quedan cartas para robar, el duelo se pierde.",
		  "At the start of each turn you draw the top card. If there are no cards left to draw, you lose the duel." },
		{ "ZONA DE FUSIÓN", "FUSION ZONE" }, { "Sin monstruos de fusión", "No Fusion Monsters" },
		{ "Aqui esperan los monstruos de fusion hasta que se invoquen con Polimerizacion.",
		  "Fusion Monsters wait here until they are summoned with Polymerization." },
		{ "CEMENTERIO DEL RIVAL", "OPPONENT'S GRAVEYARD" }, { "TU CEMENTERIO", "YOUR GRAVEYARD" },
		{ "Vacío", "Empty" }, { "Vacía", "Empty" },
		{ "Las cartas destruidas, usadas o descartadas terminan aqui, boca arriba.",
		  "Destroyed, used or discarded cards end up here, face-up." },
		{ "CARTA BOCA ABAJO", "FACE-DOWN CARD" },
		{ "Colocada por el rival", "Set by your opponent" }, { "Colocada por ti", "Set by you" },
		{ "Una magia o trampa oculta. Cuidado al atacar.", "A hidden Spell or Trap. Be careful when attacking." },
		{ "Tu carta colocada, lista para activarse.", "Your Set card, ready to be activated." },
		{ "ZONA DE MONSTRUO", "MONSTER ZONE" }, { "ZONA DE MAGIA Y TRAMPA", "SPELL & TRAP ZONE" },
		{ "ZONA DE CAMPO", "FIELD ZONE" },
		{ "Zona del rival sin cartas.", "Opponent's zone with no cards." },
		{ "Puedes colocar una carta aqui.", "You can place a card here." },
		{ "Una carta oculta del rival. Se revelara cuando la juegue o la destruyas.",
		  "A hidden card of your opponent. It will be revealed when played or destroyed." },
		{ " / Efecto", " / Effect" }, { " / Fusión", " / Fusion" }, { " / Ritual", " / Ritual" },
		// menu de acciones (control.h)
		{ "ACCIONES", "ACTIONS" },
		{ "Invocar", "Summon" }, { "Colocar boca abajo", "Set" }, { "Activar", "Activate" }, { "Colocar", "Set" },
		{ "Cancelar", "Cancel" }, { "Voltear (invocación)", "Flip Summon" }, { "Atacar", "Attack" },
		{ "Cambiar a ataque", "Change to Attack" }, { "Cambiar a defensa", "Change to Defense" },
		{ "Enviar al cementerio", "Send to Graveyard" }, { "Voltear", "Flip" }, { "Destruir", "Destroy" },
		// avisos y motivos
		{ "No hay zonas de monstruo libres", "No free Monster Zones" },
		{ "Ya hay una carta de campo: destrúyela primero", "There is already a Field card: destroy it first" },
		{ "No hay zonas de magia y trampa libres", "No free Spell & Trap Zones" },
		{ "En defensa no puede atacar", "Cannot attack in Defense Position" },
		{ "Sin modelo 3D: lo resolverá el motor de reglas", "No 3D model: the rules engine will resolve it" },
		{ "Espera a que termine el ataque", "Wait for the attack to finish" },
		{ "La carta de campo está en plena transición", "The Field card is changing" },
		{ "No se puede", "Not possible" }, { "No hay zonas libres", "No free zones" },
		{ "El rival no tiene monstruos a los que atacar", "Your opponent has no monsters to attack" },
		{ "Demo automática apagada: tienes el control", "Auto demo off: you have control" },
		// barra de ayuda del mando
		{ "Mover", "Move" }, { "Acciones", "Actions" }, { "Ver", "View" }, { "Detalle", "Details" },
		{ "Fase", "Phase" }, { "Cámara", "Camera" }, { "Opción", "Option" }, { "Confirmar", "Confirm" },
		{ "Zona", "Zone" }, { "Atrás", "Back" }, { "Objetivo", "Target" }, { "Cerrar", "Close" },
		// atributos y tipos de respaldo
		{ "Figura 3D", "3D Figure" },
		// Ajustes (ventana de OPTIONS)
		{ "Ajustes", "Settings" }, { "Idioma", "Language" },
		{ "Animar monstruos", "Animate monsters" }, { "Camara que respira", "Breathing camera" },
		{ "Dificultad adaptable", "Adaptive difficulty" },   // (rama classic-variedad)
		{ "Demo automatica", "Auto demo" }, { "Ataques automaticos", "Auto attacks" }, { "Camara", "Camera" },
		{ "Cine", "Cinema" }, { "Tablero", "Board" }, { "Cenital", "Top-down" }, { "Lateral", "Side" }, { "Libre", "Free" },
		{ "Brillo holografico", "Holo shine" }, { "Ataque por casillas", "Attack by zones" },
		{ "Combinacion", "Combination" },
		{ "Carta de campo: destruir", "Field card: destroy" }, { "Carta de campo: activar", "Field card: activate" },
		{ "Bosque automatico", "Auto Forest" }, { "Entorno: siguiente", "Environment: next" }, { "Mano de figuras nuevas", "Hand of new figures" },
		{ "Robar figura", "Draw figure" }, { "Despejar mis monstruos", "Clear my monsters" },
		{ "Atacar ahora", "Attack now" }, { "Salir", "Quit" },
		{ "Animar", "Animate" }, { "Mano nueva", "New hand" }, { "Robar carta", "Draw card" },
		{ "Cartas de monstruo: %d", "Monster cards: %d" },
		// duelo real contra la IA (rama classic-integracion: control.h, duelo_juego.h, duelo_ui.h)
		{ "Descarta hasta tener 6 cartas en la mano", "Discard down to 6 cards in hand" }, { "Descartar", "Discard" },
		{ "Descarta hasta tener %d cartas en la mano", "Discard down to %d cards in hand" },
		{ "Invocar Toon", "Toon Summon" }, { "sacrificio", "tribute" }, { "sacrificios", "tributes" },
		{ "Se invoca con su magia de ritual", "Summoned with its Ritual Spell" }, { "Fusión", "Fusion" }, { "Ritual", "Ritual" },
		{ "Faltan niveles para el sacrificio", "Not enough levels to tribute" }, { "Espera tu turno", "Wait for your turn" },
		{ "Solo en tu fase principal", "Only in your Main Phase" }, { "Faltan monstruos para el sacrificio", "Not enough monsters to tribute" },
		{ "No se puede invocar de forma normal", "Cannot be Normal Summoned" }, { "Ya invocaste este turno", "Already summoned this turn" }, { "No se puede invocar ahora", "Cannot be Summoned now" }, { "No se puede sacrificar", "Cannot Tribute" }, { "Faltan LP", "Not enough LP" },
		{ "Llegó este turno", "Arrived this turn" }, { "Ya cambió de posición este turno", "Already changed position this turn" },
		{ "Solo en la fase de batalla", "Only in the Battle Phase" }, { "No se ataca en el primer turno", "No attacks on the first turn" },
		{ "Ya atacó este turno", "Already attacked this turn" },
		{ "Elige 1 monstruo para sacrificar", "Choose 1 monster to tribute" }, { "Elige 2 monstruos para sacrificar", "Choose 2 monsters to tribute" },
		{ "Monstruo", "Monster" }, { "Sacrificar", "Tribute" },
		{ "Sin rivales (falta rivales.txt)", "No opponents (rivales.txt missing)" }, { "Empiezas tú", "You go first" },
		{ "Empieza el rival", "Your opponent goes first" }, { "Tu turno", "Your turn" }, { "Turno del rival", "Opponent's turn" },
		{ "Abandonar duelo", "Forfeit duel" }, { "ELIGE A TU RIVAL", "CHOOSE YOUR OPPONENT" }, { "Victorias", "Wins" },
		{ "Derrotas", "Losses" }, { "Nivel", "Level" }, { "Duelo", "Duel" }, { "Otro rival", "Another opponent" }, { "TÁCTIL", "TOUCH" },
		// modo historia: personajes del anime (rama classic-personajes)
		{ "MODO HISTORIA", "STORY MODE" }, { "Saga", "Saga" }, { "Mazo", "Deck" }, { "VENCIDO", "DEFEATED" }, { "DISPONIBLE", "AVAILABLE" },
		{ "BLOQUEADO: vence a todos los de la saga anterior", "LOCKED: defeat everyone in the previous saga" },
		{ "Vence a todos los de la saga anterior", "Defeat everyone in the previous saga" },
		{ "Personajes vencidos", "Characters defeated" }, { "Personaje / saga", "Character / saga" }, { "Duelo libre", "Free duel" },
		{ "Modo historia", "Story mode" }, { "¡PERSONAJE VENCIDO!", "CHARACTER DEFEATED!" },
		{ "cartas firma desbloqueadas", "signature cards unlocked" }, { "NUEVA SAGA", "NEW SAGA" },
		{ "Vence a ", "Defeat " }, { "BLOQUEADA", "LOCKED" }, { "CARTA FIRMA BLOQUEADA", "SIGNATURE CARD LOCKED" },
		{ "Carta firma bloqueada", "Signature card locked" }, { "Un Dios no se puede colocar", "A God cannot be Set" },
		{ "Construir mazo", "Build deck" }, { "¡VICTORIA!", "VICTORY!" }, { "DERROTA", "DEFEAT" }, { "EMPATE", "DRAW" },
		{ "Rival", "Opponent" }, { "NIVEL DESBLOQUEADO", "LEVEL UNLOCKED" }, { "¡ERES EL CAMPEÓN!", "YOU ARE THE CHAMPION!" },
		{ "Continuar", "Continue" },
		// menu de pausa y ayuda de fases (rama classic-arreglos: duelo_juego.h, duelo_ui.h, control.h)
		{ "PAUSA", "PAUSE" }, { "Pausa", "Pause" }, { "Volver", "Back" }, { "Abandonar duelo (derrota)", "Forfeit duel (counts as a loss)" },
		{ "Salir al menú principal", "Exit to main menu" }, { "Ir a la Batalla", "Go to Battle" }, { "Ir a Principal 2", "Go to Main 2" },
		{ "Terminar turno", "End turn" }, { "Fase (en espera)", "Phase (queued)" }, { "Cambia de fase al terminar la animación", "Phase changes when the animation ends" },
		// motor de efectos (rama classic-efectos: control.h, duelo_juego.h)
		{ "Activar efecto", "Activate effect" }, { "No activar", "Don't activate" }, { "El rival ataca con", "Your opponent attacks with" },
		{ "El rival invocó", "Your opponent summoned" }, { "Invocaste", "You summoned" }, { "El rival activó", "Your opponent activated" },
		{ "Activaste", "You activated" }, { "Fase final del rival", "Opponent's End Phase" }, { "¿Activar una carta?", "Activate a card?" },
		{ "Elige una carta para descartar", "Choose a card to discard" }, { "Elige el objetivo", "Choose the target" },
		{ "Carta", "Card" }, { "Elegir", "Choose" }, { "campo", "field" }, { "mano", "hand" }, { "cementerio", "Graveyard" },
		{ "desterrada", "banished" }, { "mazo", "deck" }, { "fusión", "Extra Deck" }, { "rival", "opponent" },
		{ "Cadena", "Chain" }, { "Un efecto le impide atacar", "An effect prevents it from attacking" },
		{ "Invocación especial", "Special Summon" },
		{ "Elige la carta a desterrar (coste)", "Choose the card to banish (cost)" },
		{ "El rival entra en la fase de batalla", "Your opponent enters the Battle Phase" }, { "Paso de daño", "Damage Step" }, { "Atacar directamente", "Attack directly" }, { "Equipar", "Equip" },
		{ "Elige el monstruo a sacrificar (coste)", "Choose the monster to Tribute (cost)" },
		{ "Ataque", "Attack" }, { "Defensa", "Defense" }, { "No", "No" }, { "Ataque negado", "Attack negated" }, { "Negado", "Negated" },
		{ "Volumen musica", "Music volume" }, { "Volumen efectos", "Effects volume" },
		{ "Figuras 3D: %d (ultima: %s)", "3D figures: %d (latest: %s)" },
		{ "Figura 3D nueva: todavía sin datos de carta (preparar_figuras.py).", "New 3D figure: no card data yet." },
	};
	return t;
}

// el texto en el idioma actual, dado en los dos (para literales de otras ramas que no quieran tocar la tabla)
inline const char* t(const char* es, const char* en) { return ingles() ? en : es; }

inline const std::string& tr(const std::string& es)
{
	if (!ingles()) return es;
	const auto& t = tabla();
	auto it = t.find(es);
	return it == t.end() ? es : it->second;
}
inline const char* tr(const char* es)
{
	if (!ingles() || !es) return es;
	const auto& t = tabla();
	auto it = t.find(es);
	return it == t.end() ? es : it->second.c_str();
}

// ---------------------------------------------------------------------------------------------- preferencia
inline void guardar()
{
	if (rutaPreferencia().empty()) return;
	if (FILE* f = fopen(rutaPreferencia().c_str(), "w")) {
		fputs(ingles() ? "en\n" : "es\n", f);
		fclose(f);
	} else {
		fprintf(stderr, "[idioma] no se pudo guardar %s\n", rutaPreferencia().c_str());
	}
}

// lee la preferencia (idioma.txt); sin archivo, espanol. YUGI_IDIOMA=en|es manda (PC)
inline void cargar(const std::string& ruta)
{
	rutaPreferencia() = ruta;
	std::string v;
	std::ifstream f(ruta);
	if (f) std::getline(f, v);
	if (const char* e = getenv("YUGI_IDIOMA")) v = e;
	actual() = (v.size() >= 2 && (v[0] == 'e' || v[0] == 'E') && (v[1] == 'n' || v[1] == 'N')) ? EN : ES;
	fprintf(stderr, "[idioma] %s (%s)\n", ingles() ? "en" : "es", ruta.c_str());
}

inline void poner(int i)
{
	i = i == EN ? EN : ES;
	if (i == actual()) return;
	actual() = i;
	version()++;
	guardar();
}

}  // namespace idioma
