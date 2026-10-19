// Yugioh Classic Remaster - animaciones de activacion de magias y trampas (rama classic-fx-md).
// Origen: Yu-Gi-Oh! Master Duel (copia del usuario en Steam, uso personal, NADA de eso va al repo). Se investigo
// con UnityPy (herramientas/fx_md/): los efectos de cartas de Master Duel son prefabs con ParticleSystem (Unity) y
// Shader Graph propios, ordenados por un Timeline (duel/timeline/card/tcg/ef<cid>, cid = id de carta de Konami) y los
// genericos del duelo (fxp_bff_apply = aplicar efecto, fxp_cardbrk = carta destruida, ...). herramientas/fx_md/sim_fx.py
// simula esos sistemas de particulas con SUS texturas y parametros y los hornea a flipbook (fx.h, assets/fx/md/).
// El DADO y la MONEDA son mallas 3D de Master Duel (dado_md.glb, moneda_md.glb) con SUS curvas de animacion
// (dice_animation2, coin02_anim3.9: dado_md.txt / moneda_md.txt), aqui reproducidas y con el resultado a la vista.
//  - Tabla efectos_cartas.txt (assets/ui): id_de_carta|efecto (o passcode|efecto). Sin linea: la activacion generica
//    de magia (verde) o de trampa (rosa) segun cartas_classic.txt.
//  - Sin estado de reglas: solo la animacion (el motor de efectos de reglas es otra etapa).
#pragma once

#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "fx.h"

namespace efc {

enum class Receta : int {
	Magia = 0,        // activacion generica de magia
	Trampa,           // activacion generica de trampa
	AgujeroNegro,     // Dark Hole (Master Duel ef04342: remolino galactico) + destruccion de todos los monstruos
	Raigeki,          // rayos sobre los monstruos del rival + destruccion
	FuerzaEspejo,     // Mirror Force: barrera + destruccion de los monstruos del rival
	Renacer,          // Monster Reborn: circulo de invocacion + materializacion
	Olla,             // Pot of Greed: destellos dorados sobre la carta y el mazo
	Espadas,          // Swords of Revealing Light: marcas doradas sobre los monstruos del rival
	Polimerizacion,   // fusion de Master Duel (fxp_somldg_fusion)
	CambioCorazon,    // Change of Heart: aura sobre un monstruo del rival
	TormentaPesada,   // Heavy Storm: tornados sobre todas las magias/trampas
	Tifon,            // Mystical Space Typhoon (Master Duel ef04909: tornado) sobre una magia/trampa rival
	CilindroMagico,   // Magic Cylinder
	Plumero,          // Harpie's Feather Duster (Master Duel ef04678: plumas doradas)
	MilCuchillos,     // Thousand Knives (Master Duel ef05166)
	Ritual,           // rituales (fxp_somldg_ritual)
	Dado,             // tirada de dado 3D (Graceful Dice, Skull Dice...)
	Moneda,           // lanzamiento de moneda 3D (Fairy Box, Second Coin Toss...)
	NUM
};

inline const char* nombre(Receta r)
{
	static const char* const N[] = { "magia", "trampa", "agujero_negro", "raigeki", "fuerza_espejo", "renacer", "olla", "espadas",
		                             "polimerizacion", "cambio_corazon", "tormenta_pesada", "tifon", "cilindro_magico", "plumero",
		                             "mil_cuchillos", "ritual", "dado", "moneda" };
	return N[(int)r];
}

// cuanto dura cada animacion (s)
inline float duracion(Receta r)
{
	switch (r) {
	case Receta::AgujeroNegro: return 3.2f;
	case Receta::Raigeki: return 2.4f;
	case Receta::FuerzaEspejo: return 2.6f;
	case Receta::TormentaPesada: return 2.4f;
	case Receta::Dado: return 4.3f;
	case Receta::Moneda: return 4.6f;
	case Receta::Espadas: return 3.2f;
	default: return 2.4f;
	}
}

struct Activo {
	Receta receta{ Receta::Magia };
	std::string carta;
	int zona{ -1 };
	bool rival{ false };
	float t{ 0.0f };
	int resultado{ 0 };    // dado 1..6, moneda 0 cara / 1 cruz
	int objetivo{ -1 };    // zona elegida (Change of Heart, MST, Thousand Knives)
};

// Las curvas del dado / moneda de Master Duel (t px py pz qx qy qz qw a 30 por segundo)
struct Curva {
	std::vector<float> d;   // 8 por muestra
	float fin{ 0.0f };
	bool cargar(const std::string& ruta)
	{
		std::ifstream f(ruta);
		if (!f) return false;
		std::string l;
		while (std::getline(f, l)) {
			if (l.empty() || l[0] == '#') continue;
			std::istringstream s(l);
			float v[8];
			for (float& x : v) s >> x;
			if (!s) continue;
			d.insert(d.end(), v, v + 8);
		}
		if (d.empty()) return false;
		fin = d[d.size() - 8];
		return true;
	}
	bool vacia() const { return d.empty(); }
	void muestra(float t, glm::vec3& p, glm::quat& q) const
	{
		const size_t n = d.size() / 8;
		t = glm::clamp(t, 0.0f, fin);
		size_t i = 0;
		while (i + 1 < n && d[(i + 1) * 8] <= t) i++;
		const size_t j = glm::min(i + 1, n - 1);
		const float t0 = d[i * 8], t1 = d[j * 8];
		const float f = t1 > t0 ? (t - t0) / (t1 - t0) : 0.0f;
		const float* a = &d[i * 8];
		const float* b = &d[j * 8];
		p = glm::mix(glm::vec3(a[1], a[2], a[3]), glm::vec3(b[1], b[2], b[3]), f);
		const glm::quat qa(a[7], a[4], a[5], a[6]), qb(b[7], b[4], b[5], b[6]);
		q = glm::normalize(glm::slerp(qa, qb, f));
	}
};

class Motor {
public:
	std::vector<Activo> activos;
	Curva dado, moneda;
	bool log{ false };

