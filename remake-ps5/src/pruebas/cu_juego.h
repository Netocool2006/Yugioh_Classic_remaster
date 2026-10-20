// Casos de uso (rama classic-ciclos): el juego SIN GPU para las pruebas del front. Incluye dentro de la clase, igual
// que yugireload.cpp, el codigo REAL de las pantallas: mazo_duelo.h (constructor y lista de cartas), duelo_juego.h
// (elegir rival, sorteo, duelo, pausa, progreso), velocidad_juego.h, resultado_juego.h (resultado, revancha),
// duelo_consulta.h (registro y visores), menu_principal.h (menu y Opciones), cosmeticos_juego.h (Personalizacion) y
// portada.h. Solo son maquetas lo que es de Vulkan / sonido / figuras / entornos (y la entrada de cartas clave).
// Cada cuadro hace lo mismo que render(): avanzarAtaque -> avanzarConstructor -> avanzarMando -> avanzarDueloIA ->
// avanzarConsulta, mas el reloj de lo que en la consola avanza la interfaz (dialogos, reglas, Personalizacion).
#pragma once

#include <glm/glm.hpp>

#include "control.h"
#include "duelo_visores.h"
#include "mazo.h"
#include "constructor.h"
#include "personajes.h"
#include "historia_dialogos.h"
#include "tutorial.h"
#include "opciones.h"
#include "velocidad.h"
#include "resultado.h"
#include "cosmeticos.h"
#include "estadisticas.h"
#include "guardado.h"
#include "idioma.h"

#include <atomic>
#include <chrono>
#include <set>

// ---- maquetas de lo que el juego tiene en Vulkan / audio / entornos / interfaz
namespace efc {
inline float duracion(int) { return 1.0f; }
}
namespace entornos {
inline int porTema(const std::string&) { return 0; }
inline int claveDe(const std::string&) { return 0; }
static const char* const CLAVES[] = { "neutro" };
}
namespace campos {
struct Campo { const char* carta; };
inline const Campo* buscar(const std::string& carta)
{
	// (la tabla real esta en campos.h, que usa Vulkan: la prueba de campos la lee del fuente)
	static std::vector<Campo> t;
	static std::vector<std::string> ids;
	for (size_t k = 0; k < ids.size(); k++)
		if (ids[k] == carta) return &t[k];
	return nullptr;
}
inline bool esCampo(const std::string& c) { return buscar(c) != nullptr; }
}
namespace interfaz {
constexpr int NUM_MANO_J = 6, NUM_MANO_R = 5, MAX_MANO_J = 12;
enum ModoCamara { CAM_CINE = 0, CAM_TABLERO, CAM_CENITAL, CAM_LATERAL, CAM_LIBRE, NUM_CAMARAS };
static const char* const CLAVE_CAMARA[NUM_CAMARAS] = { "cine", "tablero", "cenital", "lateral", "libre" };
struct Vista { glm::vec3 ojo, mira; };
inline void libreDesde(const Vista&, float&, float&, float&) {}
}

enum : uint32_t {
	PAD_L3 = 0x000002, PAD_R3 = 0x000004, PAD_OPTIONS = 0x000008, PAD_UP = 0x000010, PAD_RIGHT = 0x000020, PAD_DOWN = 0x000040, PAD_LEFT = 0x000080,
	PAD_L1 = 0x000400, PAD_R1 = 0x000800, PAD_TRIANGLE = 0x001000, PAD_CIRCLE = 0x002000, PAD_CROSS = 0x004000, PAD_SQUARE = 0x008000,
	PAD_TOUCH_PAD = 0x100000,
};
struct EstadoMando { uint32_t pressed{ 0 }, held{ 0 }; float left_x{ 0 }, left_y{ 0 }, right_x{ 0 }, right_y{ 0 }, l2{ 0 }, r2{ 0 }; };

