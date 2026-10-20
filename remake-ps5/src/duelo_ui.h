// Yugioh Classic Remaster (rama classic-integracion): se incluye DENTRO de interfaz::Interfaz (usa sus primitivas).
// Lo que se ve del duelo real fuera de la mesa (control::VistaDuelo, lo llena duelo_juego.h):
//  - ELEGIR_RIVAL: nivel (1..8, hasta el desbloqueado), el rival que toca con su lema y personalidad, y los botones.
//  - DUELO: cartel del turno del rival y "pensando..." mientras la IA decide.
//  - RESULTADO: victoria o derrota, nivel desbloqueado y continuar.
// Todos los textos por idioma.h (la tabla) o por los dos idiomas de rivales.txt / ia::textoIA.

public:
	// nombre visible de una carta en el idioma actual (para textos armados fuera de la interfaz)
	std::string nombreCarta(const std::string& id) const
	{
		const int n = buscarCarta(id);
		return n >= 0 ? cartas[n].vNombre() : id;
	}

private:
	void dibujarPantallaDuelo()
	{
		if (!ctl) return;
		if (historia_dialogos::escena().activa() || tutorial::estado().abierto) return;   // rama classic-historia: la escena o las reglas tapan la pantalla
		const control::VistaDuelo& vd = ctl->vista;
		const bool en = idioma::ingles();
		const float a = suave(0.0f, 0.25f, vd.t);
		auto A = [&](glm::vec4 c) { return glm::vec4(glm::vec3(c), c.a * a); };
		const float cx = (CAMPO_X * s + W) * 0.5f;
		if (vd.pantalla == control::VistaDuelo::DUELO && vd.sorteo) {   // rama classic-duelo-arreglos
			dibujarRetratoRival(vd);
			dibujarSorteo(vd);
			return;
		}
		if (vd.pantalla == control::VistaDuelo::DUELO) {
			dibujarRetratoRival(vd);   // rama classic-personajes (personajes_ui.h)
			if (!vd.turnoRival) return;
			// cartel arriba: TURNO DEL RIVAL y, mientras decide, "pensando..." con tres puntos que laten
			const float w = 520.0f * s, h = 64.0f * s, y = 150.0f * s;
			usar(setAtlas, false);
			caja(cx - w * 0.5f, y, w, h, 18.0f * s, PANEL, glm::vec4(glm::vec3(VIDRIO), 0.8f), CARMESI, h * 0.5f, glm::vec4(1.4f * s, 1.0f, 0.2f, 14.0f * s));
			texto(F_NUMEROS, ia::textoIA("ia.turno_rival", en), cx, y + 30.0f * s, 24.0f * s, BLANCO, CARMESI, glm::vec3(0.4f, 0.0f, 0.6f), 1, 3.0f);
			if (vd.pensando) {
				const float lat = 0.6f + 0.4f * sinf(t * 6.0f);
				texto(F_TITULO, ia::textoIA("ia.pensando", en), cx, y + 54.0f * s, 17.0f * s, glm::vec4(glm::vec3(GRIS), lat), NADA, glm::vec3(0.0f), 1, 1.0f);
			}
			return;
		}
		if (vd.pantalla != control::VistaDuelo::ELEGIR_RIVAL && vd.pantalla != control::VistaDuelo::RESULTADO) return;
		if (vd.pantalla == control::VistaDuelo::RESULTADO && resultado::resumen().activo) {   // rama classic-resultado (resultado_ui.h)
			dibujarResultado(vd);
			return;
		}
		if (vd.pantalla == control::VistaDuelo::ELEGIR_RIVAL && vd.modo == control::VistaDuelo::HISTORIA && vd.sagas > 0) {
			dibujarHistoria(vd, a);   // rama classic-personajes: modo historia (personajes_ui.h)
			return;
		}
		// velo sobre el tablero
		usar(setAtlas, false);
		caja(0.0f, 0.0f, W, H, 0.0f, PANEL, A(glm::vec4(0.01f, 0.015f, 0.03f, 0.82f)), NADA, 0.0f, glm::vec4(0.0f));
		const float pw = 900.0f * s, ph = 560.0f * s;
		const float px = cx - pw * 0.5f, py = H * 0.5f - ph * 0.5f + (1.0f - a) * 20.0f * s;
		const glm::vec4 acento = vd.pantalla == control::VistaDuelo::RESULTADO ? (vd.ganador == 0 ? ORO : CARMESI) : CIAN;
		rect(glm::vec2(cx, H * 0.5f), glm::vec2(pw * 1.3f, ph * 1.3f), 0.0f, 0.0f, PARTICULA, A(glm::vec4(glm::vec3(acento) * 0.12f, 1.0f)), NADA, 0.0f, glm::vec4(0.0f));
		caja(px, py, pw, ph, 30.0f * s, PANEL, A(VIDRIO), A(acento), 22.0f * s, glm::vec4(1.6f * s, 1.1f, 0.12f, 22.0f * s));
		float bx = px + 40.0f * s;
		const float by = py + ph - 40.0f * s;
		auto boton = [&](const std::string& b, const char* txt) {
			bx += iconoBoton(glm::vec2(bx + 14.0f * s, by), b, 14.0f * s, a) + 12.0f * s;
			bx += texto(F_TITULO, idioma::tr(txt), bx, by + 8.0f * s, 21.0f * s, A(BLANCO), NADA, glm::vec3(0.0f), 0, 1.0f) + 34.0f * s;
		};
		if (vd.pantalla == control::VistaDuelo::ELEGIR_RIVAL) {
			texto(F_NUMEROS, idioma::tr("ELIGE A TU RIVAL"), cx, py + 70.0f * s, 38.0f * s, A(BLANCO), A(CIAN), glm::vec3(0.5f, 0.0f, 0.6f), 1, 2.0f);
			// niveles: 8 orbes (los bloqueados apagados), el elegido grande
			const float ox = cx - 3.5f * 70.0f * s, oy = py + 150.0f * s;
			for (int n = 1; n <= vd.niveles; n++) {
				const bool libre = n <= vd.nivelMax, sel = n == vd.nivel;
				const glm::vec2 c(ox + (float)(n - 1) * 70.0f * s, oy);
				const glm::vec4 col = sel ? ORO : libre ? CIAN : glm::vec4(0.3f, 0.34f, 0.42f, 1.0f);
				rect(c, glm::vec2((sel ? 26.0f : 18.0f) * s), 0.0f, 6.0f * s, ORBE, A(col), A(glm::vec4(glm::vec3(col) * 0.4f, 1.0f)), 0.0f, glm::vec4(0.9f, 1.0f, 0.0f, 0.0f));
				texto(F_NUMEROS, std::to_string(n), c.x, c.y + 9.0f * s, (sel ? 24.0f : 18.0f) * s, A(sel ? glm::vec4(0.1f, 0.06f, 0.0f, 1.0f) : glm::vec4(0.05f, 0.08f, 0.12f, 1.0f)),
				      NADA, glm::vec3(0.0f), 1, 0.0f);
			}
			texto(F_TITULO, std::string(ia::textoIA("ia.nivel", en)) + " " + std::to_string(vd.nivel) + " / " + std::to_string(vd.niveles), cx, oy + 62.0f * s, 22.0f * s,
			      A(GRIS), NADA, glm::vec3(0.0f), 1, 2.0f);
			const std::string& nom = en && !vd.rivalEn.empty() ? vd.rivalEn : vd.rivalEs;
			const std::string& lema = en && !vd.lemaEn.empty() ? vd.lemaEn : vd.lemaEs;
			texto(F_NUMEROS, nom.empty() ? std::string(idioma::tr("Sin rivales (falta rivales.txt)")) : nom, cx, oy + 140.0f * s, 46.0f * s, A(ORO), A(glm::vec4(0.5f, 0.3f, 0.05f, 1.0f)),
			      glm::vec3(0.6f, 0.0f, 0.8f), 1, 1.5f);
			if (!lema.empty()) texto(F_TEXTO, "\"" + lema + "\"", cx, oy + 186.0f * s, 22.0f * s, A(BLANCO), NADA, glm::vec3(0.0f), 1, 0.0f);
			if (!vd.personalidad.empty()) {
				const std::string clave = "ia.personalidad." + vd.personalidad;
				texto(F_TITULO, ia::textoIA(clave.c_str(), en), cx, oy + 226.0f * s, 20.0f * s, A(CIAN), NADA, glm::vec3(0.0f), 1, 2.0f);
			}
			texto(F_TITULO, std::string(idioma::tr("Victorias")) + " " + std::to_string(vd.victorias) + "   ·   " + idioma::tr("Derrotas") + " " + std::to_string(vd.derrotas),
			      cx, oy + 270.0f * s, 18.0f * s, A(GRIS), NADA, glm::vec3(0.0f), 1, 1.0f);
			// rama classic-menus: el rival del nivel (izq/der) y su posicion entre los del nivel
			if (vd.rivalesNivel > 1)
				texto(F_TITULO, "<  " + std::to_string(vd.rivalIdx + 1) + " / " + std::to_string(vd.rivalesNivel) + "  >", cx, oy + 100.0f * s, 22.0f * s, A(CIAN), NADA,
				      glm::vec3(0.0f), 1, 1.0f);
			boton("CRUZ", "Nivel / rival");
			boton("X", "Duelo");
			boton("C", "Otro rival");
			if (vd.sagas > 0) boton("T", "Modo historia");   // (rama classic-personajes)
			boton(idioma::tr("TÁCTIL"), "Construir mazo");
			boton("O", "Volver");
			return;
		}
		// resultado
		const bool gano = vd.ganador == 0;
		const float pulso = 1.0f + 0.04f * sinf(t * 3.0f);
		texto(F_NUMEROS, idioma::tr(gano ? "¡VICTORIA!" : vd.ganador == 1 ? "DERROTA" : "EMPATE"), cx, py + 190.0f * s, 82.0f * s * pulso, A(BLANCO), A(acento),
		      glm::vec3(0.8f, 0.0f, 1.0f), 1, 3.0f);
		const std::string& nom = en && !vd.rivalEn.empty() ? vd.rivalEn : vd.rivalEs;
		texto(F_TITULO, std::string(idioma::tr("Rival")) + ": " + nom + "  ·  " + ia::textoIA("ia.nivel", en) + " " + std::to_string(vd.nivel), cx, py + 260.0f * s, 24.0f * s,
		      A(GRIS), NADA, glm::vec3(0.0f), 1, 1.0f);
		if (gano && vd.personajeNuevo) {
			dibujarResultadoPersonaje(vd, cx, py + 330.0f * s, a);   // rama classic-personajes
		} else if (gano && vd.subeNivel) {
			texto(F_NUMEROS, std::string(idioma::tr("NIVEL DESBLOQUEADO")) + ": " + std::to_string(vd.nivelMax), cx, py + 340.0f * s, 30.0f * s, A(ORO), A(glm::vec4(0.5f, 0.3f, 0.05f, 1.0f)),
			      glm::vec3(0.5f, 0.0f, 0.6f), 1, 2.0f);
		} else if (gano && vd.nivelMax >= vd.niveles && vd.nivel >= vd.niveles) {
			texto(F_NUMEROS, idioma::tr("¡ERES EL CAMPEÓN!"), cx, py + 340.0f * s, 30.0f * s, A(ORO), NADA, glm::vec3(0.5f, 0.0f, 0.6f), 1, 2.0f);
		}
		texto(F_TITULO, std::string(idioma::tr("Victorias")) + " " + std::to_string(vd.victorias) + "   ·   " + idioma::tr("Derrotas") + " " + std::to_string(vd.derrotas), cx,
		      py + 400.0f * s, 18.0f * s, A(GRIS), NADA, glm::vec3(0.0f), 1, 1.0f);
		boton("X", "Continuar");
		boton("O", "Menú principal");   // rama classic-menus
	}

	// rama classic-duelo-arreglos: piedra, papel o tijera (quien empieza). Tres manos dibujadas con las primitivas
	// (piedra: un orbe; papel: una hoja; tijera: dos hojas cruzadas); la elegida en dorado. Luego las dos manos frente
	// a frente con el resultado y, si gana el jugador, "Ir primero" / "Ir segundo"
	void manoSorteo(glm::vec2 c, int m, float tam, glm::vec4 col, float a)
	{
		auto A = [&](glm::vec4 x) { return glm::vec4(glm::vec3(x), x.a * a); };
		usar(setAtlas, false);
		if (m == 0) {
			rect(c, glm::vec2(tam * 0.62f), 0.0f, 12.0f * s, ORBE, A(glm::vec4(0.62f, 0.6f, 0.58f, 1.0f)), A(col), 0.0f, glm::vec4(0.9f, 1.0f, 0.0f, 0.0f));
		} else if (m == 1) {
			rect(c, glm::vec2(tam * 0.62f, tam * 0.82f), -0.12f, 10.0f * s, PANEL, A(glm::vec4(0.95f, 0.95f, 0.9f, 1.0f)), A(col), 6.0f * s, glm::vec4(1.2f * s, 0.8f, 0.0f, 6.0f * s));
			for (int k = 0; k < 3; k++)
				rect(c + glm::vec2(0.0f, (-0.2f + 0.2f * (float)k) * tam), glm::vec2(tam * 0.42f, 2.0f * s), -0.12f, 2.0f * s, LINEA, A(glm::vec4(0.5f, 0.55f, 0.65f, 1.0f)), NADA,
				     0.0f, glm::vec4(1.0f, 0.2f, 0.0f, 0.0f));
		} else {
			for (int k = 0; k < 2; k++)
				rect(c + glm::vec2(0.0f, -0.1f * tam), glm::vec2(tam * 0.16f, tam * 0.78f), k ? 0.42f : -0.42f, 8.0f * s, PANEL, A(glm::vec4(0.8f, 0.85f, 0.92f, 1.0f)), A(col), 4.0f * s,
				     glm::vec4(1.0f * s, 0.8f, 0.0f, 4.0f * s));
			for (int k = 0; k < 2; k++)
				rect(c + glm::vec2((k ? 0.17f : -0.17f) * tam, 0.32f * tam), glm::vec2(tam * 0.22f), 0.0f, 6.0f * s, ORBE, A(glm::vec4(0.85f, 0.25f, 0.25f, 1.0f)), A(col), 0.0f,
				     glm::vec4(0.9f, 1.0f, 0.0f, 0.0f));
		}
	}
	void dibujarSorteo(const control::VistaDuelo& vd)
	{
		static const char* const MANOS[] = { "Piedra", "Papel", "Tijera" };
		const float a = suave(0.0f, 0.25f, vd.t);
		auto A = [&](glm::vec4 c) { return glm::vec4(glm::vec3(c), c.a * a); };
		const float cx = (CAMPO_X * s + W) * 0.5f, cy = H * 0.47f;
		const float w = 980.0f * s, h = 520.0f * s;
		usar(setAtlas, false);
		caja(CAMPO_X * s, 0.0f, W - CAMPO_X * s, H, 0.0f, PANEL, A(glm::vec4(0.01f, 0.015f, 0.03f, 0.55f)), NADA, 0.0f, glm::vec4(0.0f));
		caja(cx - w * 0.5f, cy - h * 0.5f, w, h, 30.0f * s, PANEL, A(VIDRIO), A(glm::mix(CIAN, ORO, 0.55f)), 24.0f * s, glm::vec4(1.6f * s, 1.1f, 0.12f, 22.0f * s));
		texto(F_NUMEROS, idioma::tr("PIEDRA, PAPEL O TIJERA"), cx, cy - h * 0.5f + 64.0f * s, 36.0f * s, A(BLANCO), A(CIAN), glm::vec3(0.5f, 0.0f, 0.6f), 1, 2.0f);
		const float sub = cy - h * 0.5f + 104.0f * s;
		const float pie = cy + h * 0.5f - 40.0f * s;
		auto ayudaPie = [&](bool mover) {
			float bx = cx - 160.0f * s;
			if (mover) {
				bx += iconoBoton(glm::vec2(bx, pie), "CRUZ", 14.0f * s, a) + 10.0f * s;
				bx += texto(F_TITULO, idioma::tr("Opción"), bx, pie + 8.0f * s, 21.0f * s, A(BLANCO), NADA, glm::vec3(0.0f), 0, 1.0f) + 34.0f * s;
			}
			bx += iconoBoton(glm::vec2(bx, pie), "X", 14.0f * s, a) + 10.0f * s;
			texto(F_TITULO, idioma::tr(mover ? "Elegir" : "Continuar"), bx, pie + 8.0f * s, 21.0f * s, A(BLANCO), NADA, glm::vec3(0.0f), 0, 1.0f);
		};
		if (vd.sorteoEtapa == control::VistaDuelo::SORTEO_ELEGIR) {
			texto(F_TITULO, idioma::tr("Elige tu mano: el ganador decide quién empieza"), cx, sub, 24.0f * s, A(GRIS), NADA, glm::vec3(0.0f), 1, 1.0f);
			for (int k = 0; k < 3; k++) {
				const bool sel = k == vd.sorteoOpcion;
				const glm::vec2 c(cx + (float)(k - 1) * 290.0f * s, cy + 30.0f * s);
				const float bw = 230.0f * s, bh = 250.0f * s, lev = sel ? 10.0f * s * (0.6f + 0.4f * sinf(t * 4.0f)) : 0.0f;
				caja(c.x - bw * 0.5f, c.y - bh * 0.5f - lev, bw, bh, 16.0f * s, PANEL, A(sel ? glm::vec4(0.32f, 0.23f, 0.05f, 0.7f) : glm::vec4(glm::vec3(VIDRIO), 0.6f)),
				     A(sel ? ORO : glm::vec4(glm::vec3(CIAN) * 0.6f, 1.0f)), 18.0f * s, glm::vec4((sel ? 1.6f : 1.0f) * s, 1.0f, 0.1f, 12.0f * s));
				manoSorteo(glm::vec2(c.x, c.y - 26.0f * s - lev), k, 130.0f * s, sel ? ORO : CIAN, a);
				texto(F_TITULO, idioma::tr(MANOS[k]), c.x, c.y + bh * 0.5f - 26.0f * s - lev, 26.0f * s, A(sel ? glm::vec4(1.0f, 0.95f, 0.8f, 1.0f) : BLANCO), NADA,
				      glm::vec3(0.0f), 1, 1.0f);
			}
			ayudaPie(true);
			return;
		}
		// las dos manos frente a frente
		const bool en = idioma::ingles();
		const std::string rival = en ? vd.rivalEn : vd.rivalEs;
		const float u = suave(0.0f, 0.35f, vd.sorteoT);
		for (int lado = 0; lado < 2; lado++) {
			const int m = lado == 0 ? vd.sorteoManoJ : vd.sorteoManoR;
			if (m < 0) continue;
			const bool gana = vd.sorteoGanador == lado;
			const glm::vec2 c(cx + (lado == 0 ? -1.0f : 1.0f) * 230.0f * s * u, cy - 10.0f * s);
			manoSorteo(c, m, (130.0f + (gana ? 20.0f : 0.0f)) * s, gana ? ORO : lado ? CARMESI : CIAN, a);
			texto(F_TITULO, lado == 0 ? std::string(idioma::tr("Tú")) : rival, c.x, c.y - 110.0f * s, 22.0f * s, A(lado ? CARMESI : CIAN), NADA, glm::vec3(0.0f), 1, 1.0f);
			texto(F_TITULO, idioma::tr(MANOS[m]), c.x, c.y + 120.0f * s, 24.0f * s, A(BLANCO), NADA, glm::vec3(0.0f), 1, 1.0f);
		}
		texto(F_NUMEROS, "VS", cx, cy, 30.0f * s, A(ORO), NADA, glm::vec3(0.4f, 0.0f, 0.6f), 1, 2.0f);
		const char* res = vd.sorteoGanador < 0 ? "¡Empate! Otra vez" : vd.sorteoGanador == 0 ? "¡Ganas el sorteo!" : "Gana el rival";
		texto(F_NUMEROS, idioma::tr(res), cx, sub, 28.0f * s, A(vd.sorteoGanador == 0 ? ORO : vd.sorteoGanador == 1 ? CARMESI : BLANCO), NADA, glm::vec3(0.4f, 0.0f, 0.6f), 1,
		      1.5f);
		if (vd.sorteoEtapa == control::VistaDuelo::SORTEO_ORDEN) {
			static const char* const ORDEN[] = { "Ir primero", "Ir segundo" };
			for (int k = 0; k < 2; k++) {
				const bool sel = k == vd.sorteoOpcion;
				const float bw = 250.0f * s, bh = 52.0f * s, bx = cx + (k ? 20.0f * s : -20.0f * s - bw), by = cy + 150.0f * s;
				caja(bx, by, bw, bh, 8.0f * s, PANEL, A(sel ? glm::vec4(0.32f, 0.23f, 0.05f, 0.75f) : glm::vec4(glm::vec3(VIDRIO), 0.6f)), A(sel ? ORO : CIAN), bh * 0.5f,
				     glm::vec4(1.2f * s, 1.1f, 0.0f, 0.0f));
				texto(F_TITULO, idioma::tr(ORDEN[k]), bx + bw * 0.5f, by + bh * 0.5f + 8.0f * s, 24.0f * s, A(sel ? glm::vec4(1.0f, 0.95f, 0.8f, 1.0f) : BLANCO), NADA,
				      glm::vec3(0.0f), 1, 1.0f);
			}
			ayudaPie(true);
		} else if (vd.sorteoEtapa == control::VistaDuelo::SORTEO_RIVAL_DECIDE) {
			const std::string d = rival + " " + idioma::tr(vd.sorteoEmpieza == 1 ? "decide ir primero" : "decide ir segundo");
			texto(F_TITULO, d, cx, cy + 180.0f * s, 26.0f * s, A(glm::vec4(1.0f, 0.9f, 0.85f, 1.0f)), NADA, glm::vec3(0.0f), 1, 1.0f);
			ayudaPie(false);
		} else {
			ayudaPie(false);
		}
	}

	// rama classic-arreglos: el menu de pausa (OPTIONS corto; lo lleva duelo_juego.h en ctl->vista): velo, panel al
	// centro, las opciones (la elegida en dorado) y la ayuda (X elegir, O / OPTIONS volver)
	void dibujarPausa()
	{
		if (!ctl || !ctl->vista.pausa || ctl->vista.pausaOpciones.empty()) return;
		const control::VistaDuelo& vd = ctl->vista;
		const int n = (int)vd.pausaOpciones.size();
		const float fila = 58.0f * s, w = 560.0f * s, cab = 96.0f * s, pie = 70.0f * s;
		const float h = cab + fila * (float)n + pie;
		const float cx = W * 0.5f, x0 = cx - w * 0.5f, y0 = H * 0.5f - h * 0.5f;
		usar(setAtlas, false);
		caja(0.0f, 0.0f, W, H, 0.0f, PANEL, glm::vec4(0.01f, 0.015f, 0.03f, 0.7f), NADA, 0.0f, glm::vec4(0.0f));
		caja(x0, y0, w, h, 30.0f * s, PANEL, VIDRIO, glm::mix(CIAN, ORO, 0.55f), 22.0f * s, glm::vec4(1.6f * s, 1.1f, 0.12f, 22.0f * s));
		texto(F_NUMEROS, idioma::tr("PAUSA"), cx, y0 + 62.0f * s, 38.0f * s, BLANCO, CIAN, glm::vec3(0.5f, 0.0f, 0.6f), 1, 2.0f);
		for (int k = 0; k < n; k++) {
			const bool sel = k == vd.pausaOpcion;
			const float fy = y0 + cab + fila * (float)k;
			if (sel) {
				caja(x0 + 24.0f * s, fy + 4.0f * s, w - 48.0f * s, fila - 8.0f * s, 6.0f * s, PANEL, glm::vec4(0.32f, 0.23f, 0.05f, 0.6f), ORO, (fila - 8.0f * s) * 0.5f,
				     glm::vec4(1.2f * s, 1.1f, 0.0f, 0.0f));
				iconoBoton(glm::vec2(x0 + w - 52.0f * s, fy + fila * 0.5f), "X", 14.0f * s);
			}
			texto(F_TITULO, idioma::tr(vd.pausaOpciones[(size_t)k].c_str()), x0 + 54.0f * s, fy + fila * 0.5f + 9.0f * s, 27.0f * s,
			      sel ? glm::vec4(1.0f, 0.95f, 0.8f, 1.0f) : glm::vec4(0.86f, 0.9f, 0.97f, 1.0f), NADA, glm::vec3(0.0f), 0, 1.0f);
		}
		float bx = x0 + 40.0f * s;
		const float by = y0 + h - pie * 0.5f;
		auto boton = [&](const std::string& b, const char* txt) {
			bx += iconoBoton(glm::vec2(bx + 14.0f * s, by), b, 14.0f * s) + 12.0f * s;
			bx += texto(F_TITULO, idioma::tr(txt), bx, by + 8.0f * s, 21.0f * s, BLANCO, NADA, glm::vec3(0.0f), 0, 1.0f) + 30.0f * s;
		};
		boton("CRUZ", "Opción");
		boton("X", "Elegir");
		boton("O", "Volver");
		boton("OPTIONS", "Volver");
	}
