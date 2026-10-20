	// Maquetas (pruebas sin pantalla): lo que yugireload.cpp incluye dentro de la clase del juego despues de
	// duelo_juego.h y que depende de Vulkan, figuras o pantallas que las pruebas no tienen. Se incluye DENTRO de la
	// clase Juego de prueba_juego.cpp y prueba_certificacion.cpp, despues de duelo_juego.h. Hace lo minimo para que
	// el duelo siga igual: sin cartel de la Fase de Espera, sin entrada de cartas clave y sin pantalla de resultado.
	// resultado_juego.h (rama classic-resultado)
	bool claveSonando{ false };
	int faseVisible(int f) const { return f; }
	void mostrarFase(int faseIA)
	{
		ctl.fase = faseInterfaz((ia::Fase)faseIA);
		ctl.tFase = 0.0f;
	}
	void seguirResultado(bool, float) {}
	void prepararResultado() {}
	void reiniciarResumen() {}
	void guardarRevancha() {}
	bool avanzarResultado(uint32_t p)   // (la pantalla de resultado: X u O vuelven a elegir rival)
	{
		if (p & (control::B_X | control::B_CIRCULO)) irAElegirRival();
		return true;
	}
	void rendirse()
	{
		dueloIA.terminar(1);
		terminarDueloIA();
	}
	// velocidad_juego.h (rama classic-resultado): sin escala
	float velocidadCuadro(float dt) { return dt; }
	// entrada_clave_juego.h (rama classic-cartas-clave): ninguna carta es clave
	float empezarEntradaClave(const std::string&, int, int) { return 0.0f; }
	void avanzarEntradaClave(float) {}
	void pruebaCartaClave(float) {}   // (rama classic-visual-final: YUGI_CARTA_CLAVE, solo PC)
	// menu_principal.h y portada.h (ramas classic-menus y classic-portada): solo cambia de pantalla
	struct { bool enOpciones{ false }; int opcionOpc{ 0 }; } menuP;
	void irAMenuPrincipal() { ctl.vista.pantalla = VD::MENU; }
	bool avanzarMenuPrincipal(uint32_t) { return false; }
	void avanzarOpciones(uint32_t, bool) { menuP.enOpciones = false; }
	void mostrarMenu() {}
	bool avanzarPortada(uint32_t) { return false; }
