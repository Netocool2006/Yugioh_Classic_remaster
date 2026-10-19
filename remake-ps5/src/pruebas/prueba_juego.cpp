// Prueba de la ruta REAL del juego (la de la PS5), sin pantalla: incluye duelo_juego.h tal cual (el mismo codigo
// que va dentro de la clase del juego en yugireload.cpp) con lo de Vulkan/sonido/interfaz como maquetas, y cada
// cuadro hace lo mismo que render(): avanzarAtaque -> avanzarMando (botones -> pausa / pantallas / control.h) ->
// avanzarDueloIA. Diferencia con prueba_duelo.cpp: aqui NADIE toca ctl.bloqueado a mano; lo pone avanzarDueloIA
// como en la consola. Recorrido: arranque -> modo historia -> elegir rival con X -> duelo -> el jugador pasa por
// todas las fases con CUADRADO (y descarta si hace falta) -> turno del rival (la IA) -> vuelta al jugador, varias
// veces. Tambien prueba el menu de pausa (OPTIONS corto): Continuar, Abandonar duelo y Salir al menu principal.
// Compilar y correr (contenedor arch-ps5):
//   g++ -std=c++17 -O2 -I remake-ps5/src remake-ps5/src/pruebas/prueba_juego.cpp -o /tmp/prueba_juego &&
//   /tmp/prueba_juego <carpeta ui/ con cartas_ui.txt, rivales.txt y personajes.txt> [duelos]
#include "control.h"
#include "mazo.h"
#include "personajes.h"
#include <chrono>
#include <set>

// ---- maquetas de lo que el juego tiene en Vulkan / audio / entornos
namespace efc {
inline float duracion(int) { return 1.0f; }
}
namespace entornos {
inline int porTema(const std::string&) { return 0; }
inline int claveDe(const std::string&) { return 0; }
static const char* const CLAVES[] = { "neutro" };
}

// botones del mando de la consola (platform.h): los mismos bits
enum : uint32_t {
	PAD_OPTIONS = 0x000008, PAD_UP = 0x000010, PAD_RIGHT = 0x000020, PAD_DOWN = 0x000040, PAD_LEFT = 0x000080,
	PAD_L1 = 0x000400, PAD_R1 = 0x000800, PAD_TRIANGLE = 0x001000, PAD_CIRCLE = 0x002000, PAD_CROSS = 0x004000,
	PAD_SQUARE = 0x008000, PAD_R3 = 0x000004, PAD_TOUCH_PAD = 0x100000,
};
struct EstadoMando { uint32_t pressed{ 0 }, held{ 0 }; float left_x{ 0 }, left_y{ 0 }, right_x{ 0 }, right_y{ 0 }, l2{ 0 }, r2{ 0 }; };

static int fallos = 0;
#define FALLO(...) do { fprintf(stderr, "FALLO: " __VA_ARGS__); fprintf(stderr, "\n"); fallos++; } while (0)