struct Juego {
	std::string carpetaAssets;   // con "ui/" adentro
	std::string getAssetPath() const { return carpetaAssets; }
	// sonido: se anota lo que suena (los casos miran la musica y los sfx)
	struct Sonido {
		std::string musica;
		std::vector<std::string> sfxs;
		void tocarMusica(const char* m, float) { musica = m; }
		void sfx(const char* s, float = 1.0f, float = 0.0f, float = 0.0f) { sfxs.push_back(s); }
		bool hay(const char*) const { return true; }
		void pararMusica(float) { musica.clear(); }
		std::atomic<float> volMusica{ 1.0f }, volEfectos{ 1.0f };
	} sonido;
	struct Interfaz {
		std::function<bool(const char*, int&, int&)> statsZona;
		int lps[2]{ 8000, 8000 };
		float portadaNegro{ 0.0f };
		std::string nombreCarta(const std::string& id) const { return id; }
		void cambiarLP(bool rival, int v) { lps[rival ? 1 : 0] = v; }
		int lp(bool rival) const { return lps[rival ? 1 : 0]; }
		int mazos[3]{ -1, -1, -1 };
		void contarMazos(int j, int r, int f) { mazos[0] = j, mazos[1] = r, mazos[2] = f; }
		void conectarConsulta(const visores::Consulta*) {}
		void conectarConstructor(const constructor::Constructor*) {}
	} interfazDuelo;
	struct EfectosCartas { int receta(const std::string&) const { return 0; } } efectosCartas;
	struct EntornosRival {
		int objetivo{ 0 };
		std::string clave;
		void pedir(int e) { objetivo = e, clave.clear(); }
		void pedir(const std::string& c) { clave = c; }
		void soltarCampo() {}
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
	void activarCampo(const std::string&, int = 0) {}
	void destruirCampo(const std::string&, int = 0) {}
	bool bosqueVisible() const { return false; }
	const campos::Campo* campoMesa() const { return nullptr; }
	// camara (cosmeticos_juego.h)
	int modoCamara{ interfaz::CAM_CINE };
	float tCambioCamara{ 99.0f };
	float libreGiro{ 0 }, libreElev{ 0 }, libreDist{ 0 };
	glm::vec3 OJO{ 0.0f }, OBJETIVO{ 0.0f };
	using Vista = interfaz::Vista;
	void libreDesde(const Vista& v, float& g, float& e, float& d) { interfaz::libreDesde(v, g, e, d); }
	// figuras 3D (figuras.h): las de datos/figuras.txt
	bool figuras3D{ true };
	std::set<std::string> conFigura;
	int figuraDe(const std::string& id) const { return conFigura.count(id) ? 0 : -1; }
	// ajustes (ImGui), guion y teclado del PC
	struct { bool visible{ false }; } ui;
	struct { bool overlay{ true }; } settings;
	bool quit{ false };
	bool ajustesAbiertos() const { return settings.overlay && ui.visible; }
	uint32_t teclas{ 0 };
	control::Guion guion;
	// el mando
	control::Mando ctl;
	control::Repetidor repetidor;
	EstadoMando mando;
	float frameTimer{ 1.0f / 60.0f };
	bool mandoListo{ false };

#include "mazo_duelo.h"
#include "duelo_juego.h"
#include "velocidad_juego.h"
#include "resultado_juego.h"
#include "duelo_consulta.h"
	// entrada_clave_juego.h (camara, figura y efectos de la entrada de una carta clave): maqueta
	float empezarEntradaClave(const std::string&, int, int) { return 0.0f; }
	void avanzarEntradaClave(float) {}
#include "menu_principal.h"
#include "cosmeticos_juego.h"
#include "portada.h"

	// como el constructor de la clase y prepare() de yugireload.cpp: mazos, duelo, menu (portada)
	void preparar(const std::string& assets)
	{
		carpetaAssets = assets;
		ctl.clase = [this](const std::string& c) {
			const ia::Carta* k = baseIA.carta(c);
			return k ? k->clase : std::string();
		};
		ctl.ataqueEnCurso = [this] { return hayAtaque(); };
		ctl.campoOcupado = [] { return false; };
		ctl.preparar({}, {}, {});
		prepararMazos();
		prepararDuelo();
		prepararMenu();
		mandoListo = true;
	}

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
		if (pad & PAD_L3) r |= control::B_L3;
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
	void ejecutarEvento(const control::Evento& e)
	{
		if (e.tipo == control::EV_ATACAR) {
			zonaAtacante = e.zona;
			zonaObjetivo = e.zona2;
			tAtaque = 0.0f;
			ataqueDemo = false;
		}
		if (e.tipo == control::EV_DEMO_APAGADA) ataquesSolos = campoAuto = false;
	}
	void procesarEventos() { ctl.eventos.clear(); }
	// avanzarMando de yugireload.cpp (sin el guion ni los Ajustes de la plantilla)
	void avanzarMando()
	{
		if (!mandoListo || constructorMazo.abierto) return;
		const float dt = glm::clamp(frameTimer, 0.0f, 0.1f);
		const uint32_t recien = botonesDe(mando.pressed);
		const uint32_t pulsos = repetidor.paso(botonesDe(mando.held), recien, mando.left_x, mando.left_y, dt) | (recien & ~control::B_DIRECCIONES);
		vigilarAjustes();
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
		if (mandoConsulta(pulsos, dt)) {
			procesarEventos();
			return;
		}
		ctl.ejeDetalle = fabsf(mando.right_y) > 0.2f ? mando.right_y : 0.0f;
		ctl.avanzar(dt * escalaMesa(), pulsos);
		for (const control::Evento& e : ctl.eventos) ejecutarEvento(e);
		procesarEventos();
	}
	// un cuadro como render(): pad = lo apretado en este cuadro (held mientras dure)
	uint32_t padAntes{ 0 };
	void cuadro(uint32_t pad = 0)
	{
		mando.held = pad;
		mando.pressed = pad & ~padAntes;
		padAntes = pad;
		avanzarAtaque();
		avanzarConstructor();
		avanzarMando();
		if (mandoListo && !ajustesAbiertos() && !constructorMazo.abierto && !pausa.abierta) {
			avanzarDueloIA(glm::clamp(frameTimer, 0.0f, 0.1f));
			avanzarConsulta(glm::clamp(frameTimer, 0.0f, 0.1f));
		}
		// lo que en la consola avanza la interfaz al dibujar
		historia_dialogos::escena().cuadro(frameTimer);
		tutorial::estado().cuadro(frameTimer);
		cosmeticos::vista().t += frameTimer;
	}
	void apretar(uint32_t pad, float espera = 0.3f)
	{
		for (int k = 0; k < (int)(0.1f / frameTimer); k++) cuadro(pad);
		esperar(espera);
	}
	void esperar(float s)
	{
		for (int k = 0; k < (int)(s / frameTimer); k++) cuadro(0);
	}
	// mantener un boton s segundos
	void mantener(uint32_t pad, float s)
	{
		for (int k = 0; k < (int)(s / frameTimer); k++) cuadro(pad);
		cuadro(0);
	}
};
