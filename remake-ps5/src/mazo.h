/*
 * Yugioh Classic Remaster - la coleccion y el mazo del jugador (rama classic-constructor). SOLO logica: sin Vulkan ni
 * la interfaz (la pantalla del constructor esta en constructor.h y su dibujo en constructor_ui.h).
 *
 * Datos (assets/ui/, los prepara la rama classic-datos):
 *   cartas_ui.txt        id|nombre|clase|ATRIBUTO|raza|nivel|atk|def|texto   (clase normal/efecto/fusion/ritual/magia/trampa)
 *   cartas_classic.txt   id|passcode|limite|subtipo  (limite 0 prohibida, 1 limitada, 2 semilimitada, 3; sin linea o
 *                        sin archivo: 3). Lista prohibida/limitada del Formato Goat (abril 2005)
 *
 * Coleccion: el jugador tiene de cada carta tantas copias como permite su limite (3, o 2/1/0 segun la lista).
 * Mazo: principal 40-60 cartas (sin fusiones) y mazo de fusion aparte, hasta 15 (DECISION: en 2005 no habia tope
 * oficial para el mazo de fusion; se usa 15 para que quepa en la pantalla y siga la regla posterior). Las copias de
 * una carta (principal + fusion) no pasan de su limite.
 * El mazo se guarda en texto plano (una linea por carta: "principal <id> <n>" o "fusion <id> <n>"), ver rutaMazo().
 */
#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unordered_map>
#include <vector>

#include "constructor_textos.h"   // textos ES/EN (T)

namespace mazo {

using constructor::T;

constexpr int MIN_PRINCIPAL = 40, MAX_PRINCIPAL = 60, MAX_FUSION = 15, MANO_INICIAL = 5;

struct Carta {
	std::string id, nombre, clase, atributo, raza, texto, subtipo, passcode;
	int nivel{ 0 }, atk{ 0 }, def{ 0 }, limite{ 3 };
	// rama classic-personajes: carta firma de un personaje (cartas_classic.txt ...|firma|duenno,duenno). Bloqueada hasta
	// vencer a uno de sus duennos en el modo historia: mientras, limite = 0 (no se puede usar)
	bool firma{ false }, bloqueada{ false };
	int limiteBase{ 3 };
	std::vector<std::string> duennos;
	char inicial{ '#' };   // A..Z (sin tildes) o '#'
};

inline std::vector<std::string> partir(const std::string& linea, char sep = '|')
{
	std::vector<std::string> c;
	size_t a = 0, b;
	while ((b = linea.find(sep, a)) != std::string::npos) {
		c.push_back(linea.substr(a, b - a));
		a = b + 1;
	}
	c.push_back(linea.substr(a));
	return c;
}

// la primera letra del nombre, sin tilde y en mayuscula (UTF-8 de las vocales con tilde, la enie y la dieresis)
inline char inicialDe(const std::string& n)
{
	for (size_t i = 0; i < n.size(); i++) {
		unsigned char c = (unsigned char)n[i];
		if (c >= 'a' && c <= 'z') return (char)(c - 32);
		if (c >= 'A' && c <= 'Z') return (char)c;
		if (c >= '0' && c <= '9') return '#';
		if (c == 0xC3 && i + 1 < n.size()) {
			const unsigned char d = (unsigned char)n[i + 1] | 0x20;   // (mayuscula -> minuscula: +0x20)
			if (d == 0xA1) return 'A';
			if (d == 0xA9) return 'E';
			if (d == 0xAD) return 'I';
			if (d == 0xB3) return 'O';
			if (d == 0xBA || d == 0xBC) return 'U';
			if (d == 0xB1) return 'N';
			return '#';
		}
		if (c == 0xC2) { i++; continue; }   // signos de apertura (¡, ¿)
		if (c == '"' || c == '\'' || c == ' ' || c == '(' || c == '-') continue;
		return '#';
	}
	return '#';
}

// orden de las clases en la coleccion y en el filtro
inline int ordenClase(const std::string& c)
{
	static const char* const C[] = { "normal", "efecto", "ritual", "fusion", "magia", "trampa" };
	for (int i = 0; i < 6; i++) {
		if (c == C[i]) return i;
	}
	return 6;
}
inline bool esMonstruo(const std::string& clase) { return clase == "normal" || clase == "efecto" || clase == "ritual" || clase == "fusion"; }

class Coleccion {
public:
	std::vector<Carta> cartas;   // ordenadas: por clase y por nombre

