	// Yugioh Classic Remaster (rama classic-integracion): se incluye DENTRO de la clase del juego (yugireload.cpp).
	// El duelo real contra la IA. ia::Duelo (ia.h) es el estado autoritativo: el lado 0 es el jugador, el 1 el rival.
	//  - El jugador arma jugadas con el mando (control.h, modo duelo: ctl.duelo / ctl.jugar) y jugarJugador las aplica.
	//  - El rival juega con ia::Turno::avanzar(dt) (personalidad y tiempos de pensar del rival elegido).
	//  - presentarSucesos() lleva la mesa (control.h) detras del duelo: cada jugada deja sucesos (ia::Suceso); los
	//    ataques se animan con el ataque plano del juego (embestida, danno, destruccion) y lo demas se sincroniza con
	//    Mando::sincronizarLado (llegadas, volteos y giros animados). LP reales: los del duelo.
	//  - Victoria/derrota (LP 0 o mazo vacio): pantalla de resultado; ganar el nivel mas alto desbloquea el siguiente
	//    (8 niveles). El progreso se guarda en progreso.txt junto al mazo (mazo::rutaMazo).
	//  - Mazo del jugador = el del constructor (mazoDuelo de mazo_duelo.h); rival = baseIA.mazoRival(nivel, semilla).
	// rama classic-efectos: el motor de efectos (ia.h) va dentro del duelo; aqui se presentan sus sucesos (activaciones
	// con su animacion y la cadena en orden, LP, control, negaciones) y la pregunta al jugador la abre control.h.
	ia::Base baseIA;
	bool baseIAlista{ false };
	ia::Duelo dueloIA{ &baseIA };
	ia::Turno turnoIA;
	ia::MazoRival mazoRivalIA;
	ia::Memoria memoriaIA;   // rama classic-variedad: variedad.txt (ia_plan.h)
	size_t sucesosVistos{ 0 };
	std::string bloqueoAntes{ "?" };   // (rama classic-arreglos: el motivo del bloqueo que ya se escribio)
	bool mesaPendiente{ false };   // hay sucesos presentados que la mesa todavia no refleja
	float esperaEfecto{ 0.0f };    // rama classic-efectos: s que faltan de la animacion de una activacion (la cadena se ve en orden)
	// rama classic-duelo-arreglos: robar animado. ocultas[lado]: cartas del final de la mano del duelo que la mesa todavia
	// no muestra (llegan con su animacion); reparto: cartas del reparto inicial que faltan (alternando desde repartoLado)
	int ocultas[2]{ 0, 0 };
	int reparto{ 0 }, repartoLado{ 0 };
	uint32_t semillaRival{ 1 };
	int rivalIdx{ -1 };   // rama classic-menus: duelo libre, el rival elegido dentro del nivel (-1 = uno al azar)
	std::string rutaProgreso;
	std::string musicaActual;   // (rama classic-audio: no reiniciar la pista que ya suena)
	using VD = control::VistaDuelo;
	personajes::Historia historia;   // rama classic-personajes: modo historia (personajes.h)

	// ---- sonido (audio.h; claves de assets/audio/audio.txt)
	static constexpr int LP_TENSO = 2000;   // con LP de cualquiera por debajo, la musica tensa
	void musica(const char* clave, float fundido = 1.5f)
	{
		if (musicaActual == clave) return;
		musicaActual = clave;
		sonido.tocarMusica(clave, fundido);
	}
	// la musica que va con la pantalla (al volver del constructor, con LP bajos...)
	void musicaDePantalla()
	{
		if (ctl.vista.pantalla == VD::DUELO) {
			const int lpMin = std::min(dueloIA.l[0].lp, dueloIA.l[1].lp);
			// (rama classic-resultado: con la carta clave del jugador en juego, su musica hasta la tensa)
			musica(lpMin <= LP_TENSO ? "musica_duelo_tensa" : claveSonando && sonido.hay("musica_carta_clave") ? "musica_carta_clave"
			       : (semillaRival & 2u) && sonido.hay("musica_duelo_2") ? "musica_duelo_2" : "musica_duelo");
		} else if (ctl.vista.pantalla == VD::ELEGIR_RIVAL || ctl.vista.pantalla == VD::MENU || ctl.vista.pantalla == VD::PORTADA) {
			musica("musica_menu");
		}
	}
	// lo que suena por la jugada (sacrificios, fusion, ritual); lo demas (invocar, colocar, atacar...) lo pone
	// audio::Duelo mirando los eventos y la mesa
	void sonidoJugada(const ia::Jugada& j)
	{
		if (j.tipo == ia::J_FUSION) {
			sonido.sfx("sfx_fusion");
		} else if (j.tipo == ia::J_RITUAL) {
			sonido.sfx("sfx_ritual");
		} else if (!j.tributos.empty() && (j.tipo == ia::J_INVOCAR || j.tipo == ia::J_COLOCAR || j.tipo == ia::J_TOON)) {
			sonido.sfx("sfx_tributo");
			if (j.tipo != ia::J_COLOCAR) sonido.sfx("sfx_invocar_tributo", 1.0f, 0.0f, 0.35f);
		}
	}

	void prepararDuelo()
	{
		baseIAlista = baseIA.leer(getAssetPath() + "ui/");
		if (!baseIAlista) fprintf(stderr, "[duelo] sin cartas o sin rivales.txt: no se puede jugar contra la IA\n");
		const size_t barra = rutaMazoJ.find_last_of('/');
		rutaProgreso = (barra == std::string::npos ? std::string() : rutaMazoJ.substr(0, barra + 1)) + "progreso.txt";
		leerProgreso();
		// rama classic-variedad: memoria del rival (planes y mazos usados, lectura del jugador, racha) junto a progreso.txt
		memoriaIA.leer(rutaProgreso.substr(0, rutaProgreso.size() - std::string("progreso.txt").size()) + "variedad.txt");
		// rama classic-personajes: los personajes del anime (modo historia, el modo por defecto si estan)
		if (historia.leer(getAssetPath() + "ui/")) {
			historia.leerProgreso(rutaProgreso.substr(0, rutaProgreso.size() - std::string("progreso.txt").size()) + "historia.txt");
			ctl.vista.modo = VD::HISTORIA;
			ctl.vista.sagas = personajes::NUM_SAGAS;
			// cartas firma: el jugador las tiene desde que vence a su duenno
			coleccion.nombrePersonaje = [this](const std::string& id) {
				const personajes::Personaje* x = historia.buscar(id);
				return x ? x->nombre(idioma::ingles()) : id;
			};
			coleccion.desbloquear(historia.vencidos);
			ctl.vista.saga = historia.sagaActual();
			ctl.vista.idx = -1;   // (el primero sin vencer)
			// rama classic-historia: dialogos (historia_dialogos.h) y el Modo Desafio (premio del final)
			historia_dialogos::base().leer(getAssetPath() + "ui/");
			historia_dialogos::desafioDisponible() = historia.finalVisto || historia.completa();
		}
		ctl.duelo = &dueloIA;
		ctl.jugar = [this](const ia::Jugada& j) { return jugarJugador(j); };
		ctl.nombre = [this](const std::string& id) { return interfazDuelo.nombreCarta(id); };
		// rama classic-efectos: los rotulos ATK/DEF de la mesa con equipos, campos y efectos
		interfazDuelo.statsZona = [this](const char* zona, int& a, int& d) {
			if (ctl.vista.pantalla != VD::DUELO) return false;
			const std::string z = zona;
			const bool rival = z.rfind("rival_monstruo_", 0) == 0;
			if ((!rival && z.rfind("jugador_monstruo_", 0) != 0) || z.empty()) return false;
			const int i = z.back() - '0';
			if (i < 0 || i >= ia::ZONAS) return false;
			const ia::Monstruo& m = dueloIA.l[rival ? 1 : 0].m[(size_t)i];
			if (m.vacia()) return false;
			a = m.atkEf;
			d = m.defEf;
			return true;
		};
		ctl.demo = false;
		ataquesSolos = false;
		campoAuto = false;
		estadoCampo = CAMPO_VACIO;   // sin la carta de campo de la demo
		entornosRival.soltarCampo();   // (rama classic-campos: ni su fondo)
		irAElegirRival();
	}

	// ---- menu de pausa (rama classic-arreglos). OPTIONS corto (soltar antes de OPCIONES_CORTO s) lo abre o lo cierra;
	// mantener OPTIONS 2 s sigue siendo la ventana de Ajustes de la plantilla (parche_base.py), asi que un toque nunca
	// abre los Ajustes por accidente: desde la pausa se abren a proposito ("Ajustes") y un OPTIONS corto los cierra.
	// Mientras esta abierta el duelo se detiene (avanzarDueloIA no corre: el rival no juega). Opciones:
	//  - en el duelo: Continuar, Construir mazo, Abandonar duelo (cuenta como derrota) y Salir al menu principal (el
	//    menu principal de menu_principal.h; el duelo en curso se descarta sin contar)
	//  - en elegir rival / resultado: Continuar, Modo historia o Duelo libre (el otro modo), Construir mazo y Salir al
	//    menu principal
	// rama classic-menus: sin "Ajustes" (FPS, render... son de desarrollo: menu_principal.h, vigilarAjustes)
	enum AccionPausa { PA_CONTINUAR = 0, PA_AJUSTES, PA_CONSTRUIR, PA_ABANDONAR, PA_MENU_PRINCIPAL, PA_HISTORIA, PA_DUELO_LIBRE, PA_OPCIONES };
	static constexpr float OPCIONES_CORTO = 0.8f;   // s (los Ajustes de la plantilla piden 2 s)
	struct Pausa { bool abierta{ false }; int opcion{ 0 }; float abajo{ 0.0f }; } pausa;

	std::vector<int> opcionesPausa() const
	{
		if (ctl.vista.pantalla == VD::DUELO) return { PA_CONTINUAR, PA_OPCIONES, PA_CONSTRUIR, PA_ABANDONAR, PA_MENU_PRINCIPAL };
		std::vector<int> r{ PA_CONTINUAR, PA_OPCIONES };
		if (ctl.vista.pantalla == VD::ELEGIR_RIVAL && !historia.lista.empty()) r.push_back(ctl.vista.modo == VD::HISTORIA ? PA_DUELO_LIBRE : PA_HISTORIA);
		r.push_back(PA_CONSTRUIR);
		r.push_back(PA_MENU_PRINCIPAL);
		return r;
	}
	static const char* textoPausa(int a)
	{
		switch (a) {
		case PA_CONTINUAR: return "Continuar";
		case PA_AJUSTES: return "Ajustes";
		case PA_CONSTRUIR: return "Construir mazo";
		case PA_ABANDONAR: return "Abandonar duelo (derrota)";
		case PA_MENU_PRINCIPAL: return "Salir al menú principal";
		case PA_HISTORIA: return "Modo historia";
		case PA_DUELO_LIBRE: return "Duelo libre";
		case PA_OPCIONES: return "Opciones";   // (rama classic-portada: las Opciones del menu, y vuelve a la pausa)
		default: return "";
		}
	}
	// lo que dibuja la interfaz (duelo_ui.h: dibujarPausa)
	void mostrarPausa()
	{
		VD& v = ctl.vista;
		v.pausa = pausa.abierta && !menuP.enOpciones;   // (con las Opciones abiertas desde la pausa, se ven ellas)
		v.pausaOpcion = pausa.opcion;
		v.pausaOpciones.clear();
		for (int a : opcionesPausa()) v.pausaOpciones.push_back(textoPausa(a));
	}
	void cerrarPausa(const char* por)
	{
		pausa.abierta = false;
		mostrarPausa();
		fprintf(stderr, "[pausa] cerrada (%s)\n", por);
	}

	// cada cuadro, con los pulsos del mando y ANTES de mirar si los Ajustes estan abiertos. Devuelve true si la pausa
	// (o los Ajustes) se quedaron con el mando en este cuadro
	bool avanzarPausa(uint32_t p)
	{
		const float dt = std::min(std::max(frameTimer, 0.0f), 0.1f);
		bool corto = false;
		if (mando.held & PAD_OPTIONS) {
			pausa.abajo += dt;
		} else {
			corto = pausa.abajo > 0.0f && pausa.abajo < OPCIONES_CORTO;
			if (pausa.abajo > 0.0f) fprintf(stderr, "[pausa] OPTIONS soltado tras %.2f s (%s)\n", pausa.abajo, corto ? "corto: pausa" : "largo: Ajustes de la plantilla");
			pausa.abajo = 0.0f;
		}
		if (ajustesAbiertos()) {
			if (corto) {
				ui.visible = false;   // (un toque de OPTIONS cierra los Ajustes)
				fprintf(stderr, "[pausa] Ajustes cerrados con OPTIONS\n");
			}
			return true;
		}
		if (constructorMazo.abierto || ctl.vista.pantalla == VD::NINGUNA || ctl.vista.pantalla == VD::MENU || ctl.vista.pantalla == VD::PORTADA) return false;
		if (!pausa.abierta) {
			if (!corto) return false;
			pausa.abierta = true;
			pausa.opcion = 0;
			mostrarPausa();
			sonido.sfx("sfx_menu_confirmar");
			fprintf(stderr, "[pausa] abierta (pantalla %d, turno %d, fase %d)\n", ctl.vista.pantalla, dueloIA.turno, (int)dueloIA.fase);
			return true;
		}
		if (menuP.enOpciones) {   // rama classic-portada: Opciones abiertas desde la pausa (menu_principal.h)
			avanzarOpciones(p, corto);
			return true;
		}
		const std::vector<int> ops = opcionesPausa();
		const int n = (int)ops.size();
		pausa.opcion = std::max(0, std::min(pausa.opcion, n - 1));
		if (corto || (p & control::B_CIRCULO)) {
			sonido.sfx("sfx_menu_cancelar");
			cerrarPausa(corto ? "OPTIONS" : "circulo");
			return true;
		}
		if (p & control::B_ARRIBA) pausa.opcion = (pausa.opcion + n - 1) % n;
		if (p & control::B_ABAJO) pausa.opcion = (pausa.opcion + 1) % n;
		if (p & (control::B_ARRIBA | control::B_ABAJO)) sonido.sfx("sfx_menu_cursor", 0.7f);
		if (p & control::B_X) {
			const int a = ops[(size_t)pausa.opcion];
			fprintf(stderr, "[pausa] elige '%s'\n", textoPausa(a));
			sonido.sfx("sfx_menu_confirmar");
			if (a == PA_OPCIONES) {   // (la pausa sigue abierta debajo: el duelo queda detenido)
				menuP.enOpciones = true;
				menuP.opcionOpc = 0;
				mostrarMenu();
				mostrarPausa();
				return true;
			}
			cerrarPausa("eleccion");
			switch (a) {
			case PA_AJUSTES: ui.visible = true; break;
			case PA_CONSTRUIR: pedidoConstructor = true; break;
			case PA_ABANDONAR: rendirse(); break;   // rama classic-resultado: derrota con su pantalla (resultado_juego.h)
			case PA_MENU_PRINCIPAL: irAMenuPrincipal(); break;   // rama classic-menus: el menu principal de verdad
			case PA_HISTORIA:
			case PA_DUELO_LIBRE:
				ctl.vista.modo = a == PA_HISTORIA ? VD::HISTORIA : VD::LIBRE;
				ctl.vista.t = 0.0f;
				elegirRival(true);
				break;
			default: break;
			}
			return true;
		}
		mostrarPausa();
		return true;
	}

	// ---- progreso (nivel desbloqueado, victorias, derrotas)
	void leerProgreso()
	{
		std::ifstream f(rutaProgreso);
		std::string clave;
		int v;
		while (f >> clave >> v) {
			if (clave == "nivel_max") ctl.vista.nivelMax = std::max(1, std::min(v, ia::NIVEL_MAX));
			if (clave == "victorias") ctl.vista.victorias = std::max(0, v);
			if (clave == "derrotas") ctl.vista.derrotas = std::max(0, v);
		}
		ctl.vista.niveles = ia::NIVEL_MAX;
		fprintf(stderr, "[duelo] progreso: nivel %d, %d victorias, %d derrotas (%s)\n", ctl.vista.nivelMax, ctl.vista.victorias, ctl.vista.derrotas,
		        rutaProgreso.c_str());
	}
	void guardarProgreso()
	{
		std::string s;   // (rama classic-pulido: guardado atomico, guardado.h)
		guardado::agregar(s, "nivel_max %d\nvictorias %d\nderrotas %d\n", ctl.vista.nivelMax, ctl.vista.victorias, ctl.vista.derrotas);
		guardado::guardarAtomico(rutaProgreso, s);
	}

	// ---- pantallas
	void irAElegirRival()
	{
		ctl.vista.pantalla = VD::ELEGIR_RIVAL;
		ctl.vista.t = 0.0f;
		ctl.vista.turnoRival = ctl.vista.pensando = false;
		ctl.vista.sorteo = false;
		ctl.robo.activo = false;
		reparto = ocultas[0] = ocultas[1] = 0;
		ctl.vista.nivel = ctl.vista.nivelMax;
		rivalIdx = -1;
		ctl.bloqueado = true;
		musica("musica_menu");
		ctl.preparar({}, {}, {});   // mesa vacia
		ctl.manoR = 0;
		interfazDuelo.contarMazos(0, 0, 0);
		elegirRival(true);
	}

	void elegirRival(bool nuevaSemilla)
	{
		if (nuevaSemilla) semillaRival = (uint32_t)std::chrono::steady_clock::now().time_since_epoch().count() | 1u;
		if (ctl.vista.modo == VD::HISTORIA && !historia.lista.empty()) {
			elegirPersonaje();
			return;
		}
		ctl.vista.personaje = ctl.vista.entorno = ctl.vista.varianteEs = ctl.vista.varianteEn = "";
		// rama classic-menus: el rival del nivel lo elige el jugador (izq/der); rivalIdx -1 = uno al azar
		const std::vector<const ia::Rival*> delNivel = baseIAlista ? baseIA.deNivel(ctl.vista.nivel) : std::vector<const ia::Rival*>{};
		ctl.vista.rivalesNivel = (int)delNivel.size();
		if (delNivel.empty()) {
			mazoRivalIA = ia::MazoRival{};
		} else {
			if (rivalIdx < 0) {
				mazoRivalIA = baseIA.mazoRival(ctl.vista.nivel, semillaRival);
				rivalIdx = 0;
				for (size_t k = 0; k < delNivel.size(); k++)
					if (delNivel[k] == mazoRivalIA.rival) rivalIdx = (int)k;
			}
			rivalIdx = ((rivalIdx % (int)delNivel.size()) + (int)delNivel.size()) % (int)delNivel.size();
			mazoRivalIA = baseIA.mazoDe(*delNivel[(size_t)rivalIdx], semillaRival);
		}
		ctl.vista.rivalIdx = rivalIdx;
		// rama classic-variedad: el mismo rival, pero su plan y su mazo se arman para este duelo (prepararRival)
		if (mazoRivalIA.rival) mazoRivalIA = ia::prepararRival(baseIA, *mazoRivalIA.rival, semillaRival, &memoriaIA);
		VD& v = ctl.vista;
		v.rivalEs = v.rivalEn = v.lemaEs = v.lemaEn = v.personalidad = "";
		if (const ia::Rival* r = mazoRivalIA.rival) {
			v.rivalEs = r->nombreEs;
			v.rivalEn = r->nombreEn;
			v.lemaEs = r->lemaEs;
			v.lemaEn = r->lemaEn;
			v.personalidad = r->clavePersonalidad;
			v.nivelRival = r->nivel;
		}
		entornoDelRival(mazoRivalIA.rival);   // rama classic-entornos: el lugar del rival (por su tema) rodea la mesa
	}

	// rama classic-personajes: el personaje elegido en el modo historia (saga v.saga, posicion v.idx; -1 = el primero
	// sin vencer) con su mazo (baseIA.mazoRival(id): su variante de siempre el 60 %)
	void elegirPersonaje()
	{
		VD& v = ctl.vista;
		v.sagas = personajes::NUM_SAGAS;
		v.saga = std::max(0, std::min(v.saga, v.sagas - 1));
		const std::vector<int> fila = historia.deSaga(v.saga);
		v.sagaAbierta = historia.sagaAbierta(v.saga);
		v.sagaEs = personajes::SAGAS[v.saga].es;
		v.sagaEn = personajes::SAGAS[v.saga].en;
		v.fila.clear();
		v.filaEstado.clear();
		for (int i : fila) {
			const personajes::Personaje& x = historia.lista[(size_t)i];
			v.fila.push_back(x.id);
			v.filaEstado.push_back(!v.sagaAbierta ? 0 : historia.vencido(x.id) ? 2 : 1);
		}
		v.vencidos = (int)historia.vencidos.size();
		v.totalPersonajes = (int)historia.lista.size();
		if (fila.empty()) return;
		if (v.idx < 0) {
			v.idx = 0;
			while (v.idx + 1 < (int)fila.size() && v.filaEstado[(size_t)v.idx] == 2) v.idx++;
		}
		v.idx = std::max(0, std::min(v.idx, (int)fila.size() - 1));
		const personajes::Personaje& p = historia.lista[(size_t)fila[(size_t)v.idx]];
		mazoRivalIA = baseIAlista ? baseIA.mazoRival(p.id, semillaRival) : ia::MazoRival{};
		if (mazoRivalIA.rival) mazoRivalIA = ia::prepararRival(baseIA, *mazoRivalIA.rival, semillaRival, &memoriaIA);   // (classic-variedad)
		v.personaje = p.id;
		v.entorno = p.entorno;
		v.rivalEs = p.nombreEs;
		v.rivalEn = p.nombreEn;
		v.lemaEs = p.lemaEs;
		v.lemaEn = p.lemaEn;
		v.personalidad = p.personalidad;
		v.nivelRival = p.nivel;
		v.varianteEs = v.varianteEn = "";
		if (const ia::Rival* r = mazoRivalIA.rival) {
			const size_t k = (size_t)mazoRivalIA.variante;
			if (k < r->varEs.size()) v.varianteEs = r->varEs[k];
			if (k < r->varEn.size()) v.varianteEn = r->varEn[k];
			// rama classic-mazos-torneo: un mazo de torneo se nombra por su arquetipo
			if (mazoRivalIA.torneo >= 0 && (size_t)mazoRivalIA.torneo < baseIA.torneos.size()) {
				v.varianteEs = baseIA.torneos[(size_t)mazoRivalIA.torneo].nombreEs + " (torneo)";
				v.varianteEn = baseIA.torneos[(size_t)mazoRivalIA.torneo].nombreEn + " (tournament)";
			}
		}
		// rama classic-entornos: el lugar donde duela el personaje en la serie (su campo "entorno") rodea la mesa
		// (rama classic-fondos: el "entorno" es el id del personaje = su fondo propio; una clave de tema tambien vale)
		if (!entornoFijo) {
			if (p.entorno.empty()) {
				entornosRival.pedir(entornos::porTema(mazoRivalIA.rival ? mazoRivalIA.rival->tema : ""));
			} else {
				entornosRival.pedir(p.entorno);
			}
		}
		entornoVista = v.entorno;
	}

	// rama classic-historia: las reglas (tutorial.h) y la escena del modo historia (historia_dialogos.h: dialogo antes
	// del duelo y final) se quedan con el mando mientras estan abiertas. Devuelve true si se quedo con el mando
	bool finalPendiente{ false };   // se completo la historia: al salir del resultado, el final
	bool avanzarHistoriaDialogos(uint32_t p)
	{
		VD& v = ctl.vista;
		tutorial::Estado& tut = tutorial::estado();
		if (tut.abierto) {
			const int r = tut.avanzar(p);
			if (r == 1) sonido.sfx("sfx_menu_cursor", 0.7f);
			if (r == 2) sonido.sfx("sfx_menu_cancelar");
			return true;
		}
		historia_dialogos::Escena& e = historia_dialogos::escena();
		if (e.activa()) {
			const int tipo = e.tipo;
			if (p & (control::B_X | control::B_CIRCULO)) sonido.sfx("sfx_menu_cursor", 0.6f);
			if (e.avanzar(p)) {
				if (tipo == historia_dialogos::INTRO) {
					empezarDueloIA();
				} else {
					irAElegirRival();
				}
			}
			return true;
		}
		if (v.pantalla != VD::ELEGIR_RIVAL) return false;
		if (p & control::B_R1) {   // las reglas del duelo
			sonido.sfx("sfx_menu_confirmar");
			tut.abrir();
			return true;
		}
		if ((p & control::B_L1) && v.modo == VD::HISTORIA && historia_dialogos::desafioDisponible()) {   // Modo Desafio
			historia_dialogos::desafioActivo() = !historia_dialogos::desafioActivo();
			sonido.sfx("sfx_menu_confirmar");
			fprintf(stderr, "[historia] modo desafio: %s\n", historia_dialogos::desafioActivo() ? "si" : "no");
			return true;
		}
		return false;
	}

	// botones fuera de la mesa (elegir rival, resultado). Devuelve true si la pantalla se quedo con el mando
	bool avanzarPantallaDuelo(uint32_t p)
	{
		VD& v = ctl.vista;
		if (avanzarHistoriaDialogos(p)) return true;   // rama classic-historia
		if (v.pantalla == VD::DUELO && v.sorteo) return avanzarSorteo(p);   // rama classic-duelo-arreglos
		if (v.pantalla == VD::MENU) return avanzarMenuPrincipal(p);   // rama classic-menus (menu_principal.h)
		if (v.pantalla == VD::PORTADA) return avanzarPortada(p);       // rama classic-portada (portada.h)
		if ((v.pantalla == VD::ELEGIR_RIVAL || (v.pantalla == VD::RESULTADO && v.t > 0.8f)) && (p & control::B_CIRCULO)) {   // volver
			sonido.sfx("sfx_menu_cancelar");
			irAMenuPrincipal();
			return true;
		}
		if (v.pantalla == VD::ELEGIR_RIVAL && (p & control::B_TRIANGULO) && !historia.lista.empty()) {   // historia <-> libre
			v.modo = v.modo == VD::HISTORIA ? VD::LIBRE : VD::HISTORIA;
			v.t = 0.0f;
			elegirRival(true);
			return true;
		}
		if (v.pantalla == VD::ELEGIR_RIVAL && v.modo == VD::HISTORIA && !historia.lista.empty()) {
			const int s0 = v.saga, i0 = v.idx;
			if (p & control::B_ARRIBA) v.saga = std::max(0, v.saga - 1);
			if (p & control::B_ABAJO) v.saga = std::min(v.sagas - 1, v.saga + 1);
			if (v.saga != s0) v.idx = -1;
			if (p & control::B_IZQ) v.idx = std::max(0, v.idx - 1);
			if (p & control::B_DER) v.idx = std::min((int)v.fila.size() - 1, v.idx + 1);
			if (v.saga != s0 || v.idx != i0) elegirRival(false);
			if (p & control::B_CUADRADO) elegirRival(true);   // (otra variante de su mazo)
			if (p & control::B_X) {
				if (v.sagaAbierta) {
					// rama classic-historia: primero la escena del rival (si tiene dialogo); al terminarla, el duelo
					if (historia_dialogos::escena().abrir(v.personaje, v.rivalEs, v.rivalEn)) {
						sonido.sfx("sfx_menu_confirmar");
					} else {
						empezarDueloIA();
					}
				} else {
					ctl.aviso = "Vence a todos los de la saga anterior";
					ctl.tAviso = 0.0f;
				}
			}
			return true;
		}
		if (v.pantalla == VD::ELEGIR_RIVAL) {
			// rama classic-menus: arriba/abajo cambian el nivel (hasta el desbloqueado) e izq/der el rival del nivel
			const int antes = v.nivel, rivalAntes = rivalIdx;
			if (p & control::B_ABAJO) v.nivel = std::max(1, v.nivel - 1);
			if (p & control::B_ARRIBA) v.nivel = std::min(v.nivelMax, v.nivel + 1);
			if (v.nivel != antes) {
				rivalIdx = -1;
				elegirRival(false);
				sonido.sfx("sfx_menu_cursor");
			} else if (v.rivalesNivel > 1 && (p & (control::B_IZQ | control::B_DER))) {
				rivalIdx += (p & control::B_DER) ? 1 : -1;
				elegirRival(false);
				if (rivalIdx != rivalAntes) sonido.sfx("sfx_menu_cursor");
			} else if (p & control::B_DIRECCIONES) {
				sonido.sfx("sfx_error", 0.5f);   // (nivel bloqueado o el primero, o un solo rival)
			}
			if (p & control::B_CUADRADO) {
				rivalIdx = -1;
				elegirRival(true);
				sonido.sfx("sfx_menu_cursor");
			}
			if (p & control::B_X) {
				sonido.sfx("sfx_menu_confirmar");
				empezarDueloIA();
			}
			return true;
		}
		if (v.pantalla == VD::RESULTADO && finalPendiente && (p & control::B_X) && v.t > 0.8f) {
			// rama classic-historia: completada la historia, el final (texto, creditos y el Modo Desafio)
			const personajes::Personaje* jefe = historia.lista.empty() ? nullptr : &historia.lista.back();
			if (finalPendiente && jefe && historia_dialogos::escena().abrirFinal(jefe->id, jefe->nombreEs, jefe->nombreEn)) {
				finalPendiente = false;
				historia.finalVisto = true;
				historia.guardar();
				historia_dialogos::desafioDisponible() = true;
				musica("musica_menu");
				return true;
			}
		}
		if (v.pantalla == VD::RESULTADO) return avanzarResultado(p);   // rama classic-resultado: revancha / otro rival / menu
		return false;
	}

	// rama classic-duelo-arreglos: antes del duelo, piedra, papel o tijera (el ganador decide quien empieza). La mesa
	// queda vacia y el mando lo lleva avanzarSorteo; al decidir, arrancarDueloIA(empieza)
	void empezarDueloIA()
	{
		if (!mazoRivalIA.rival || mazoRivalIA.principal.empty()) {
			ctl.aviso = "Sin rivales (falta rivales.txt)";
			ctl.tAviso = 0.0f;
			return;
		}
		VD& v = ctl.vista;
		ctl.preparar({}, {}, {});
		ctl.manoR = 0;
		ctl.robo.activo = false;
		interfazDuelo.contarMazos(0, 0, 0);
		v.pantalla = VD::DUELO;
		v.t = 0.0f;
		v.ganador = -1;
		v.subeNivel = false;
		v.turnoRival = v.pensando = false;
		v.sorteo = true;
		v.sorteoEtapa = VD::SORTEO_ELEGIR;
		v.sorteoOpcion = 0;
		v.sorteoManoJ = v.sorteoManoR = v.sorteoGanador = v.sorteoEmpieza = -1;
		v.sorteoT = 0.0f;
		ctl.bloqueado = true;
		guardarRevancha();   // rama classic-resultado
		fprintf(stderr, "[sorteo] piedra, papel o tijera contra %s\n", mazoRivalIA.rival->nombreEs.c_str());
	}

	static const char* nombreMano(int m) { return m == 0 ? "Piedra" : m == 1 ? "Papel" : "Tijera"; }
	uint32_t azarSorteo()
	{
		semillaRival = semillaRival * 1664525u + 1013904223u;
		return (semillaRival >> 8) ^ (uint32_t)std::chrono::steady_clock::now().time_since_epoch().count();
	}
	// el rival que gana el sorteo decide con su personalidad (los agresivos casi siempre empiezan; los pacientes a veces
	// prefieren ir segundos) y algo de azar: no siempre lo mismo
	int decisionRival()
	{
		const ia::Personalidad p = mazoRivalIA.rival ? mazoRivalIA.rival->personalidad() : ia::PERSONALIDADES[0];
		const float primero = std::max(0.2f, std::min(0.95f, 0.45f + 0.5f * p.agresividad - 0.3f * p.paciencia));
		return (float)(azarSorteo() & 0xFFFFu) / 65536.0f < primero ? 1 : 0;
	}
	// el sorteo con el mando (devuelve true: se queda con el mando)
	bool avanzarSorteo(uint32_t p)
	{
		VD& v = ctl.vista;
		if (!v.sorteo) return false;
		if (v.sorteoEtapa == VD::SORTEO_ELEGIR) {
			if (p & (control::B_IZQ | control::B_ARRIBA)) v.sorteoOpcion = (v.sorteoOpcion + 2) % 3;
			if (p & (control::B_DER | control::B_ABAJO)) v.sorteoOpcion = (v.sorteoOpcion + 1) % 3;
			if (p & control::B_DIRECCIONES) sonido.sfx("sfx_menu_cursor", 0.7f);
			if ((p & control::B_X) && v.sorteoT > 0.3f) {
				sonido.sfx("sfx_menu_confirmar");
				v.sorteoManoJ = v.sorteoOpcion;
				v.sorteoManoR = (int)((azarSorteo() >> 3) % 3u);
				const int d = (v.sorteoManoJ - v.sorteoManoR + 3) % 3;   // 1: gana el jugador, 2: gana el rival, 0: empate
				v.sorteoGanador = d == 0 ? -1 : d == 1 ? 0 : 1;
				v.sorteoEtapa = VD::SORTEO_MOSTRAR;
				v.sorteoT = 0.0f;
				fprintf(stderr, "[sorteo] jugador %s, rival %s: %s\n", nombreMano(v.sorteoManoJ), nombreMano(v.sorteoManoR),
				        v.sorteoGanador < 0 ? "empate" : v.sorteoGanador == 0 ? "gana el jugador" : "gana el rival");
			}
			return true;
		}
		if (v.sorteoEtapa == VD::SORTEO_MOSTRAR) {
			if (v.sorteoT < 1.8f && !((p & control::B_X) && v.sorteoT > 0.6f)) return true;
			v.sorteoT = 0.0f;
			if (v.sorteoGanador < 0) {   // empate: otra vez
				v.sorteoEtapa = VD::SORTEO_ELEGIR;
				v.sorteoManoJ = v.sorteoManoR = -1;
			} else if (v.sorteoGanador == 0) {
				v.sorteoEtapa = VD::SORTEO_ORDEN;
				v.sorteoOpcion = 0;
			} else {
				v.sorteoEmpieza = decisionRival();
				v.sorteoEtapa = VD::SORTEO_RIVAL_DECIDE;
				fprintf(stderr, "[sorteo] el rival decide ir %s\n", v.sorteoEmpieza == 1 ? "primero" : "segundo");
			}
			return true;
		}
		if (v.sorteoEtapa == VD::SORTEO_ORDEN) {
			if (p & control::B_DIRECCIONES) {
				v.sorteoOpcion = 1 - v.sorteoOpcion;
				sonido.sfx("sfx_menu_cursor", 0.7f);
			}
			if ((p & control::B_X) && v.sorteoT > 0.3f) {
				sonido.sfx("sfx_menu_confirmar");
				v.sorteoEmpieza = v.sorteoOpcion == 0 ? 0 : 1;
				fprintf(stderr, "[sorteo] el jugador decide ir %s\n", v.sorteoEmpieza == 0 ? "primero" : "segundo");
				v.sorteo = false;
				arrancarDueloIA(v.sorteoEmpieza);
			}
			return true;
		}
		// el rival decidio: se lee y sigue (X o a los 2.4 s)
		if (v.sorteoT > 2.4f || ((p & control::B_X) && v.sorteoT > 0.6f)) {
			v.sorteo = false;
			arrancarDueloIA(v.sorteoEmpieza);
		}
		return true;
	}

	void arrancarDueloIA(int empieza)
	{
		repartir();   // (mazo_duelo.h: el mazo armado si es valido; si no, el inicial)
		const uint32_t semilla = semillaRival ^ (uint32_t)std::chrono::steady_clock::now().time_since_epoch().count();
		dueloIA = ia::Duelo(&baseIA);
		dueloIA.empezar(mazoDuelo.principal, mazoDuelo.fusion, mazoRivalIA.principal, mazoRivalIA.fusion, semilla, empieza);
		const ia::Rival& r = *mazoRivalIA.rival;
		// rama classic-historia: Modo Desafio (personajes, tras el final): el rival juega al nivel maximo
		const bool desafio = historia_dialogos::desafioActivo() && historia_dialogos::desafioDisponible() && ctl.vista.modo == VD::HISTORIA && !ctl.vista.personaje.empty();
		turnoIA = ia::Turno(&dueloIA, 1, r.personalidad(), desafio ? ia::NIVEL_MAX : r.nivel, semilla);
		// rama classic-variedad: el plan de este duelo, lo que recuerda del jugador y la dificultad adaptable
		turnoIA.cerebro.usarPlan(mazoRivalIA.plan, memoriaIA.jugador, memoriaIA.dificultad(), memoriaIA.factorRuido());
		// rama classic-ia-humana (ia_humana.h): imagina jugadas y piensa como una persona (Opciones: IA Clasica / Humana)
		turnoIA.usarHumana(memoriaIA.humana, r.clave, semilla);
		variedadPendiente = true;
		fprintf(stderr, "[variedad] plan %s, dificultad %+.0f (ruido x%.2f), %d cartas\n", mazoRivalIA.plan >= 0 ? ia::PLANES[mazoRivalIA.plan].clave : "(sin plan)",
		        memoriaIA.dificultad(), memoriaIA.factorRuido(), (int)mazoRivalIA.principal.size());
		sucesosVistos = 0;
		mesaPendiente = true;
		esperaEfecto = 0.0f;
		reiniciarResumen();   // rama classic-resultado (y la velocidad de animaciones)
		estadoCampo = CAMPO_VACIO;   // (rama classic-efectos: sin el bosque del duelo anterior)
		entornosRival.soltarCampo();   // (rama classic-campos: ni el fondo de la carta de campo anterior)
		interfazDuelo.cambiarLP(false, ia::LP_INICIAL);
		interfazDuelo.cambiarLP(true, ia::LP_INICIAL);
		if (desafio) {   // (y el jugador empieza con menos LP)
			dueloIA.l[0].lp = historia_dialogos::LP_DESAFIO;
			interfazDuelo.cambiarLP(false, historia_dialogos::LP_DESAFIO);
			fprintf(stderr, "[historia] duelo de desafio: %d LP, rival al nivel %d\n", historia_dialogos::LP_DESAFIO, ia::NIVEL_MAX);
		}
		ctl.preparar({}, {}, {});
		ctl.vista.pantalla = VD::DUELO;
		ctl.vista.t = 0.0f;
		ctl.vista.ganador = -1;
		ctl.vista.subeNivel = false;
		// rama classic-duelo-arreglos: las manos empiezan vacias y se reparten carta por carta desde el mazo (5 y 5,
		// alternadas); el robo del primer turno sale despues, con su suceso
		ocultas[0] = (int)dueloIA.l[0].mano.size();
		ocultas[1] = (int)dueloIA.l[1].mano.size();
		reparto = std::min(ocultas[0], ia::MANO_INICIAL) + std::min(ocultas[1], ia::MANO_INICIAL);
		repartoLado = dueloIA.activo;
		ctl.robo.activo = false;
		sincronizarMesaDuelo();
		sonido.sfx("sfx_duelo_inicio");
		musicaActual.clear();
		musicaDePantalla();
		ctl.aviso = dueloIA.activo == 0 ? "Empiezas tú" : "Empieza el rival";
		ctl.tAviso = 0.0f;
		fprintf(stderr, "[duelo] contra %s (nivel %d, %s, variante %d): empieza %s\n", r.nombreEs.c_str(), r.nivel, r.clavePersonalidad.c_str(), mazoRivalIA.variante,
		        dueloIA.activo == 0 ? "el jugador" : "el rival");
	}

	// ---- jugadas
	bool jugarJugador(const ia::Jugada& j)
	{
		if (ctl.vista.pantalla != VD::DUELO || dueloIA.activo != 0 || dueloIA.ganador >= 0) return false;
		const ia::Fase antes = dueloIA.fase;
		const bool ok = dueloIA.aplicar(j);
		if (ok) sonidoJugada(j);
		// (rama classic-arreglos: siempre al klog, para validar en la consola)
		fprintf(stderr, "[duelo] jugador: %s %s (turno %d, fase %s -> %s%s)\n", ia::NOMBRE_JUGADA[j.tipo], ok ? "ok" : "rechazada", dueloIA.turno, NOMBRE_FASE_IA[antes],
		        NOMBRE_FASE_IA[dueloIA.fase], ok ? "" : dueloIA.ocupado() ? ", el duelo esta ocupado: cadena/pregunta/combate" : "");
		return ok;
	}

	static constexpr const char* NOMBRE_FASE_IA[] = { "ROBO", "PRINCIPAL 1", "BATALLA", "PRINCIPAL 2", "FINAL" };

	// rama classic-arreglos: cada boton (no direcciones) que llega, con el estado, al klog ([boton] ...)
	void registrarBotones(uint32_t recien)
	{
		static const char* const N[] = { "X", "CIRCULO", "TRIANGULO", "CUADRADO", "", "", "", "", "L1", "R1", "R3", "OPTIONS" };
		const uint32_t b = recien & ~control::B_DIRECCIONES;
		for (int k = 0; k < 12; k++) {
			if (!(b & (1u << k))) continue;
			fprintf(stderr, "[boton] %s: pantalla %d, estado %s, bloqueado %d, pausa %d, ajustes %d, turno %d de %s, fase %s\n", N[k], ctl.vista.pantalla,
			        control::NOMBRE_ESTADO[ctl.estado], (int)ctl.bloqueado, (int)pausa.abierta, (int)ajustesAbiertos(), dueloIA.turno,
			        dueloIA.activo == 0 ? "jugador" : "rival", NOMBRE_FASE_IA[dueloIA.fase]);
		}
	}

	static int faseInterfaz(ia::Fase f)
	{
		static const int M[] = { 0, 2, 3, 4, 5 };
		return M[(int)f];
	}
	static int zonaMesaIA(int lado, int i, bool magia = false)
	{
		if (i < 0 || i >= ia::ZONAS) return -1;
		return control::zonaPorId(std::string(lado ? "rival_" : "jugador_") + (magia ? "magia_" : "monstruo_") + std::to_string(i));
	}

	// la mesa (control.h) queda igual al duelo
	void sincronizarMesaDuelo()
	{
		const ia::Lado& j = dueloIA.l[0];
		const ia::Lado& r = dueloIA.l[1];
		ctl.sincronizarLado(false, j);
		ctl.sincronizarLado(true, r);
		ctl.manoJ = j.mano;
		ocultas[0] = std::max(0, std::min(ocultas[0], (int)j.mano.size()));
		ocultas[1] = std::max(0, std::min(ocultas[1], (int)r.mano.size()));
		ctl.manoJ.resize(j.mano.size() - (size_t)ocultas[0]);
		ctl.manoR = (int)r.mano.size() - ocultas[1];
		const int fj = control::zonaPorId("jugador_fusion"), fr = control::zonaPorId("rival_fusion");
		if (fj >= 0) {
			ctl.zonas[fj] = control::Celda{};
			if (!j.extra.empty()) ctl.zonas[fj].carta = j.extra[0];
		}
		if (fr >= 0) {
			ctl.zonas[fr] = control::Celda{};
			if (!r.extra.empty()) {
				ctl.zonas[fr].carta = "?";   // (la del rival, boca abajo)
				ctl.zonas[fr].bocaAbajo = ctl.zonas[fr].antesBocaAbajo = true;
			}
		}
		// rama classic-efectos: la zona de magia de campo de cada lado
		for (int lado = 0; lado < 2; lado++) {
			const int zc = control::zonaPorId(lado ? "rival_campo" : "jugador_campo");
			const ia::MagiaTrampa& mc = dueloIA.l[(size_t)lado].campo;
			if (zc < 0) continue;
			control::Celda& c = ctl.zonas[zc];
			if (c.carta != mc.id) {
				c = control::Celda{};
				c.carta = mc.id;
				c.bocaAbajo = c.antesBocaAbajo = mc.bocaAbajo;
				c.tLlega = 0.0f;
			} else if (c.bocaAbajo != mc.bocaAbajo) {
				c.antesBocaAbajo = c.bocaAbajo;
				c.bocaAbajo = mc.bocaAbajo;
				c.tVolteo = 0.0f;
			}
		}
		ctl.fase = faseVisible(faseInterfaz(dueloIA.fase));   // (rama classic-resultado: la Fase de Espera se ve)
		// (las ocultas siguen en el mazo de la mesa hasta que salen volando)
		interfazDuelo.contarMazos((int)j.mazo.size() + ocultas[0], (int)r.mazo.size() + ocultas[1], (int)j.extra.size());
		mesaPendiente = false;
	}

	// los sucesos nuevos del duelo, en orden: destellos, cartel de fase, avisos de turno y los ataques animados
	void presentarSucesos()
	{
		const auto& su = dueloIA.sucesos;
		// rama classic-duelo-arreglos: el reparto inicial, carta por carta (alternando), antes que nada
		if (reparto > 0) {
			if (ctl.robo.activo) return;
			for (int intento = 0; intento < 2; intento++) {
				const int lado = repartoLado;
				repartoLado = 1 - repartoLado;
				const int n = (int)dueloIA.l[(size_t)lado].mano.size();
				if (ocultas[lado] <= 0 || n - ocultas[lado] >= ia::MANO_INICIAL) continue;
				ocultas[lado]--;
				reparto--;
				sincronizarMesaDuelo();
				const int k = n - ocultas[lado] - 1;
				ctl.robarAnimado(lado == 1, lado == 0 ? dueloIA.l[0].mano[(size_t)k] : std::string(), k);
				if (lado == 1) sonido.sfx("sfx_robar", 0.7f, 0.3f);   // (la del jugador suena por audio.h: su mano crece)
				return;
			}
			reparto = 0;   // (no quedaba nada por repartir)
		}
		while (sucesosVistos < su.size() && !hayAtaque() && !ctl.vuelo.activo && !ctl.robo.activo && esperaEfecto <= 0.0f) {
			const ia::Suceso s = su[sucesosVistos];
			// rama classic-efectos: S_ATAQUE es la declaracion (luego la ventana de respuesta); el combate es S_COMBATE
			if (s.tipo == ia::S_COMBATE || (s.tipo == ia::S_ATAQUE && !dueloIA.efectos)) {
				if (mesaPendiente) sincronizarMesaDuelo();   // (no deberia pasar: cada jugada se presenta antes de la siguiente)
				bool muereA = false, muereO = false;
				int danno = 0;
				bool alRival = s.lado == 0;
				size_t k = sucesosVistos + 1;
				for (; k < su.size(); k++) {
					const ia::Suceso& x = su[k];
					if (x.tipo == ia::S_AL_CEMENTERIO && x.zona >= 0 && !x.magia) {
						if (x.lado == s.lado && x.zona == s.zona) muereA = true;
						if (x.lado != s.lado && x.zona == s.zona2) muereO = true;
					} else if (x.tipo == ia::S_AL_CEMENTERIO || x.tipo == ia::S_MODIFICA) {
						// (sus equipos se van con el: siguen siendo parte del combate)
					} else if (x.tipo == ia::S_DANNO) {
						danno = x.valor;
						alRival = x.lado == 1;
					} else if (x.tipo != ia::S_VOLTEA && x.tipo != ia::S_FIN) {
						break;
					}
				}
				const int a = zonaMesaIA(s.lado, s.zona);
				const int o = s.zona2 >= 0 ? zonaMesaIA(1 - s.lado, s.zona2) : zonaMesaIA(1 - s.lado, 2);   // directo: hacia el centro
				animarAtaqueDuelo(a, o, danno, alRival, muereA, muereO);
				sucesosVistos = k;
				mesaPendiente = true;
				break;
			}
			switch (s.tipo) {
			case ia::S_ROBO: {   // rama classic-duelo-arreglos: la carta sale del mazo y va a la mano
				const int lado = s.lado == 1 ? 1 : 0;
				if (ocultas[lado] > 0) ocultas[lado]--;
				sincronizarMesaDuelo();
				int k = lado == 0 ? -1 : ctl.manoR - 1;
				for (int i = (int)ctl.manoJ.size() - 1; lado == 0 && i >= 0 && k < 0; i--)   // (donde quedo; si ya no esta, no se anima)
					if (ctl.manoJ[(size_t)i] == s.carta) k = i;
				if (k >= 0) {
					ctl.robarAnimado(lado == 1, lado == 0 ? s.carta : std::string(), k);
					if (lado == 1) sonido.sfx("sfx_robar", 0.7f, 0.3f);   // (la del jugador suena por audio.h: su mano crece)
				}
				sucesosVistos++;
				return;   // (el siguiente suceso, cuando llegue la carta)
			}
			case ia::S_INVOCA:
			case ia::S_VOLTEA: {
				const int z = zonaMesaIA(s.lado, s.zona);
				if (z >= 0) tInvocada[z] = 0.0f;   // destello de aparicion
				if (s.tipo == ia::S_INVOCA) esperaEfecto = std::max(esperaEfecto, empezarEntradaClave(s.carta, s.lado, z));   // rama classic-cartas-clave (entrada_clave_juego.h)
				// el rival llega por sincronizarLado (sin evento): su sonido aca; el del jugador sale de EV_INVOCAR
				if (s.lado == 1 && z >= 0) sonido.sfx(s.tipo == ia::S_INVOCA ? "sfx_invocar" : "sfx_voltear", 1.0f, control::ZONAS_MESA[z].x / 12.0f);
				break;
			}
			case ia::S_POSICION:
				if (s.lado == 1) sonido.sfx("sfx_posicion");
				break;
			case ia::S_ACTIVA_MT:       // rama classic-fx-md: la animacion de la magia/trampa (efectos_cartas.h)
			case ia::S_ACTIVA_EFECTO: { // rama classic-efectos: la carta boca arriba en su zona, su animacion, su sonido y la cadena
				const bool campo = s.tipo == ia::S_ACTIVA_MT && s.zona == ia::ZONA_CAMPO;
				const int z = campo ? control::zonaPorId(s.lado ? "rival_campo" : "jugador_campo") : zonaMesaIA(s.lado, s.zona, s.tipo == ia::S_ACTIVA_MT);
				if (z >= 0) {
					control::Celda& c = ctl.zonas[z];
					if (s.tipo == ia::S_ACTIVA_MT) {
						if (c.carta != s.carta) {   // (de la mano: aparece en su zona)
							c = control::Celda{};
							c.carta = s.carta;
							c.tLlega = 0.0f;
							if (s.lado == 0) {
								auto it = std::find(ctl.manoJ.begin(), ctl.manoJ.end(), s.carta);
								if (it != ctl.manoJ.end()) ctl.manoJ.erase(it);
							} else if (ctl.manoR > 0) {
								ctl.manoR--;
							}
						} else if (c.bocaAbajo) {   // (colocada: se voltea)
							c.antesBocaAbajo = true;
							c.bocaAbajo = false;
							c.tVolteo = 0.0f;
						}
						c.tActiva = 0.0f;
						activarEfectoCarta(s.carta, z);
						if (campo && campos::esCampo(s.carta)) activarCampo(s.carta, s.lado);   // (rama classic-campos: cualquier carta de campo, de los dos lados, cambia el escenario)
						esperaEfecto = std::min(efc::duracion(efectosCartas.receta(s.carta)), 1.8f);
					} else {
						tInvocada[z] = 0.0f;   // (efecto de monstruo: el destello)
						esperaEfecto = 0.9f;
					}
				}
				if (dueloIA.efectos) {
					sonido.sfx("sfx_activar", 1.0f, z >= 0 ? control::ZONAS_MESA[z].x / 12.0f : 0.0f);
					const std::string nom = interfazDuelo.nombreCarta(s.carta);
					ctl.aviso = s.valor >= 2 ? std::string(idioma::tr("Cadena")) + " " + std::to_string(s.valor) + ": " + nom
					            : (s.tipo == ia::S_ACTIVA_EFECTO ? std::string(ia::textoEfecto("ef.efecto", idioma::ingles())) + " " + nom : nom);
					ctl.tAviso = 0.0f;
				}
				break;
			}
			case ia::S_AL_CEMENTERIO:   // rama classic-efectos: el escenario de la carta de campo se retira con ella
				if (s.magia && s.zona == ia::ZONA_CAMPO && campos::esCampo(s.carta)) destruirCampo(s.carta, s.lado);
				break;
			case ia::S_RESUELVE:   // rama classic-efectos: lo que hizo el eslabon se ve antes del siguiente
				sincronizarMesaDuelo();
				interfazDuelo.cambiarLP(false, lpTras(0, sucesosVistos));   // (rama classic-probador: los de ese momento)
				interfazDuelo.cambiarLP(true, lpTras(1, sucesosVistos));
				esperaEfecto = 0.35f;
				break;
			case ia::S_NEGADO:
			case ia::S_ATAQUE_NEGADO:
				ctl.aviso = std::string(ia::textoEfecto(s.tipo == ia::S_NEGADO ? "ef.negado" : "ef.ataque_negado", idioma::ingles())) +
				            (s.carta.empty() ? std::string() : ": " + interfazDuelo.nombreCarta(s.carta));
				ctl.tAviso = 0.0f;
				esperaEfecto = 0.6f;
				break;
			case ia::S_LP:
				interfazDuelo.cambiarLP(s.lado == 1, lpTras(s.lado, sucesosVistos));
				break;
			case ia::S_CONTROL:
				ctl.aviso = std::string(ia::textoEfecto("ef.control", idioma::ingles())) + " " + interfazDuelo.nombreCarta(s.carta);
				ctl.tAviso = 0.0f;
				sincronizarMesaDuelo();
				esperaEfecto = 0.5f;
				break;
			case ia::S_FASE:
				mostrarFase(s.valor);   // (rama classic-resultado: con la Fase de Espera antes de la principal 1)
				fprintf(stderr, "[fase] turno %d (%s): %s\n", dueloIA.turno, s.lado == 0 ? "jugador" : "rival", NOMBRE_FASE_IA[std::max(0, std::min(s.valor, 4))]);
				break;
			case ia::S_TURNO:
				fprintf(stderr, "[turno] %d: %s (LP %d / %d)\n", s.valor, s.lado == 0 ? "jugador" : "rival", dueloIA.l[0].lp, dueloIA.l[1].lp);
				ctl.aviso = s.lado == 0 ? "Tu turno" : "Turno del rival";
				ctl.tAviso = 0.0f;
				if (s.lado == 1 && s.valor > 1) sonido.sfx("sfx_turno_rival", 0.9f, 0.0f, 0.3f);
				break;
			case ia::S_REVELA:   // rama classic-efectos: una carta oculta se muestra (A Man with Wdjat, The Stern Mystic)
				ctl.aviso = std::string(ia::textoEfecto("ef.revelada", idioma::ingles())) + ": " + interfazDuelo.nombreCarta(s.carta);
				ctl.tAviso = 0.0f;
				esperaEfecto = 0.8f;
				break;
			case ia::S_DANNO:   // (fuera de un ataque: danno de efectos y pagos de LP)
				interfazDuelo.cambiarLP(s.lado == 1, lpTras(s.lado, sucesosVistos));
				break;
			default: break;
			}
			sucesosVistos++;
			mesaPendiente = true;
		}
		if (mesaPendiente && !hayAtaque() && !ctl.vuelo.activo && !ctl.robo.activo) sincronizarMesaDuelo();
		// rama classic-probador: con todo presentado, los LP mostrados son los del duelo (los que cambio un efecto sin
		// suceso, o un combate con dos dannos, como Reflect Bounder, que el ataque animado no cuenta)
		if (sucesosVistos >= su.size() && !hayAtaque())
			for (int lado = 0; lado < 2; lado++)
				if (interfazDuelo.lp(lado == 1) != std::max(0, dueloIA.l[(size_t)lado].lp)) interfazDuelo.cambiarLP(lado == 1, dueloIA.l[(size_t)lado].lp);
	}

	// rama classic-probador: los LP de un lado justo despues del suceso k, cuando los que siguen todavia no se
	// presentaron: los del duelo menos lo que cambiaron despues (S_DANNO, S_LP). Antes S_RESUELVE ponia los LP finales
	// y, si en la misma jugada seguia un combate (responder a un ataque con una trampa que no lo para), el ataque
	// animado restaba su danno otra vez: el jugador veia LP de menos (8000 - 2 x 1200 tras un ataque directo de 1200)
	int lpTras(int lado, size_t k) const
	{
		int lp = dueloIA.l[(size_t)lado].lp;
		const auto& su = dueloIA.sucesos;
		for (size_t j = k + 1; j < su.size(); j++) {
			if (su[j].lado != lado) continue;
			if (su[j].tipo == ia::S_DANNO) lp += su[j].valor;
			else if (su[j].tipo == ia::S_LP) lp -= su[j].valor;
		}
		return std::max(0, lp);
	}

	void animarAtaqueDuelo(int a, int o, int danno, bool alRival, bool mA, bool mO)
	{
		if (a < 0 || o < 0) return;
		zonaAtacante = a;
		zonaObjetivo = o;
		danio = danno;
		danioAlRival = alRival;
		muereA = mA;
		muereO = mO;
		tAtaque = 0.0f;
		ataqueDemo = false;
		ataquesVistos++;
	}

	// cada cuadro (despues del mando): presenta, deja jugar al rival y termina el duelo
	void avanzarDueloIA(float dt)
	{
		VD& v = ctl.vista;
		dt = velocidadCuadro(dt);   // rama classic-resultado: velocidad de animaciones y R2 (velocidad_juego.h)
#if defined(PS5_HOST_REFERENCE)
		pruebaResultado(dt);
#endif
		v.t += dt;
		if (v.pantalla != VD::DUELO || v.sorteo) {   // (rama classic-duelo-arreglos: en el sorteo no hay duelo todavia)
			if (v.sorteo) v.sorteoT += dt;
			ctl.bloqueado = true;
			return;
		}
		esperaEfecto = std::max(0.0f, esperaEfecto - dt);
		avanzarEntradaClave(dt);   // rama classic-cartas-clave: la entrada de una carta clave (entrada_clave_juego.h)
		presentarSucesos();
		const bool alDia = sucesosVistos >= dueloIA.sucesos.size() && !hayAtaque() && !ctl.vuelo.activo && !ctl.robo.activo && reparto <= 0 && !mesaPendiente &&
		                   esperaEfecto <= 0.0f;
		// rama classic-efectos: decide el que tiene la pregunta del duelo (responder o elegir) o el del turno
		ctl.bloqueado = dueloIA.decide() != 0 || dueloIA.ganador >= 0 || !alDia;
		// rama classic-arreglos: por que esta bloqueado el jugador (solo cuando cambia), al klog
		{
			char por[160];
			snprintf(por, sizeof(por), "%s%s%s%s%s%s%s%s", ctl.robo.activo || reparto > 0 ? " robando" : "", dueloIA.decide() != 0 ? " decide-el-rival" : "",
			         dueloIA.ganador >= 0 ? " fin" : "",
			         sucesosVistos < dueloIA.sucesos.size() ? " sucesos-sin-mostrar" : "", hayAtaque() ? " ataque-animado" : "", ctl.vuelo.activo ? " carta-volando" : "",
			         mesaPendiente ? " mesa-pendiente" : "", esperaEfecto > 0.0f ? " animacion-efecto" : "");
			if (por != bloqueoAntes) {
				fprintf(stderr, "[bloqueo] turno %d, fase %s: %s\n", dueloIA.turno, NOMBRE_FASE_IA[dueloIA.fase], por[0] ? por + 1 : "libre (juega el jugador)");
				bloqueoAntes = por;
			}
		}
		seguirResultado(alDia, dt);   // rama classic-resultado: resumen, carta clave y Fase de Espera
		v.turnoRival = dueloIA.activo == 1 && dueloIA.ganador < 0;
		v.pensando = dueloIA.decide() == 1 && dueloIA.ganador < 0 && turnoIA.pensando();
		v.turno = dueloIA.turno;
		if (ctl.bloqueado && ctl.estado != control::NAVEGAR && ctl.estado != control::DETALLE && ctl.estado != control::VOLANDO && dueloIA.decide() != 0) {
			ctl.estado = control::NAVEGAR;   // (por si quedo un menu abierto al pasar el turno)
			ctl.enPregunta = ctl.armando = ctl.eleccionDuelo = false;
		}
		if (!alDia) return;
		if (dueloIA.ganador >= 0) {
			terminarDueloIA();
			return;
		}
		// rama classic-ciclos: Opciones > "Preguntar antes de activar". Siempre: el motor ofrece al jugador cada ventana;
		// Nunca: el duelo no se detiene a preguntarle (pasa solo; las elecciones al resolver si se preguntan); Auto: pregunta
		// solo si tiene algo que activar
		dueloIA.preguntarSiempre[0] = opciones::preguntarActivacion() == opciones::PREGUNTAR_SIEMPRE;
		if (opciones::preguntarActivacion() == opciones::PREGUNTAR_NUNCA && dueloIA.ef.preg.tipo == ia::P_RESPUESTA && dueloIA.ef.preg.lado == 0) {
			ia::Jugada pasa;
			pasa.tipo = ia::J_PASAR;
			dueloIA.aplicar(pasa);
			if (ctl.enPregunta) {
				ctl.estado = control::NAVEGAR;
				ctl.enPregunta = ctl.armando = ctl.eleccionDuelo = false;
			}
			fprintf(stderr, "[duelo] preguntar antes de activar: nunca (el jugador pasa solo)\n");
			return;
		}
		musicaDePantalla();   // (pasa a la tensa con LP bajos)
		if (dueloIA.decide() == 1 && turnoIA.avanzar(dt)) {
			sonidoJugada(turnoIA.ultima);
			// la frase del rival (si la jugada tiene una)
			if (const char* fr = turnoIA.ultima.frase) {
				const ia::Rival* r = mazoRivalIA.rival;
				ctl.aviso = (r ? r->nombre(idioma::ingles()) + ": " : std::string()) + ia::textoIA(fr, idioma::ingles());
				ctl.tAviso = 0.0f;
			}
		}
	}

	// rama classic-variedad (ia_plan.h): al terminar (o abandonar) un duelo, el rival recuerda el plan y el mazo que
	// uso (para no repetirlos), lo que vio hacer al jugador en la mesa y la racha (dificultad adaptable)
	bool variedadPendiente{ false };
	void registrarVariedad(int ganador)
	{
		if (!variedadPendiente || !mazoRivalIA.rival) return;
		variedadPendiente = false;
		ia::EstadoPlan& ep = turnoIA.cerebro.plan;
		ep.obs.mirar(dueloIA, 0);
		memoriaIA.registrar(mazoRivalIA.rival->clave, ep.planesDuelo, mazoRivalIA.principal, ep.obs, ganador);
		memoriaIA.guardar();
		fprintf(stderr, "[variedad] %s: planes del duelo %d, racha del jugador %d (dificultad %+.0f%s), lectura trampas %.2f faroles %.2f ataques %.2f muros %.2f\n",
		        mazoRivalIA.rival->clave.c_str(), (int)ep.planesDuelo.size(), memoriaIA.racha, memoriaIA.dificultad(), memoriaIA.adaptable ? "" : ", apagada",
		        memoriaIA.jugador.trampas, memoriaIA.jugador.faroles, memoriaIA.jugador.ataques, memoriaIA.jugador.muros);
	}

	void terminarDueloIA()
	{
		VD& v = ctl.vista;
		v.ganador = dueloIA.ganador;
		v.subeNivel = false;
		v.personajeNuevo = v.sagaNueva = false;
		v.firmaNuevas = 0;
		const bool deHistoria = v.modo == VD::HISTORIA && !v.personaje.empty();
		if (v.ganador == 0 && deHistoria) {   // rama classic-personajes: vencido en el modo historia
			v.victorias++;
			const int antes = historia.sagaActual();
			if (historia.vencer(v.personaje)) {
				v.personajeNuevo = true;
				v.idx = -1;   // (al volver: el siguiente sin vencer)
				if (const personajes::Personaje* x = historia.buscar(v.personaje)) {
					for (const std::string& id : x->firma) {   // (las que aun no tenia por otro duenno)
						const mazo::Carta* k = coleccion.carta(id);
						v.firmaNuevas += k && k->bloqueada ? 1 : 0;
					}
				}
				coleccion.desbloquear(historia.vencidos);
				v.sagaNueva = historia.sagaActual() > antes;
				if (historia.completa() && !historia.finalVisto) finalPendiente = true;   // rama classic-historia: el final
				if (v.sagaNueva) {
					v.saga = historia.sagaActual();
					v.sagaEs = personajes::SAGAS[v.saga].es;
					v.sagaEn = personajes::SAGAS[v.saga].en;
				}
			}
		} else if (v.ganador == 0) {
			v.victorias++;
			if (v.nivel >= v.nivelMax && v.nivelMax < ia::NIVEL_MAX) {
				v.nivelMax++;
				v.subeNivel = true;
			}
		} else if (v.ganador == 1) {
			v.derrotas++;
		}
		guardarProgreso();
		registrarVariedad(v.ganador);   // rama classic-variedad: plan, mazo, lectura del jugador y racha
		v.pantalla = VD::RESULTADO;
		v.t = 0.0f;
		v.turnoRival = v.pensando = false;
		ctl.estado = control::NAVEGAR;
		prepararResultado();   // rama classic-resultado: motivo, resumen, desbloqueos y musica (resultado_juego.h)
		fprintf(stderr, "[duelo] fin: gana %s (LP %d / %d, turno %d)\n", v.ganador == 0 ? "el jugador" : v.ganador == 1 ? "el rival" : "nadie", dueloIA.l[0].lp,
		        dueloIA.l[1].lp, dueloIA.turno);
	}