	// carpetaUi: efectos_cartas.txt y cartas_classic.txt; carpetaFx: dado_md.txt y moneda_md.txt
	void cargar(const std::string& carpetaUi, const std::string& carpetaFx)
	{
		log = getenv("YUGI_FX_LOG") != nullptr;
		std::ifstream c(carpetaUi + "cartas_classic.txt");
		std::string l;
		while (std::getline(c, l)) {
			std::vector<std::string> k = partir(l);
			if (k.size() >= 4) {
				subtipo[k[0]] = k[3];
				porPasscode[k[1]] = k[0];
			}
		}
		std::ifstream t(carpetaUi + "efectos_cartas.txt");
		int n = 0;
		while (std::getline(t, l)) {
			if (l.empty() || l[0] == '#') continue;
			std::vector<std::string> k = partir(l);
			if (k.size() < 2) continue;
			std::string id = k[0];
			if (porPasscode.count(id)) id = porPasscode[id];
			for (int r = 0; r < (int)Receta::NUM; r++) {
				if (k[1] == nombre((Receta)r)) {
					tabla[id] = (Receta)r;
					n++;
				}
			}
		}
		const bool d = dado.cargar(carpetaFx + "dado_md.txt");
		const bool m = moneda.cargar(carpetaFx + "moneda_md.txt");
		fprintf(stderr, "[efectos] %d cartas con animacion propia (efectos_cartas.txt), %zu subtipos; dado %s, moneda %s\n", n, subtipo.size(),
		        d ? "si" : "NO", m ? "si" : "NO");
	}

	Receta receta(const std::string& carta) const
	{
		auto it = tabla.find(carta);
		if (it != tabla.end()) return it->second;
		auto s = subtipo.find(carta);
		if (s != subtipo.end() && s->second.rfind("trampa", 0) == 0) return Receta::Trampa;
		if (s != subtipo.end() && s->second == "magia_ritual") return Receta::Ritual;
		return Receta::Magia;
	}

	// una magia/trampa se activo en la zona (de cualquier lado). El mismo pedido dos veces seguidas (el control y el
	// duelo avisan los dos) se ignora
	void activar(const std::string& carta, int zona, bool rival, uint32_t semilla)
	{
		for (const Activo& a : activos) {
			if (a.zona == zona && a.carta == carta && a.t < 0.8f) return;
		}
		Activo a;
		a.receta = receta(carta);
		a.carta = carta;
		a.zona = zona;
		a.rival = rival;
		semilla = semilla * 1664525u + 1013904223u;
		a.resultado = a.receta == Receta::Dado ? 1 + (int)((semilla >> 8) % 6u) : (int)((semilla >> 9) & 1u);
		activos.push_back(a);
		if (log) fprintf(stderr, "[efectos] %s (zona %d, %s): %s\n", carta.c_str(), zona, rival ? "rival" : "jugador", nombre(a.receta));
	}

