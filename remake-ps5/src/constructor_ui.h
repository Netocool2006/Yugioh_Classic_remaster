// Yugioh Classic Remaster (rama classic-constructor): se incluye DENTRO de interfaz::Interfaz (usa sus primitivas).
//  1) La cache de caras: con 1690 cartas no se cargan todas al arrancar. Quien dibuja una carta pregunta
//     listaCarta / miniLista / caraLista: eso la marca como usada en este cuadro y, si falta, la pide; procesarCargas
//     (al empezar cada cuadro, fuera del dibujo) carga unas pocas por cuadro y descarga las que hace mas que no se
//     usan cuando se pasa del tope (MAX_CARAS / MAX_MINIS). Solo se descarga lo que no se dibujo en los ultimos
//     cuadros en vuelo (la GPU ya no lo usa); su descriptor queda y se reescribe al volver a cargarla.
//  2) La pantalla "Construir mazo" (constructor.h es la logica): rejilla de minis, la carta grande con su texto,
//     filtros, el conteo del mazo y la barra de botones.

public:
	static constexpr int MAX_CARAS = 40, MAX_MINIS = 220;   // ~29 MB y ~41 MB de texturas como mucho
	static constexpr int CARGAS_POR_CUADRO = 8;
	static constexpr float MS_POR_CUADRO = 7.0f;

	// lista para dibujar (cara y mini); si no, la pide
	bool listaCarta(int n) const
	{
		if (n < 0 || n >= (int)cartas.size()) return false;
		const bool c = caraLista(n), m = miniLista(n);
		return c && m;
	}
	bool miniLista(int n) const
	{
		if (n < 0 || n >= (int)cartas.size()) return false;
		CartaUI& k = cartas[n];
		k.usoMini = cuadroN;
		if (!k.tieneMini && !k.falloMini) k.quiereMini = true;
		return k.tieneMini;
	}
	bool caraLista(int n) const
	{
		if (n < 0 || n >= (int)cartas.size()) return false;
		CartaUI& k = cartas[n];
		k.usoCara = cuadroN;
		if (!k.tieneCara && !k.falloCara) k.quiereCara = true;
		return k.tieneCara;
	}

