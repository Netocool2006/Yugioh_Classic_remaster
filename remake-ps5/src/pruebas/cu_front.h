// Casos de uso del FRONT (rama classic-ciclos): portada, menu principal, Opciones, Personalizacion, constructor y
// lista de cartas (entrada y salida), elegir rival (historia, libre, desafio, reglas), dialogos, duelo (sorteo,
// reparto, fases, detalle, registro y visores, cadena visible, jugables), pausa, resultado (motivos, revancha,
// desbloqueos, final), idioma ES/EN, mando y guardado. Todo con el juego sin GPU (cu_juego.h) y el mando.
#pragma once

#include "casos_uso.h"
#include "cu_juego.h"

#include <dirent.h>
#include <sys/stat.h>

namespace cuf {

using VD = control::VistaDuelo;

inline std::string leerArchivo(const std::string& ruta)
{
	std::ifstream f(ruta);
	std::stringstream ss;
	ss << f.rdbuf();
	return ss.str();
}
inline bool existe(const std::string& ruta)
{
	struct stat st{};
	return stat(ruta.c_str(), &st) == 0;
}
inline bool contiene(const std::string& s, const std::string& x) { return s.find(x) != std::string::npos; }
// el texto tiene version inglesa (o es igual en los dos idiomas)
inline bool traducido(const std::string& es)
{
	static const std::set<std::string> iguales = { "Auto", "Normal", "English", "Español", "IA", "OK" };
	if (iguales.count(es)) return true;
	return idioma::tabla().count(es) > 0;
}

// ------------------------------------------------------------------------------------------------- portada
inline void portada(Juego& g)
{
	VD& v = g.ctl.vista;
	cu::caso("CU-FE-PORTADA-ARRANQUE", v.pantalla == VD::PORTADA, "al arrancar la pantalla es %d (esperada la portada)", v.pantalla);
	cu::caso("CU-FE-PORTADA-ARRANQUE", g.sonido.musica == "musica_menu", "musica '%s' en la portada", g.sonido.musica.c_str());
	cu::caso("CU-FE-PORTADA-ARRANQUE", g.interfazDuelo.portadaNegro >= 0.99f, "la portada no empieza desde negro");
	g.esperar(0.2f);
	g.apretar(PAD_CROSS, 0.05f);   // (antes de 0.8 s: no cuenta)
	cu::caso("CU-FE-PORTADA-ESPERA", g.portadaSalida < 0.0f && v.pantalla == VD::PORTADA, "X antes del fundido de entrada ya sale de la portada");
	g.esperar(1.0f);
	g.apretar(PAD_OPTIONS, 0.1f);
	cu::caso("CU-FE-PORTADA-SIN-PAUSA", !g.pausa.abierta, "OPTIONS en la portada abrio la pausa");
	g.esperar(1.0f);
	cu::caso("CU-FE-PORTADA-OPTIONS", v.pantalla == VD::MENU, "OPTIONS en la portada no fue al menu (pantalla %d)", v.pantalla);
	g.irAPortada();
	g.esperar(1.0f);
	g.apretar(PAD_CROSS, 0.1f);
	cu::caso("CU-FE-PORTADA-X", g.portadaSalida >= 0.0f || v.pantalla == VD::MENU, "X no empezo el fundido de salida");
	cu::caso("CU-FE-PORTADA-X", std::find(g.sonido.sfxs.begin(), g.sonido.sfxs.end(), "sfx_menu_confirmar") != g.sonido.sfxs.end(), "X no sono");
	g.esperar(1.0f);
	cu::caso("CU-FE-PORTADA-X", v.pantalla == VD::MENU, "X en la portada no llego al menu (pantalla %d)", v.pantalla);
}

// ------------------------------------------------------------------------------------------------- menu principal
inline void irAlMenu(Juego& g)
{
	if (g.constructorMazo.abierto) g.constructorMazo.abierto = false;
	g.pausa.abierta = false;
	g.irAMenuPrincipal();
	g.esperar(0.2f);
}
inline void elegirMenu(Juego& g, int accion)
{
	const std::vector<int> ops = g.accionesMenu();
	for (int k = 0; k < 10 && ops[(size_t)g.menuP.opcion] != accion; k++) g.apretar(PAD_DOWN, 0.15f);
	g.apretar(PAD_CROSS, 0.3f);
}

inline void menu(Juego& g)
{
	VD& v = g.ctl.vista;
	irAlMenu(g);
	const std::vector<std::string> esperado = { "MODO DUELO", "CONSTRUCCIÓN DE DECK", "LISTA DE CARTAS", "OPCIONES", "SALIR" };
	cu::caso("CU-FE-MENU-BOTONES-5", v.menuOpciones == esperado, "el menu muestra %d botones (esperados 5 en orden)", (int)v.menuOpciones.size());
	cu::caso("CU-FE-MENU-MUSICA", g.sonido.musica == "musica_menu", "musica '%s' en el menu", g.sonido.musica.c_str());
	// navegar: arriba desde el primero va al ultimo; abajo desde el ultimo vuelve al primero
	g.menuP.opcion = 0;
	g.mostrarMenu();
	g.apretar(PAD_UP, 0.15f);
	cu::caso("CU-FE-MENU-NAVEGAR", g.menuP.opcion == 4 && v.menuOpcion == 4, "arriba desde MODO DUELO fue a %d", g.menuP.opcion);
	g.apretar(PAD_DOWN, 0.15f);
	cu::caso("CU-FE-MENU-NAVEGAR", g.menuP.opcion == 0, "abajo desde SALIR fue a %d", g.menuP.opcion);
	// la cruz mantenida repite (rama classic-menus)
	g.mantener(PAD_DOWN, 0.55f);   // (un paso al apretar y uno o dos de repeticion: 0.40 s y luego cada 0.09 s)
	cu::caso("CU-FE-MENU-NAVEGAR", g.menuP.opcion == 2 || g.menuP.opcion == 3, "mantener abajo 0.55 s movio a %d (esperado 2 o 3)", g.menuP.opcion);
	// MODO DUELO -> elegir rival
	irAlMenu(g);
	g.menuP.opcion = 0;
	elegirMenu(g, Juego::MP_DUELO);
	cu::caso("CU-FE-MENU-DUELO", v.pantalla == VD::ELEGIR_RIVAL, "MODO DUELO no fue a elegir rival (pantalla %d)", v.pantalla);
	// CONSTRUCCION DE DECK -> el constructor (se abre en el cuadro siguiente)
	irAlMenu(g);
	elegirMenu(g, Juego::MP_CONSTRUIR);
	cu::caso("CU-FE-MENU-CONSTRUIR", g.constructorMazo.abierto && !g.constructorMazo.soloVer, "CONSTRUCCION DE DECK no abrio el constructor");
	cu::caso("CU-FE-MENU-CONSTRUIR", g.sonido.musica == "musica_constructor", "musica '%s' en el constructor", g.sonido.musica.c_str());
	cu::caso("CU-FE-CONS-MIS-MAZOS", g.constructorMazo.pantalla == constructor::P_LISTA && !g.biblioteca.mazos.empty(), "el constructor no abrio en MIS MAZOS");
	// circulo en MIS MAZOS: vuelve al menu
	g.apretar(PAD_CIRCLE, 0.3f);
	cu::caso("CU-FE-CONS-CERRAR", !g.constructorMazo.abierto && v.pantalla == VD::MENU, "circulo en MIS MAZOS no volvio al menu");
	cu::caso("CU-FE-CONS-CERRAR", g.sonido.musica == "musica_menu", "al cerrar el constructor suena '%s'", g.sonido.musica.c_str());
	// LISTA DE CARTAS -> la coleccion en modo solo ver
	elegirMenu(g, Juego::MP_LISTA);
	cu::caso("CU-FE-MENU-LISTA", g.constructorMazo.abierto && g.constructorMazo.soloVer, "LISTA DE CARTAS no abrio la coleccion en modo solo ver");
	cu::caso("CU-FE-LISTA-CARTAS", g.constructorMazo.lista.size() > 1000, "la lista de cartas muestra %d cartas", (int)g.constructorMazo.lista.size());
	g.apretar(PAD_CIRCLE, 0.3f);
	cu::caso("CU-FE-LISTA-CARTAS", !g.constructorMazo.abierto, "circulo no cerro la lista de cartas");
	// OPCIONES
	irAlMenu(g);
	elegirMenu(g, Juego::MP_OPCIONES);
	cu::caso("CU-FE-MENU-OPCIONES", g.menuP.enOpciones && v.menuEnOpciones && (int)v.menuOpciones.size() == Juego::NUM_OPCIONES, "OPCIONES no abrio las 11 opciones");
	g.apretar(PAD_CIRCLE, 0.2f);
	cu::caso("CU-FE-MENU-OPCIONES", !g.menuP.enOpciones && v.pantalla == VD::MENU, "circulo en Opciones no volvio al menu");
	// SALIR: pide confirmar; otro boton cancela; X dos veces sale
	elegirMenu(g, Juego::MP_SALIR);
	cu::caso("CU-FE-MENU-SALIR", g.menuP.confirmarSalir && v.menuSalir && !g.quit, "SALIR no pidio confirmar");
	g.apretar(PAD_CIRCLE, 0.2f);
	cu::caso("CU-FE-MENU-SALIR-CANCELA", !g.menuP.confirmarSalir && !g.quit, "circulo no cancelo salir");
	g.apretar(PAD_CROSS, 0.2f);
	g.apretar(PAD_CROSS, 0.2f);
	cu::caso("CU-FE-MENU-SALIR", g.quit, "X dos veces en SALIR no cerro el juego");
	g.quit = false;
	// la pausa no se abre en el menu
	g.apretar(PAD_OPTIONS, 0.2f);
	cu::caso("CU-FE-MENU-SIN-PAUSA", !g.pausa.abierta, "OPTIONS en el menu principal abrio la pausa");
}

// ------------------------------------------------------------------------------------------------- Opciones
inline void irAOpcion(Juego& g, int o)
{
	if (!g.menuP.enOpciones) {
		irAlMenu(g);
		elegirMenu(g, Juego::MP_OPCIONES);
	}
	for (int k = 0; k < 14 && g.menuP.opcionOpc != o; k++) g.apretar(PAD_DOWN, 0.12f);
}
inline std::string valorOpcion(Juego& g, int o)
{
	g.mostrarMenu();
	return o < (int)g.ctl.vista.menuValores.size() ? g.ctl.vista.menuValores[(size_t)o] : std::string("?");
}

inline void opcionesMenu(Juego& g, const std::string& dir)
{
	VD& v = g.ctl.vista;
	const std::string archivo = g.rutaOpciones;
	cu::caso("CU-FE-OPC-RUTA", archivo == dir + "opciones.txt", "opciones.txt en '%s' (esperado junto a progreso.txt)", archivo.c_str());
	irAlMenu(g);
	elegirMenu(g, Juego::MP_OPCIONES);
	const char* const NOMBRES[] = { "Volumen de la música", "Volumen de los efectos", "Permitir cartas prohibidas", "Preguntar antes de activar", "Velocidad de animación",
		                            "Dificultad adaptable", "IA", "Figuras 3D", "Idioma", "Personalización", "Volver" };
	bool orden = (int)v.menuOpciones.size() == Juego::NUM_OPCIONES;
	for (int o = 0; orden && o < Juego::NUM_OPCIONES; o++) orden = v.menuOpciones[(size_t)o] == NOMBRES[o];
	cu::caso("CU-FE-OPC-LISTA", orden, "las opciones no son las 11 esperadas en orden");
	// musica: der/izq de a 1/10, entre 0 y 10, se guarda al instante
	irAOpcion(g, Juego::OP_MUSICA);
	g.sonido.volMusica = 1.0f;
	g.apretar(PAD_LEFT, 0.12f);
	g.apretar(PAD_LEFT, 0.12f);
	g.apretar(PAD_LEFT, 0.12f);
	cu::caso("CU-FE-OPC-MUSICA", fabsf(g.sonido.volMusica - 0.7f) < 0.01f && valorOpcion(g, Juego::OP_MUSICA) == "7 / 10", "volumen de la musica %.2f ('%s') tras 3 a la izquierda",
	         g.sonido.volMusica.load(), valorOpcion(g, Juego::OP_MUSICA).c_str());
	cu::caso("CU-FE-OPC-MUSICA", contiene(leerArchivo(archivo), "musica=0.70"), "opciones.txt no guardo musica=0.70");
	for (int k = 0; k < 6; k++) g.apretar(PAD_RIGHT, 0.12f);
	cu::caso("CU-FE-OPC-MUSICA", fabsf(g.sonido.volMusica - 1.0f) < 0.01f, "el volumen pasa de 10 (%.2f)", g.sonido.volMusica.load());
	for (int k = 0; k < 12; k++) g.apretar(PAD_LEFT, 0.12f);
	cu::caso("CU-FE-OPC-MUSICA", g.sonido.volMusica.load() == 0.0f, "el volumen baja de 0 (%.2f)", g.sonido.volMusica.load());
	g.sonido.volMusica = 0.8f;
	// efectos: igual y suena de prueba
	irAOpcion(g, Juego::OP_EFECTOS);
	g.sonido.volEfectos = 0.5f;
	g.sonido.sfxs.clear();
	g.apretar(PAD_RIGHT, 0.12f);
	cu::caso("CU-FE-OPC-EFECTOS", fabsf(g.sonido.volEfectos - 0.6f) < 0.01f && contiene(leerArchivo(archivo), "efectos=0.60"), "efectos %.2f tras uno a la derecha", g.sonido.volEfectos.load());
	cu::caso("CU-FE-OPC-EFECTOS", std::find(g.sonido.sfxs.begin(), g.sonido.sfxs.end(), "sfx_menu_confirmar") != g.sonido.sfxs.end(), "el volumen de efectos no suena de prueba");
	// permitir prohibidas: X cambia; el constructor lo aplica a la coleccion
	irAOpcion(g, Juego::OP_PROHIBIDAS);
	const bool antesP = opciones::permitirProhibidas();
	g.apretar(PAD_CROSS, 0.12f);
	cu::caso("CU-FE-OPC-PROHIBIDAS", opciones::permitirProhibidas() != antesP && valorOpcion(g, Juego::OP_PROHIBIDAS) == (antesP ? "No" : "Sí"), "X no cambio Permitir prohibidas");
	cu::caso("CU-FE-OPC-PROHIBIDAS", contiene(leerArchivo(archivo), std::string("permitir_prohibidas=") + (antesP ? "0" : "1")), "no se guardo permitir_prohibidas");
	g.leerOpcionBanlist();
	const mazo::Carta* prohibida = nullptr;
	for (const mazo::Carta& k : g.coleccion.cartas)
		if (k.limiteBase == 0 && !k.firma) prohibida = &k;
	cu::caso("CU-FE-OPC-PROHIBIDAS", g.coleccion.permitirProhibidas == opciones::permitirProhibidas(), "la coleccion no aplico la opcion");
	if (prohibida) cu::caso("CU-FE-OPC-PROHIBIDAS", prohibida->limite == (opciones::permitirProhibidas() ? 3 : 0), "%s: limite %d con la opcion en %d", prohibida->id.c_str(), prohibida->limite, (int)opciones::permitirProhibidas());
	irAOpcion(g, Juego::OP_PROHIBIDAS);
	g.apretar(PAD_CROSS, 0.12f);   // (de vuelta)
	g.leerOpcionBanlist();
	if (prohibida) cu::caso("CU-FE-OPC-PROHIBIDAS", prohibida->limite == (opciones::permitirProhibidas() ? 3 : 0), "%s: limite %d al volver la opcion", prohibida->id.c_str(), prohibida->limite);
	// preguntar antes de activar: Auto -> Siempre -> Nunca -> Auto
	irAOpcion(g, Juego::OP_PREGUNTAR);
	opciones::preguntar() = opciones::PREGUNTAR_AUTO;
	const char* const PREG[] = { "Auto", "Siempre", "Nunca" };
	bool ciclo = valorOpcion(g, Juego::OP_PREGUNTAR) == "Auto";
	for (int k = 1; k <= 3; k++) {
		g.apretar(PAD_CROSS, 0.12f);
		ciclo = ciclo && valorOpcion(g, Juego::OP_PREGUNTAR) == PREG[k % 3] && opciones::preguntarActivacion() == k % 3;
	}
	cu::caso("CU-FE-OPC-PREGUNTAR", ciclo, "Preguntar antes de activar no recorre Auto, Siempre, Nunca");
	g.apretar(PAD_LEFT, 0.12f);
	cu::caso("CU-FE-OPC-PREGUNTAR", opciones::preguntarActivacion() == opciones::PREGUNTAR_NUNCA && contiene(leerArchivo(archivo), "preguntar_activacion=2"),
	         "izquierda no vuelve a Nunca o no se guardo");
	g.apretar(PAD_RIGHT, 0.12f);
	// velocidad de animacion: Normal -> Rapida -> Muy rapida; el duelo la usa (velocidad::leer)
	irAOpcion(g, Juego::OP_VELOCIDAD);
	opciones::velocidad() = opciones::VELOCIDAD_NORMAL;
	float factorAntes = 0.0f;
	bool sube = true;
	for (int k = 0; k < 3; k++) {
		g.leerVelocidad();
		sube = sube && velocidad::nivel() == opciones::velocidadAnim() && velocidad::factor() > factorAntes;
		factorAntes = velocidad::factor();
		if (k < 2) g.apretar(PAD_CROSS, 0.12f);
	}
	cu::caso("CU-FE-OPC-VELOCIDAD", sube && valorOpcion(g, Juego::OP_VELOCIDAD) == "Muy rápida", "la velocidad no sube con cada paso (o el duelo no la lee)");
	g.apretar(PAD_CROSS, 0.12f);
	g.leerVelocidad();
	cu::caso("CU-FE-OPC-VELOCIDAD", velocidad::nivel() == 0 && valorOpcion(g, Juego::OP_VELOCIDAD) == "Normal", "la velocidad no vuelve a Normal");
	// dificultad adaptable e IA: en variedad.txt
	irAOpcion(g, Juego::OP_ADAPTABLE);
	const bool adapt = g.memoriaIA.adaptable;
	g.apretar(PAD_CROSS, 0.12f);
	{
		ia::Memoria m;
		m.leer(g.memoriaIA.ruta);
		cu::caso("CU-FE-OPC-ADAPTABLE", g.memoriaIA.adaptable != adapt && m.adaptable == g.memoriaIA.adaptable, "Dificultad adaptable no cambio o no se guardo en variedad.txt");
		cu::caso("CU-FE-OPC-ADAPTABLE", g.memoriaIA.adaptable || g.memoriaIA.dificultad() == 0.0f, "apagada, la dificultad no es 0");
	}
	g.apretar(PAD_CROSS, 0.12f);
	irAOpcion(g, Juego::OP_IA);
	const bool humana = g.memoriaIA.humana;
	g.apretar(PAD_CROSS, 0.12f);
	{
		ia::Memoria m;
		m.leer(g.memoriaIA.ruta);
		cu::caso("CU-FE-OPC-IA", g.memoriaIA.humana != humana && m.humana == g.memoriaIA.humana && valorOpcion(g, Juego::OP_IA) == (g.memoriaIA.humana ? "Humana" : "Clásica"),
		         "IA Clasica/Humana no cambio o no se guardo");
	}
	g.apretar(PAD_CROSS, 0.12f);
	// figuras 3D
	irAOpcion(g, Juego::OP_FIGURAS);
	const bool fig = g.figuras3D;
	g.apretar(PAD_CROSS, 0.12f);
	cu::caso("CU-FE-OPC-FIGURAS", g.figuras3D != fig && contiene(leerArchivo(archivo), std::string("figuras_3d=") + (g.figuras3D ? "1" : "0")), "Figuras 3D no cambio o no se guardo");
	g.apretar(PAD_CROSS, 0.12f);
	// idioma: en caliente y en idioma.txt
	irAOpcion(g, Juego::OP_IDIOMA);
	const int ver = idioma::version();
	g.apretar(PAD_CROSS, 0.12f);
	cu::caso("CU-FE-OPC-IDIOMA", idioma::ingles() && idioma::version() == ver + 1 && valorOpcion(g, Juego::OP_IDIOMA) == "English", "Idioma no paso a ingles");
	cu::caso("CU-FE-OPC-IDIOMA", leerArchivo(dir + "idioma.txt") == "en\n", "idioma.txt no dice en");
	g.apretar(PAD_CROSS, 0.12f);
	cu::caso("CU-FE-OPC-IDIOMA", !idioma::ingles() && leerArchivo(dir + "idioma.txt") == "es\n", "Idioma no volvio a espanol");
	// Personalizacion: X la abre
	irAOpcion(g, Juego::OP_PERSONALIZACION);
	g.apretar(PAD_CROSS, 0.15f);
	cu::caso("CU-FE-OPC-PERSONALIZACION", cosmeticos::vista().pantalla == cosmeticos::P_MENU, "X en Personalizacion no la abrio");
	g.apretar(PAD_CIRCLE, 0.15f);
	cu::caso("CU-FE-PERSO-MENU", cosmeticos::vista().pantalla == cosmeticos::P_CERRADA && g.menuP.enOpciones, "circulo en Personalizacion no volvio a Opciones");
	// Volver (X) guarda y vuelve
	irAOpcion(g, Juego::OP_VOLVER);
	g.apretar(PAD_CROSS, 0.15f);
	cu::caso("CU-FE-OPC-VOLVER", !g.menuP.enOpciones && v.pantalla == VD::MENU, "X en Volver no volvio al menu");
	// guardar y cargar: todo vuelve igual al leer opciones.txt de nuevo; las claves de otras partes se conservan
	{
		std::string s = leerArchivo(archivo);
		guardado::guardarAtomico(archivo, s + "clave_ajena=7\n");
		g.sonido.volMusica = 0.3f;
		g.sonido.volEfectos = 0.9f;
		opciones::prohibidas() = true;
		opciones::preguntar() = 1;
		opciones::velocidad() = 2;
		g.figuras3D = false;
		g.guardarOpciones();
		g.sonido.volMusica = g.sonido.volEfectos = 0.0f;
		opciones::prohibidas() = false;
		opciones::preguntar() = 0;
		opciones::velocidad() = 0;
		g.figuras3D = true;
		g.leerOpciones();
		cu::caso("CU-FE-OPC-GUARDAR-CARGAR", fabsf(g.sonido.volMusica - 0.3f) < 0.01f && fabsf(g.sonido.volEfectos - 0.9f) < 0.01f && opciones::permitirProhibidas() &&
		                                         opciones::preguntarActivacion() == 1 && opciones::velocidadAnim() == 2 && !g.figuras3D,
		         "al leer opciones.txt no vuelven los valores guardados (musica %.2f efectos %.2f prohibidas %d preguntar %d velocidad %d figuras %d)", g.sonido.volMusica.load(),
		         g.sonido.volEfectos.load(), (int)opciones::permitirProhibidas(), opciones::preguntarActivacion(), opciones::velocidadAnim(), (int)g.figuras3D);
		cu::caso("CU-FE-OPC-CLAVES-AJENAS", contiene(leerArchivo(archivo), "clave_ajena=7"), "guardar las opciones borro una clave de otra parte del juego");
		cu::caso("CU-FE-OPC-GUARDADO-ATOMICO", !existe(archivo + ".tmp"), "quedo opciones.txt.tmp");
		opciones::prohibidas() = false;
		opciones::preguntar() = 0;
		opciones::velocidad() = 0;
		g.figuras3D = true;
		g.sonido.volMusica = g.sonido.volEfectos = 0.8f;
		g.guardarOpciones();
	}
}

// ------------------------------------------------------------------------------------------------- Personalizacion
inline void personalizacion(Juego& g, const std::string& dir)
{
	cosmeticos::VistaPerso& pv = cosmeticos::vista();
	cosmeticos::Estado& e = cosmeticos::estado();
	irAOpcion(g, Juego::OP_PERSONALIZACION);
	g.apretar(PAD_CROSS, 0.15f);
	cu::caso("CU-FE-PERSO-MENU", pv.pantalla == cosmeticos::P_MENU && pv.opcion == 0, "Personalizacion no abrio su menu");
	// funda: sin vencer a nadie solo la clasica esta libre
	g.historia.vencidos.clear();
	g.actualizarDesbloqueos();
	pv.opcion = cosmeticos::PERSO_FUNDA;
	g.apretar(PAD_CROSS, 0.15f);
	cu::caso("CU-FE-PERSO-FUNDA", pv.pantalla == cosmeticos::P_FUNDA, "X en Funda no abrio las fundas");
	e.funda = cosmeticos::F_CLASICA;
	pv.sel = 0;
	g.apretar(PAD_RIGHT, 0.12f);
	g.apretar(PAD_CROSS, 0.12f);
	cu::caso("CU-FE-PERSO-FUNDA-BLOQUEADA", e.funda == cosmeticos::F_CLASICA && !pv.aviso.empty(), "se equipo una funda bloqueada (o no hubo aviso)");
	cu::caso("CU-FE-PERSO-FUNDA-BLOQUEADA", !e.fundaPorEs[1].empty() && e.fundaPorEs[1] != cosmeticos::FUNDAS[1].personaje, "el aviso no tiene el nombre del personaje ('%s')", e.fundaPorEs[1].c_str());
	g.historia.vencidos.insert(cosmeticos::FUNDAS[1].personaje);
	g.actualizarDesbloqueos();
	g.apretar(PAD_CROSS, 0.12f);
	cu::caso("CU-FE-PERSO-FUNDA", e.funda == 1 && cosmeticos::fundaVisible() == 1, "vencido %s, su funda no se equipo", cosmeticos::FUNDAS[1].personaje);
	cu::caso("CU-FE-PERSO-FUNDA", contiene(leerArchivo(dir + "opciones.txt"), std::string("funda=") + cosmeticos::FUNDAS[1].clave), "la funda no se guardo");
	g.apretar(PAD_DOWN, 0.12f);   // (5 columnas: abajo salta 5)
	cu::caso("CU-FE-PERSO-FUNDA", pv.sel == 6, "abajo en la rejilla de fundas fue a %d (esperado 6)", pv.sel);
	g.apretar(PAD_CIRCLE, 0.12f);
	cu::caso("CU-FE-PERSO-MENU", pv.pantalla == cosmeticos::P_MENU, "circulo en fundas no volvio al menu de Personalizacion");
	// tapete
	pv.opcion = cosmeticos::PERSO_TAPETE;
	g.apretar(PAD_CROSS, 0.15f);
	cu::caso("CU-FE-PERSO-TAPETE", pv.pantalla == cosmeticos::P_TAPETE, "X en Tapete no abrio los tapetes");
	pv.sel = 0;
	g.apretar(PAD_DOWN, 0.12f);
	g.apretar(PAD_CROSS, 0.12f);
	cu::caso("CU-FE-PERSO-TAPETE-BLOQUEADO", e.tapete == cosmeticos::T_AUTO && !pv.aviso.empty(), "se equipo un tapete bloqueado");
	g.historia.vencidos.insert(cosmeticos::TAPETES[1].personaje);
	g.actualizarDesbloqueos();
	g.apretar(PAD_CROSS, 0.12f);
	cu::caso("CU-FE-PERSO-TAPETE", e.tapete == 1 && cosmeticos::tapeteVisible() == 1 && contiene(leerArchivo(dir + "opciones.txt"), "tapete=pradera"), "el tapete no se equipo o no se guardo");
	g.apretar(PAD_CIRCLE, 0.12f);
	// controles: 3 pestannas (R1/L1) y filas
	pv.opcion = cosmeticos::PERSO_CONTROLES;
	g.apretar(PAD_CROSS, 0.15f);
	cu::caso("CU-FE-PERSO-CONTROLES", pv.pantalla == cosmeticos::P_CONTROLES && pv.pestana == cosmeticos::PEST_MENUS, "Controles no abrio en la pestanna de menus (fuera del duelo)");
	g.apretar(PAD_R1, 0.12f);
	const int p1 = pv.pestana;
	g.apretar(PAD_R1, 0.12f);
	const int p2 = pv.pestana;
	g.apretar(PAD_L1, 0.12f);
	cu::caso("CU-FE-PERSO-CONTROLES", p1 == cosmeticos::PEST_DUELO && p2 == cosmeticos::PEST_CONSTRUCTOR && pv.pestana == cosmeticos::PEST_DUELO, "R1/L1 no recorren las 3 pestannas (%d %d %d)", p1, p2, pv.pestana);
	const int n = (int)cosmeticos::controles(pv.pestana).size();
	g.apretar(PAD_UP, 0.12f);
	cu::caso("CU-FE-PERSO-CONTROLES", pv.sel == n - 1, "arriba en la primera fila no fue a la ultima");
	g.apretar(PAD_CIRCLE, 0.12f);
	// estadisticas
	estadisticas::registrar("p:tea", "Téa", "Téa", 0, "Mazo X", { "dark_magician", "dark_magician", "raigeki" });
	pv.opcion = cosmeticos::PERSO_ESTADISTICAS;
	g.apretar(PAD_CROSS, 0.15f);
	const estadisticas::Datos& d = estadisticas::datos();
	cu::caso("CU-FE-PERSO-ESTADISTICAS", pv.pantalla == cosmeticos::P_ESTADISTICAS && d.firmaTotal > 0 && d.firmaLibres <= d.firmaTotal, "Estadisticas no abrio o no cuenta las firma (%d / %d)", d.firmaLibres, d.firmaTotal);
	const auto* top = estadisticas::masAlto(d.cartas);
	cu::caso("CU-FE-PERSO-ESTADISTICAS", top && top->first == "dark_magician" && top->second >= 2, "la carta mas jugada no es dark_magician");
	g.apretar(PAD_CIRCLE, 0.12f);
	pv.opcion = cosmeticos::PERSO_VOLVER;
	g.apretar(PAD_CROSS, 0.12f);
	cu::caso("CU-FE-PERSO-MENU", pv.pantalla == cosmeticos::P_CERRADA, "X en Volver no cerro Personalizacion");
	g.apretar(PAD_CIRCLE, 0.12f);
	// camara recordada (L1/R1 en el duelo la guarda; al arrancar se usa)
	g.recordarCamara(2);
	cu::caso("CU-FE-PERSO-CAMARA", contiene(leerArchivo(dir + "opciones.txt"), "camara=2"), "la camara no se guardo");
	g.modoCamara = 0;
	cosmeticos::leer(dir + "opciones.txt");
	g.aplicarCamaraGuardada();
	cu::caso("CU-FE-PERSO-CAMARA", g.modoCamara == 2, "al arrancar no se uso la camara guardada (%d)", g.modoCamara);
	// lo de la Personalizacion vuelve igual al leer
	e.funda = 0;
	e.tapete = 0;
	cosmeticos::leer(dir + "opciones.txt");
	cu::caso("CU-FE-PERSO-GUARDAR-CARGAR", e.funda == 1 && e.tapete == 1, "al leer opciones.txt la funda/tapete no vuelven (%d %d)", e.funda, e.tapete);
	// dejar todo como estaba
	g.historia.vencidos.clear();
	g.actualizarDesbloqueos();
	e.funda = e.tapete = 0;
	cosmeticos::guardar();
}

// ------------------------------------------------------------------------------------------------- elegir rival
inline void irARivales(Juego& g, int modo)
{
	irAlMenu(g);
	elegirMenu(g, Juego::MP_DUELO);
	if (g.ctl.vista.modo != modo) g.apretar(PAD_TRIANGLE, 0.2f);
}

inline void elegirRival(Juego& g)
{
	VD& v = g.ctl.vista;
	irARivales(g, VD::HISTORIA);
	cu::caso("CU-FE-RIVAL-HISTORIA", v.modo == VD::HISTORIA && v.pantalla == VD::ELEGIR_RIVAL, "MODO DUELO no abrio el modo historia");
	cu::caso("CU-FE-RIVAL-HISTORIA", v.saga == 0 && !v.fila.empty() && v.personaje == v.fila[0] && v.sagaAbierta, "sin vencidos no esta en el primero de la primera saga ('%s')",
	         v.personaje.c_str());
	cu::caso("CU-FE-RIVAL-HISTORIA", g.mazoRivalIA.rival && g.mazoRivalIA.principal.size() >= 40, "el personaje no tiene mazo");
	const std::string p0 = v.personaje;
	g.apretar(PAD_RIGHT, 0.15f);
	cu::caso("CU-FE-RIVAL-HISTORIA-NAVEGAR", v.idx == 1 && v.personaje != p0 && g.mazoRivalIA.rival, "derecha no paso al siguiente personaje");
	g.apretar(PAD_LEFT, 0.15f);
	cu::caso("CU-FE-RIVAL-HISTORIA-NAVEGAR", v.idx == 0 && v.personaje == p0, "izquierda no volvio al primero");
	g.apretar(PAD_DOWN, 0.15f);
	cu::caso("CU-FE-RIVAL-HISTORIA-NAVEGAR", v.saga == 1 && !v.sagaAbierta, "abajo no paso a la saga 2 (cerrada sin vencer la 1)");
	cu::caso("CU-FE-RIVAL-SAGA-CERRADA", !v.filaEstado.empty() && v.filaEstado[0] == 0, "los de la saga cerrada no se ven cerrados");
	g.ctl.aviso.clear();
	g.apretar(PAD_CROSS, 0.2f);
	cu::caso("CU-FE-RIVAL-SAGA-CERRADA", v.pantalla == VD::ELEGIR_RIVAL && !historia_dialogos::escena().activa() && !g.ctl.aviso.empty(), "X en una saga cerrada empezo algo o no aviso");
	cu::caso("CU-FE-IDIOMA-AVISOS", traducido(g.ctl.aviso), "el aviso '%s' no tiene traduccion", g.ctl.aviso.c_str());
	g.apretar(PAD_UP, 0.15f);
	cu::caso("CU-FE-RIVAL-HISTORIA-NAVEGAR", v.saga == 0 && v.sagaAbierta, "arriba no volvio a la saga 1");
	const uint32_t s0 = g.semillaRival;
	g.apretar(PAD_SQUARE, 0.15f);
	cu::caso("CU-FE-RIVAL-VARIANTE", g.semillaRival != s0 && g.mazoRivalIA.principal.size() >= 40, "cuadrado no armo otra variante del mazo");
	// reglas (R1): paginas con la cruz, circulo cierra
	tutorial::Estado& tut = tutorial::estado();
	g.apretar(PAD_R1, 0.15f);
	cu::caso("CU-FE-RIVAL-REGLAS", tut.abierto && tut.pagina == 0, "R1 no abrio las reglas");
	g.apretar(PAD_RIGHT, 0.15f);
	cu::caso("CU-FE-RIVAL-REGLAS", tut.pagina == 1, "derecha no paso de pagina");
	for (int k = 0; k < tutorial::NUM_PAGINAS + 2; k++) g.apretar(PAD_RIGHT, 0.1f);
	cu::caso("CU-FE-RIVAL-REGLAS", tut.pagina == tutorial::NUM_PAGINAS - 1, "las reglas pasan de la ultima pagina");
	g.apretar(PAD_LEFT, 0.1f);
	cu::caso("CU-FE-RIVAL-REGLAS", tut.pagina == tutorial::NUM_PAGINAS - 2, "izquierda no volvio una pagina");
	g.apretar(PAD_CIRCLE, 0.15f);
	cu::caso("CU-FE-RIVAL-REGLAS", !tut.abierto && v.pantalla == VD::ELEGIR_RIVAL, "circulo no cerro las reglas (o salio de elegir rival)");
	// desafio (L1): solo con la historia terminada
	historia_dialogos::desafioDisponible() = false;
	historia_dialogos::desafioActivo() = false;
	g.apretar(PAD_L1, 0.15f);
	cu::caso("CU-FE-RIVAL-DESAFIO", !historia_dialogos::desafioActivo(), "L1 activo el desafio sin terminar la historia");
	historia_dialogos::desafioDisponible() = true;
	g.apretar(PAD_L1, 0.15f);
	cu::caso("CU-FE-RIVAL-DESAFIO", historia_dialogos::desafioActivo(), "L1 no activo el desafio con la historia terminada");
	g.apretar(PAD_L1, 0.15f);
	cu::caso("CU-FE-RIVAL-DESAFIO", !historia_dialogos::desafioActivo(), "L1 no apago el desafio");
	historia_dialogos::desafioDisponible() = false;
	// triangulo: duelo libre (por nivel)
	g.apretar(PAD_TRIANGLE, 0.15f);
	cu::caso("CU-FE-RIVAL-CAMBIO-MODO", v.modo == VD::LIBRE && v.personaje.empty() && !v.rivalEs.empty() && v.rivalesNivel > 0, "triangulo no paso al duelo libre");
	g.apretar(PAD_TRIANGLE, 0.15f);
	cu::caso("CU-FE-RIVAL-CAMBIO-MODO", v.modo == VD::HISTORIA && !v.personaje.empty(), "triangulo no volvio a la historia");
	g.apretar(PAD_TRIANGLE, 0.15f);
	// niveles: hasta el desbloqueado
	v.nivelMax = 1;
	v.nivel = 1;
	g.elegirRival(true);
	g.apretar(PAD_UP, 0.15f);
	cu::caso("CU-FE-RIVAL-LIBRE-NIVELES", v.nivel == 1, "arriba subio a un nivel bloqueado (%d)", v.nivel);
	v.nivelMax = 3;
	g.apretar(PAD_UP, 0.15f);
	g.apretar(PAD_UP, 0.15f);
	g.apretar(PAD_UP, 0.15f);
	cu::caso("CU-FE-RIVAL-LIBRE-NIVELES", v.nivel == 3, "con el nivel 3 desbloqueado arriba llego a %d", v.nivel);
	cu::caso("CU-FE-RIVAL-LIBRE-NIVELES", g.mazoRivalIA.rival && g.mazoRivalIA.rival->nivel == 3, "el rival no es del nivel 3");
	g.apretar(PAD_DOWN, 0.15f);
	cu::caso("CU-FE-RIVAL-LIBRE-NIVELES", v.nivel == 2, "abajo no bajo un nivel");
	for (int k = 0; k < 4; k++) g.apretar(PAD_DOWN, 0.1f);
	cu::caso("CU-FE-RIVAL-LIBRE-NIVELES", v.nivel == 1, "el nivel baja de 1");
	if (v.rivalesNivel > 1) {
		const std::string r0 = v.rivalEs;
		const int i0 = g.rivalIdx;
		g.apretar(PAD_RIGHT, 0.15f);
		cu::caso("CU-FE-RIVAL-LIBRE-RIVAL", g.rivalIdx == (i0 + 1) % v.rivalesNivel && v.rivalIdx == g.rivalIdx, "derecha no paso al siguiente rival del nivel");
		g.apretar(PAD_LEFT, 0.15f);
		cu::caso("CU-FE-RIVAL-LIBRE-RIVAL", g.rivalIdx == i0 && v.rivalEs == r0, "izquierda no volvio al rival anterior");
	} else {
		cu::caso("CU-FE-RIVAL-LIBRE-RIVAL", false, "el nivel 1 tiene un solo rival");
	}
	g.apretar(PAD_SQUARE, 0.15f);
	cu::caso("CU-FE-RIVAL-LIBRE-AZAR", g.rivalIdx >= 0 && g.rivalIdx < v.rivalesNivel && g.mazoRivalIA.rival, "cuadrado no eligio un rival al azar del nivel");
	g.apretar(PAD_CIRCLE, 0.15f);
	cu::caso("CU-FE-RIVAL-VOLVER", v.pantalla == VD::MENU, "circulo en elegir rival no volvio al menu");
}

// ------------------------------------------------------------------------------------------------- dialogos
inline void dialogos(Juego& g)
{
	VD& v = g.ctl.vista;
	historia_dialogos::Escena& e = historia_dialogos::escena();
	// contenido: cada personaje con intro, linea de victoria y de derrota en los dos idiomas
	int sinDialogo = 0;
	std::string faltan;
	for (const personajes::Personaje& x : g.historia.lista) {
		const historia_dialogos::Personaje* p = historia_dialogos::base().de(x.id);
		bool ok = p && !p->intro.empty() && !p->gana.es.empty() && !p->gana.en.empty() && !p->pierde.es.empty() && !p->pierde.en.empty();
		if (p)
			for (const historia_dialogos::Linea& l : p->intro) ok = ok && !l.es.empty() && !l.en.empty();
		if (!ok) {
			sinDialogo++;
			faltan += x.id + " ";
		}
	}
	cu::caso("CU-FE-DIALOGO-CONTENIDO", sinDialogo == 0, "%d personajes sin dialogo completo ES/EN: %s", sinDialogo, faltan.c_str());
	const historia_dialogos::Dialogos& b = historia_dialogos::base();
	bool fin = !b.finalTexto.empty() && !b.creditos.empty() && !b.desbloqueo.es.empty() && !b.desbloqueo.en.empty();
	for (const auto& l : b.finalTexto) fin = fin && !l.es.empty() && !l.en.empty();
	cu::caso("CU-FE-DIALOGO-CONTENIDO", fin, "el final de la historia no tiene texto, creditos o desbloqueo en ES/EN");
	// la intro: X avanza linea por linea (X con la linea a medias la completa) y al final empieza el duelo
	irARivales(g, VD::HISTORIA);
	g.apretar(PAD_CROSS, 0.3f);
	cu::caso("CU-FE-DIALOGO-INTRO", e.activa() && e.tipo == historia_dialogos::INTRO && e.personaje == v.personaje, "X sobre el rival no abrio su dialogo");
	const int total = e.total();
	g.apretar(PAD_CROSS, 0.05f);
	cu::caso("CU-FE-DIALOGO-INTRO", e.linea == 0 && e.lineaCompleta(), "X con la linea a medias no la completo (linea %d)", e.linea);
	int pasos = 0;
	while (e.activa() && pasos++ < 40) g.apretar(PAD_CROSS, 0.25f);
	cu::caso("CU-FE-DIALOGO-INTRO", !e.activa() && v.pantalla == VD::DUELO && v.sorteo, "terminar el dialogo (%d lineas) no empezo el duelo", total);
	// circulo salta la presentacion
	irARivales(g, VD::HISTORIA);
	g.apretar(PAD_CROSS, 0.3f);
	g.apretar(PAD_CIRCLE, 0.2f);
	cu::caso("CU-FE-DIALOGO-SALTAR", !e.activa() && v.pantalla == VD::DUELO, "circulo no salto el dialogo");
	cu::caso("CU-FE-DIALOGO-CIERRE", !historia_dialogos::dialogoCierre(v.personaje, true).empty() && !historia_dialogos::dialogoCierre(v.personaje, false).empty(),
	         "el personaje no tiene linea de cierre");
}

// ------------------------------------------------------------------------------------------------- duelo
// hasta que el jugador pueda jugar (su turno, libre) o pasen s segundos
inline bool esperarJugador(Juego& g, float s)
{
	for (float t = 0.0f; t < s; t += 0.1f) {
		if (g.dueloIA.ganador >= 0) return false;
		if (g.dueloIA.activo == 0 && g.dueloIA.decide() == 0 && !g.ctl.bloqueado && g.ctl.estado == control::NAVEGAR && g.dueloIA.fase == ia::F_PRINCIPAL1) return true;
		if (g.dueloIA.decide() == 0 && g.ctl.estado == control::MENU && g.ctl.enPregunta) {   // (pregunta del duelo: no activar)
			g.ctl.opcion = (int)g.ctl.opciones.size() - 1;
			g.apretar(PAD_CROSS, 0.1f);
		}
		if (g.dueloIA.decide() == 0 && g.dueloIA.ef.preg.tipo == ia::P_ELEGIR && g.ctl.estado == control::ELEGIR_CARTA) g.apretar(PAD_CROSS, 0.1f);
		if (g.dueloIA.activo == 0 && g.dueloIA.decide() == 0 && !g.ctl.bloqueado && g.ctl.estado == control::NAVEGAR && g.dueloIA.fase != ia::F_PRINCIPAL1) {
			if (g.dueloIA.fase == ia::F_FINAL && (int)g.dueloIA.l[0].mano.size() > ia::MAX_MANO) {
				g.ctl.foco = control::Foco{ control::F_MANO_J, 0 };
				g.apretar(PAD_CROSS, 0.2f);
				g.apretar(PAD_CROSS, 0.4f);
			} else {
				g.apretar(PAD_SQUARE, 0.4f);
			}
		}
		g.esperar(0.1f);
	}
	return false;
}

// el sorteo: X elige; si gana el jugador elige ir "segundo" (derecha + X) para ver que obedece. Devuelve el resultado
inline int sorteo(Juego& g, bool segundo)
{
	VD& v = g.ctl.vista;
	for (int k = 0; k < 20 && v.sorteo && v.sorteoEtapa == VD::SORTEO_ELEGIR; k++) {
		g.esperar(0.35f);
		g.apretar(PAD_CROSS, 0.2f);
		if (v.sorteoEtapa != VD::SORTEO_MOSTRAR) continue;
		const int jm = v.sorteoManoJ, rm = v.sorteoManoR;
		const int d = (jm - rm + 3) % 3;
		cu::caso("CU-FE-DUELO-SORTEO-REGLA", v.sorteoGanador == (d == 0 ? -1 : d == 1 ? 0 : 1), "piedra/papel/tijera mal resuelto (%d contra %d: gana %d)", jm, rm, v.sorteoGanador);
		const int gan = v.sorteoGanador;
		g.esperar(2.0f);
		if (gan < 0) {
			cu::caso("CU-FE-DUELO-SORTEO-EMPATE", v.sorteoEtapa == VD::SORTEO_ELEGIR && v.sorteo, "el empate no vuelve a elegir");
			continue;
		}
		if (gan == 0) {
			cu::caso("CU-FE-DUELO-SORTEO-GANA", v.sorteoEtapa == VD::SORTEO_ORDEN, "el jugador gano y no le pregunto el orden");
			if (segundo) g.apretar(PAD_RIGHT, 0.15f);
			g.esperar(0.4f);
			const int quiere = segundo ? 1 : 0;
			g.apretar(PAD_CROSS, 0.2f);
			cu::caso("CU-FE-DUELO-SORTEO-GANA", !v.sorteo && g.dueloIA.activo == quiere, "eligio ir %s y empieza %d", segundo ? "segundo" : "primero", g.dueloIA.activo);
			return 0;
		}
		cu::caso("CU-FE-DUELO-SORTEO-PIERDE", v.sorteoEtapa == VD::SORTEO_RIVAL_DECIDE || !v.sorteo, "gano el rival y no decidio");
		const int empieza = v.sorteoEmpieza;
		g.esperar(2.6f);
		cu::caso("CU-FE-DUELO-SORTEO-PIERDE", !v.sorteo && g.dueloIA.activo == empieza, "el rival decidio %d y empieza %d", empieza, g.dueloIA.activo);
		return 1;
	}
	return -1;
}

inline void abandonar(Juego& g)
{
	if (g.ctl.vista.pantalla != VD::DUELO) return;
	g.esperar(0.2f);
	g.apretar(PAD_OPTIONS, 0.2f);
	if (!g.pausa.abierta) return;
	const std::vector<int> ops = g.opcionesPausa();
	for (int k = 0; k < 8 && ops[(size_t)g.pausa.opcion] != Juego::PA_ABANDONAR; k++) g.apretar(PAD_DOWN, 0.12f);
	g.apretar(PAD_CROSS, 0.3f);
}

inline void duelo(Juego& g)
{
	VD& v = g.ctl.vista;
	// el sorteo: varios duelos hasta ver los tres resultados (empate, gana el jugador, gana el rival)
	bool vioGana = false, vioPierde = false;
	for (int n = 0; n < 40 && !(vioGana && vioPierde && cu::de("CU-FE-DUELO-SORTEO-EMPATE").comprobaciones > 0); n++) {
		irARivales(g, VD::LIBRE);
		g.apretar(PAD_CROSS, 0.3f);
		if (n == 0) {
			cu::caso("CU-FE-DUELO-SORTEO", v.pantalla == VD::DUELO && v.sorteo && v.sorteoEtapa == VD::SORTEO_ELEGIR, "X en el rival no empezo el sorteo");
			g.esperar(0.4f);
			g.apretar(PAD_RIGHT, 0.12f);
			cu::caso("CU-FE-DUELO-SORTEO", v.sorteoOpcion == 1, "derecha no cambio la mano elegida");
			g.apretar(PAD_LEFT, 0.12f);
		}
		const int r = sorteo(g, n % 2 == 0);
		vioGana |= r == 0;
		vioPierde |= r == 1;
		abandonar(g);
	}
	cu::caso("CU-FE-DUELO-SORTEO-GANA", vioGana, "en 40 sorteos el jugador nunca gano");
	cu::caso("CU-FE-DUELO-SORTEO-PIERDE", vioPierde, "en 40 sorteos el rival nunca gano");
	cu::caso("CU-FE-DUELO-SORTEO-EMPATE", cu::de("CU-FE-DUELO-SORTEO-EMPATE").comprobaciones > 0, "en 40 sorteos nunca hubo empate");
	// un duelo de verdad: reparto animado, fases, detalle, jugables, registro y visores
	irARivales(g, VD::LIBRE);
	g.apretar(PAD_CROSS, 0.3f);
	const int serie0 = g.ctl.robo.serie;
	if (sorteo(g, false) < 0) {
		cu::caso("CU-FE-DUELO-SORTEO", false, "el sorteo no termino");
		return;
	}
	cu::caso("CU-FE-DUELO-INICIO", g.dueloIA.l[0].lp == ia::LP_INICIAL && g.dueloIA.l[1].lp == ia::LP_INICIAL && g.interfazDuelo.lp(false) == ia::LP_INICIAL,
	         "el duelo no empieza con 8000 LP");
	cu::caso("CU-FE-DUELO-INICIO", g.mazoDuelo.principal.size() >= 40 && g.dueloIA.l[0].mazo.size() + g.dueloIA.l[0].mano.size() == g.mazoDuelo.principal.size(),
	         "el jugador no juega con su mazo activo");
	{
		int robos = 0, serie = g.ctl.robo.serie;
		robos = serie - serie0;
		for (int k = 0; k < 600 && (g.reparto > 0 || g.ctl.robo.activo || g.sucesosVistos < g.dueloIA.sucesos.size()); k++) {
			g.cuadro(0);
			if (g.ctl.robo.serie != serie) robos++, serie = g.ctl.robo.serie;
		}
		cu::caso("CU-FE-DUELO-REPARTO", robos >= 10, "el reparto anima %d cartas (esperadas 10 u 11)", robos);
		cu::caso("CU-FE-DUELO-REPARTO", g.ctl.manoJ == g.dueloIA.l[0].mano && g.ctl.manoR == (int)g.dueloIA.l[1].mano.size(), "tras el reparto la mano en pantalla no es la del duelo");
	}
	cu::caso("CU-FE-DUELO-MUSICA", contiene(g.sonido.musica, "musica_duelo"), "en el duelo suena '%s'", g.sonido.musica.c_str());
	// la fase de espera se ve (cartel) al empezar el turno del jugador: robo (0) -> espera (1) -> principal 1 (2)
	{
		bool vioEspera = false, vioM1 = false;
		for (int k = 0; k < 6000 && !(vioEspera && vioM1); k++) {
			const int t0 = g.dueloIA.turno;
			g.cuadro(0);
			if (g.dueloIA.activo == 0 && g.dueloIA.turno == t0) {
				vioEspera |= g.ctl.fase == 1;
				vioM1 |= vioEspera && g.ctl.fase == 2;
			}
			if (g.dueloIA.decide() == 0 && g.ctl.estado == control::MENU && g.ctl.enPregunta) {
				g.ctl.opcion = (int)g.ctl.opciones.size() - 1;
				g.apretar(PAD_CROSS, 0.1f);
			}
			if (g.dueloIA.activo == 0 && !g.ctl.bloqueado && g.ctl.estado == control::NAVEGAR && g.dueloIA.ganador < 0 && !vioEspera) g.apretar(PAD_SQUARE, 0.3f);
		}
		cu::caso("CU-FE-DUELO-FASE-ESPERA", vioEspera && vioM1, "la fase de espera no se ve antes de la principal 1 (espera %d, M1 %d)", (int)vioEspera, (int)vioM1);
	}
	if (!esperarJugador(g, 60.0f)) {
		cu::caso("CU-FE-DUELO-TURNO-JUGADOR", false, "el jugador nunca quedo libre en su principal 1");
		abandonar(g);
		return;
	}
	cu::caso("CU-FE-DUELO-TURNO-JUGADOR", g.ctl.fase == Juego::faseInterfaz(ia::F_PRINCIPAL1), "la fase en pantalla (%d) no es la principal 1", g.ctl.fase);
	// jugables: lo que se puede jugar brilla (al menos una carta de la mano en la principal 1 con mano llena)
	g.esperar(0.3f);
	{
		bool hayMonstruo = false;
		for (const std::string& id : g.dueloIA.l[0].mano) {
			const ia::Carta* k = g.baseIA.carta(id);
			hayMonstruo |= k && k->monstruo && ia::Duelo::tributosPara(k) == 0 && !k->noInvocable && (k->clase == "normal" || k->clase == "efecto");
		}
		if (hayMonstruo && !g.dueloIA.l[0].invocoNormal)
			cu::caso("CU-FE-DUELO-JUGABLES", !g.consulta.jug.mano.empty(), "con un monstruo invocable en la mano, nada brilla");
		for (int i : g.consulta.jug.mano) {
			const std::vector<control::Opcion> ops = g.ctl.opcionesPara({ control::F_MANO_J, i });
			bool util = false;
			for (const control::Opcion& o : ops) util |= o.activa && o.accion != control::AC_CANCELAR;
			cu::caso("CU-FE-DUELO-JUGABLES", util, "la carta %d brilla sin tener nada que hacer", i);
		}
	}
	// los textos del menu de cada carta de la mano se ven distintos en ES y en EN (como los dibuja la interfaz: tr)
	{
		std::vector<std::string> vEs, vEn;
		for (int lengua = 0; lengua < 2; lengua++) {
			idioma::actual() = lengua == 0 ? idioma::ES : idioma::EN;
			std::vector<std::string>& v = lengua == 0 ? vEs : vEn;
			for (int i = 0; i < (int)g.ctl.manoJ.size(); i++)
				for (const control::Opcion& o : g.ctl.opcionesPara({ control::F_MANO_J, i })) {
					v.push_back(idioma::tr(o.texto));
					v.push_back(o.motivo.empty() ? std::string() : idioma::tr(o.motivo));
				}
		}
		idioma::actual() = idioma::ES;
		cu::caso("CU-FE-IDIOMA-MENU-CARTA", vEs.size() == vEn.size() && !vEs.empty(), "el menu de las cartas cambia de largo con el idioma");
		for (size_t k = 0; k < vEs.size() && k < vEn.size(); k++)
			if (!vEs[k].empty()) cu::caso("CU-FE-IDIOMA-MENU-CARTA", vEs[k] != vEn[k] || traducido(vEs[k]), "'%s' del menu de la carta se ve igual en ingles", vEs[k].c_str());
	}
	// detalle: triangulo sobre una carta de la mano; el stick derecho desplaza el texto; circulo vuelve
	g.ctl.foco = control::Foco{ control::F_MANO_J, 0 };
	g.apretar(PAD_TRIANGLE, 0.2f);
	cu::caso("CU-FE-DUELO-DETALLE", g.ctl.estado == control::DETALLE && g.ctl.cartaDetalle == g.dueloIA.l[0].mano[0], "triangulo no abrio el detalle de la carta (estado %s)",
	         control::NOMBRE_ESTADO[g.ctl.estado]);
	if (g.ctl.estado == control::DETALLE) {
		const float d0 = g.ctl.desplDetalle;
		g.mando.right_y = 1.0f;
		for (int k = 0; k < 30; k++) {
			g.mando.right_y = 1.0f;
			g.cuadro(0);
		}
		g.mando.right_y = 0.0f;
		cu::caso("CU-FE-DUELO-DETALLE-SCROLL", g.ctl.desplDetalle >= d0, "el stick derecho no desplaza el texto del detalle");
		g.apretar(PAD_CIRCLE, 0.2f);
		cu::caso("CU-FE-DUELO-DETALLE", g.ctl.estado == control::NAVEGAR, "circulo no cerro el detalle");
	}
	// invocar con el mando: la primera de la mano que se pueda invocar (X, Invocar, X en la zona)
	{
		int k = -1;
		for (int i = 0; i < (int)g.ctl.manoJ.size() && k < 0; i++)
			for (const control::Opcion& o : g.ctl.opcionesPara({ control::F_MANO_J, i }))
				if (o.activa && o.accion == control::AC_INVOCAR && o.jugada.tributos.empty()) k = i;
		if (k >= 0) {
			const std::string id = g.ctl.manoJ[(size_t)k];
			g.ctl.foco = control::Foco{ control::F_MANO_J, k };
			g.apretar(PAD_CROSS, 0.2f);
			for (int o = 0; o < (int)g.ctl.opciones.size(); o++)
				if (g.ctl.opciones[(size_t)o].accion == control::AC_INVOCAR) g.ctl.opcion = o;
			g.apretar(PAD_CROSS, 0.2f);
			for (int n = 0; n < 4 && g.ctl.estado != control::NAVEGAR; n++) g.apretar(PAD_CROSS, 0.3f);
			g.esperar(2.0f);
			bool esta = false;
			for (const ia::Monstruo& m : g.dueloIA.l[0].m) esta |= m.id == id && !m.bocaAbajo;
			cu::caso("CU-FE-DUELO-INVOCAR", esta, "invocar %s con el mando no la puso en el campo", id.c_str());
			bool enMesa = false;
			for (int i = 0; i < ia::ZONAS; i++) enMesa |= g.ctl.zonas[(size_t)Juego::zonaMesaIA(0, i)].carta == id;
			cu::caso("CU-FE-DUELO-INVOCAR", enMesa, "la mesa no muestra %s invocada", id.c_str());
			bool anotada = false;
			for (const registro::Linea& l : g.consulta.reg.lineas) anotada |= l.tipo == ia::S_INVOCA && l.carta == id;
			cu::caso("CU-FE-DUELO-REGISTRO", anotada, "el registro no anoto la invocacion de %s", id.c_str());
		}
	}
	// el registro (L3) y los visores
	g.esperar(0.5f);
	if (g.ctl.estado == control::NAVEGAR && !g.consulta.vis.abierto) {
		g.apretar(PAD_L3, 0.2f);
		cu::caso("CU-FE-DUELO-REGISTRO", g.consulta.vis.abierto && g.consulta.vis.pestana == visores::P_REGISTRO && !g.consulta.reg.lineas.empty(), "L3 no abrio el registro");
		g.apretar(PAD_RIGHT, 0.15f);
		cu::caso("CU-FE-DUELO-VISORES", g.consulta.vis.pestana == visores::P_CEMENTERIO && g.consulta.filas(g.ctl) == (int)g.dueloIA.l[0].cementerio.size(), "el visor del cementerio no coincide");
		g.apretar(PAD_L3, 0.2f);
		cu::caso("CU-FE-DUELO-REGISTRO", !g.consulta.vis.abierto, "L3 no cerro el registro");
	}
	// pausa en el duelo
	g.esperar(0.3f);
	g.apretar(PAD_OPTIONS, 0.2f);
	const std::vector<std::string> pausaEsperada = { "Continuar", "Opciones", "Construir mazo", "Abandonar duelo (derrota)", "Salir al menú principal" };
	cu::caso("CU-FE-PAUSA-ABRE", g.pausa.abierta && v.pausa && v.pausaOpciones == pausaEsperada, "OPTIONS corto no abrio la pausa con sus 5 opciones");
	for (const std::string& s : v.pausaOpciones) cu::caso("CU-FE-IDIOMA-PAUSA", traducido(s), "'%s' de la pausa no tiene traduccion", s.c_str());
	{
		const size_t su = g.dueloIA.sucesos.size();
		const int tu = g.dueloIA.turno;
		const ia::Fase fa = g.dueloIA.fase;
		g.esperar(3.0f);
		cu::caso("CU-FE-PAUSA-DETIENE", g.dueloIA.sucesos.size() == su && g.dueloIA.turno == tu && g.dueloIA.fase == fa, "con la pausa abierta el duelo siguio");
	}
	g.pausa.opcion = 0;
	g.apretar(PAD_CROSS, 0.2f);
	cu::caso("CU-FE-PAUSA-CONTINUAR", !g.pausa.abierta && v.pantalla == VD::DUELO, "Continuar no volvio al duelo");
	g.apretar(PAD_OPTIONS, 0.2f);
	g.pausa.opcion = 1;
	g.apretar(PAD_CROSS, 0.2f);
	cu::caso("CU-FE-PAUSA-OPCIONES", g.menuP.enOpciones && g.pausa.abierta && !v.pausa && v.menuEnOpciones, "Opciones de la pausa no abrio las Opciones");
	g.apretar(PAD_CIRCLE, 0.2f);
	cu::caso("CU-FE-PAUSA-OPCIONES", !g.menuP.enOpciones && g.pausa.abierta && v.pausa && v.pantalla == VD::DUELO, "circulo en las Opciones de la pausa no volvio a la pausa");
	g.apretar(PAD_OPTIONS, 0.2f);
	cu::caso("CU-FE-PAUSA-CERRAR", !g.pausa.abierta, "OPTIONS corto no cerro la pausa");
	// Construir mazo desde la pausa: el constructor y de vuelta al duelo (el mazo nuevo vale para el proximo)
	g.apretar(PAD_OPTIONS, 0.2f);
	g.pausa.opcion = 2;
	g.apretar(PAD_CROSS, 0.2f);
	g.cuadro(0);
	cu::caso("CU-FE-PAUSA-CONSTRUIR", g.constructorMazo.abierto, "Construir mazo de la pausa no abrio el constructor");
	const int turnoAntes = g.dueloIA.turno;
	g.apretar(PAD_CIRCLE, 0.3f);
	cu::caso("CU-FE-PAUSA-CONSTRUIR", !g.constructorMazo.abierto && v.pantalla == VD::DUELO && g.dueloIA.turno == turnoAntes, "al cerrar el constructor no volvio al mismo duelo");
	g.esperar(0.5f);
	// OPTIONS largo (2 s): no es la pausa (son los Ajustes de la plantilla)
	g.mantener(PAD_OPTIONS, 2.1f);
	g.esperar(0.2f);
	cu::caso("CU-FE-PAUSA-LARGO", !g.pausa.abierta, "mantener OPTIONS 2 s abrio la pausa");
	// Abandonar: derrota por rendicion con su pantalla
	const int derrotas = v.derrotas;
	abandonar(g);
	g.esperar(0.3f);
	cu::caso("CU-FE-PAUSA-ABANDONAR", v.pantalla == VD::RESULTADO && v.derrotas == derrotas + 1 && v.ganador == 1, "Abandonar no termino en derrota con su pantalla");
	cu::caso("CU-FE-RESULTADO-MOTIVO", resultado::resumen().motivo == resultado::M_RENDICION, "el motivo de abandonar no es la rendicion (%s)", resultado::nombreMotivo(resultado::resumen().motivo));
	// Salir al menu principal desde la pausa del duelo
	irARivales(g, VD::LIBRE);
	g.apretar(PAD_CROSS, 0.3f);
	sorteo(g, false);
	g.esperar(1.0f);
	const int derr = v.derrotas;
	g.apretar(PAD_OPTIONS, 0.2f);
	const std::vector<int> ops = g.opcionesPausa();
	for (int k = 0; k < 8 && ops[(size_t)g.pausa.opcion] != Juego::PA_MENU_PRINCIPAL; k++) g.apretar(PAD_DOWN, 0.12f);
	g.apretar(PAD_CROSS, 0.3f);
	cu::caso("CU-FE-PAUSA-MENU", v.pantalla == VD::MENU && !g.pausa.abierta && v.derrotas == derr, "Salir al menu principal no fue al menu (o conto una derrota)");
	// la pausa en elegir rival cambia de modo
	irARivales(g, VD::HISTORIA);
	g.apretar(PAD_OPTIONS, 0.2f);
	const std::vector<int> ops2 = g.opcionesPausa();
	cu::caso("CU-FE-PAUSA-ELEGIR-RIVAL", g.pausa.abierta && std::find(ops2.begin(), ops2.end(), (int)Juego::PA_DUELO_LIBRE) != ops2.end(), "la pausa en elegir rival no ofrece Duelo libre");
	for (int k = 0; k < 8 && ops2[(size_t)g.pausa.opcion] != Juego::PA_DUELO_LIBRE; k++) g.apretar(PAD_DOWN, 0.12f);
	g.apretar(PAD_CROSS, 0.3f);
	cu::caso("CU-FE-PAUSA-ELEGIR-RIVAL", v.modo == VD::LIBRE && !g.pausa.abierta, "la pausa no cambio a Duelo libre");
}

// ------------------------------------------------------------------------------------------------- Preguntar antes de activar
// el efecto de la opcion en el duelo: Nunca = el juego no se detiene a preguntar (pasa solo); Auto = pregunta (con el
// menu de la pregunta) cuando el jugador tiene algo que activar
inline void preguntarEfecto(Juego& g)
{
	irARivales(g, VD::LIBRE);
	g.apretar(PAD_CROSS, 0.3f);
	sorteo(g, false);
	if (!esperarJugador(g, 90.0f)) {
		cu::caso("CU-FE-OPC-PREGUNTAR-EFECTO", false, "el jugador nunca quedo libre para probar la pregunta");
		abandonar(g);
		return;
	}
	for (int modo : { opciones::PREGUNTAR_AUTO, opciones::PREGUNTAR_NUNCA }) {
		opciones::preguntar() = modo;
		g.dueloIA.ef.preg = ia::Pregunta{};
		g.dueloIA.ef.preg.tipo = ia::P_RESPUESTA;
		g.dueloIA.ef.preg.lado = 0;
		g.esperar(0.5f);
		const bool sigue = g.dueloIA.ef.preg.tipo == ia::P_RESPUESTA && g.dueloIA.ef.preg.lado == 0;
		if (modo == opciones::PREGUNTAR_AUTO)
			cu::caso("CU-FE-OPC-PREGUNTAR-EFECTO", sigue && g.ctl.estado == control::MENU && g.ctl.enPregunta, "con Auto la pregunta del duelo no se mostro al jugador");
		else
			cu::caso("CU-FE-OPC-PREGUNTAR-EFECTO", !sigue, "con Nunca el duelo se detuvo a preguntar (la opcion no tiene efecto)");
		if (sigue) {
			g.ctl.opcion = (int)g.ctl.opciones.size() - 1;
			g.apretar(PAD_CROSS, 0.2f);
		}
	}
	opciones::preguntar() = opciones::PREGUNTAR_SIEMPRE;
	g.esperar(0.3f);
	cu::caso("CU-FE-OPC-PREGUNTAR-EFECTO", g.dueloIA.preguntarSiempre[0] && !g.dueloIA.preguntarSiempre[1], "con Siempre el duelo no pregunta al jugador en cada ventana");
	opciones::preguntar() = opciones::PREGUNTAR_AUTO;
	g.esperar(0.3f);
	cu::caso("CU-FE-OPC-PREGUNTAR-EFECTO", !g.dueloIA.preguntarSiempre[0], "con Auto el duelo sigue preguntando siempre");
	abandonar(g);
}

// ------------------------------------------------------------------------------------------------- resultado
// un duelo que termina al instante como se pida
inline void dueloQueTermina(Juego& g, int modo, int ganador, bool exodia = false)
{
	irARivales(g, modo);
	g.apretar(PAD_CROSS, 0.3f);
	if (historia_dialogos::escena().activa()) g.apretar(PAD_CIRCLE, 0.2f);
	sorteo(g, false);
	g.esperar(2.0f);
	if (g.ctl.vista.pantalla != VD::DUELO) return;
	if (exodia) {
		for (const char* p : { "exodia_the_forbidden_one", "left_arm_of_the_forbidden_one", "right_arm_of_the_forbidden_one", "left_leg_of_the_forbidden_one",
		                       "right_leg_of_the_forbidden_one" })
			g.dueloIA.l[(size_t)ganador].mano.push_back(p);
	} else {
		g.dueloIA.l[(size_t)(1 - ganador)].lp = 0;
	}
	g.dueloIA.terminar(ganador);
	for (int k = 0; k < 900 && g.ctl.vista.pantalla == VD::DUELO; k++) g.cuadro(0);
}

inline void resultadoPantalla(Juego& g)
{
	VD& v = g.ctl.vista;
	resultado::Resumen& r = resultado::resumen();
	// textos de los motivos en los dos idiomas
	for (int m = resultado::M_LP; m <= resultado::M_EFECTO; m++)
		for (int gan = 0; gan <= 2; gan++) {
			idioma::actual() = idioma::ES;
			const std::string es = resultado::textoMotivo((resultado::Motivo)m, gan);
			idioma::actual() = idioma::EN;
			const std::string en = resultado::textoMotivo((resultado::Motivo)m, gan);
			idioma::actual() = idioma::ES;
			cu::caso("CU-FE-IDIOMA-RESULTADO", !es.empty() && !en.empty() && es != en, "el motivo %s no tiene texto ES/EN", resultado::nombreMotivo((resultado::Motivo)m));
		}
	// victoria en duelo libre al nivel maximo desbloqueado: sube de nivel
	v.nivelMax = 1;
	const int vict = v.victorias, duelos0 = estadisticas::datos().duelos;
	dueloQueTermina(g, VD::LIBRE, 0);
	cu::caso("CU-FE-RESULTADO-PANTALLA", v.pantalla == VD::RESULTADO && r.activo && v.ganador == 0, "ganar no mostro el resultado");
	cu::caso("CU-FE-RESULTADO-MOTIVO", r.motivo == resultado::M_LP, "LP a 0: motivo %s", resultado::nombreMotivo(r.motivo));
	cu::caso("CU-FE-RESULTADO-SUBE-NIVEL", v.subeNivel && v.nivelMax == 2 && v.victorias == vict + 1, "ganar al nivel maximo no subio de nivel (%d)", v.nivelMax);
	cu::caso("CU-FE-RESULTADO-ESTADISTICAS", estadisticas::datos().duelos == duelos0 + 1, "el duelo no se conto en las estadisticas");
	{
		std::ifstream f(g.rutaProgreso);
		std::string cl;
		int val, nmax = 0;
		while (f >> cl >> val)
			if (cl == "nivel_max") nmax = val;
		cu::caso("CU-FE-GUARDADO-PROGRESO", nmax == 2, "progreso.txt no guardo el nivel 2 (%d)", nmax);
	}
	g.sonido.sfxs.clear();
	g.esperar(5.6f);
	cu::caso("CU-FE-RESULTADO-MUSICA", g.sonido.musica == "musica_victoria", "tras la victoria suena '%s'", g.sonido.musica.c_str());
	// Revancha: el mismo rival, otra vez con sorteo
	const std::string rivalAntes = g.mazoRivalIA.rival ? g.mazoRivalIA.rival->clave : "";
	r.opcion = resultado::O_REVANCHA;
	g.apretar(PAD_CROSS, 0.3f);
	cu::caso("CU-FE-RESULTADO-REVANCHA", v.pantalla == VD::DUELO && v.sorteo && g.mazoRivalIA.rival && g.mazoRivalIA.rival->clave == rivalAntes, "Revancha no repitio el rival '%s'", rivalAntes.c_str());
	abandonar(g);
	g.esperar(1.0f);
	// Elegir otro rival
	g.apretar(PAD_RIGHT, 0.2f);
	cu::caso("CU-FE-RESULTADO-NAVEGAR", r.opcion == resultado::O_OTRO_RIVAL, "derecha no paso a Elegir otro rival (%d)", r.opcion);
	g.apretar(PAD_CROSS, 0.3f);
	cu::caso("CU-FE-RESULTADO-OTRO-RIVAL", v.pantalla == VD::ELEGIR_RIVAL, "Elegir otro rival no fue a elegir rival");
	// derrota y Menu principal
	dueloQueTermina(g, VD::LIBRE, 1);
	cu::caso("CU-FE-RESULTADO-DERROTA", v.pantalla == VD::RESULTADO && v.ganador == 1 && r.motivo == resultado::M_LP, "la derrota no mostro su resultado");
	g.esperar(6.0f);
	cu::caso("CU-FE-RESULTADO-MUSICA", g.sonido.musica == "musica_derrota", "tras la derrota suena '%s'", g.sonido.musica.c_str());
	r.opcion = resultado::O_MENU;
	g.apretar(PAD_CROSS, 0.3f);
	cu::caso("CU-FE-RESULTADO-MENU", v.pantalla == VD::MENU, "Menu principal no fue al menu");
	// Exodia
	dueloQueTermina(g, VD::LIBRE, 0, true);
	cu::caso("CU-FE-RESULTADO-MOTIVO", r.motivo == resultado::M_EXODIA, "con las 5 piezas el motivo es %s", resultado::nombreMotivo(r.motivo));
	g.esperar(1.0f);
	g.apretar(PAD_CIRCLE, 0.3f);
	cu::caso("CU-FE-RESULTADO-CIRCULO", v.pantalla == VD::MENU, "circulo en el resultado no volvio al menu");
	// historia: vencer a un personaje desbloquea sus firma y dice el siguiente rival
	g.historia.vencidos.clear();
	g.historia.guardar();
	g.coleccion.desbloquear(g.historia.vencidos);
	const personajes::Personaje* conFirma = nullptr;
	for (int i : g.historia.deSaga(0))
		if (!g.historia.lista[(size_t)i].firma.empty() && !conFirma) conFirma = &g.historia.lista[(size_t)i];
	irARivales(g, VD::HISTORIA);
	if (conFirma)
		for (int k = 0; k < 12 && v.personaje != conFirma->id; k++) g.apretar(PAD_RIGHT, 0.12f);
	const std::string pj = v.personaje;
	g.apretar(PAD_CROSS, 0.3f);
	if (historia_dialogos::escena().activa()) g.apretar(PAD_CIRCLE, 0.2f);
	sorteo(g, false);
	g.esperar(1.5f);
	g.dueloIA.l[1].lp = 0;
	g.dueloIA.terminar(0);
	for (int k = 0; k < 900 && v.pantalla == VD::DUELO; k++) g.cuadro(0);
	cu::caso("CU-FE-RESULTADO-HISTORIA", v.pantalla == VD::RESULTADO && v.personajeNuevo && g.historia.vencido(pj), "vencer a %s no lo marco vencido", pj.c_str());
	if (conFirma && pj == conFirma->id) {
		cu::caso("CU-FE-RESULTADO-FIRMA", !r.firmaNuevas.empty() && v.firmaNuevas == (int)r.firmaNuevas.size(), "vencer a %s no desbloqueo sus cartas firma", pj.c_str());
		for (const std::string& id : r.firmaNuevas) {
			const mazo::Carta* k = g.coleccion.carta(id);
			cu::caso("CU-FE-RESULTADO-FIRMA", k && !k->bloqueada, "la firma %s sigue bloqueada", id.c_str());
		}
	} else {
		cu::caso("CU-FE-RESULTADO-FIRMA", false, "no se encontro un personaje con cartas firma en la saga 1");
	}
	cu::caso("CU-FE-RESULTADO-SIGUIENTE", !r.siguienteEs.empty() && !r.siguienteEn.empty(), "el resultado no dice el siguiente rival de la historia");
	{
		personajes::Historia h2;
		h2.leer(g.carpetaAssets + "ui/");
		h2.leerProgreso(g.rutaProgreso.substr(0, g.rutaProgreso.size() - 12) + "historia.txt");
		cu::caso("CU-FE-GUARDADO-HISTORIA", h2.vencido(pj), "historia.txt no guardo que %s fue vencido", pj.c_str());
	}
	g.apretar(PAD_CIRCLE, 0.3f);
	// el final de la historia: vencer al ultimo abre el texto, los creditos y el Modo Desafio
	for (const personajes::Personaje& x : g.historia.lista) g.historia.vencidos.insert(x.id);
	const std::string jefe = g.historia.lista.back().id;
	g.historia.vencidos.erase(jefe);
	g.historia.finalVisto = false;
	g.historia.guardar();
	historia_dialogos::desafioDisponible() = false;
	irARivales(g, VD::HISTORIA);
	v.saga = g.historia.lista.back().sagaIdx;
	v.idx = -1;
	g.elegirRival(true);
	cu::caso("CU-FE-RIVAL-HISTORIA", v.personaje == jefe && v.sagaAbierta, "con todos vencidos menos el jefe, no se elige al jefe (%s)", v.personaje.c_str());
	g.apretar(PAD_CROSS, 0.3f);
	if (historia_dialogos::escena().activa()) g.apretar(PAD_CIRCLE, 0.2f);
	sorteo(g, false);
	g.esperar(1.5f);
	g.dueloIA.l[1].lp = 0;
	g.dueloIA.terminar(0);
	for (int k = 0; k < 900 && v.pantalla == VD::DUELO; k++) g.cuadro(0);
	cu::caso("CU-FE-FINAL", g.finalPendiente && g.historia.completa(), "vencer al jefe no dejo el final pendiente");
	g.esperar(1.0f);
	g.apretar(PAD_CROSS, 0.3f);
	historia_dialogos::Escena& e = historia_dialogos::escena();
	cu::caso("CU-FE-FINAL", e.activa() && e.tipo == historia_dialogos::FINAL, "X en el resultado no abrio el final");
	cu::caso("CU-FE-FINAL-DESAFIO", historia_dialogos::desafioDisponible() && g.historia.finalVisto, "el final no desbloqueo el Modo Desafio");
	int pasos = 0;
	bool vioCreditos = false;
	while (e.activa() && pasos++ < 80) {
		vioCreditos |= e.etapa == historia_dialogos::F_CREDITOS;
		g.apretar(PAD_CROSS, 0.3f);
		if (e.etapa == historia_dialogos::F_CREDITOS) g.esperar(1.2f);
	}
	cu::caso("CU-FE-FINAL-CREDITOS", vioCreditos, "el final no paso por los creditos");
	cu::caso("CU-FE-FINAL", !e.activa() && v.pantalla == VD::ELEGIR_RIVAL, "al terminar el final no volvio a elegir rival");
	{
		personajes::Historia h2;
		h2.leer(g.carpetaAssets + "ui/");
		h2.leerProgreso(g.rutaProgreso.substr(0, g.rutaProgreso.size() - 12) + "historia.txt");
		cu::caso("CU-FE-GUARDADO-HISTORIA", h2.finalVisto && h2.completa(), "historia.txt no guardo el final visto");
	}
	// el desafio: el rival al nivel maximo y el jugador con 4000 LP
	historia_dialogos::desafioActivo() = true;
	irARivales(g, VD::HISTORIA);
	g.apretar(PAD_CROSS, 0.3f);
	if (historia_dialogos::escena().activa()) g.apretar(PAD_CIRCLE, 0.2f);
	sorteo(g, false);
	cu::caso("CU-FE-DESAFIO-DUELO", g.dueloIA.l[0].lp == historia_dialogos::LP_DESAFIO, "el desafio no empezo con %d LP (%d)", historia_dialogos::LP_DESAFIO, g.dueloIA.l[0].lp);
	abandonar(g);
	historia_dialogos::desafioActivo() = false;
	historia_dialogos::desafioDisponible() = false;
	g.historia.vencidos.clear();
	g.historia.finalVisto = false;
	g.historia.guardar();
	g.coleccion.desbloquear(g.historia.vencidos);
	irAlMenu(g);
}

// ------------------------------------------------------------------------------------------------- idioma
inline void idiomaTodo(Juego& g)
{
	// menu principal, Opciones y sus valores, pausa
	for (int a : { Juego::MP_DUELO, Juego::MP_CONSTRUIR, Juego::MP_LISTA, Juego::MP_OPCIONES, Juego::MP_SALIR })
		cu::caso("CU-FE-IDIOMA-MENU", traducido(Juego::textoMenu(a)), "'%s' del menu no tiene traduccion", Juego::textoMenu(a));
	for (int o = 0; o < Juego::NUM_OPCIONES; o++) cu::caso("CU-FE-IDIOMA-OPCIONES", traducido(Juego::textoOpcion(o)), "'%s' de Opciones no tiene traduccion", Juego::textoOpcion(o));
	for (const char* s : { "Sí", "No", "Siempre", "Nunca", "Auto", "Muy rápida", "Rápida", "Normal", "Humana", "Clásica" })
		cu::caso("CU-FE-IDIOMA-OPCIONES", traducido(s), "el valor '%s' de Opciones no tiene traduccion", s);
	for (int a = Juego::PA_CONTINUAR; a <= Juego::PA_OPCIONES; a++)
		if (a != Juego::PA_AJUSTES) cu::caso("CU-FE-IDIOMA-PAUSA", traducido(Juego::textoPausa(a)), "'%s' de la pausa no tiene traduccion", Juego::textoPausa(a));
	for (const char* s : { "Empiezas tú", "Empieza el rival", "Vence a todos los de la saga anterior", "No activar", "Cancelar", "Activar" })
		cu::caso("CU-FE-IDIOMA-AVISOS", traducido(s), "el aviso '%s' no tiene traduccion", s);
	for (const char* f : control::NOMBRE_FASE_LARGO) cu::caso("CU-FE-IDIOMA-FASES", traducido(f), "la fase '%s' no tiene traduccion", f);
	// resultado
	for (int o = 0; o < resultado::NUM_OPCIONES; o++) {
		idioma::actual() = idioma::EN;
		const std::string en = resultado::textoOpcion(o);
		idioma::actual() = idioma::ES;
		cu::caso("CU-FE-IDIOMA-RESULTADO", en != resultado::textoOpcion(o), "la opcion %d del resultado no cambia de idioma", o);
	}
	// reglas (tutorial), controles, fundas, tapetes, sagas
	for (int p = 0; p < tutorial::NUM_PAGINAS; p++) {
		const tutorial::Pagina& pg = tutorial::PAGINAS[p];
		cu::caso("CU-FE-IDIOMA-REGLAS", pg.titulo.es && pg.titulo.en && *pg.titulo.es && *pg.titulo.en && strcmp(pg.titulo.es, pg.titulo.en) != 0, "la pagina %d de las reglas sin titulo ES/EN", p);
	}
	for (int b = 0; b < tutorial::NUM_BOTONES; b++)
		cu::caso("CU-FE-IDIOMA-REGLAS", *tutorial::BOTONES[b].texto.es && *tutorial::BOTONES[b].texto.en, "el boton %d de la ayuda sin texto ES/EN", b);
	for (int p = 0; p < cosmeticos::NUM_PESTANAS; p++)
		for (const cosmeticos::Control& c : cosmeticos::controles(p))
			cu::caso("CU-FE-IDIOMA-CONTROLES", c.es && c.en && *c.es && *c.en && strcmp(c.es, c.en) != 0, "un control de la pestanna %d sin texto ES/EN", p);
	for (int k = 0; k < cosmeticos::NUM_FUNDAS; k++) cu::caso("CU-FE-IDIOMA-PERSO", *cosmeticos::FUNDAS[k].es && *cosmeticos::FUNDAS[k].en, "funda %d sin nombre ES/EN", k);
	for (int k = 0; k < cosmeticos::NUM_TAPETES; k++) cu::caso("CU-FE-IDIOMA-PERSO", *cosmeticos::TAPETES[k].es && *cosmeticos::TAPETES[k].en, "tapete %d sin nombre ES/EN", k);
	for (int s = 0; s < personajes::NUM_SAGAS; s++)
		cu::caso("CU-FE-IDIOMA-SAGAS", *personajes::SAGAS[s].es && *personajes::SAGAS[s].en, "la saga %d sin nombre ES/EN", s);
	// personajes y rivales: nombre y lema en los dos idiomas
	for (const personajes::Personaje& x : g.historia.lista)
		cu::caso("CU-FE-IDIOMA-PERSONAJES", !x.nombreEs.empty() && !x.nombreEn.empty() && !x.lemaEs.empty() && !x.lemaEn.empty(), "%s sin nombre o lema ES/EN", x.id.c_str());
	for (const ia::Rival& r : g.baseIA.rivales)
		cu::caso("CU-FE-IDIOMA-RIVALES", !r.nombreEs.empty() && !r.nombreEn.empty() && !r.lemaEs.empty() && !r.lemaEn.empty(), "rival %s sin nombre o lema ES/EN", r.clave.c_str());
	// la coleccion en ingles: la del idioma actual (constructor_textos.h)
	idioma::actual() = idioma::EN;
	mazo::Coleccion en;
	en.leer(g.carpetaAssets + "ui/");
	idioma::actual() = idioma::ES;
	const mazo::Carta* be = en.carta("blue_eyes");
	cu::caso("CU-FE-IDIOMA-CARTAS", be && be->nombre == "Blue-Eyes White Dragon", "con English la coleccion no usa los nombres en ingles ('%s')", be ? be->nombre.c_str() : "-");
	cu::caso("CU-FE-IDIOMA-CARTAS", be && be->atributoClave == "LUZ", "la clave del atributo cambia con el idioma (filtros)");
	// constructor: sus textos (constructor_textos.h) en ingles
	idioma::actual() = idioma::EN;
	const std::string t1 = constructor::T("Máximo 3 copias");
	idioma::actual() = idioma::ES;
	cu::caso("CU-FE-IDIOMA-CONSTRUCTOR", t1 != "Máximo 3 copias", "el constructor no traduce sus avisos");
}

// ------------------------------------------------------------------------------------------------- mando
inline void mando()
{
	// la cruz: un toque = un paso; mantenida, repite tras 0.40 s y luego cada 0.09 s
	control::Repetidor r;
	int pulsos = r.paso(control::B_ABAJO, control::B_ABAJO, 0, 0, 1.0f / 60.0f) ? 1 : 0;
	for (int k = 1; k < 6; k++) pulsos += r.paso(control::B_ABAJO, 0, 0, 0, 1.0f / 60.0f) ? 1 : 0;
	r.paso(0, 0, 0, 0, 1.0f / 60.0f);
	cu::caso("CU-FE-MANDO-CRUZ-TOQUE", pulsos == 1, "un toque de 0.1 s dio %d pulsos", pulsos);
	control::Repetidor r2;
	pulsos = r2.paso(control::B_ABAJO, control::B_ABAJO, 0, 0, 1.0f / 60.0f) ? 1 : 0;
	for (int k = 1; k < 60; k++) pulsos += r2.paso(control::B_ABAJO, 0, 0, 0, 1.0f / 60.0f) ? 1 : 0;
	cu::caso("CU-FE-MANDO-CRUZ-REPETIR", pulsos >= 6 && pulsos <= 9, "mantener 1 s dio %d pulsos (esperados ~7)", pulsos);
	// el stick izquierdo hace de cruz
	control::Repetidor r3;
	uint32_t b = 0;
	for (int k = 0; k < 3; k++) b |= r3.paso(0, 0, 0.0f, 0.9f, 1.0f / 60.0f);
	cu::caso("CU-FE-MANDO-STICK", (b & control::B_ABAJO) != 0, "el stick abajo no movio");
	b = 0;
	for (int k = 0; k < 3; k++) b |= r3.paso(0, 0, 0.0f, 0.1f, 1.0f / 60.0f);
	cu::caso("CU-FE-MANDO-STICK", b == 0, "el stick suelto siguio moviendo");
	// R2: mantener acelera, un toque salta, y solo si hay animacion o juega el rival
	velocidad::poner(0);
	velocidad::Control v;
	float e = 0.0f;
	for (int k = 0; k < 30; k++) e = v.cuadro(1.0f, true, false, 1.0f / 60.0f);
	cu::caso("CU-FE-MANDO-R2-ACELERA", fabsf(e - velocidad::factor() * velocidad::ACELERA) < 0.01f && v.consume, "mantener R2 con animacion: escala %.2f", e);
	v.reiniciar();
	for (int k = 0; k < 30; k++) e = v.cuadro(1.0f, false, false, 1.0f / 60.0f);
	cu::caso("CU-FE-MANDO-R2-ACELERA", fabsf(e - velocidad::factor()) < 0.01f && !v.consume, "R2 sin animacion acelero (escala %.2f): debe ser zoom", e);
	v.reiniciar();
	for (int k = 0; k < 5; k++) v.cuadro(1.0f, true, false, 1.0f / 60.0f);
	e = v.cuadro(0.0f, true, false, 1.0f / 60.0f);
	cu::caso("CU-FE-MANDO-R2-SALTA", e >= velocidad::SALTO, "un toque de R2 no salto la animacion (escala %.2f)", e);
	// los factores de velocidad suben
	cu::caso("CU-FE-MANDO-VELOCIDAD", velocidad::factor(0) < velocidad::factor(1) && velocidad::factor(1) < velocidad::factor(2), "los factores de velocidad no suben");
}

// ------------------------------------------------------------------------------------------------- guardado
inline void guardadoAtomico(const std::string& dir)
{
	const std::string ruta = dir + "prueba_guardado.txt";
	const int serie = guardado::estado().serie;
	std::string grande;
	for (int k = 0; k < 2000; k++) guardado::agregar(grande, "linea %d con texto largo para el guardado atomico\n", k);
	const bool ok = guardado::guardarAtomico(ruta, grande);
	cu::caso("CU-FE-GUARDADO-ATOMICO", ok && leerArchivo(ruta) == grande && !existe(ruta + ".tmp"), "guardarAtomico no dejo el archivo entero (o quedo el .tmp)");
	cu::caso("CU-FE-GUARDADO-ATOMICO", guardado::estado().serie == serie, "un guardado bueno dio aviso");
	// sobrescribir: el archivo viejo se reemplaza entero
	guardado::guardarAtomico(ruta, "corto\n");
	cu::caso("CU-FE-GUARDADO-ATOMICO", leerArchivo(ruta) == "corto\n", "sobrescribir no reemplazo el archivo entero");
	// fallo: carpeta que no existe -> false, aviso con el nombre del archivo
	const bool mal = guardado::guardarAtomico(dir + "no_existe/x/opciones.txt", "x");
	cu::caso("CU-FE-GUARDADO-AVISO", !mal && guardado::estado().ultimoFallo && guardado::estado().archivo == "opciones.txt" && guardado::estado().serie == serie + 1,
	         "un guardado imposible no dio aviso");
	// sin carpeta de datos: un solo aviso
	guardado::avisarSinCarpeta();
	guardado::avisarSinCarpeta();
	cu::caso("CU-FE-GUARDADO-SIN-CARPETA", guardado::estado().sinCarpeta && guardado::estado().serie == serie + 2, "el aviso de sin carpeta salio %d veces", guardado::estado().serie - serie - 1);
	// estadisticas: guardar y leer
	estadisticas::Datos copia = estadisticas::datos();
	const std::string rutaEst = estadisticas::datos().ruta;   // (copia: leer() reinicia datos() antes de abrir)
	estadisticas::leer(rutaEst);
	const estadisticas::Datos& d = estadisticas::datos();
	cu::caso("CU-FE-GUARDADO-ESTADISTICAS", d.duelos == copia.duelos && d.rivales.size() == copia.rivales.size() && d.cartas.size() == copia.cartas.size() && d.mazos.size() == copia.mazos.size() &&
	                                             d.racha == copia.racha && d.mejorRacha == copia.mejorRacha,
	         "estadisticas.txt no vuelve igual al leerlo: duelos %d/%d rivales %d/%d cartas %d/%d mazos %d/%d racha %d/%d (%s)", d.duelos, copia.duelos, (int)d.rivales.size(),
	         (int)copia.rivales.size(), (int)d.cartas.size(), (int)copia.cartas.size(), (int)d.mazos.size(), (int)copia.mazos.size(), d.racha, copia.racha, copia.ruta.c_str());
}

}  // namespace cuf