	int version{ -1 };   // versionIdioma() con que se leyo (al cambiar el idioma se vuelve a leer)
	// rama classic-personajes: personajes vencidos en el modo historia (desbloquean sus cartas firma) y el nombre de
	// un personaje en el idioma actual (para "Vence a X"); los pone el juego (duelo_juego.h)
	std::set<std::string> vencidos;
	std::function<std::string(const std::string&)> nombrePersonaje;

	// aplica los bloqueos de las cartas firma (tras leer y cada vez que cambian los vencidos)
	void desbloquear(const std::set<std::string>& v)
	{
		vencidos = v;
		int bloq = 0;
		for (Carta& k : cartas) {
			if (!k.firma) continue;
			k.bloqueada = true;
			for (const std::string& d : k.duennos)
				if (vencidos.count(d)) k.bloqueada = false;
			k.limite = k.bloqueada ? 0 : k.limiteBase;
			bloq += k.bloqueada ? 1 : 0;
		}
		fprintf(stderr, "[mazo] cartas firma bloqueadas: %d\n", bloq);
	}
	// "Vence a X" (o a X / Y) de una carta firma bloqueada
	std::string venceA(const Carta& k) const
	{
		std::string r;
		for (const std::string& d : k.duennos) {
			if (!r.empty()) r += " / ";
			r += nombrePersonaje ? nombrePersonaje(d) : d;
		}
		return T("Vence a ") + r;
	}

	// lee cartas_ui.txt y cartas_classic.txt de la carpeta ui (con "/" al final); en ingles los textos salen de
	// cartas_ui_en.txt (la carta que falte ahi queda en espanol)
	bool leer(const std::string& carpetaUi)
	{
		cartas.clear();
		indice.clear();
		version = constructor::versionIdioma();
		std::string l;
		for (int pasada = 0; pasada < 2; pasada++) {
			const std::string suf = pasada ? constructor::sufijoIdioma() : "";
			if (pasada && suf.empty()) break;
			std::ifstream f(carpetaUi + "cartas_ui" + suf + ".txt");
			while (std::getline(f, l)) {
				if (!l.empty() && l.back() == '\r') l.pop_back();
				if (l.empty() || l[0] == '#') continue;
				const std::vector<std::string> c = partir(l);
				if (c.size() < 9 || c[0].empty()) continue;
				if (pasada) {   // (ingles: solo los textos de las que ya estan)
					auto it = indice.find(c[0]);
					if (it == indice.end()) continue;
					Carta& k = cartas[it->second];
					k.nombre = c[1]; k.atributo = c[3]; k.raza = c[4]; k.texto = c[8];
					k.inicial = inicialDe(k.nombre);
					continue;
				}
				if (indice.count(c[0])) continue;
				Carta k;
				k.id = c[0]; k.nombre = c[1]; k.clase = c[2]; k.atributo = c[3]; k.raza = c[4];
				k.nivel = atoi(c[5].c_str()); k.atk = atoi(c[6].c_str()); k.def = atoi(c[7].c_str()); k.texto = c[8];
				k.inicial = inicialDe(k.nombre);
				indice[k.id] = (int)cartas.size();
				cartas.push_back(std::move(k));
			}
		}
		std::ifstream g(carpetaUi + "cartas_classic.txt");
		int limitadas = 0;
		while (std::getline(g, l)) {
			if (!l.empty() && l.back() == '\r') l.pop_back();
			if (l.empty() || l[0] == '#') continue;
			const std::vector<std::string> c = partir(l);
			auto it = indice.find(c[0]);
			if (it == indice.end()) continue;
			Carta& k = cartas[it->second];
			if (c.size() > 1) k.passcode = c[1];
			if (c.size() > 2 && !c[2].empty()) k.limite = std::clamp(atoi(c[2].c_str()), 0, 3);
			if (c.size() > 3) k.subtipo = c[3];
			k.limiteBase = k.limite;
			if (c.size() > 5 && c[4] == "firma") {
				k.firma = true;
				k.duennos = partir(c[5], ',');
			}
			limitadas += k.limite < 3 ? 1 : 0;
		}
		std::stable_sort(cartas.begin(), cartas.end(), [](const Carta& a, const Carta& b) {
			const int ca = ordenClase(a.clase), cb = ordenClase(b.clase);
			return ca != cb ? ca < cb : a.nombre < b.nombre;
		});
		indice.clear();
		for (size_t i = 0; i < cartas.size(); i++) indice[cartas[i].id] = (int)i;
		fprintf(stderr, "[mazo] coleccion: %d cartas (%d con limite < 3)\n", (int)cartas.size(), limitadas);
		desbloquear(vencidos);   // (rama classic-personajes: cartas firma)
		return !cartas.empty();
	}