private:
	void cargarImagenCarta(int n, bool cara)
	{
		CartaUI& k = cartas[n];
		bool& tiene = cara ? k.tieneCara : k.tieneMini;
		bool& fallo = cara ? k.falloCara : k.falloMini;
		(cara ? k.quiereCara : k.quiereMini) = false;
		if (tiene || fallo) return;
		vks::Texture2D& tex = cara ? k.cara : k.mini;
		// en ingles cara_en_<id> / mini_en_<id>; si falta, la espanola
		const std::string base = rutaAssets + (cara ? "ui/cara" : "ui/mini");
		const std::string suf = constructor::sufijoIdioma();
		bool ok = false;
		if (!suf.empty()) {
			struct stat st{};
			const std::string en = base + suf + "_" + k.id + ".png";
			ok = stat(en.c_str(), &st) == 0 && (cara ? cargarConMips(en, tex, true) : cargarImagen(en, tex));
		}
		// la cara con mipmaps (classic-base: nitida chica e inclinada en el tablero, con el filtro anisotropo)
		if (!ok && !(cara ? cargarConMips(base + "_" + k.id + ".png", tex, true) : cargarImagen(base + "_" + k.id + ".png", tex))) {
			fallo = true;   // (sin imagen: el dorso; no se reintenta)
			return;
		}
		VkDescriptorSet& set = cara ? k.setCara : k.setMini;
		if (set == VK_NULL_HANDLE) {
			set = nuevoSet(tex);
		} else {
			VkDescriptorImageInfo img{ muestreador, tex.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
			VkWriteDescriptorSet w = vks::initializers::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &img);
			vkUpdateDescriptorSets(dev, 1, &w, 0, nullptr);
		}
		tiene = true;
		k.lista = k.tieneCara && k.tieneMini;
	}

	int versionCaras{ 0 };   // versionIdioma() de las caras cargadas

	void procesarCargas()
	{
		cuadroN++;
		if (versionCaras != constructor::versionIdioma()) {
			// cambio el idioma: fuera todas las caras (se vuelven a pedir con el sufijo nuevo)
			versionCaras = constructor::versionIdioma();
			vkDeviceWaitIdle(dev);
			for (CartaUI& k : cartas) {
				if (k.tieneCara) k.cara.destroy();
				if (k.tieneMini) k.mini.destroy();
				k.tieneCara = k.tieneMini = k.lista = false;
				k.falloCara = k.falloMini = false;
			}
			for (CaraHD& h : hd) {
				if (h.carta >= 0) h.tex.destroy();
				h.carta = -1;
			}
			hdFalta.clear();
		}
		cargarHD();
		const auto t0 = std::chrono::steady_clock::now();
		int hechas = 0;
		// primero las minis (la rejilla), despues las caras; solo lo pedido en el ultimo cuadro
		for (int pasada = 0; pasada < 2; pasada++) {
			const bool cara = pasada == 1;
			for (int n = 0; n < (int)cartas.size(); n++) {
				CartaUI& k = cartas[n];
				const bool quiere = cara ? k.quiereCara : k.quiereMini;
				if (!quiere) continue;
				if ((cara ? k.usoCara : k.usoMini) + 2 < cuadroN) {   // ya no se dibuja: se olvida el pedido
					(cara ? k.quiereCara : k.quiereMini) = false;
					continue;
				}
				if (hechas >= CARGAS_POR_CUADRO ||
				    std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - t0).count() > MS_POR_CUADRO) {
					break;
				}
				cargarImagenCarta(n, cara);
				hechas++;
			}
		}
		expulsar(true, MAX_CARAS);
		expulsar(false, MAX_MINIS);
	}

	void expulsar(bool cara, int maximo)
	{
		int cargadas = 0;
		for (const CartaUI& k : cartas) cargadas += (cara ? k.tieneCara : k.tieneMini) ? 1 : 0;
		while (cargadas > maximo) {
			int peor = -1;
			for (int n = 0; n < (int)cartas.size(); n++) {
				const CartaUI& k = cartas[n];
				if (!(cara ? k.tieneCara : k.tieneMini)) continue;
				const uint64_t uso = cara ? k.usoCara : k.usoMini;
				if (uso + (uint64_t)maxConcurrentFrames + 2 >= cuadroN) continue;   // (la GPU puede estar usandola)
				if (peor < 0 || uso < (cara ? cartas[peor].usoCara : cartas[peor].usoMini)) peor = n;
			}
			if (peor < 0) return;
			CartaUI& k = cartas[peor];
			(cara ? k.cara : k.mini).destroy();
			(cara ? k.tieneCara : k.tieneMini) = false;
			k.lista = false;
			cargadas--;
		}
	}

	// ---- caras HD de la carta grande del constructor (el juego sera 4K): cara_hd_<id>.jpg (ingles cara_hd_en_<id>.jpg;
	// si falta, la espanola; si no hay HD, la cara_ normal). Cache propia de MAX_HD; un descriptor fijo por casillero
	static constexpr int MAX_HD = 6;
	struct CaraHD { int carta{ -1 }; vks::Texture2D tex; VkDescriptorSet set{ VK_NULL_HANDLE }; uint64_t uso{ 0 }; };
	std::array<CaraHD, MAX_HD> hd{};
	mutable int hdPedida{ -1 };
	std::unordered_map<int, bool> hdFalta;   // cartas sin HD (no se reintenta)

	const CaraHD* caraHD(int n) const
	{
		for (const CaraHD& h : hd) {
			if (h.carta == n) {
				const_cast<CaraHD&>(h).uso = cuadroN;
				return &h;
			}
		}
		if (!hdFalta.count(n)) hdPedida = n;
		return nullptr;
	}
	void cargarHD()
	{
		const int n = hdPedida;
		hdPedida = -1;
		if (n < 0 || n >= (int)cartas.size() || hdFalta.count(n)) return;
		int lugar = -1;
		for (int i = 0; i < MAX_HD; i++) {
			if (hd[i].carta == n) return;
			if (hd[i].carta >= 0 && hd[i].uso + (uint64_t)maxConcurrentFrames + 2 >= cuadroN) continue;   // (en uso por la GPU)
			if (lugar < 0 || hd[i].carta < 0 || (hd[lugar].carta >= 0 && hd[i].uso < hd[lugar].uso)) lugar = i;
		}
		if (lugar < 0) return;
		const std::string base = rutaAssets + "ui/cara_hd", suf = constructor::sufijoIdioma();
		std::string ruta;
		struct stat st{};
		for (const std::string& r : { base + suf + "_" + cartas[n].id + ".jpg", base + "_" + cartas[n].id + ".jpg" }) {
			if (stat(r.c_str(), &st) == 0) {
				ruta = r;
				break;
			}
		}
		if (ruta.empty()) {
			hdFalta[n] = true;
			return;
		}
		CaraHD& h = hd[lugar];
		if (h.carta >= 0) h.tex.destroy();
		h.carta = -1;
		if (!cargarConMips(ruta, h.tex, true)) {   // (con mipmaps: se ve a 4K inclinada)
			hdFalta[n] = true;
			return;
		}
		if (h.set == VK_NULL_HANDLE) {
			h.set = nuevoSet(h.tex);
		} else {
			VkDescriptorImageInfo img{ muestreador, h.tex.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
			VkWriteDescriptorSet w = vks::initializers::writeDescriptorSet(h.set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &img);
			vkUpdateDescriptorSets(dev, 1, &w, 0, nullptr);
		}
		h.carta = n;
		h.uso = cuadroN;
	}

	// ------------------------------------------------------------------------------------------ Construir mazo
	glm::vec4 colorClase(const std::string& c) const
	{
		if (c == "normal") return glm::vec4(0.95f, 0.80f, 0.45f, 1.0f);
		if (c == "efecto") return glm::vec4(1.0f, 0.55f, 0.30f, 1.0f);
		if (c == "ritual") return glm::vec4(0.40f, 0.62f, 1.0f, 1.0f);
		if (c == "fusion") return glm::vec4(0.75f, 0.48f, 1.0f, 1.0f);
		if (c == "magia") return glm::vec4(0.25f, 0.95f, 0.75f, 1.0f);
		if (c == "trampa") return glm::vec4(1.0f, 0.38f, 0.72f, 1.0f);
		return GRIS;
	}

	void dibujarConstructor()
	{
		using constructor::T;   // (textos ES/EN: constructor_textos.h)
		const constructor::Constructor& c = *cons;
		if (!c.col) return;
		usar(setAtlas, false);
		// fondo opaco (el duelo sigue debajo) con un velo de color
		caja(0.0f, 0.0f, W, H, 0.0f, PANEL, glm::vec4(0.012f, 0.018f, 0.036f, 1.0f), NADA, 0.0f, glm::vec4(0.0f));
		rect(glm::vec2(W * 0.5f, H * 0.42f), glm::vec2(W * 1.1f, H * 0.9f), 0.0f, 0.0f, PARTICULA, glm::vec4(glm::vec3(CIAN) * 0.06f, 1.0f), NADA, 0.0f,
		     glm::vec4(0.0f));

		// ---- cabecera: titulo, filtros (L2/R2), inicial (stick der), pagina
		texto(F_NUMEROS, T("CONSTRUIR MAZO"), 44.0f * s, 64.0f * s, 34.0f * s, BLANCO, CIAN, glm::vec3(0.5f, 0.0f, 0.6f), 0, 2.0f);
		texto(F_TITULO, T(c.verMazo ? "MI MAZO" : "COLECCIÓN · FORMATO GOAT"), 44.0f * s, 96.0f * s, 20.0f * s, c.verMazo ? ORO : GRIS, NADA, glm::vec3(0.0f), 0, 1.5f);
		float x = 560.0f * s;
		const float yF = 40.0f * s, hF = 36.0f * s;
		x += iconoBoton(glm::vec2(x + 12.0f * s, yF + hF * 0.5f), "L2", 12.0f * s) + 12.0f * s;
		for (int i = 0; i < constructor::NUM_FILTROS; i++) {
			const bool sel = i == c.filtro;
			const float w = medir(F_TITULO, T(constructor::FILTROS[i]), 18.0f * s, 1.0f) + 28.0f * s;
			const glm::vec4 col = i == 0 ? CIAN : colorClase(constructor::CLASE_FILTRO[i]);
			usar(setAtlas, false);
			caja(x, yF, w, hF, 8.0f * s, PANEL, sel ? glm::vec4(glm::vec3(col) * 0.30f, 0.95f) : glm::vec4(glm::vec3(VIDRIO), 0.7f),
			     sel ? col : glm::vec4(glm::vec3(col) * 0.35f, 1.0f), hF * 0.5f, glm::vec4((sel ? 1.8f : 1.0f) * s, sel ? 1.2f : 0.5f, 0.0f, 0.0f));
			texto(F_TITULO, T(constructor::FILTROS[i]), x + w * 0.5f, yF + hF * 0.5f + 7.0f * s, 18.0f * s, sel ? BLANCO : GRIS, NADA, glm::vec3(0.0f), 1, 1.0f);
			x += w + 8.0f * s;
		}
		x += iconoBoton(glm::vec2(x + 12.0f * s, yF + hF * 0.5f), "R2", 12.0f * s) + 26.0f * s;
		{
			const char l = constructor::letraInicial(c.inicial);
			const std::string ini = l ? T("INICIAL ") + l : T("INICIAL: TODAS");
			x += iconoBoton(glm::vec2(x + 12.0f * s, yF + hF * 0.5f), "STICK_R", 12.0f * s) + 10.0f * s;
			texto(F_TITULO, ini, x, yF + hF * 0.5f + 7.0f * s, 19.0f * s, l ? ORO : GRIS, NADA, glm::vec3(0.0f), 0, 1.0f);
		}
		const int pag = c.pagina(), pags = c.paginas();
		texto(F_NUMEROS, std::to_string(c.lista.size()) + T(" CARTAS · PÁG ") + std::to_string(pag + 1) + "/" + std::to_string(pags), W - 44.0f * s, 98.0f * s,
		      18.0f * s, GRIS, NADA, glm::vec3(0.0f), 2, 1.0f);

		// ---- rejilla de minis
		const float gx = 560.0f * s, gy = 128.0f * s, celW = 186.0f * s, celH = 268.0f * s;
		const glm::vec2 tamMini = glm::vec2(166.0f, 241.0f) * s;
		const int desde = pag * constructor::POR_PAGINA;
		for (int i = 0; i < constructor::POR_PAGINA; i++) {
			const int li = desde + i;
			if (li >= (int)c.lista.size()) break;
			const int ci = c.lista[li];
			const mazo::Carta& mc = c.col->cartas[ci];
			const int ui = buscarCarta(mc.id);
			const glm::vec2 centro(gx + (float)(i % constructor::COLUMNAS) * celW + celW * 0.5f, gy + (float)(i / constructor::COLUMNAS) * celH + celH * 0.5f);
			const bool sel = li == c.cursor;
			const int enMazo = c.mazo.cuantas(mc.id);
			const bool agotada = mc.limite == 0 || enMazo >= mc.limite;
			const float esc = sel ? 1.0f + 0.06f * suave(0.0f, 0.12f, c.tCursor) : 1.0f;
			const glm::vec2 tam = tamMini * esc;
			usar(setAtlas, false);
			rect(centro + glm::vec2(0.0f, 6.0f * s), tam, 0.0f, 16.0f * s, PANEL, glm::vec4(0, 0, 0, 0.6f), NADA, 8.0f * s, glm::vec4(0, 0, 0, 12.0f * s));
			const float brillo = c.verMazo ? 1.0f : agotada ? 0.42f : 0.95f;
			if (ui >= 0 && miniLista(ui)) {
				usar(cartas[ui].setMini, false);
				const float u = 0.5f / cartas[ui].mini.width, vv = 0.5f / cartas[ui].mini.height;
				rect(centro, tam, 0.0f, 0.0f, IMAGEN, glm::vec4(glm::vec3(sel ? 1.1f : brillo), 1.0f), glm::vec4(0.6f, 0.8f, 1.0f, 1.0f), 7.0f * s,
				     glm::vec4(sel ? 0.45f : 0.0f, sel ? 1.0f : 0.0f, (float)i * 0.13f, 0.0f), glm::vec2(u, vv), glm::vec2(1.0f - u, 1.0f - vv));
			} else {
				// (aun sin imagen: una ficha con el color de su clase y el nombre)
				const glm::vec4 cc = colorClase(mc.clase);
				caja(centro.x - tam.x * 0.5f, centro.y - tam.y * 0.5f, tam.x, tam.y, 0.0f, PANEL, glm::vec4(glm::vec3(cc) * 0.18f * brillo, 1.0f), cc, 7.0f * s,
				     glm::vec4(1.2f * s, 0.5f, 0.0f, 0.0f));
				parrafo(F_TITULO, mc.nombre, centro.x - tam.x * 0.5f + 10.0f * s, centro.y - 20.0f * s, 17.0f * s, tam.x - 20.0f * s, 19.0f * s, 3, BLANCO);
			}
			usar(setAtlas, false);
			// limite (prohibida / limitada / semilimitada) arriba a la izquierda
			if (mc.bloqueada) {   // rama classic-personajes: carta firma aun bloqueada (candado)
				const glm::vec2 cc = centro + glm::vec2(0.0f, -6.0f * s);
				const glm::vec4 col(0.85f, 0.88f, 0.95f, 1.0f);
				rect(cc + glm::vec2(0.0f, -16.0f * s), glm::vec2(30.0f * s), 0.0f, 0.0f, PANEL, NADA, col, 15.0f * s, glm::vec4(4.0f * s, 2.4f, 0.0f, 0.0f));
				caja(cc.x - 24.0f * s, cc.y - 6.0f * s, 48.0f * s, 38.0f * s, 4.0f * s, PANEL, glm::vec4(0.05f, 0.06f, 0.1f, 0.95f), col, 6.0f * s,
				     glm::vec4(1.6f * s, 1.0f, 0.0f, 0.0f));
				rect(cc + glm::vec2(0.0f, 11.0f * s), glm::vec2(7.0f * s, 12.0f * s), 0.0f, 0.0f, PANEL, col, NADA, 3.0f * s, glm::vec4(0.0f));
			}
			if (mc.limite < 3) {
				static const char* const LIM[] = { "PROHIBIDA", "LIMITADA", "SEMI" };
				const char* etq = mc.bloqueada ? "BLOQUEADA" : LIM[mc.limite];
				const glm::vec4 col = mc.bloqueada ? glm::vec4(0.75f, 0.8f, 0.9f, 1.0f) : mc.limite == 0 ? CARMESI : mc.limite == 1 ? ORO : glm::vec4(1.0f, 0.62f, 0.25f, 1.0f);
				const float w = medir(F_NUMEROS, T(etq), 12.0f * s, 1.0f) + 14.0f * s;
				caja(centro.x - tam.x * 0.5f + 4.0f * s, centro.y - tam.y * 0.5f + 4.0f * s, w, 20.0f * s, 4.0f * s, PANEL, glm::vec4(0.05f, 0.03f, 0.05f, 0.92f), col,
				     6.0f * s, glm::vec4(1.0f * s, 1.0f, 0.0f, 0.0f));
				texto(F_NUMEROS, T(etq), centro.x - tam.x * 0.5f + 4.0f * s + w * 0.5f, centro.y - tam.y * 0.5f + 19.0f * s, 12.0f * s, col, NADA,
				      glm::vec3(0.0f), 1, 1.0f);
			}
			// copias en el mazo / permitidas, abajo a la derecha
			if (enMazo > 0 || sel) {
				const std::string n = std::to_string(enMazo) + "/" + std::to_string(mc.limite);
				const float w = medir(F_NUMEROS, n, 17.0f * s, 1.0f) + 18.0f * s, h = 26.0f * s;
				const float bx = centro.x + tam.x * 0.5f - w - 4.0f * s, by = centro.y + tam.y * 0.5f - h - 4.0f * s;
				caja(bx, by, w, h, 6.0f * s, PANEL, glm::vec4(0.02f, 0.04f, 0.08f, 0.94f), enMazo > 0 ? ORO : GRIS, h * 0.5f, glm::vec4(1.4f * s, 1.0f, 0.0f, 0.0f));
				texto(F_NUMEROS, n, bx + w * 0.5f, by + h * 0.5f + 6.0f * s, 17.0f * s, enMazo > 0 ? ORO : BLANCO, NADA, glm::vec3(0.0f), 1, 1.0f);
			}
			if (sel) {
				// el cursor: un marco dorado que respira
				const float pulso = 0.6f + 0.4f * sinf(t * 5.0f);
				caja(centro.x - tam.x * 0.5f - 5.0f * s, centro.y - tam.y * 0.5f - 5.0f * s, tam.x + 10.0f * s, tam.y + 10.0f * s, 14.0f * s, PANEL, NADA, ORO, 10.0f * s,
				     glm::vec4(3.0f * s, 1.0f + pulso, 0.6f, 0.0f));
			}
		}
		if (c.lista.empty()) {
			texto(F_TITULO, T(c.verMazo ? "El mazo está vacío: Triángulo vuelve a la colección" : "Ninguna carta con estos filtros (L3 los quita)"),
			      gx + 3.5f * celW, gy + 1.5f * celH, 24.0f * s, GRIS, NADA, glm::vec3(0.0f), 1, 1.0f);
		}

		// ---- panel izquierdo: la carta elegida grande y su ficha
		const float px = 40.0f * s, py = 128.0f * s, pw = 490.0f * s, ph = 804.0f * s;
		const mazo::Carta* e = c.elegida();
		const glm::vec4 acento = e ? colorClase(e->clase) : CIAN;
		usar(setAtlas, false);
		caja(px, py, pw, ph, 22.0f * s, PANEL, VIDRIO, glm::mix(CIAN, acento, 0.5f), 16.0f * s, glm::vec4(1.4f * s, 0.8f, 0.07f, 16.0f * s));
		if (e) {
			const int ui = buscarCarta(e->id);
			const glm::vec2 tam = glm::vec2(300.0f, 435.0f) * s;
			const glm::vec2 centro(px + pw * 0.5f, py + 24.0f * s + tam.y * 0.5f);
			const float angY = glm::radians(5.0f) * sinf(t * 0.6f), angX = glm::radians(2.5f) * sinf(t * 0.45f + 1.0f);
			if (const CaraHD* h = ui >= 0 ? caraHD(ui) : nullptr) {   // la cara HD (4K) si la hay
				usar(h->set, false);
				const float u = 0.5f / h->tex.width, vv = 0.5f / h->tex.height;
				rectInclinado(centro, tam, angY, angX, 0.0f, IMAGEN, glm::vec4(1.0f), acento, 10.0f * s, glm::vec4(0.5f, 1.0f, (float)ui * 0.13f, 0.0f),
				              glm::vec2(u, vv), glm::vec2(1.0f - u, 1.0f - vv));
			} else if (ui >= 0 && caraLista(ui)) {
				usar(cartas[ui].setCara, false);
				const float u = 0.5f / cartas[ui].cara.width, vv = 0.5f / cartas[ui].cara.height;
				rectInclinado(centro, tam, angY, angX, 0.0f, IMAGEN, glm::vec4(1.0f), acento, 10.0f * s, glm::vec4(0.5f, 1.0f, (float)ui * 0.13f, 0.0f),
				              glm::vec2(u, vv), glm::vec2(1.0f - u, 1.0f - vv));
			} else if (ui >= 0 && miniLista(ui)) {   // (mientras llega la cara grande, la mini)
				usar(cartas[ui].setMini, false);
				const float u = 0.5f / cartas[ui].mini.width, vv = 0.5f / cartas[ui].mini.height;
				rectInclinado(centro, tam, angY, angX, 0.0f, IMAGEN, glm::vec4(1.0f), acento, 10.0f * s, glm::vec4(0.0f), glm::vec2(u, vv), glm::vec2(1.0f - u, 1.0f - vv));
			} else {
				const Dorso dz = usarDorso(false, false, false, 1.0f, 0.0f);
				rectInclinado(centro, tam, angY, angX, 0.0f, DORSO, dz.col, dz.col2, 10.0f * s, dz.extra, uvDorso0(false), uvDorso1(false));
			}
			usar(setAtlas, false);
			float y = py + 24.0f * s + tam.y + 40.0f * s;
			const float tx = px + 26.0f * s, tw = pw - 52.0f * s;
			parrafo(F_TITULO, e->nombre, tx, y, 27.0f * s, tw, 28.0f * s, 2, BLANCO);
			y += (medir(F_TITULO, e->nombre, 27.0f * s) > tw ? 56.0f : 30.0f) * s;
			std::string linea;
			if (mazo::esMonstruo(e->clase)) {
				linea = e->atributo + " · " + e->raza + " · " + T("NIVEL ") + std::to_string(e->nivel);
			} else {
				linea = T(e->clase == "magia" ? "MAGIA" : "TRAMPA") + (e->subtipo.empty() ? "" : " · " + mayusculas(e->subtipo));
			}
			texto(F_TITULO, linea, tx, y, 19.0f * s, acento, NADA, glm::vec3(0.0f), 0, 1.0f);
			y += 26.0f * s;
			if (mazo::esMonstruo(e->clase)) {
				texto(F_NUMEROS, "ATK " + std::to_string(e->atk) + "   DEF " + std::to_string(e->def), tx, y, 20.0f * s, ORO, NADA, glm::vec3(0.0f), 0, 1.0f);
				y += 26.0f * s;
			}
			const int resto = std::max(1, (int)((py + ph - 64.0f * s - y) / (21.0f * s)));
			parrafo(F_TEXTO, e->texto, tx, y, 18.0f * s, tw, 21.0f * s, resto, glm::vec4(0.82f, 0.87f, 0.95f, 1.0f));
			// copias: en el mazo / limite
			static const char* const LIM[] = { "PROHIBIDA", "LIMITADA (1)", "SEMILIMITADA (2)", "SIN LÍMITE (3)" };
			const std::string cop = e->bloqueada ? T("CARTA FIRMA BLOQUEADA") + "  ·  " + c.col->venceA(*e)   // (rama classic-personajes)
			                                     : T("EN EL MAZO ") + std::to_string(c.mazo.cuantas(e->id)) + "/" + std::to_string(e->limite) + "  ·  " + T(LIM[std::clamp(e->limite, 0, 3)]);
			texto(F_NUMEROS, cop, tx, py + ph - 24.0f * s, 16.0f * s, e->bloqueada ? glm::vec4(0.75f, 0.8f, 0.9f, 1.0f) : e->limite == 0 ? CARMESI : GRIS, NADA, glm::vec3(0.0f), 0, 1.0f);
		}

		// ---- el mazo: conteo y validez
		{
			int m = 0, mg = 0, tr = 0;
			c.conteo(m, mg, tr);
			std::string motivo;
			const bool ok = c.valido(&motivo);
			const float by = 948.0f * s, bh = 52.0f * s, bx = 40.0f * s, bw = W - 80.0f * s;
			usar(setAtlas, false);
			caja(bx, by, bw, bh, 14.0f * s, PANEL, glm::vec4(glm::vec3(VIDRIO), 0.9f), ok ? glm::vec4(0.45f, 1.0f, 0.62f, 1.0f) : ORO, 12.0f * s,
			     glm::vec4(1.4f * s, 0.8f, 0.0f, 10.0f * s));
			float xx = bx + 24.0f * s;
			const float base = by + bh * 0.5f + 8.0f * s;
			auto dato = [&](const std::string& rot, const std::string& val, glm::vec4 col) {
				xx += texto(F_TITULO, T(rot), xx, base, 19.0f * s, GRIS, NADA, glm::vec3(0.0f), 0, 1.0f) + 8.0f * s;
				xx += texto(F_NUMEROS, val, xx, base, 22.0f * s, col, NADA, glm::vec3(0.0f), 0, 1.0f) + 30.0f * s;
			};
			const int n = (int)c.mazo.principal.size();
			dato("MAZO", std::to_string(n) + " / 40-60", n >= mazo::MIN_PRINCIPAL && n <= mazo::MAX_PRINCIPAL ? BLANCO : ORO);
			dato("MONSTRUOS", std::to_string(m), colorClase("efecto"));
			dato("MAGIAS", std::to_string(mg), colorClase("magia"));
			dato("TRAMPAS", std::to_string(tr), colorClase("trampa"));
			dato("FUSIÓN", std::to_string(c.mazo.fusion.size()) + " / 15", colorClase("fusion"));
			texto(F_TITULO, ok ? T("MAZO VÁLIDO") : motivo, bx + bw - 24.0f * s, base, 20.0f * s, ok ? glm::vec4(0.45f, 1.0f, 0.62f, 1.0f) : ORO, NADA, glm::vec3(0.0f), 2,
			      1.0f);
		}

		// ---- barra de botones
		{
			static const char* const B[][2] = { { "CRUZ", "Mover" }, { "L1R1", "Página" }, { "X", "Agregar" }, { "C", "Quitar" },
				                               { "T", "Colección / Mi mazo" }, { "L3", "Sin filtros" }, { "O", "Guardar y salir" } };
			float xx = 60.0f * s;
			const float yy = 1040.0f * s, r = 13.0f * s;
			for (const auto& b : B) {
				const float wi = iconoBoton(glm::vec2(xx + r, yy), b[0], r);
				xx += wi + 8.0f * s;
				xx += texto(F_TITULO, T(b[1]), xx, yy + 7.0f * s, 19.0f * s, glm::vec4(0.88f, 0.92f, 0.98f, 1.0f), NADA, glm::vec3(0.0f), 0, 1.0f) + 28.0f * s;
			}
		}

		// ---- aviso
		if (!c.aviso.empty() && c.tAviso < 2.6f) {
			const float a = suave(0.0f, 0.15f, c.tAviso) * (1.0f - suave(2.1f, 2.6f, c.tAviso));
			auto A = [&](glm::vec4 col) { return glm::vec4(glm::vec3(col), col.a * a); };
			const float tt = 22.0f * s, w = medir(F_TITULO, c.aviso, tt, 1.0f) + 60.0f * s, h = 44.0f * s;
			const float cx = gx + 3.5f * celW, y = 884.0f * s;
			usar(setAtlas, false);
			caja(cx - w * 0.5f, y, w, h, 16.0f * s, PANEL, A(VIDRIO), A(ORO), h * 0.5f, glm::vec4(1.2f * s, 1.0f, 0.2f, 12.0f * s));
			texto(F_TITULO, c.aviso, cx, y + h * 0.5f + 8.0f * s, tt, A(BLANCO), NADA, glm::vec3(0.0f), 1, 1.0f);
		}
	}