struct Juego {
	std::string carpetaUi;
	std::string getAssetPath() const { return carpetaUi + "../"; }
	// sonido
	struct Sonido {
		void tocarMusica(const char*, float) {}
		void sfx(const char*, float = 1.0f, float = 0.0f, float = 0.0f) {}
		bool hay(const char*) const { return false; }
		void pararMusica(float) {}
	} sonido;
	// interfaz
	struct Interfaz {
		std::function<bool(const char*, int&, int&)> statsZona;
		int lps[2]{ 8000, 8000 };
		std::string nombreCarta(const std::string& id) const { return id; }
		void cambiarLP(bool rival, int v) { lps[rival ? 1 : 0] = v; }
		int lp(bool rival) const { return lps[rival ? 1 : 0]; }
		void contarMazos(int, int, int) {}
	} interfazDuelo;
	struct EfectosCartas { int receta(const std::string&) const { return 0; } } efectosCartas;
	// (como entornos.h: un tema por numero o, desde classic-fondos, la clave de tema / el id del personaje)
	struct EntornosRival {
		int objetivo{ 0 };
		std::string clave;
		void pedir(int e) { objetivo = e, clave.clear(); }
		void pedir(const std::string& c) { clave = c; }
	} entornosRival;
	bool entornoFijo{ false };
	std::string entornoVista;
	void entornoDelRival(const ia::Rival*) {}
	void siguienteEntorno() {}
	// mesa / ataque (yugireload.cpp)
	enum { CAMPO_VACIO = 0, CAMPO_COLOCANDO, CAMPO_BOCA_ABAJO, CAMPO_ACTIVANDO, CAMPO_ACTIVO, CAMPO_DESTRUYENDO };
	int estadoCampo{ CAMPO_VACIO };
	bool ataquesSolos{ true }, campoAuto{ true };
	float tInvocada[control::NUM_ZONAS_MESA]{};
	int zonaAtacante{ -1 }, zonaObjetivo{ -1 }, danio{ 0 }, ataquesVistos{ 0 };
	bool danioAlRival{ false }, muereA{ false }, muereO{ false }, ataqueDemo{ false };
	float tAtaque{ -1.0f };
	static constexpr float T_IMPACTO = 0.55f, DURACION_ATAQUE = 1.6f;
	bool hayAtaque() const { return tAtaque >= 0.0f && zonaAtacante >= 0 && zonaObjetivo >= 0; }
	void activarEfectoCarta(const std::string&, int) {}
	void activarCampo(const std::string&) {}
	void destruirCampo(const std::string&) {}
	// mazo (mazo_duelo.h: lo que usa el duelo)
	mazo::Coleccion coleccion;
	mazo::Mazo mazoJugador, mazoDuelo;
	std::string rutaMazoJ;
	bool pedidoConstructor{ false };
	struct { bool abierto{ false }; } constructorMazo;
	void repartir()
	{
		std::string motivo;
		mazoDuelo = mazoJugador.valido(coleccion, &motivo) ? mazoJugador : mazo::Mazo::porDefecto(coleccion);
	}
	// ajustes (la ventana ImGui de la plantilla)
	struct { bool visible{ false }; } ui;
	struct { bool overlay{ true }; } settings;
	bool quit{ false };
	bool ajustesAbiertos() const { return settings.overlay && ui.visible; }
	// el mando
	control::Mando ctl;
	control::Repetidor repetidor;
	EstadoMando mando;
	float frameTimer{ 1.0f / 60.0f };
	bool mandoListo{ false };

#include "duelo_juego.h"

	void preparar(const std::string& ui_)
	{
		carpetaUi = ui_;
		rutaMazoJ = "/tmp/prueba_juego_mazo.txt";
		remove("/tmp/prueba_juego_progreso.txt");
		remove("/tmp/historia.txt");
		coleccion.leer(carpetaUi);
		mazoJugador = mazo::Mazo::porDefecto(coleccion);
		ctl.clase = [this](const std::string& c) {
			const ia::Carta* k = baseIA.carta(c);
			return k ? k->clase : std::string();
		};
		ctl.ataqueEnCurso = [this] { return hayAtaque(); };
		ctl.campoOcupado = [] { return false; };
		ctl.preparar({}, {}, {});
		ctl.log = getenv("PRUEBA_LOG") != nullptr;
		baseIA.leer(carpetaUi);
		prepararDuelo();
		mandoListo = true;
	}

