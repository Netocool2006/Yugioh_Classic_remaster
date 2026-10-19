/*
 * Yugioh Classic Remaster - la pantalla "Construir mazo" (rama classic-constructor): SOLO logica (filtros, cursor,
 * paginas, agregar/quitar, guardar). La dibuja interfaz.h (constructor_ui.h, incluido dentro de interfaz::Interfaz);
 * la abre el juego (panel tactil o Ajustes > "Construir mazo").
 *
 * Mando (DualSense):
 *   cruceta / stick izq   mover en la rejilla          L1 / R1      pagina anterior / siguiente
 *   L2 / R2               filtro de clase (Todas, Normal, Efecto, Ritual, Fusion, Magia, Trampa)
 *   stick der izq/der     inicial del nombre (Todas, A..Z, #)        L3   quitar los filtros
 *   X                     agregar una copia al mazo                  cuadrado   quitar una copia
 *   triangulo             ver la coleccion / ver mi mazo             circulo    guardar y salir
 * En el PC con ventana (teclas del mando de yugireload.cpp): flechas, Enter = X, F = cuadrado, T = triangulo,
 * Retroceso = circulo, Q/E = L1/R1.
 */
#pragma once

#include "mazo.h"

#include <cmath>
#include <string>
#include <vector>

namespace constructor {

// (textos ES; se muestran con T())
static const char* const FILTROS[] = { "TODAS", "NORMAL", "EFECTO", "RITUAL", "FUSIÓN", "MAGIA", "TRAMPA" };
static const char* const CLASE_FILTRO[] = { "", "normal", "efecto", "ritual", "fusion", "magia", "trampa" };
constexpr int NUM_FILTROS = 7;
constexpr int COLUMNAS = 7, FILAS = 3, POR_PAGINA = COLUMNAS * FILAS;
// iniciales: 0 = todas, 1..26 = A..Z, 27 = '#'
constexpr int NUM_INICIALES = 28;
inline char letraInicial(int i) { return i <= 0 ? 0 : i <= 26 ? (char)('A' + i - 1) : '#'; }

// botones logicos (los mismos bits que control::Boton, mas los de esta pantalla)
enum : uint32_t {
	C_X = 1u << 0, C_CIRCULO = 1u << 1, C_TRIANGULO = 1u << 2, C_CUADRADO = 1u << 3,
	C_ARRIBA = 1u << 4, C_ABAJO = 1u << 5, C_IZQ = 1u << 6, C_DER = 1u << 7,
	C_L1 = 1u << 8, C_R1 = 1u << 9, C_L2 = 1u << 12, C_R2 = 1u << 13, C_L3 = 1u << 14,
	C_INI_ANT = 1u << 15, C_INI_SIG = 1u << 16,
};

class Constructor {
public:
	const mazo::Coleccion* col{ nullptr };
	mazo::Mazo mazo;              // el que se esta armando (al abrir: copia del guardado)
	std::string ruta;             // donde se guarda
	bool abierto{ false };
	bool verMazo{ false };        // triangulo: la rejilla muestra las cartas del mazo (para quitar)
	int filtro{ 0 }, inicial{ 0 };
	int cursor{ 0 };              // indice en "lista"
	std::vector<int> lista;       // indices de col->cartas visibles con los filtros (o del mazo)
	std::string aviso;
	float tAviso{ 99.0f }, t{ 0.0f }, tCursor{ 0.0f };
	bool cambiado{ false };       // se toco el mazo desde que se abrio
	bool guardadoAlSalir{ false };   // (lo lee el juego al cerrarse: re-repartir)

	void abrir(const mazo::Coleccion& c, const mazo::Mazo& actual, const std::string& r)
	{
		col = &c;
		mazo = actual;
		ruta = r;
		abierto = true;
		cambiado = false;
		guardadoAlSalir = false;
		verMazo = false;
		t = 0.0f;
		filtrar();
		avisar(T("X agrega, Cuadrado quita, Círculo guarda y sale"));
	}

	// pagina y posicion del cursor
	int pagina() const { return cursor / POR_PAGINA; }
	int paginas() const { return std::max(1, ((int)lista.size() + POR_PAGINA - 1) / POR_PAGINA); }
	const mazo::Carta* elegida() const
	{
		if (!col || cursor < 0 || cursor >= (int)lista.size()) return nullptr;
		return &col->cartas[lista[cursor]];
	}
	// cuantas hay de cada clase en el mazo principal
	void conteo(int& monstruos, int& magias, int& trampas) const
	{
		monstruos = magias = trampas = 0;
		for (const std::string& id : mazo.principal) {
			const mazo::Carta* k = col ? col->carta(id) : nullptr;
			if (!k) continue;
			if (k->clase == "magia") magias++;
			else if (k->clase == "trampa") trampas++;
			else monstruos++;
		}
	}