	int buscar(const std::string& id) const
	{
		auto it = indice.find(id);
		return it == indice.end() ? -1 : it->second;
	}
	const Carta* carta(const std::string& id) const
	{
		const int n = buscar(id);
		return n >= 0 ? &cartas[n] : nullptr;
	}
	// copias que tiene el jugador (= su limite en el Formato Goat)
	int copias(const std::string& id) const
	{
		const Carta* k = carta(id);
		return k ? k->limite : 0;
	}
	bool esFusion(const std::string& id) const
	{
		const Carta* k = carta(id);
		return k && k->clase == "fusion";
	}

private:
	std::unordered_map<std::string, int> indice;
};

// generador simple (sin <random>: igual en el PC y en la consola)
struct Azar {
	uint32_t s{ 0x9E3779B9u };
	explicit Azar(uint32_t semilla = 0x9E3779B9u) : s(semilla ? semilla : 1u) {}
	uint32_t siguiente()
	{
		s ^= s << 13;
		s ^= s >> 17;
		s ^= s << 5;
		return s;
	}
	int hasta(int n) { return n > 0 ? (int)(siguiente() % (uint32_t)n) : 0; }
};

template <class T> void barajar(std::vector<T>& v, Azar& a)
{
	for (int i = (int)v.size() - 1; i > 0; i--) std::swap(v[i], v[a.hasta(i + 1)]);
}

struct Mazo {
	std::vector<std::string> principal, fusion;   // una entrada por copia