	void avanzar(float dt)
	{
		for (Activo& a : activos) a.t += dt;
		std::vector<Activo> quedan;
		for (Activo& a : activos) {
			if (a.t < duracion(a.receta)) quedan.push_back(a);
		}
		activos.swap(quedan);
	}

	bool hay() const { return !activos.empty(); }

	// Lo que dibuja cada activacion con los flipbooks. centro(z): centro de una zona en el mundo; ocupada(z): hay carta;
	// zonas: los indices de monstruo y de magia de cada lado (0 jugador, 1 rival)
	void pedir(fx::Flipbooks& fb, const std::function<glm::vec3(int)>& centro, const std::function<bool(int)>& ocupada,
	           const std::vector<int> monstruos[2], const std::vector<int> magias[2], const int mazos[2]) const
	{
		using fx::Fx;
		const glm::vec3 arriba(0.0f, 1.0f, 0.0f);
		for (const Activo& a : activos) {
			const float t = a.t;
			const glm::vec3 c = a.zona >= 0 ? centro(a.zona) : glm::vec3(0.0f);
			const int yo = a.rival ? 1 : 0, otro = 1 - yo;
			const float fin = duracion(a.receta);
			const float sale = 1.0f - glm::smoothstep(fin - 0.35f, fin, t);   // todo se apaga al final
			// la carta que se activa siempre brilla (verde la magia, rosa la trampa, como el marco de la carta)
			const bool trampa = a.receta == Receta::Trampa || a.receta == Receta::FuerzaEspejo || a.receta == Receta::CilindroMagico ||
			                    subtipoDe(a.carta).rfind("trampa", 0) == 0;
			const glm::vec3 tinte = trampa ? glm::vec3(1.9f, 0.55f, 1.35f) : glm::vec3(0.55f, 1.9f, 1.0f);
			fb.pedir(Fx::MdActivar, c + arriba * 0.15f, 3.0f, t, glm::vec4(tinte, sale));
			fb.pedir(Fx::Aparicion, c + arriba * 0.06f, 2.4f, t, glm::vec4(tinte * 0.8f, sale));
			// el centro de la mesa, el del lado del rival (de quien activa) y el propio
			const glm::vec3 medio(0.0f, 0.0f, 0.0f);
			const glm::vec3 ladoOtro = centro(monstruos[otro][2]);
			const glm::vec3 ladoYo = centro(monstruos[yo][2]);
			switch (a.receta) {
			case Receta::AgujeroNegro: {
				// el remolino se abre plano sobre la mesa (md_agujero_negro con vista "arriba"), traga todo y se cierra
				const float abre = glm::smoothstep(0.2f, 0.7f, t) * (1.0f - glm::smoothstep(2.4f, 3.0f, t));
				fb.pedir(Fx::MdAgujeroNegro, medio + glm::vec3(0.0f, 0.05f, 0.0f), 15.0f * glm::max(abre, 0.01f), fmodf(glm::max(t - 0.2f, 0.0f), 2.1f),
				         glm::vec4(1.0f, 1.0f, 1.0f, abre));
				for (int lado = 0; lado < 2; lado++) {
					for (int z : monstruos[lado]) {
						if (ocupada(z)) fb.pedir(Fx::MdDestruir, centro(z) + arriba * 0.3f, 3.2f, t - 1.6f, glm::vec4(1.0f));
					}
				}
				break;
			}
			case Receta::Raigeki:
				for (size_t k = 0; k < monstruos[otro].size(); k++) {
					const int z = monstruos[otro][k];
					const float d = 0.35f + 0.12f * (float)k;   // (los rayos los dibuja yugireload.cpp: dibujarDadoMoneda)
					fb.pedir(Fx::Tesla, centro(z) + arriba * 0.1f, 3.4f, t - d, glm::vec4(1.4f, 1.4f, 2.2f, sale), arriba);
					fb.pedir(Fx::Electricidad, centro(z) + arriba * 1.2f, 2.0f, t - d, glm::vec4(1.2f, 1.3f, 2.0f, sale));
					fb.pedir(Fx::Chispas, centro(z) + arriba * 0.3f, 2.4f, t - d, glm::vec4(1.2f, 1.3f, 2.0f, 1.0f));
					if (ocupada(z)) fb.pedir(Fx::MdDestruir, centro(z) + arriba * 0.3f, 3.2f, t - d - 0.5f, glm::vec4(1.0f));
				}
				break;
			case Receta::FuerzaEspejo: {
				// la barrera se levanta entre los dos lados y devuelve el golpe
				const glm::vec3 barrera = glm::mix(ladoYo, ladoOtro, 0.35f) + arriba * 1.4f;
				fb.pedir(Fx::MdActivar, barrera, 9.0f, t * 0.8f, glm::vec4(1.2f, 1.6f, 2.2f, sale));
				fb.pedir(Fx::MdFusionMarco, barrera, 8.0f, t - 0.2f, glm::vec4(0.8f, 1.4f, 2.0f, sale));
				for (int z : monstruos[otro]) {
					if (ocupada(z)) fb.pedir(Fx::MdDestruir, centro(z) + arriba * 0.3f, 3.2f, t - 0.9f, glm::vec4(1.0f));
				}
				break;
			}
			case Receta::Renacer: {
				const glm::vec3 z = centro(monstruos[yo][2]);
				fb.pedir(Fx::MdRitual, z, 4.2f, t - 0.2f, glm::vec4(1.0f, 1.2f, 1.4f, sale));
				fb.pedir(Fx::TeleportIn, z, 3.0f, t - 0.9f, glm::vec4(0.9f, 1.2f, 1.6f, sale));
				break;
			}
			case Receta::Olla:
				fb.pedir(Fx::MdPlumero, c + arriba * 0.8f, 3.2f, t, glm::vec4(1.4f, 1.2f, 0.7f, sale));
				fb.pedir(Fx::Aparicion, centro(mazos[yo]) + arriba * 0.1f, 2.4f, t - 0.8f, glm::vec4(1.6f, 1.3f, 0.6f, sale));
				fb.pedir(Fx::Aparicion, centro(mazos[yo]) + arriba * 0.1f, 2.4f, t - 1.3f, glm::vec4(1.6f, 1.3f, 0.6f, sale));
				break;
			case Receta::Espadas:
				for (int z : monstruos[otro]) {
					fb.pedir(Fx::MdPlumero, centro(z) + arriba * 1.4f, 2.6f, t - 0.3f, glm::vec4(1.5f, 1.3f, 0.6f, sale));
					if (ocupada(z)) fb.pedir(Fx::Marca, centro(z), 2.6f, t - 0.5f, glm::vec4(1.6f, 1.3f, 0.5f, sale));
				}
				break;
			case Receta::Polimerizacion:
				fb.pedir(Fx::MdFusion, c + arriba * 0.2f, 4.0f, t - 0.1f, glm::vec4(1.0f, 1.0f, 1.0f, sale));
				fb.pedir(Fx::MdFusionMarco, ladoYo + arriba * 0.1f, 3.4f, fmodf(t, 0.4f), glm::vec4(1.0f, 1.0f, 1.0f, sale));
				break;
			case Receta::CambioCorazon:
				if (a.objetivo >= 0 || primeraOcupada(monstruos[otro], ocupada) >= 0) {
					const int z = a.objetivo >= 0 ? a.objetivo : primeraOcupada(monstruos[otro], ocupada);
					fb.pedir(Fx::MdActivar, centro(z) + arriba * 0.3f, 3.4f, t - 0.4f, glm::vec4(2.0f, 0.6f, 1.2f, sale));
					fb.pedir(Fx::Aura, centro(z), 2.6f, t - 0.6f, glm::vec4(1.8f, 0.6f, 1.2f, sale));
				}
				break;
			case Receta::TormentaPesada:
				for (int lado = 0; lado < 2; lado++) {
					for (int z : magias[lado]) {
						if (z == a.zona) continue;
						fb.pedir(Fx::MdTornado, centro(z), 3.6f, t - 0.25f, glm::vec4(1.0f, 1.0f, 1.0f, sale), arriba);
						if (ocupada(z)) fb.pedir(Fx::MdDestruir, centro(z) + arriba * 0.3f, 3.0f, t - 0.9f, glm::vec4(1.0f));
					}
				}
				break;
			case Receta::Tifon: {
				int z = a.objetivo >= 0 ? a.objetivo : primeraOcupada(magias[otro], ocupada);
				if (z < 0) z = magias[otro][2];
				fb.pedir(Fx::MdTornado, centro(z), 6.5f, t - 0.25f, glm::vec4(1.0f, 1.0f, 1.0f, sale), arriba);
				if (ocupada(z)) fb.pedir(Fx::MdDestruir, centro(z) + arriba * 0.3f, 3.0f, t - 0.9f, glm::vec4(1.0f));
				break;
			}
			case Receta::CilindroMagico: {
				const glm::vec3 boca = ladoYo + arriba * 1.2f;
				fb.pedir(Fx::MdTornado, boca, 2.6f, t - 0.2f, glm::vec4(0.8f, 0.8f, 1.6f, sale), arriba);
				fb.pedir(Fx::ExplosionMediana, ladoOtro + arriba * 0.6f, 3.0f, t - 1.1f, glm::vec4(1.2f, 0.8f, 1.4f, 1.0f));
				break;
			}
			case Receta::Plumero:
				fb.pedir(Fx::MdPlumero, glm::mix(ladoOtro, centro(magias[otro][2]), 0.5f) + arriba * 2.0f, 13.0f, t, glm::vec4(1.3f, 1.2f, 0.8f, sale));
				for (int z : magias[otro]) {
					if (ocupada(z)) fb.pedir(Fx::MdDestruir, centro(z) + arriba * 0.3f, 3.0f, t - 1.2f, glm::vec4(1.0f));
				}
				break;
			case Receta::MilCuchillos: {
				int z = a.objetivo >= 0 ? a.objetivo : primeraOcupada(monstruos[otro], ocupada);
				if (z < 0) z = monstruos[otro][2];
				fb.pedir(Fx::MdCuchillos, centro(z) + arriba * 1.6f, 7.5f, t - 0.2f, glm::vec4(1.0f, 1.0f, 1.0f, sale));
				if (ocupada(z)) fb.pedir(Fx::MdDestruir, centro(z) + arriba * 0.3f, 3.2f, t - 1.0f, glm::vec4(1.0f));
				break;
			}
			case Receta::Ritual:
				fb.pedir(Fx::MdRitual, ladoYo, 4.4f, t - 0.1f, glm::vec4(1.0f, 1.0f, 1.0f, sale));
				break;
			case Receta::Dado:
			case Receta::Moneda:
				// el objeto 3D lo dibuja yugireload.cpp (matrizObjeto); aqui el polvo al caer
				fb.pedir(Fx::Polvo, ladoYo + arriba * 0.05f, 1.6f, t - (a.receta == Receta::Dado ? 0.55f : 2.55f), glm::vec4(1.0f, 1.0f, 1.0f, 0.7f));
				break;
			default: break;
			}
		}
	}