	// un cuadro: pulsos con los bits de arriba (ya con la repeticion)
	void avanzar(float dt, uint32_t p)
	{
		if (!abierto || !col) return;
		t += dt;
		tAviso += dt;
		tCursor += dt;
		const int n = (int)lista.size();
		const int antes = cursor;
		if (n > 0) {
			const int enPag = cursor % POR_PAGINA, pag = cursor / POR_PAGINA;
			const int fila = enPag / COLUMNAS, colu = enPag % COLUMNAS;
			if ((p & C_IZQ) && colu > 0) cursor--;
			if ((p & C_DER) && colu < COLUMNAS - 1) cursor++;
			if ((p & C_ARRIBA) && fila > 0) cursor -= COLUMNAS;
			if ((p & C_ABAJO) && fila < FILAS - 1) cursor += COLUMNAS;
			if (p & C_L1) cursor -= POR_PAGINA;
			if (p & C_R1) cursor += POR_PAGINA;
			// en el borde: a la misma fila de la pagina anterior / siguiente
			if ((p & C_IZQ) && colu == 0 && pag > 0) cursor = (pag - 1) * POR_PAGINA + fila * COLUMNAS + COLUMNAS - 1;
			if ((p & C_DER) && colu == COLUMNAS - 1) cursor = (pag + 1) * POR_PAGINA + fila * COLUMNAS;
			if (cursor >= n) cursor = (cursor / POR_PAGINA > (n - 1) / POR_PAGINA && !(p & C_R1)) ? antes : n - 1;
			if (cursor < 0) cursor = std::min(antes % POR_PAGINA, n - 1);
		}
		if (cursor != antes) tCursor = 0.0f;
		if (p & (C_L2 | C_R2)) {
			filtro = (filtro + ((p & C_R2) ? 1 : NUM_FILTROS - 1)) % NUM_FILTROS;
			cursor = 0;
			filtrar();
		}
		if (p & (C_INI_ANT | C_INI_SIG)) {
			inicial = (inicial + ((p & C_INI_SIG) ? 1 : NUM_INICIALES - 1)) % NUM_INICIALES;
			cursor = 0;
			filtrar();
		}
		if (p & C_L3) {
			filtro = inicial = 0;
			cursor = 0;
			filtrar();
		}
		if (p & C_TRIANGULO) {
			verMazo = !verMazo;
			cursor = 0;
			filtrar();
			avisar(T(verMazo ? "Mi mazo: Cuadrado quita una copia" : "Colección"));
		}
		const mazo::Carta* k = elegida();
		if ((p & C_X) && k) {
			std::string motivo;
			if (mazo.agregar(*col, k->id, &motivo)) {
				cambiado = true;
				avisar("+1 " + k->nombre + " (" + std::to_string(mazo.cuantas(k->id)) + "/" + std::to_string(k->limite) + ")");
				if (verMazo) refiltrarConservando(k->id);
			} else {
				avisar(motivo);
			}
		}
		if ((p & C_CUADRADO) && k) {
			const std::string id = k->id, nombre = k->nombre;
			if (mazo.quitar(id)) {
				cambiado = true;
				avisar("-1 " + nombre + " (" + std::to_string(mazo.cuantas(id)) + ")");
				if (verMazo) refiltrarConservando(id);
			} else {
				avisar(T("No está en el mazo"));
			}
		}
		if (p & C_CIRCULO) cerrar();
	}

	void cerrar()
	{
		std::string motivo;
		const bool ok = mazo.valido(*col, &motivo);
		guardadoAlSalir = mazo.guardar(ruta);
		abierto = false;
		if (!ok) fprintf(stderr, "[mazo] guardado pero no valido: %s (el duelo usa el mazo inicial)\n", motivo.c_str());
	}

	// "Mazo: 40/40-60  Fusión: 3/15" y si es valido
	bool valido(std::string* motivo = nullptr) const { return col && mazo.valido(*col, motivo); }

	void avisar(const std::string& s)
	{
		aviso = s;
		tAviso = 0.0f;
	}

private:
	void filtrar()
	{
		lista.clear();
		if (!col) return;
		const char letra = letraInicial(inicial);
		auto pasa = [&](const mazo::Carta& k) {
			if (filtro > 0 && k.clase != CLASE_FILTRO[filtro]) return false;
			if (letra && k.inicial != letra) return false;
			return true;
		};
		if (verMazo) {
			// una celda por carta distinta del mazo (principal y fusion), en el orden de la coleccion
			std::vector<int> v;
			for (const auto* m : { &mazo.principal, &mazo.fusion }) {
				for (const std::string& id : *m) {
					const int i = col->buscar(id);
					if (i >= 0 && std::find(v.begin(), v.end(), i) == v.end()) v.push_back(i);
				}
			}
			std::sort(v.begin(), v.end());
			for (int i : v) {
				if (pasa(col->cartas[i])) lista.push_back(i);
			}
		} else {
			for (int i = 0; i < (int)col->cartas.size(); i++) {
				if (pasa(col->cartas[i])) lista.push_back(i);
			}
		}
		if (cursor >= (int)lista.size()) cursor = std::max(0, (int)lista.size() - 1);
		tCursor = 0.0f;
	}
	void refiltrarConservando(const std::string& id)
	{
		const int antes = cursor;
		filtrar();
		const int i = col->buscar(id);
		auto it = std::find(lista.begin(), lista.end(), i);
		cursor = it != lista.end() ? (int)(it - lista.begin()) : std::min(antes, std::max(0, (int)lista.size() - 1));
	}
};

}   // namespace constructor