	int cuantas(const std::string& id) const
	{
		return (int)(std::count(principal.begin(), principal.end(), id) + std::count(fusion.begin(), fusion.end(), id));
	}
	// se puede agregar una copia mas? (motivo: por que no)
	bool sePuedeAgregar(const Coleccion& col, const std::string& id, std::string* motivo = nullptr) const
	{
		const Carta* k = col.carta(id);
		auto no = [&](const std::string& m) {
			if (motivo) *motivo = m;
			return false;
		};
		if (!k) return no(T("Carta desconocida"));
		if (k->bloqueada) return no(T("Carta firma bloqueada") + ": " + col.venceA(*k));
		if (k->limite == 0) return no(T("Prohibida en el Formato Goat"));
		if (cuantas(id) >= k->limite) return no(T(k->limite == 1 ? "Limitada: 1 copia" : k->limite == 2 ? "Semilimitada: 2 copias" : "Máximo 3 copias"));
		if (k->clase == "fusion") {
			if ((int)fusion.size() >= MAX_FUSION) return no(T("Mazo de fusión lleno (15)"));
		} else if ((int)principal.size() >= MAX_PRINCIPAL) {
			return no(T("Mazo principal lleno (60)"));
		}
		return true;
	}
	bool agregar(const Coleccion& col, const std::string& id, std::string* motivo = nullptr)
	{
		if (!sePuedeAgregar(col, id, motivo)) return false;
		(col.esFusion(id) ? fusion : principal).push_back(id);
		ordenar(col);
		return true;
	}
	bool quitar(const std::string& id)
	{
		for (auto* v : { &principal, &fusion }) {
			auto it = std::find(v->begin(), v->end(), id);
			if (it != v->end()) {
				v->erase(it);
				return true;
			}
		}
		return false;
	}
	// por clase y nombre, como la coleccion
	void ordenar(const Coleccion& col)
	{
		auto cmp = [&](const std::string& a, const std::string& b) { return col.buscar(a) < col.buscar(b); };
		std::stable_sort(principal.begin(), principal.end(), cmp);
		std::stable_sort(fusion.begin(), fusion.end(), cmp);
	}
	// valido para jugar? (motivo: el primer problema)
	bool valido(const Coleccion& col, std::string* motivo = nullptr) const
	{
		auto no = [&](const std::string& m) {
			if (motivo) *motivo = m;
			return false;
		};
		const int n = (int)principal.size();
		if (n < MIN_PRINCIPAL) return no(T("Faltan ") + std::to_string(MIN_PRINCIPAL - n) + T(" cartas (mínimo 40)"));
		if (n > MAX_PRINCIPAL) return no(T("Sobran ") + std::to_string(n - MAX_PRINCIPAL) + T(" cartas (máximo 60)"));
		if ((int)fusion.size() > MAX_FUSION) return no(T("Mazo de fusión: máximo 15"));
		std::unordered_map<std::string, int> c;
		for (const std::string& id : principal) {
			if (col.esFusion(id)) return no(T("Una fusión en el mazo principal"));
			c[id]++;
		}
		for (const std::string& id : fusion) {
			if (!col.esFusion(id)) return no(T("Una carta que no es fusión en el mazo de fusión"));
			c[id]++;
		}
		for (const auto& p : c) {
			const Carta* k = col.carta(p.first);
			if (!k) return no(T("Carta desconocida: ") + p.first);
			if (p.second > k->limite) return no(k->nombre + T(": más copias de las permitidas"));
		}
		return true;
	}

	// ---- archivo
	bool guardar(const std::string& ruta) const
	{
		FILE* f = fopen(ruta.c_str(), "w");
		if (!f) {
			fprintf(stderr, "[mazo] no se pudo escribir %s\n", ruta.c_str());
			return false;
		}
		fprintf(f, "# Yugioh Classic Remaster - mazo del jugador (principal %d, fusion %d)\n", (int)principal.size(), (int)fusion.size());
		for (int lado = 0; lado < 2; lado++) {
			const std::vector<std::string>& v = lado ? fusion : principal;
			for (size_t i = 0; i < v.size();) {
				size_t j = i;
				while (j < v.size() && v[j] == v[i]) j++;
				fprintf(f, "%s %s %d\n", lado ? "fusion" : "principal", v[i].c_str(), (int)(j - i));
				i = j;
			}
		}
		fclose(f);
		fprintf(stderr, "[mazo] guardado en %s (%d + %d)\n", ruta.c_str(), (int)principal.size(), (int)fusion.size());
		return true;
	}
	// solo las cartas que estan en la coleccion (y donde corresponden: las fusiones al mazo de fusion)
	bool cargar(const std::string& ruta, const Coleccion& col)
	{
		std::ifstream f(ruta);
		if (!f) return false;
		principal.clear();
		fusion.clear();
		std::string l;
		while (std::getline(f, l)) {
			if (l.empty() || l[0] == '#') continue;
			std::istringstream s(l);
			std::string lado, id;
			int n = 1;
			if (!(s >> lado >> id)) continue;
			s >> n;
			if (col.buscar(id) < 0) continue;
			for (int k = 0; k < std::clamp(n, 0, 3); k++) (col.esFusion(id) ? fusion : principal).push_back(id);
		}
		ordenar(col);
		fprintf(stderr, "[mazo] cargado de %s (%d + %d)\n", ruta.c_str(), (int)principal.size(), (int)fusion.size());
		return true;
	}