	// matriz del dado o la moneda de una activacion (false si no le toca). base: donde rueda (el centro del lado de
	// quien activa, sobre la mesa); ojo: la camara. Unidades de Master Duel a metros: 0.9 (arista del dado ~1.3 m, la casilla mide ~3.8 m)
	bool matrizObjeto(const Activo& a, glm::vec3 base, glm::vec3 ojo, glm::mat4& m) const
	{
		const bool esDado = a.receta == Receta::Dado;
		if (!esDado && a.receta != Receta::Moneda) return false;
		const Curva& cv = esDado ? dado : moneda;
		if (cv.vacia()) return false;
		const float u = 0.9f;
		const float t = a.t;
		glm::vec3 p;
		glm::quat q;
		cv.muestra(glm::min(t, cv.fin), p, q);
		p.z = glm::min(p.z, 2.5f);   // al final Master Duel lo acerca a su camara: aqui solo un poco
		// marco: Y arriba, Z hacia la camara (en el plano de la mesa)
		glm::vec3 zc = ojo - base;
		zc.y = 0.0f;
		zc = glm::length(zc) > 1e-3f ? glm::normalize(zc) : glm::vec3(0.0f, 0.0f, 1.0f);
		const glm::vec3 yc(0.0f, 1.0f, 0.0f);
		const glm::vec3 xc = glm::normalize(glm::cross(yc, zc));
		glm::mat4 marco(1.0f);
		marco[0] = glm::vec4(xc, 0.0f);
		marco[1] = glm::vec4(yc, 0.0f);
		marco[2] = glm::vec4(zc, 0.0f);
		marco[3] = glm::vec4(base, 1.0f);
		// resultado: la cara pedida queda arriba (dado) o mirando a la camara (moneda)
		glm::quat res(1.0f, 0.0f, 0.0f, 0.0f);
		if (esDado) {
			// caras de la malla (dado_md.glb, medidas con herramientas/fx_md/blender/caras_glb.py): 1 +Y (la estrella:
			// arriba en reposo, como en Master Duel), 6 -Y, 2 +Z, 5 -Z, 3 +X, 4 -X
			static const glm::vec3 N[7] = { {}, { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0, 0 }, { -1, 0, 0 }, { 0, 0, -1 }, { 0, -1, 0 } };
			const glm::vec3 n = N[glm::clamp(a.resultado, 1, 6)];
			const glm::vec3 eje = glm::cross(n, glm::vec3(0.0f, 1.0f, 0.0f));
			if (glm::length(eje) > 1e-4f) {
				res = glm::angleAxis(acosf(glm::clamp(n.y, -1.0f, 1.0f)), glm::normalize(eje));
			} else if (n.y < 0.0f) {
				res = glm::angleAxis(glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
			}
		} else if (a.resultado == 1) {
			res = glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));   // cruz
		}
		// al final: queda quieto mostrando el resultado (el dado se inclina hacia la camara) y se encoge
		float incl = 0.0f, esc = 1.0f;
		if (t > cv.fin) {
			incl = esDado ? glm::smoothstep(cv.fin, cv.fin + 0.35f, t) * glm::radians(55.0f) : 0.0f;
			esc = 1.0f - glm::smoothstep(duracion(a.receta) - 0.4f, duracion(a.receta), t);
		}
		const float alto = esDado ? 0.731f : 0.55f;   // la malla tiene el centro en el medio: que apoye en la mesa
		m = marco * glm::scale(glm::mat4(1.0f), glm::vec3(u * esc)) * glm::translate(glm::mat4(1.0f), p + glm::vec3(0.0f, alto, 0.0f)) *
		    glm::mat4_cast(glm::angleAxis(incl, glm::vec3(1.0f, 0.0f, 0.0f)) * q * res);
		return true;
	}