	// ---- copia de yugireload.cpp (botonesDe, avanzarAtaque sin Vulkan, avanzarMando, procesarEventos)
	static uint32_t botonesDe(uint32_t pad)
	{
		uint32_t r = 0;
		if (pad & PAD_CROSS) r |= control::B_X;
		if (pad & PAD_CIRCLE) r |= control::B_CIRCULO;
		if (pad & PAD_TRIANGLE) r |= control::B_TRIANGULO;
		if (pad & PAD_SQUARE) r |= control::B_CUADRADO;
		if (pad & PAD_UP) r |= control::B_ARRIBA;
		if (pad & PAD_DOWN) r |= control::B_ABAJO;
		if (pad & PAD_LEFT) r |= control::B_IZQ;
		if (pad & PAD_RIGHT) r |= control::B_DER;
		if (pad & PAD_L1) r |= control::B_L1;
		if (pad & PAD_R1) r |= control::B_R1;
		if (pad & PAD_R3) r |= control::B_R3;
		return r;
	}
	void avanzarAtaque()
	{
		for (float& t : tInvocada) t += frameTimer;
		if (hayAtaque()) {
			tAtaque += frameTimer;
			if (tAtaque > DURACION_ATAQUE) {
				if (!ataqueDemo) {
					if (muereO) ctl.alCementerio(zonaObjetivo);
					if (muereA) ctl.alCementerio(zonaAtacante);
				}
				tAtaque = -1.0f;
				zonaAtacante = zonaObjetivo = -1;
			}
		}
	}
	void atacar(int a, int o, bool demo)
	{
		zonaAtacante = a;
		zonaObjetivo = o;
		tAtaque = 0.0f;
		ataqueDemo = demo;
	}
	void ejecutarEvento(const control::Evento& e)
	{
		if (e.tipo == control::EV_ATACAR) atacar(e.zona, e.zona2, false);
		if (e.tipo == control::EV_DEMO_APAGADA) ataquesSolos = campoAuto = false;
	}
	void procesarEventos()
	{
		ctl.eventos.clear();
	}
	// (el mismo orden que yugireload.cpp: pulsos -> pausa / Ajustes -> pantallas -> control.h)
	void avanzarMando()
	{
		const float dt = frameTimer;
		const uint32_t recien = botonesDe(mando.pressed);
		const uint32_t pulsos = repetidor.paso(botonesDe(mando.held), recien, 0.0f, 0.0f, dt) | (recien & ~control::B_DIRECCIONES);
		if (ctl.log) registrarBotones(recien);
		if (avanzarPausa(pulsos)) {
			procesarEventos();
			return;
		}
		if (ajustesAbiertos()) return;
		const bool toco = (mando.pressed & ~(uint32_t)(PAD_OPTIONS | PAD_TOUCH_PAD)) != 0;
		if (toco) ctl.tocar();
		if (avanzarPantallaDuelo(pulsos)) {
			procesarEventos();
			return;
		}
		ctl.avanzar(dt, pulsos);
		for (const control::Evento& e : ctl.eventos) ejecutarEvento(e);
		procesarEventos();
	}

	// un cuadro como render(): pad = lo apretado en este cuadro (se mantiene "held" mientras dure)
	void cuadro(uint32_t pad = 0)
	{
		mando.held = pad;
		mando.pressed = pad & ~padAntes;
		padAntes = pad;
		avanzarAtaque();
		avanzarMando();
		if (mandoListo && !ajustesAbiertos() && !constructorMazo.abierto && !pausa.abierta) avanzarDueloIA(frameTimer);
	}
	uint32_t padAntes{ 0 };
	// apretar y soltar un boton (como el usuario: ~0.1 s abajo) y esperar "espera" s
	void apretar(uint32_t pad, float espera = 0.3f)
	{
		for (int k = 0; k < (int)(0.1f / frameTimer); k++) cuadro(pad);
		esperar(espera);
	}
	void esperar(float s)
	{
		for (int k = 0; k < (int)(s / frameTimer); k++) cuadro(0);
	}
};

static Juego juego;

static const char* nombreFase(ia::Fase f)
{
	static const char* const N[] = { "ROBO", "PRINCIPAL 1", "BATALLA", "PRINCIPAL 2", "FINAL" };
	return N[(int)f];
}