	// ---- mazos armados por el juego
	// El mazo inicial: monstruos normales y de efecto de mas ataque, luego magias y trampas, hasta 40 (con las copias
	// que permite cada limite) y todas las fusiones que entren. Si la coleccion no alcanza, lo que haya
	static Mazo porDefecto(const Coleccion& col)
	{
		Mazo m;
		std::vector<const Carta*> mons, mt;
		for (const Carta& k : col.cartas) {
			if (k.limite == 0) continue;
			if (k.clase == "fusion") {
				if ((int)m.fusion.size() < MAX_FUSION) m.fusion.push_back(k.id);
			} else if (k.clase == "normal" || k.clase == "efecto") {
				if (k.nivel <= 4) mons.push_back(&k);   // (sin tributos: se puede jugar sin motor de reglas)
			} else if (k.clase == "magia" || k.clase == "trampa") {
				mt.push_back(&k);
			}
		}
		std::stable_sort(mons.begin(), mons.end(), [](const Carta* a, const Carta* b) { return a->atk > b->atk; });
		auto llenar = [&](const std::vector<const Carta*>& v, int hasta) {
			for (int ronda = 1; ronda <= 3; ronda++) {   // una copia de cada, despues la segunda, la tercera
				for (const Carta* k : v) {
					if ((int)m.principal.size() >= hasta) return;
					if (k->limite >= ronda) m.principal.push_back(k->id);
				}
			}
		};
		llenar(mons, 24);
		llenar(mt, MIN_PRINCIPAL);
		// si faltan (pocas magias/trampas): mas monstruos, y si aun faltan, los de nivel alto
		llenar(mons, MIN_PRINCIPAL);
		if ((int)m.principal.size() < MIN_PRINCIPAL) {
			std::vector<const Carta*> resto;
			for (const Carta& k : col.cartas) {
				if (k.limite > 0 && k.clase != "fusion" && std::find(mons.begin(), mons.end(), &k) == mons.end() &&
				    std::find(mt.begin(), mt.end(), &k) == mt.end()) resto.push_back(&k);
			}
			llenar(resto, MIN_PRINCIPAL);
		}
		m.ordenar(col);
		return m;
	}
	// Un mazo aleatorio valido de 40 (el del rival): ~22 monstruos y el resto magias y trampas, sin pasar los limites
	static Mazo aleatorio(const Coleccion& col, Azar& a)
	{
		Mazo m;
		std::vector<std::string> mons, mt, fus;
		for (const Carta& k : col.cartas) {
			for (int c = 0; c < k.limite; c++) {
				if (k.clase == "fusion") fus.push_back(k.id);
				else if (k.clase == "normal" || k.clase == "efecto") (k.nivel <= 6 ? mons : mt).push_back(k.id);
				else if (k.clase == "magia" || k.clase == "trampa") mt.push_back(k.id);
				else mt.push_back(k.id);   // (rituales: con las magias, pocas)
			}
		}
		barajar(mons, a);
		barajar(mt, a);
		barajar(fus, a);
		for (const std::string& id : mons) {
			if ((int)m.principal.size() >= 22) break;
			m.principal.push_back(id);
		}
		for (auto* v : { &mt, &mons }) {
			for (const std::string& id : *v) {
				if ((int)m.principal.size() >= MIN_PRINCIPAL) break;
				if (m.cuantas(id) < col.copias(id)) m.principal.push_back(id);
			}
		}
		for (const std::string& id : fus) {
			if ((int)m.fusion.size() >= 5) break;
			if (m.cuantas(id) < col.copias(id)) m.fusion.push_back(id);
		}
		m.ordenar(col);
		return m;
	}
};

// Donde se guarda el mazo. En la consola: /data/yugioh_classic/mazo.txt (fuera de la carpeta del juego: sobrevive a
// las reinstalaciones); si no se puede crear, junto al juego (<assets>/../mazo.txt = /data/homebrew/<titulo>/). En el
// PC: YUGI_MAZO=<ruta> o ./mazo.txt
inline std::string rutaMazo(const std::string& assets)
{
#if defined(PS5_HOST_REFERENCE)
	(void)assets;
	if (const char* v = getenv("YUGI_MAZO")) return v;
	return "mazo.txt";
#else
	mkdir("/data/yugioh_classic", 0777);
	struct stat st{};
	if (stat("/data/yugioh_classic", &st) == 0 && S_ISDIR(st.st_mode)) return "/data/yugioh_classic/mazo.txt";
	return assets + "../mazo.txt";
#endif
}

}   // namespace mazo