	// el resultado a la vista en el aviso de la interfaz, al terminar de rodar
	bool resultadoListo(const Activo& a, float dt, std::string& texto) const
	{
		const bool esDado = a.receta == Receta::Dado;
		if (!esDado && a.receta != Receta::Moneda) return false;
		const float fin = (esDado ? dado.fin : moneda.fin) - 0.6f;
		if (!(a.t >= fin && a.t - dt < fin)) return false;
		texto = esDado ? "Dado: " + std::to_string(a.resultado) : std::string(a.resultado == 0 ? "Moneda: cara" : "Moneda: cruz");
		return true;
	}

private:
	std::unordered_map<std::string, Receta> tabla;
	std::unordered_map<std::string, std::string> subtipo, porPasscode;

	std::string subtipoDe(const std::string& carta) const
	{
		auto s = subtipo.find(carta);
		return s == subtipo.end() ? std::string() : s->second;
	}
	static std::vector<std::string> partir(const std::string& l)
	{
		std::vector<std::string> k;
		std::string x;
		std::istringstream s(l);
		while (std::getline(s, x, '|')) {
			while (!x.empty() && (x.back() == '\r' || x.back() == ' ')) x.pop_back();
			k.push_back(x);
		}
		return k;
	}
	static int primeraOcupada(const std::vector<int>& zs, const std::function<bool(int)>& ocupada)
	{
		for (int z : zs) {
			if (ocupada(z)) return z;
		}
		return -1;
	}
};

}   // namespace efc