// una jugada con el mando: la primera carta de la mano con algo que hacer (o, en la batalla, el primer monstruo que
// pueda atacar): X, la opcion, X, y X en cada eleccion (zona, sacrificio, objetivo, carta); luego espera la animacion
static int jugadasHechas = 0;
static void jugarAlgo(Juego& g, bool atacar)
{
	using namespace control;
	std::vector<Foco> focos;
	if (atacar) {
		for (int i = 0; i < ia::ZONAS; i++) focos.push_back({ F_ZONA, zonaPorId("jugador_monstruo_" + std::to_string(i)) });
	} else {
		for (int k = 0; k < (int)g.ctl.manoJ.size(); k++) focos.push_back({ F_MANO_J, k });
	}
	for (const Foco& f : focos) {
		if (g.ctl.bloqueado || g.ctl.estado != NAVEGAR) return;
		g.ctl.foco = f;
		g.apretar(PAD_CROSS, 0.2f);
		if (g.ctl.estado != MENU) {
			if (g.ctl.estado == DETALLE) g.apretar(PAD_CIRCLE, 0.2f);
			continue;
		}
		int elegida = -1;
		for (int k = 0; k < (int)g.ctl.opciones.size() && elegida < 0; k++) {
			const Opcion& o = g.ctl.opciones[(size_t)k];
			if (!o.activa) continue;
			if (atacar ? o.accion == AC_ATACAR : (o.accion == AC_INVOCAR || o.accion == AC_COLOCAR_MONSTRUO || o.accion == AC_ACTIVAR_EFECTO || o.accion == AC_COLOCAR_MT))
				elegida = k;
		}
		if (elegida < 0) {
			g.apretar(PAD_CIRCLE, 0.2f);
			continue;
		}
		g.ctl.opcion = elegida;
		g.apretar(PAD_CROSS, 0.3f);
		for (int k = 0; k < 8 && g.ctl.estado != NAVEGAR; k++) {
			if (g.ctl.estado == MENU) g.ctl.opcion = 0;
			g.apretar(PAD_CROSS, 0.4f);
		}
		jugadasHechas++;
		g.esperar(2.5f);
		return;
	}
}

