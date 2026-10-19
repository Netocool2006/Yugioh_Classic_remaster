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
	uint32_t semillaRival{ 1 };
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
			musica(lpMin <= LP_TENSO ? "musica_duelo_tensa" : (semillaRival & 2u) && sonido.hay("musica_duelo_2") ? "musica_duelo_2" : "musica_duelo");
		} else if (ctl.vista.pantalla == VD::ELEGIR_RIVAL) {
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
	//  - en el duelo: Continuar, Ajustes, Construir mazo, Abandonar duelo (cuenta como derrota) y Salir al menu
	//    principal (la pantalla de elegir rival; el duelo en curso se descarta sin contar)
	//  - en elegir rival / resultado: Continuar, Modo historia o Duelo libre (el otro modo), Ajustes y Construir mazo
	enum AccionPausa { PA_CONTINUAR = 0, PA_AJUSTES, PA_CONSTRUIR, PA_ABANDONAR, PA_MENU_PRINCIPAL, PA_HISTORIA, PA_DUELO_LIBRE };
	static constexpr float OPCIONES_CORTO = 0.8f;   // s (los Ajustes de la plantilla piden 2 s)
	struct Pausa { bool abierta{ false }; int opcion{ 0 }; float abajo{ 0.0f }; } pausa;

	std::vector<int> opcionesPausa() const
	{
		if (ctl.vista.pantalla == VD::DUELO) return { PA_CONTINUAR, PA_AJUSTES, PA_CONSTRUIR, PA_ABANDONAR, PA_MENU_PRINCIPAL };
		std::vector<int> r{ PA_CONTINUAR };
		if (ctl.vista.pantalla == VD::ELEGIR_RIVAL && !historia.lista.empty()) r.push_back(ctl.vista.modo == VD::HISTORIA ? PA_DUELO_LIBRE : PA_HISTORIA);
		if (ctl.vista.pantalla == VD::RESULTADO) r.push_back(PA_MENU_PRINCIPAL);
		r.push_back(PA_AJUSTES);
		r.push_back(PA_CONSTRUIR);
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
		default: return "";
		}
	}
	// lo que dibuja la interfaz (duelo_ui.h: dibujarPausa)
	void mostrarPausa()
	{
		VD& v = ctl.vista;
		v.pausa = pausa.abierta;
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
		if (constructorMazo.abierto || ctl.vista.pantalla == VD::NINGUNA) return false;
		if (!pausa.abierta) {
			if (!corto) return false;
			pausa.abierta = true;
			pausa.opcion = 0;
			mostrarPausa();
			sonido.sfx("sfx_menu_confirmar");
			fprintf(stderr, "[pausa] abierta (pantalla %d, turno %d, fase %d)\n", ctl.vista.pantalla, dueloIA.turno, (int)dueloIA.fase);
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
			cerrarPausa("eleccion");
			switch (a) {
			case PA_AJUSTES: ui.visible = true; break;
			case PA_CONSTRUIR: pedidoConstructor = true; break;
			case PA_ABANDONAR:
				ctl.vista.derrotas++;
				guardarProgreso();
				registrarVariedad(1);
				irAElegirRival();
				break;
			case PA_MENU_PRINCIPAL: irAElegirRival(); break;
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
		if (FILE* f = fopen(rutaProgreso.c_str(), "w")) {
			fprintf(f, "nivel_max %d\nvictorias %d\nderrotas %d\n", ctl.vista.nivelMax, ctl.vista.victorias, ctl.vista.derrotas);
			fclose(f);
		} else {
			fprintf(stderr, "[duelo] no se pudo guardar %s\n", rutaProgreso.c_str());
		}
	}

	// ---- pantallas
	void irAElegirRival()
	{
		ctl.vista.pantalla = VD::ELEGIR_RIVAL;
		ctl.vista.t = 0.0f;
		ctl.vista.turnoRival = ctl.vista.pensando = false;
		ctl.vista.nivel = ctl.vista.nivelMax;
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
		mazoRivalIA = baseIAlista ? baseIA.mazoRival(ctl.vista.nivel, semillaRival) : ia::MazoRival{};
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

	// botones fuera de la mesa (elegir rival, resultado). Devuelve true si la pantalla se quedo con el mando
	bool avanzarPantallaDuelo(uint32_t p)
	{
		VD& v = ctl.vista;
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
					empezarDueloIA();
				} else {
					ctl.aviso = "Vence a todos los de la saga anterior";
					ctl.tAviso = 0.0f;
				}
			}
			return true;
		}
		if (v.pantalla == VD::ELEGIR_RIVAL) {
			const int antes = v.nivel;
			if (p & (control::B_IZQ | control::B_ABAJO)) v.nivel = std::max(1, v.nivel - 1);
			if (p & (control::B_DER | control::B_ARRIBA)) v.nivel = std::min(v.nivelMax, v.nivel + 1);
			if (v.nivel != antes) {
				elegirRival(false);
				sonido.sfx("sfx_menu_cursor");
			} else if (p & (control::B_IZQ | control::B_DER | control::B_ARRIBA | control::B_ABAJO)) {
				sonido.sfx("sfx_error", 0.5f);   // (nivel bloqueado o el primero)
			}
			if (p & control::B_CUADRADO) {
				elegirRival(true);
				sonido.sfx("sfx_menu_cursor");
			}
			if (p & control::B_X) {
				sonido.sfx("sfx_menu_confirmar");
				empezarDueloIA();
			}
			return true;
		}
		if (v.pantalla == VD::RESULTADO) {
			if ((p & control::B_X) && v.t > 0.8f) {
				sonido.sfx("sfx_menu_confirmar");
				irAElegirRival();
			}
			return true;
		}
		return false;
	}

	void empezarDueloIA()
	{
		if (!mazoRivalIA.rival || mazoRivalIA.principal.empty()) {
			ctl.aviso = "Sin rivales (falta rivales.txt)";
			ctl.tAviso = 0.0f;
			return;
		}
		repartir();   // (mazo_duelo.h: el mazo armado si es valido; si no, el inicial)
		const uint32_t semilla = semillaRival ^ (uint32_t)std::chrono::steady_clock::now().time_since_epoch().count();
		dueloIA = ia::Duelo(&baseIA);
		dueloIA.empezar(mazoDuelo.principal, mazoDuelo.fusion, mazoRivalIA.principal, mazoRivalIA.fusion, semilla);
		const ia::Rival& r = *mazoRivalIA.rival;
		turnoIA = ia::Turno(&dueloIA, 1, r.personalidad(), r.nivel, semilla);
		// rama classic-variedad: el plan de este duelo, lo que recuerda del jugador y la dificultad adaptable
		turnoIA.cerebro.usarPlan(mazoRivalIA.plan, memoriaIA.jugador, memoriaIA.dificultad(), memoriaIA.factorRuido());
		variedadPendiente = true;
		fprintf(stderr, "[variedad] plan %s, dificultad %+.0f (ruido x%.2f), %d cartas\n", mazoRivalIA.plan >= 0 ? ia::PLANES[mazoRivalIA.plan].clave : "(sin plan)",
		        memoriaIA.dificultad(), memoriaIA.factorRuido(), (int)mazoRivalIA.principal.size());
		sucesosVistos = 0;
		mesaPendiente = true;
		esperaEfecto = 0.0f;
		estadoCampo = CAMPO_VACIO;   // (rama classic-efectos: sin el bosque del duelo anterior)
		entornosRival.soltarCampo();   // (rama classic-campos: ni el fondo de la carta de campo anterior)
		interfazDuelo.cambiarLP(false, ia::LP_INICIAL);
		interfazDuelo.cambiarLP(true, ia::LP_INICIAL);
		ctl.preparar({}, {}, {});
		ctl.vista.pantalla = VD::DUELO;
		ctl.vista.t = 0.0f;
		ctl.vista.ganador = -1;
		ctl.vista.subeNivel = false;
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
		ctl.fase = faseInterfaz(dueloIA.fase);
		interfazDuelo.contarMazos((int)j.mazo.size(), (int)r.mazo.size(), (int)j.extra.size());
		mesaPendiente = false;
	}

	// los sucesos nuevos del duelo, en orden: destellos, cartel de fase, avisos de turno y los ataques animados
	void presentarSucesos()
	{
		const auto& su = dueloIA.sucesos;
		while (sucesosVistos < su.size() && !hayAtaque() && !ctl.vuelo.activo && esperaEfecto <= 0.0f) {
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
			case ia::S_INVOCA:
			case ia::S_VOLTEA: {
				const int z = zonaMesaIA(s.lado, s.zona);
				if (z >= 0) tInvocada[z] = 0.0f;   // destello de aparicion
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
				interfazDuelo.cambiarLP(false, dueloIA.l[0].lp);
				interfazDuelo.cambiarLP(true, dueloIA.l[1].lp);
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
				interfazDuelo.cambiarLP(s.lado == 1, dueloIA.l[(size_t)s.lado].lp);
				break;
			case ia::S_CONTROL:
				ctl.aviso = std::string(ia::textoEfecto("ef.control", idioma::ingles())) + " " + interfazDuelo.nombreCarta(s.carta);
				ctl.tAviso = 0.0f;
				sincronizarMesaDuelo();
				esperaEfecto = 0.5f;
				break;
			case ia::S_FASE:
				ctl.fase = faseInterfaz((ia::Fase)s.valor);
				ctl.tFase = 0.0f;
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
				interfazDuelo.cambiarLP(s.lado == 1, dueloIA.l[(size_t)s.lado].lp);
				break;
			default: break;
			}
			sucesosVistos++;
			mesaPendiente = true;
		}
		if (mesaPendiente && !hayAtaque() && !ctl.vuelo.activo) sincronizarMesaDuelo();
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
		v.t += dt;
		if (v.pantalla != VD::DUELO) {
			ctl.bloqueado = true;
			return;
		}
		esperaEfecto = std::max(0.0f, esperaEfecto - dt);
		presentarSucesos();
		const bool alDia = sucesosVistos >= dueloIA.sucesos.size() && !hayAtaque() && !ctl.vuelo.activo && !mesaPendiente && esperaEfecto <= 0.0f;
		// rama classic-efectos: decide el que tiene la pregunta del duelo (responder o elegir) o el del turno
		ctl.bloqueado = dueloIA.decide() != 0 || dueloIA.ganador >= 0 || !alDia;
		// rama classic-arreglos: por que esta bloqueado el jugador (solo cuando cambia), al klog
		{
			char por[160];
			snprintf(por, sizeof(por), "%s%s%s%s%s%s%s", dueloIA.decide() != 0 ? " decide-el-rival" : "", dueloIA.ganador >= 0 ? " fin" : "",
			         sucesosVistos < dueloIA.sucesos.size() ? " sucesos-sin-mostrar" : "", hayAtaque() ? " ataque-animado" : "", ctl.vuelo.activo ? " carta-volando" : "",
			         mesaPendiente ? " mesa-pendiente" : "", esperaEfecto > 0.0f ? " animacion-efecto" : "");
			if (por != bloqueoAntes) {
				fprintf(stderr, "[bloqueo] turno %d, fase %s: %s\n", dueloIA.turno, NOMBRE_FASE_IA[dueloIA.fase], por[0] ? por + 1 : "libre (juega el jugador)");
				bloqueoAntes = por;
			}
		}
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
		sonido.pararMusica(0.8f);
		musicaActual.clear();
		sonido.sfx(v.ganador == 0 ? "sfx_victoria" : "sfx_derrota");
		v.pantalla = VD::RESULTADO;
		v.t = 0.0f;
		v.turnoRival = v.pensando = false;
		ctl.estado = control::NAVEGAR;
		fprintf(stderr, "[duelo] fin: gana %s (LP %d / %d, turno %d)\n", v.ganador == 0 ? "el jugador" : v.ganador == 1 ? "el rival" : "nadie", dueloIA.l[0].lp,
		        dueloIA.l[1].lp, dueloIA.turno);
	}
