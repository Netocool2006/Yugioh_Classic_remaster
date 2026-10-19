	// Yugioh Classic Remaster (rama classic-constructor): se incluye DENTRO de la clase del juego (yugireload.cpp).
	// La coleccion (mazo.h), el mazo del jugador (guardado en mazo::rutaMazo), el reparto del duelo y la pantalla
	// "Construir mazo" (constructor.h; la dibuja interfaz.h). Ganchos en el juego: prepararMazos() al preparar el
	// mando (la mano interactiva = 5 robadas del mazo, la fusion a su zona), robarDelMazo() en la fase de robo y
	// avanzarConstructor() al empezar cada cuadro (panel tactil, tecla M en el PC o Ajustes > "Construir mazo").
	mazo::Coleccion coleccion;
	mazo::Mazo mazoJugador;   // el guardado (puede no ser valido: se sigue editando)
	mazo::Mazo mazoDuelo;     // con el que se juega: el guardado si es valido; si no, el inicial
	mazo::Mazo mazoRival;     // aleatorio valido de 40
	std::vector<std::string> pilaJ, pilaR, manoInicialJ;   // lo que queda en cada mazo (se roba de back())
	std::string rutaMazoJ;
	constructor::Constructor constructorMazo;
	control::Repetidor repetidorConstructor;
	bool pedidoConstructor{ false };
	bool l2Antes{ false }, r2Antes{ false };
	float stickIniT{ 0.0f };

	void prepararMazos()
	{
		coleccion.leer(getAssetPath() + "ui/");
		rutaMazoJ = mazo::rutaMazo(getAssetPath());
		if (!mazoJugador.cargar(rutaMazoJ, coleccion)) {
			mazoJugador = mazo::Mazo::porDefecto(coleccion);   // primera vez: el mazo inicial, y queda guardado
			mazoJugador.guardar(rutaMazoJ);
		}
		repartir();
		interfazDuelo.conectarConstructor(&constructorMazo);
#if defined(PS5_HOST_REFERENCE)
		if (getenv("YUGI_CONSTRUCTOR")) pedidoConstructor = true;   // PC: arranca en "Construir mazo"
#endif
	}

	// baraja los dos mazos y roba la mano inicial del jugador
	void repartir()
	{
		std::string motivo;
		if (mazoJugador.valido(coleccion, &motivo)) {
			mazoDuelo = mazoJugador;
		} else {
			fprintf(stderr, "[mazo] el mazo guardado no es valido (%s): se juega con el mazo inicial\n", motivo.c_str());
			mazoDuelo = mazo::Mazo::porDefecto(coleccion);
		}
		mazo::Azar azar((uint32_t)std::chrono::steady_clock::now().time_since_epoch().count() ^ 0x5bd1e995u);
		pilaJ = mazoDuelo.principal;
		mazo::barajar(pilaJ, azar);
		manoInicialJ.clear();
		for (int k = 0; k < mazo::MANO_INICIAL && !pilaJ.empty(); k++) {
			manoInicialJ.push_back(pilaJ.back());
			pilaJ.pop_back();
		}
		mazoRival = mazo::Mazo::aleatorio(coleccion, azar);
		pilaR = mazoRival.principal;
		mazo::barajar(pilaR, azar);
		for (int k = 0; k < interfaz::NUM_MANO_R && !pilaR.empty(); k++) pilaR.pop_back();   // (la mano del rival: 5 ocultas)
		contarMazos();
		fprintf(stderr, "[mazo] duelo: jugador %d en el mazo + %d en la mano, fusion %d; rival %d en el mazo\n", (int)pilaJ.size(), (int)manoInicialJ.size(),
		        (int)mazoDuelo.fusion.size(), (int)pilaR.size());
	}

	void contarMazos() { interfazDuelo.contarMazos((int)pilaJ.size(), (int)pilaR.size(), (int)mazoDuelo.fusion.size()); }

	// fase de robo: la carta de arriba del mazo a la mano (si cabe)
	void robarDelMazo()
	{
		if (pilaJ.empty()) {
			fprintf(stderr, "[mazo] no quedan cartas para robar\n");
			return;
		}
		if (ctl.robar(pilaJ.back(), interfaz::MAX_MANO_J)) {
			pilaJ.pop_back();
			contarMazos();
		}
	}

	// la zona de fusion del jugador muestra la primera fusion de su mazo
	void ponerFusionEnZona()
	{
		const int z = control::zonaPorId("jugador_fusion");
		if (z < 0) return;
		ctl.zonas[z] = control::Celda{};
		if (!mazoDuelo.fusion.empty()) ctl.zonas[z].carta = mazoDuelo.fusion[0];
	}

	// cada cuadro, despues de leer el mando: abre, maneja y cierra "Construir mazo". Abierto se queda con el mando
	// (el resto del cuadro no ve ningun boton)
	void avanzarConstructor()
	{
		if (!mandoListo) {
			return;
		}
		if (!constructorMazo.abierto) {
			if (pedidoConstructor || ((mando.pressed & PAD_TOUCH_PAD) && !ajustesAbiertos())) {
				pedidoConstructor = false;
				if (coleccion.version != constructor::versionIdioma()) coleccion.leer(getAssetPath() + "ui/");   // (cambio el idioma)
				ui.visible = false;   // (cierra Ajustes si se abrio desde ahi)
				constructorMazo.abrir(coleccion, mazoJugador, rutaMazoJ);
				sonido.sfx("sfx_constructor_abrir");   // rama classic-audio
				musica("musica_constructor");
				repetidorConstructor = control::Repetidor{};
				l2Antes = mando.l2 > 0.55f;
				r2Antes = mando.r2 > 0.55f;
			}
			if (!constructorMazo.abierto) return;
		}
		const float dt = glm::clamp(frameTimer, 0.0f, 0.1f);
		uint32_t extra = teclas;   // (PC: el teclado como botones)
		teclas = 0;
		const uint32_t recien = botonesDe(mando.pressed) | extra;
		uint32_t p = repetidorConstructor.paso(botonesDe(mando.held), recien, mando.left_x, mando.left_y, dt) | (recien & ~control::B_DIRECCIONES);
		// L2/R2 (gatillos): filtro de clase; L3: sin filtros; stick derecho a los lados: la inicial (con repeticion)
		const bool l2 = mando.l2 > 0.55f, r2 = mando.r2 > 0.55f;
		if (l2 && !l2Antes) p |= constructor::C_L2;
		if (r2 && !r2Antes) p |= constructor::C_R2;
		l2Antes = l2;
		r2Antes = r2;
		if (mando.pressed & PAD_L3) p |= constructor::C_L3;
		if (fabsf(mando.right_x) > 0.6f) {
			stickIniT -= dt;
			if (stickIniT <= 0.0f) {
				p |= mando.right_x > 0.0f ? constructor::C_INI_SIG : constructor::C_INI_ANT;
				stickIniT = stickIniT < -50.0f ? 0.38f : 0.16f;
			}
		} else {
			stickIniT = -99.0f;
		}
		const size_t antes = constructorMazo.mazo.principal.size() + constructorMazo.mazo.fusion.size();
		constructorMazo.avanzar(dt, p);
		const size_t despues = constructorMazo.mazo.principal.size() + constructorMazo.mazo.fusion.size();
		if (despues > antes) sonido.sfx("sfx_constructor_anadir");   // rama classic-audio
		else if (despues < antes) sonido.sfx("sfx_constructor_quitar");
		else if (p & control::B_DIRECCIONES) sonido.sfx("sfx_menu_cursor", 0.7f);
		if (!constructorMazo.abierto) {
			// se cerro (circulo): el mazo guardado manda; el duelo se vuelve a repartir con el
			mazoJugador = constructorMazo.mazo;
			repartir();   // (el mazo nuevo vale para el proximo duelo)
			sonido.sfx("sfx_menu_cancelar");
			musicaDePantalla();
			if (ctl.vista.pantalla == control::VistaDuelo::DUELO) sincronizarMesaDuelo();
		}
		// el resto del cuadro no ve el mando
		mando.pressed = mando.held = 0;
		mando.left_x = mando.left_y = mando.right_x = mando.right_y = 0.0f;
		mando.l2 = mando.r2 = 0.0f;
	}