int main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "uso: prueba_juego <carpeta ui/> [duelos]\n");
		return 2;
	}
	const int N = argc > 2 ? atoi(argv[2]) : 3;
	Juego& g = juego;
	if (const char* f = getenv("PRUEBA_HZ")) g.frameTimer = 1.0f / (float)atof(f);   // (la consola: 120 Hz)
	g.preparar(std::string(argv[1]) + "/");
	using VD = control::VistaDuelo;
	if (!g.baseIAlista) {
		fprintf(stderr, "sin datos en %s\n", argv[1]);
		return 2;
	}
	int turnosVistos = 0, fasesJugador = 0, jugadasTurno = 0, ataquesTurno = 0;
	for (int n = 0; n < N; n++) {
		// arranque: pantalla de elegir rival (modo historia si hay personajes)
		g.esperar(1.0f);
		if (g.ctl.vista.pantalla != VD::ELEGIR_RIVAL) FALLO("no arranca en elegir rival (pantalla %d)", g.ctl.vista.pantalla);
		if (n == 0 && g.ctl.vista.modo != VD::HISTORIA) FALLO("no arranca en modo historia");
		if (const char* r = getenv("PRUEBA_RIVAL")) {   // (otro personaje de la saga: con la cruceta, como el usuario)
			for (int k = 0; k < 12 && g.ctl.vista.personaje != r; k++) g.apretar(PAD_RIGHT, 0.2f);
		}
		g.apretar(PAD_CROSS, 1.0f);   // elegir el rival (el primero sin vencer)
		if (g.ctl.vista.pantalla != VD::DUELO) {
			FALLO("X no empieza el duelo (pantalla %d, modo %d, saga %d idx %d abierta %d, aviso %s)", g.ctl.vista.pantalla, g.ctl.vista.modo, g.ctl.vista.saga, g.ctl.vista.idx, (int)g.ctl.vista.sagaAbierta, g.ctl.aviso.c_str());
			break;
		}
		g.esperar(2.0f);   // el cartel de inicio y los sucesos del reparto
		const int turnoInicial = g.dueloIA.turno;
		int vueltas = 0;
		// el jugador pasa por todas sus fases con CUADRADO; el rival juega solo; 4 turnos completos
		while (g.dueloIA.ganador < 0 && g.dueloIA.turno < turnoInicial + 4 && vueltas++ < 400) {
			if (g.dueloIA.activo == 0 && g.dueloIA.decide() == 0) {
				const ia::Fase antes = g.dueloIA.fase;
				const int turnoAntes = g.dueloIA.turno;
				if (g.ctl.estado == control::MENU) {
					// una pregunta del duelo (responder): "No activar"
					g.ctl.opcion = (int)g.ctl.opciones.size() - 1;
					g.apretar(PAD_CROSS, 0.6f);
					continue;
				}
				if (g.dueloIA.fase == ia::F_FINAL && (int)g.dueloIA.l[0].mano.size() > ia::MAX_MANO) {
					// descartar: la primera de la mano (menu de la carta -> "Descartar")
					g.ctl.foco = control::Foco{ control::F_MANO_J, 0 };
					g.apretar(PAD_CROSS, 0.3f);
					g.apretar(PAD_CROSS, 1.0f);
					continue;
				}
				// como un jugador: en la principal 1 juega algo de la mano (invocar, colocar, activar) y en la batalla ataca,
				// todo con el mando (menus, zonas, objetivos); luego CUADRADO
				if (g.dueloIA.fase == ia::F_PRINCIPAL1 && jugadasTurno < 2) {
					jugadasTurno++;
					jugarAlgo(g, false);
					continue;
				}
				if (g.dueloIA.fase == ia::F_BATALLA && ataquesTurno < 2) {
					ataquesTurno++;
					jugarAlgo(g, true);
					continue;
				}
				g.apretar(PAD_SQUARE, 1.0f);
				// UN solo cuadrado: si llego con el duelo ocupado (animacion, el rival responde) queda pedido y se cumple
				// cuando el jugador queda libre (antes se perdia y "no pasaba de fase")
				for (int k = 0; k < 30 && (g.ctl.bloqueado || g.ctl.fasePedida) && g.dueloIA.fase == antes && g.dueloIA.activo == 0; k++) g.esperar(0.5f);
				if (g.dueloIA.fase == antes && g.dueloIA.turno == turnoAntes && g.dueloIA.activo == 0 && g.dueloIA.ganador < 0 &&
				    g.dueloIA.decide() == 0 && g.ctl.estado != control::MENU) {
					FALLO("CUADRADO no cambia de fase: turno %d, fase %s, bloqueado %d, estado %s, aviso '%s'", g.dueloIA.turno, nombreFase(antes),
					      (int)g.ctl.bloqueado, control::NOMBRE_ESTADO[g.ctl.estado], g.ctl.aviso.c_str());
					fprintf(stderr, "  duelo: preg %d (lado %d), cadena %d, resolviendo %d, pendiente %d, decide %d, sucesos %d/%d, esperaEfecto %.2f, ataque %d\n",
					        (int)g.dueloIA.ef.preg.tipo, g.dueloIA.ef.preg.lado, (int)g.dueloIA.ef.cadena.size(), (int)g.dueloIA.ef.resolviendo, (int)g.dueloIA.ef.pendiente,
					        g.dueloIA.decide(), (int)g.sucesosVistos, (int)g.dueloIA.sucesos.size(), g.esperaEfecto, (int)g.hayAtaque());
					for (size_t k = g.dueloIA.sucesos.size() > 12 ? g.dueloIA.sucesos.size() - 12 : 0; k < g.dueloIA.sucesos.size(); k++)
						fprintf(stderr, "  suceso %d lado %d zona %d valor %d carta %s\n", (int)g.dueloIA.sucesos[k].tipo, g.dueloIA.sucesos[k].lado, g.dueloIA.sucesos[k].zona,
						        g.dueloIA.sucesos[k].valor, g.dueloIA.sucesos[k].carta.c_str());
					break;
				}
				fasesJugador++;
				if (g.dueloIA.turno != turnoAntes || g.dueloIA.activo != 0) jugadasTurno = ataquesTurno = 0;
				if (g.ctl.fase != Juego::faseInterfaz(g.dueloIA.fase) && g.dueloIA.activo == 0)
					FALLO("la fase en pantalla (%d) no es la del duelo (%s)", g.ctl.fase, nombreFase(g.dueloIA.fase));
			} else {
				// turno del rival (o una pregunta del rival): solo esperar; si el jugador tiene que responder, "No"
				const int t0 = g.dueloIA.turno;
				float s = 0.0f;
				while (g.dueloIA.ganador < 0 && (g.dueloIA.activo == 1 || g.dueloIA.decide() == 1) && s < 120.0f) {
					if (g.dueloIA.decide() == 0 && g.ctl.estado == control::MENU) {
						g.ctl.opcion = (int)g.ctl.opciones.size() - 1;
						g.apretar(PAD_CROSS, 0.3f);
					}
					g.esperar(0.5f);
					s += 0.5f;
				}
				if (g.dueloIA.activo == 1 && g.dueloIA.ganador < 0) {
					FALLO("el turno %d del rival no termina (fase %s)", t0, nombreFase(g.dueloIA.fase));
					break;
				}
				g.esperar(1.0f);
			}
			turnosVistos = std::max(turnosVistos, g.dueloIA.turno);
		}
		if (g.dueloIA.ganador < 0 && g.dueloIA.turno < turnoInicial + 4) FALLO("el duelo %d no llego a 4 turnos (turno %d)", n, g.dueloIA.turno);
		printf("duelo %d: contra %s, turnos %d -> %d, fases del jugador con CUADRADO %d, LP %d / %d\n", n, g.ctl.vista.rivalEs.c_str(), turnoInicial,
		       g.dueloIA.turno, fasesJugador, g.dueloIA.l[0].lp, g.dueloIA.l[1].lp);
		// salir: con la pausa (OPTIONS corto). Alternando: Abandonar duelo / Salir al menu principal
		if (g.ctl.vista.pantalla == VD::DUELO) {
			g.esperar(1.0f);
			if (g.ctl.vista.pantalla == VD::DUELO) {
				const int derrotas = g.ctl.vista.derrotas;
				g.apretar(PAD_OPTIONS, 0.3f);
				if (!g.pausa.abierta) FALLO("OPTIONS (corto) no abre la pausa");
				if (g.ajustesAbiertos()) FALLO("OPTIONS corto abrio los Ajustes");
				// Continuar: cierra y el duelo sigue
				g.pausa.opcion = 0;
				g.apretar(PAD_CROSS, 0.3f);
				if (g.pausa.abierta || g.ctl.vista.pantalla != VD::DUELO) FALLO("Continuar no vuelve al duelo");
				g.apretar(PAD_OPTIONS, 0.3f);
				const int quiero = n % 2 == 0 ? Juego::PA_ABANDONAR : Juego::PA_MENU_PRINCIPAL;
				for (int k = 0; k < 8 && g.opcionesPausa()[(size_t)g.pausa.opcion] != quiero; k++) g.apretar(PAD_DOWN, 0.2f);
				g.apretar(PAD_CROSS, 0.5f);
				if (quiero == Juego::PA_ABANDONAR && g.ctl.vista.derrotas != derrotas + 1) FALLO("Abandonar duelo no cuenta la derrota");
				if (g.ctl.vista.pantalla != VD::ELEGIR_RIVAL || g.pausa.abierta) FALLO("la pausa no vuelve a elegir rival (pantalla %d)", g.ctl.vista.pantalla);
			}
		} else if (g.ctl.vista.pantalla == VD::RESULTADO) {
			g.esperar(1.0f);
			g.apretar(PAD_CROSS, 1.0f);
		}
		// en elegir rival: la pausa tambien sirve (cambiar de modo historia / libre)
		if (n == 1 && g.ctl.vista.pantalla == VD::ELEGIR_RIVAL) {
			const int modo = g.ctl.vista.modo;
			g.apretar(PAD_OPTIONS, 0.3f);
			if (!g.pausa.abierta) FALLO("OPTIONS no abre la pausa en elegir rival");
			const int quiero = modo == VD::HISTORIA ? Juego::PA_DUELO_LIBRE : Juego::PA_HISTORIA;
			for (int k = 0; k < 8 && g.opcionesPausa()[(size_t)g.pausa.opcion] != quiero; k++) g.apretar(PAD_DOWN, 0.2f);
			g.apretar(PAD_CROSS, 0.5f);
			if (g.ctl.vista.modo == modo) FALLO("la pausa no cambia de modo");
		}
	}
	// OPTIONS mantenido 2 s (lo hace la plantilla): no abre la pausa al soltar
	{
		g.apretar(PAD_CIRCLE, 0.2f);
		const bool antes = g.pausa.abierta;
		for (int k = 0; k < 130; k++) g.cuadro(PAD_OPTIONS);
		g.esperar(0.3f);
		if (g.pausa.abierta != antes) FALLO("mantener OPTIONS 2 s abre/cierra la pausa (debe ser solo Ajustes)");
	}
	printf("turnos jugados hasta %d, fases cambiadas por el jugador %d\n", turnosVistos, fasesJugador);
	printf(fallos ? "PRUEBA CON FALLOS (%d)\n" : "PRUEBA OK (%d fallos)\n", fallos);
	return fallos ? 1 : 0;
}
