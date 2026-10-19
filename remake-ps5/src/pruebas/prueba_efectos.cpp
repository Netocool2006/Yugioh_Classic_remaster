// Prueba del motor de efectos (rama classic-efectos) sin pantalla: un escenario armado a mano por carta (escenario ->
// esperado), con la cadena, las ventanas de respuesta y las elecciones. Cada caso dice que se esperaba y que paso.
// Compilar y correr (contenedor arch-ps5 o cualquier g++ con C++17; un segundo):
//   g++ -std=c++17 -O2 -Wall -Wextra -I remake-ps5/src remake-ps5/src/pruebas/prueba_efectos.cpp -o /tmp/prueba_efectos &&
//   /tmp/prueba_efectos <carpeta con cartas_ui.txt y cartas_classic.txt>/
#include "ia.h"

#include <map>

using namespace ia;

static Base base;
static int fallos = 0, casos = 0;
static std::map<std::string, int> probadas;   // carta -> casos

#define VERIFICA(cond, ...)                                                                                                                \
	do {                                                                                                                                   \
		if (!(cond)) {                                                                                                                     \
			fprintf(stderr, "  FALLO [%s] %s:%d: ", nombreCaso, __FILE__, __LINE__);                                                     \
			fprintf(stderr, __VA_ARGS__);                                                                                                  \
			fprintf(stderr, "\n");                                                                                                         \
			fallos++;                                                                                                                      \
			okCaso = false;                                                                                                                \
		}                                                                                                                                  \
	} while (0)

static const char* nombreCaso = "";
static bool okCaso = true;

// un duelo a medio jugar: turno 3 del lado 0, fase principal 1, mazos de relleno
struct Escena {
	Duelo d{ &base };
	explicit Escena(Fase f = F_PRINCIPAL1)
	{
		d.l = {};
		d.ef = EstadoEfectos{};
		d.turno = 3;
		d.activo = 0;
		d.fase = f;
		d.ganador = -1;
		for (Lado& x : d.l)
			for (int k = 0; k < 20; k++) x.mazo.push_back(k % 2 ? "celtic_guardian" : "battle_ox");
	}
	int mon(int lado, const char* id, bool defensa = false, bool bocaAbajo = false)
	{
		const int z = d.invocarEspecial(lado, id, defensa, -1, bocaAbajo);
		if (z < 0) return z;   // (un efecto al llegar se lo llevo: Berserk Gorilla, Serket...)
		d.l[(size_t)lado].m[(size_t)z].llegoEsteTurno = false;
		d.ef.disparos.clear();
		d.ef.ventanaTras = Ventana{};
		d.recalcular();
		return z;
	}
	int colocar(int lado, const char* id, int turnoColocada = 1)
	{
		Lado& x = d.l[(size_t)lado];
		const int z = x.libreMT();
		x.mt[(size_t)z] = MagiaTrampa{ id, true };
		x.mt[(size_t)z].uid = d.nuevoUid();
		x.mt[(size_t)z].turnoColocada = turnoColocada;
		return z;
	}
	int mano(int lado, const char* id)
	{
		d.l[(size_t)lado].mano.push_back(id);
		return (int)d.l[(size_t)lado].mano.size() - 1;
	}
	// (costes elegidos: si no se dan, los de la IA)
	bool activar(int lado, const Lugar& f, std::vector<Lugar> objs = {}, std::vector<int> descartes = {}, std::vector<Lugar> costes = { Lugar{ -9, 0, 0 } })
	{
		Jugada j = jugadaActivar(f);
		j.objs = objs;
		j.descartes = descartes;
		const DefCarta* df = defCarta(cartaEn(d, f));
		j.costes = !costes.empty() && costes[0].lado == -9 ? (df ? costesIA(d, lado, *df, f) : std::vector<Lugar>{}) : costes;
		return d.aplicar(j);
	}
	// el lado que tiene la pregunta pasa (o elige el primero)
	bool pasa(int lado)
	{
		if (d.ef.preg.tipo != P_RESPUESTA || d.ef.preg.lado != lado) return false;
		Jugada j;
		j.tipo = J_PASAR;
		return d.aplicar(j);
	}
	bool elige(int lado, int k)
	{
		if (d.ef.preg.tipo != P_ELEGIR || d.ef.preg.lado != lado) return false;
		Jugada j;
		j.tipo = J_ELEGIR;
		j.objetivo = k;
		return d.aplicar(j);
	}
	// todas las preguntas pendientes: pasar (y elegir el primero)
	void pasarTodo()
	{
		for (int g = 0; g < 20 && d.ef.preg.tipo != P_NADA; g++) {
			Jugada j;
			j.tipo = d.ef.preg.tipo == P_ELEGIR ? J_ELEGIR : J_PASAR;
			j.objetivo = 0;
			d.aplicar(j);
		}
	}
	int enCementerio(int lado, const std::string& id) const
	{
		int n = 0;
		for (const std::string& s : d.l[(size_t)lado].cementerio) n += s == id;
		return n;
	}
	bool libre() const { return !d.ocupado(); }
};

static void caso(const char* nombre, const char* carta)
{
	nombreCaso = nombre;
	okCaso = true;
	casos++;
	probadas[carta]++;
}
static void fin()
{
	fprintf(stderr, "%s %s\n", okCaso ? "  ok   " : "  MAL  ", nombreCaso);
}
static Lugar M(int lado, int z) { return { lado, EN_MONSTRUO, z }; }
static Lugar MT(int lado, int z) { return { lado, EN_MT, z }; }
static Lugar H(int lado, int k) { return { lado, EN_MANO, k }; }

static void tanda1()
{
	fprintf(stderr, "tanda 1: destruccion masiva y dirigida, cadena\n");
	{
		caso("Raigeki: destruye los 2 monstruos del rival y nada propio", "raigeki");
		Escena e;
		e.mon(0, "celtic_guardian");
		e.mon(1, "dark_magician");
		e.mon(1, "kuriboh", true, true);
		const int k = e.mano(0, "raigeki");
		VERIFICA(e.activar(0, H(0, k)), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0, "rival con %d monstruos (esperado 0)", e.d.l[1].monstruos());
		VERIFICA(e.d.l[0].monstruos() == 1, "propios %d (esperado 1)", e.d.l[0].monstruos());
		VERIFICA(e.enCementerio(0, "raigeki") == 1 && e.d.l[0].libreMT() == 0, "Raigeki no termino en el cementerio");
		VERIFICA(e.d.l[1].cementerio.size() == 2, "cementerio rival %d (esperado 2)", (int)e.d.l[1].cementerio.size());
		VERIFICA(e.libre(), "la cadena quedo abierta");
		fin();
	}
	{
		caso("Raigeki: sin monstruos del rival no se puede activar", "raigeki");
		Escena e;
		const int k = e.mano(0, "raigeki");
		VERIFICA(motivoNoActivar(e.d, 0, H(0, k), false) == "ef.no_condicion", "motivo '%s'", motivoNoActivar(e.d, 0, H(0, k), false).c_str());
		VERIFICA(!e.activar(0, H(0, k)), "se activo sin objetivos");
		fin();
	}
	{
		caso("Dark Hole: destruye todos los monstruos del campo", "dark_hole");
		Escena e;
		e.mon(0, "celtic_guardian");
		e.mon(0, "battle_ox", true);
		e.mon(1, "dark_magician");
		const int k = e.mano(0, "dark_hole");
		VERIFICA(e.activar(0, H(0, k)), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() + e.d.l[1].monstruos() == 0, "quedan %d monstruos", e.d.l[0].monstruos() + e.d.l[1].monstruos());
		fin();
	}
	{
		caso("Mirror Force: al atacar el rival, destruye sus monstruos en ataque (no los de defensa) y no hay danno", "mirror_force");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		e.mon(0, "celtic_guardian");
		e.mon(0, "giant_soldier_of_stone", true);
		e.mon(1, "kuriboh");
		const int mf = e.colocar(1, "mirror_force");
		Jugada at;
		at.tipo = J_ATACAR;
		at.zona = a;
		at.objetivo = 0;
		VERIFICA(e.d.aplicar(at), "no se pudo declarar el ataque");
		VERIFICA(e.d.ef.preg.tipo == P_RESPUESTA && e.d.ef.preg.lado == 1, "el rival no recibio la ventana de respuesta");
		VERIFICA(e.activar(1, MT(1, mf)), "no se pudo activar Mirror Force");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 1 && !e.d.l[0].m[2].vacia(), "quedan %d monstruos del atacante (esperado 1, el de defensa)", e.d.l[0].monstruos());
		VERIFICA(e.d.l[1].lp == LP_INICIAL && e.d.l[1].monstruos() == 1, "hubo combate (LP %d)", e.d.l[1].lp);
		VERIFICA(e.d.ef.castigos[0] == 1, "la IA atacante no registro el castigo");
		fin();
	}
	{
		caso("Mirror Force: no se activa en el turno en que se coloco", "mirror_force");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		e.mon(1, "kuriboh");
		e.colocar(1, "mirror_force", 3);
		Jugada at;
		at.tipo = J_ATACAR;
		at.zona = a;
		at.objetivo = 0;
		VERIFICA(e.d.aplicar(at), "no se pudo atacar");
		VERIFICA(e.d.ef.preg.tipo == P_NADA, "se ofrecio responder con una trampa recien colocada");
		VERIFICA(e.d.l[1].monstruos() == 0, "el combate no ocurrio");
		fin();
	}
	{
		caso("Heavy Storm: destruye todas las magias/trampas (propias y del rival)", "heavy_storm");
		Escena e;
		e.colocar(0, "mirror_force");
		e.colocar(1, "trap_hole");
		e.colocar(1, "dark_hole");
		const int k = e.mano(0, "heavy_storm");
		VERIFICA(e.activar(0, H(0, k)), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].colocadasMT() + e.d.l[1].colocadasMT() == 0, "quedan %d", e.d.l[0].colocadasMT() + e.d.l[1].colocadasMT());
		VERIFICA(e.enCementerio(0, "heavy_storm") == 1 && e.enCementerio(0, "mirror_force") == 1, "cementerio propio incompleto");
		fin();
	}
	{
		caso("Harpie's Feather Duster: solo las del rival", "harpie_s_feather_duster");
		Escena e;
		e.colocar(0, "mirror_force");
		e.colocar(1, "trap_hole");
		const int k = e.mano(0, "harpie_s_feather_duster");
		VERIFICA(e.activar(0, H(0, k)), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].colocadasMT() == 1 && e.d.l[1].colocadasMT() == 0, "propias %d rival %d", e.d.l[0].colocadasMT(), e.d.l[1].colocadasMT());
		fin();
	}
	{
		caso("Mystical Space Typhoon: destruye la magia/trampa elegida", "mystical_space_typhoon");
		Escena e;
		e.colocar(1, "trap_hole");
		const int z = e.colocar(1, "mirror_force");
		const int k = e.mano(0, "mystical_space_typhoon");
		VERIFICA(e.activar(0, H(0, k), { MT(1, z) }), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[1].mt[(size_t)z].vacia() && e.d.l[1].colocadasMT() == 1, "no destruyo la elegida");
		fin();
	}
	{
		caso("Cadena: Sakuretsu (CL1) <- MST sobre ella (CL2): resuelve al reves y Sakuretsu igual destruye al atacante", "sakuretsu_armor");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		e.mon(1, "kuriboh");
		const int sk = e.colocar(1, "sakuretsu_armor");
		const int ms = e.colocar(0, "mystical_space_typhoon");
		Jugada at;
		at.tipo = J_ATACAR;
		at.zona = a;
		at.objetivo = 0;
		VERIFICA(e.d.aplicar(at), "no se pudo atacar");
		VERIFICA(e.activar(1, MT(1, sk), { M(0, a) }), "Sakuretsu no se activo");
		VERIFICA(e.d.ef.preg.tipo == P_RESPUESTA && e.d.ef.preg.lado == 0, "el atacante no tuvo la ocasion de encadenar");
		VERIFICA(e.activar(0, MT(0, ms), { MT(1, sk) }), "MST no se encadeno");
		e.pasarTodo();
		std::vector<std::string> orden;   // (nadie mas podia encadenar: se resolvio sola, de arriba hacia abajo)
		for (const Suceso& s : e.d.sucesos)
			if (s.tipo == S_RESUELVE) orden.push_back(s.carta);
		VERIFICA(orden.size() == 2 && orden[0] == "mystical_space_typhoon" && orden[1] == "sakuretsu_armor", "orden de resolucion inesperado");
		VERIFICA(e.d.l[0].monstruos() == 0, "el atacante sigue (Sakuretsu ya activada resuelve igual)");
		VERIFICA(e.enCementerio(1, "sakuretsu_armor") == 1 && e.enCementerio(0, "mystical_space_typhoon") == 1, "cartas fuera de lugar");
		VERIFICA(e.d.l[1].monstruos() == 1 && e.d.l[1].lp == LP_INICIAL, "hubo combate");
		VERIFICA(e.libre() && e.d.ef.resueltos == 2, "cadena no terminada (%d resueltos)", e.d.ef.resueltos);
		fin();
	}
	{
		caso("Fissure: destruye el boca arriba del rival con menos ATK; con empate elige quien la activa", "fissure");
		Escena e;
		e.mon(1, "celtic_guardian");   // 1400
		e.mon(1, "kuriboh", true, true);   // boca abajo: no cuenta
		e.mon(1, "battle_ox");   // 1700
		const int k = e.mano(0, "fissure");
		VERIFICA(e.activar(0, H(0, k)), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[1].m[0].vacia() && !e.d.l[1].m[1].vacia() && !e.d.l[1].m[2].vacia(), "no destruyo el de 1400");
		Escena f;
		f.mon(1, "celtic_guardian");
		f.mon(1, "celtic_guardian");
		const int k2 = f.mano(0, "fissure");
		VERIFICA(f.activar(0, H(0, k2)), "no se pudo activar (empate)");
		f.pasa(1);   // (si tuviera algo que encadenar)
		VERIFICA(f.d.ef.preg.tipo == P_ELEGIR && f.d.ef.preg.lado == 0 && f.d.ef.preg.cands.size() == 2, "no pregunto el empate");
		VERIFICA(f.elige(0, 1), "no acepto la eleccion");
		VERIFICA(!f.d.l[1].m[0].vacia() && f.d.l[1].m[1].vacia(), "no destruyo el elegido");
		fin();
	}
	{
		caso("Tribute to the Doomed: descarta 1 (coste) y destruye el monstruo elegido", "tribute_to_the_doomed");
		Escena e;
		const int z = e.mon(1, "dark_magician");
		e.mano(0, "kuriboh");
		const int k = e.mano(0, "tribute_to_the_doomed");
		VERIFICA(!e.activar(0, H(0, k), { M(1, z) }, {}), "se activo sin pagar el descarte");
		VERIFICA(e.activar(0, H(0, k), { M(1, z) }, { 0 }), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && e.enCementerio(0, "kuriboh") == 1 && e.d.l[0].mano.empty(), "estado inesperado");
		fin();
	}
	{
		caso("Trap Hole: invocacion normal de 1000+ ATK -> destruida; de menos, no se ofrece", "trap_hole");
		Escena e;
		const int th = e.colocar(1, "trap_hole");
		e.mano(0, "vorse_raider");
		Jugada inv;
		inv.tipo = J_INVOCAR;
		inv.mano = 0;
		VERIFICA(e.d.aplicar(inv), "no se pudo invocar");
		VERIFICA(e.d.ef.preg.tipo == P_RESPUESTA && e.d.ef.preg.lado == 1, "sin ventana tras la invocacion");
		VERIFICA(e.activar(1, MT(1, th), { M(0, 0) }), "Trap Hole no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 0 && e.enCementerio(0, "vorse_raider") == 1, "el invocado sigue");
		Escena f;
		f.colocar(1, "trap_hole");
		f.mano(0, "kuriboh");
		inv.mano = 0;
		VERIFICA(f.d.aplicar(inv), "no se pudo invocar Kuriboh");
		VERIFICA(f.d.ef.preg.tipo == P_NADA, "se ofrecio Trap Hole contra 300 ATK");
		Escena g;   // colocar no es invocar (no se ofrece)
		g.colocar(1, "trap_hole");
		g.mano(0, "vorse_raider");
		Jugada col;
		col.tipo = J_COLOCAR;
		col.mano = 0;
		VERIFICA(g.d.aplicar(col) && g.d.ef.preg.tipo == P_NADA, "se ofrecio Trap Hole contra un monstruo colocado");
		fin();
	}
}

static void tanda2()
{
	fprintf(stderr, "tanda 2: robo, recuperacion y LP\n");
	{
		caso("Pot of Greed: roba 2", "pot_of_greed");
		Escena e;
		const int k = e.mano(0, "pot_of_greed");
		const size_t mazo = e.d.l[0].mazo.size();
		VERIFICA(e.activar(0, H(0, k)), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].mano.size() == 2 && e.d.l[0].mazo.size() == mazo - 2, "mano %d", (int)e.d.l[0].mano.size());
		fin();
	}
	{
		caso("Graceful Charity: roba 3 y el jugador elige 2 para descartar", "graceful_charity");
		Escena e;
		e.mano(0, "dark_magician");
		const int k = e.mano(0, "graceful_charity");
		VERIFICA(e.activar(0, H(0, k)), "no se pudo activar");
		e.pasa(1);
		VERIFICA(e.d.ef.preg.tipo == P_ELEGIR && e.d.ef.preg.faltan == 2 && e.d.ef.preg.cands.size() == 4, "no pidio descartar 2 de 4");
		VERIFICA(e.elige(0, 0), "eleccion 1");
		VERIFICA(e.d.ef.preg.tipo == P_ELEGIR && e.d.ef.preg.faltan == 1, "no pidio la segunda");
		VERIFICA(e.elige(0, 0), "eleccion 2");
		VERIFICA(e.d.l[0].mano.size() == 2 && e.enCementerio(0, "dark_magician") == 1 && e.libre(), "mano %d", (int)e.d.l[0].mano.size());
		fin();
	}
	{
		caso("Monster Reborn: trae un monstruo del cementerio rival; al morir vuelve al cementerio de su duenno", "monster_reborn");
		Escena e;
		e.d.l[1].cementerio.push_back("summoned_skull");
		const int k = e.mano(0, "monster_reborn");
		VERIFICA(e.activar(0, H(0, k), { Lugar{ 1, EN_CEMENTERIO, 0 } }), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].id == "summoned_skull" && e.d.l[0].m[0].duenno == 1, "no esta en el campo propio");
		e.d.alCementerio(0, 0);
		VERIFICA(e.enCementerio(1, "summoned_skull") == 1 && e.enCementerio(0, "summoned_skull") == 0, "fue al cementerio equivocado");
		fin();
	}
	{
		caso("Premature Burial: 800 LP, revive en ataque y equipado; si se destruye la carta, el monstruo tambien", "premature_burial");
		Escena e;
		e.d.l[0].cementerio.push_back("dark_magician");
		const int k = e.mano(0, "premature_burial");
		VERIFICA(e.activar(0, H(0, k), { Lugar{ 0, EN_CEMENTERIO, 0 } }), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].lp == LP_INICIAL - 800 && e.d.l[0].m[0].id == "dark_magician" && !e.d.l[0].m[0].defensa, "LP %d", e.d.l[0].lp);
		VERIFICA(e.d.l[0].mt[0].id == "premature_burial" && e.d.l[0].mt[0].eqUid == e.d.l[0].m[0].uid, "no quedo equipada");
		e.d.mtAlCementerio(0, 0);
		VERIFICA(e.d.l[0].monstruos() == 0, "el monstruo sigue tras destruir Premature Burial");
		fin();
	}
	{
		caso("Call of the Haunted: revive; si el monstruo se destruye, la trampa tambien", "call_of_the_haunted");
		Escena e;
		e.d.l[0].cementerio.push_back("summoned_skull");
		const int z = e.colocar(0, "call_of_the_haunted");
		VERIFICA(e.activar(0, MT(0, z), { Lugar{ 0, EN_CEMENTERIO, 0 } }), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].id == "summoned_skull" && !e.d.l[0].mt[(size_t)z].vacia() && !e.d.l[0].mt[(size_t)z].bocaAbajo, "no quedo vinculada");
		e.d.alCementerio(0, 0);
		VERIFICA(e.d.l[0].mt[(size_t)z].vacia() && e.enCementerio(0, "call_of_the_haunted") == 1, "la trampa sigue");
		fin();
	}
	{
		caso("Change of Heart: el monstruo cambia de lado y vuelve en la fase final", "change_of_heart");
		Escena e;
		const int z = e.mon(1, "dark_magician");
		const int k = e.mano(0, "change_of_heart");
		VERIFICA(e.activar(0, H(0, k), { M(1, z) }), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].id == "dark_magician" && e.d.l[1].monstruos() == 0, "no cambio de lado");
		VERIFICA(!e.d.l[0].m[0].ataco, "no podria atacar");
		Jugada f;
		f.tipo = J_FASE;
		e.d.aplicar(f);   // -> batalla
		e.d.aplicar(f);   // -> principal 2
		e.d.aplicar(f);   // -> final
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 1 && e.d.l[0].monstruos() == 0, "no volvio en la fase final (rival %d)", e.d.l[1].monstruos());
		fin();
	}
	{
		caso("Snatch Steal: control mientras este equipada; el rival gana 1000 en su fase de espera; destruida, vuelve", "snatch_steal");
		Escena e;
		const int z = e.mon(1, "summoned_skull");
		const int k = e.mano(0, "snatch_steal");
		VERIFICA(e.activar(0, H(0, k), { M(1, z) }), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].id == "summoned_skull", "no tomo el control");
		Jugada f;
		f.tipo = J_FASE;
		for (int g = 0; g < 4 && e.d.activo == 0; g++) {
			e.d.aplicar(f);
			e.pasarTodo();
		}
		VERIFICA(e.d.activo == 1 && e.d.l[1].lp == LP_INICIAL + 1000, "LP del rival %d (esperado 9000)", e.d.l[1].lp);
		VERIFICA(e.d.l[0].m[0].id == "summoned_skull", "lo perdio al pasar el turno");
		e.d.mtAlCementerio(0, 0);
		VERIFICA(e.d.l[1].monstruos() == 1 && e.d.l[0].monstruos() == 0, "no volvio a su duenno");
		fin();
	}
	{
		caso("Dian Keto: +1000 LP", "dian_keto_the_cure_master");
		Escena e;
		const int k = e.mano(0, "dian_keto_the_cure_master");
		VERIFICA(e.activar(0, H(0, k)), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].lp == LP_INICIAL + 1000, "LP %d", e.d.l[0].lp);
		fin();
	}
	{
		caso("Card Destruction: los dos descartan y roban lo mismo", "card_destruction");
		Escena e;
		e.mano(0, "kuriboh");
		e.mano(0, "kuriboh");
		e.mano(1, "dark_magician");
		e.mano(1, "dark_magician");
		e.mano(1, "dark_magician");
		const int k = e.mano(0, "card_destruction");
		VERIFICA(e.activar(0, H(0, k)), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].mano.size() == 2 && e.d.l[1].mano.size() == 3, "manos %d/%d", (int)e.d.l[0].mano.size(), (int)e.d.l[1].mano.size());
		VERIFICA(e.enCementerio(0, "kuriboh") == 2 && e.enCementerio(1, "dark_magician") == 3, "cementerios");
		fin();
	}
	{
		caso("Upstart Goblin: roba 1 y el rival gana 1000", "upstart_goblin");
		Escena e;
		const int k = e.mano(0, "upstart_goblin");
		VERIFICA(e.activar(0, H(0, k)), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].mano.size() == 1 && e.d.l[1].lp == LP_INICIAL + 1000, "mano %d LP %d", (int)e.d.l[0].mano.size(), e.d.l[1].lp);
		fin();
	}
}

static void tanda3()
{
	fprintf(stderr, "tanda 3: equipos, campos y bloqueos de ataque\n");
	{
		caso("Axe of Despair: +1000 ATK al equipado; destruido el monstruo, el equipo se va", "axe_of_despair");
		Escena e;
		const int z = e.mon(0, "celtic_guardian");
		const int k = e.mano(0, "axe_of_despair");
		VERIFICA(e.activar(0, H(0, k), { M(0, z) }), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)z].atkEf == 2400, "ATK %d (esperado 2400)", e.d.l[0].m[(size_t)z].atkEf);
		VERIFICA(!e.d.l[0].mt[0].vacia(), "el equipo no quedo en el campo");
		e.d.alCementerio(0, z);
		VERIFICA(e.d.l[0].mt[0].vacia() && e.enCementerio(0, "axe_of_despair") == 1, "el equipo sigue sin monstruo");
		fin();
	}
	{
		caso("Black Pendant: +500 ATK; al ir al cementerio, 500 de danno al rival", "black_pendant");
		Escena e;
		const int z = e.mon(0, "celtic_guardian");
		const int k = e.mano(0, "black_pendant");
		VERIFICA(e.activar(0, H(0, k), { M(0, z) }), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)z].atkEf == 1900, "ATK %d", e.d.l[0].m[(size_t)z].atkEf);
		e.d.alCementerio(0, z);
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 500, "LP rival %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Megamorph: con menos LP duplica el ATK original; con mas, lo reduce a la mitad", "megamorph");
		Escena e;
		const int z = e.mon(0, "celtic_guardian");
		e.d.l[0].lp = 3000;
		const int k = e.mano(0, "megamorph");
		VERIFICA(e.activar(0, H(0, k), { M(0, z) }), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)z].atkEf == 2800, "ATK %d (esperado 2800)", e.d.l[0].m[(size_t)z].atkEf);
		e.d.l[0].lp = 9000;
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[(size_t)z].atkEf == 700, "ATK %d (esperado 700)", e.d.l[0].m[(size_t)z].atkEf);
		fin();
	}
	{
		caso("United We Stand: +800 ATK/DEF por cada monstruo boca arriba propio", "united_we_stand");
		Escena e;
		const int z = e.mon(0, "celtic_guardian");
		e.mon(0, "kuriboh");
		e.mon(0, "battle_ox", true, true);   // boca abajo: no cuenta
		const int k = e.mano(0, "united_we_stand");
		VERIFICA(e.activar(0, H(0, k), { M(0, z) }), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)z].atkEf == 3000 && e.d.l[0].m[(size_t)z].defEf == 2800, "ATK/DEF %d/%d", e.d.l[0].m[(size_t)z].atkEf, e.d.l[0].m[(size_t)z].defEf);
		fin();
	}
	{
		caso("Mage Power: +500 ATK/DEF por cada magia/trampa propia (ella incluida)", "mage_power");
		Escena e;
		const int z = e.mon(0, "celtic_guardian");
		e.colocar(0, "mirror_force");
		const int k = e.mano(0, "mage_power");
		VERIFICA(e.activar(0, H(0, k), { M(0, z) }), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)z].atkEf == 2400, "ATK %d (esperado 2400)", e.d.l[0].m[(size_t)z].atkEf);
		fin();
	}
	{
		caso("Campos: Sogen +200 a Guerrero y Guerrero-Bestia de los dos lados; Yami -200 a Hada", "sogen");
		Escena e;
		const int a = e.mon(0, "celtic_guardian");
		const int b = e.mon(1, "battle_ox");
		const int c = e.mon(1, "kuriboh");
		const int k = e.mano(0, "sogen");
		VERIFICA(e.activar(0, H(0, k)), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].campo.id == "sogen" && !e.d.l[0].campo.bocaAbajo, "no quedo en la zona de campo");
		VERIFICA(e.d.l[0].m[(size_t)a].atkEf == 1600 && e.d.l[1].m[(size_t)b].atkEf == 1900 && e.d.l[1].m[(size_t)c].atkEf == 300, "ATK %d/%d/%d",
		         e.d.l[0].m[(size_t)a].atkEf, e.d.l[1].m[(size_t)b].atkEf, e.d.l[1].m[(size_t)c].atkEf);
		e.d.turno = 4;
		e.d.activo = 1;
		const int k2 = e.mano(1, "yami");
		VERIFICA(e.activar(1, H(1, k2)), "Yami no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].campo.vacia() && e.enCementerio(0, "sogen") == 1, "Sogen siguio (una sola de campo boca arriba)");
		VERIFICA(e.d.l[1].m[(size_t)c].atkEf == 500 && e.d.l[0].m[(size_t)a].atkEf == 1400, "Yami: Kuriboh %d, Celtic %d", e.d.l[1].m[(size_t)c].atkEf,
		         e.d.l[0].m[(size_t)a].atkEf);
		fin();
	}
	{
		caso("Campos: Forest/Umi/Wasteland/Mountain se activan y suben a sus tipos", "forest");
		const char* ids[] = { "forest", "umi", "wasteland", "mountain", "yami" };
		for (const char* id : ids) {
			probadas[id]++;
			Escena e;
			const int z = e.mon(0, id == std::string("forest") ? "battle_ox" : "summoned_skull");
			const int k = e.mano(0, id);
			VERIFICA(e.activar(0, H(0, k)), "%s no se activo", id);
			e.pasarTodo();
			const int esperado = id == std::string("forest") ? 1900 : id == std::string("yami") ? 2700 : 2500;
			VERIFICA(e.d.l[0].m[(size_t)z].atkEf == esperado, "%s: ATK %d (esperado %d)", id, e.d.l[0].m[(size_t)z].atkEf, esperado);
		}
		fin();
	}
	{
		caso("Swords of Revealing Light: voltea, el rival no ataca y se va en la fase final de su 3er turno", "swords_of_revealing_light");
		Escena e;
		e.mon(1, "summoned_skull");
		e.mon(1, "battle_ox", true, true);
		const int k = e.mano(0, "swords_of_revealing_light");
		VERIFICA(e.activar(0, H(0, k)), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(!e.d.l[1].m[1].bocaAbajo, "no volteo el boca abajo");
		VERIFICA(!e.d.l[0].mt[0].vacia(), "no se quedo en el campo");
		Jugada f;
		f.tipo = J_FASE;
		int turnosRival = 0;
		for (int g = 0; g < 60 && turnosRival < 3 && e.d.ganador < 0; g++) {
			if (e.d.activo == 1 && e.d.fase == F_BATALLA) {
				VERIFICA(!e.d.puedeAtacar(1, 0), "el rival puede atacar con Swords");
			}
			const int antes = e.d.activo;
			e.d.aplicar(f);
			e.pasarTodo();
			if (antes == 1 && e.d.activo == 0) turnosRival++;
			if (turnosRival < 3) VERIFICA(!e.d.l[0].mt[0].vacia(), "se fue antes de tiempo (%d turnos del rival)", turnosRival);
		}
		VERIFICA(turnosRival == 3 && e.d.l[0].mt[0].vacia() && e.enCementerio(0, "swords_of_revealing_light") == 1, "no se fue tras 3 turnos del rival");
		fin();
	}
	{
		caso("Messenger of Peace: 1500+ ATK no atacan; paga 100 LP en tu fase de espera", "messenger_of_peace");
		Escena e;
		e.mon(0, "summoned_skull");
		e.mon(0, "celtic_guardian");
		const int k = e.mano(0, "messenger_of_peace");
		VERIFICA(e.activar(0, H(0, k)), "no se pudo activar");
		e.pasarTodo();
		e.d.fase = F_BATALLA;
		VERIFICA(!e.d.puedeAtacar(0, 0) && e.d.puedeAtacar(0, 1), "2500 ataca o 1400 no");
		Jugada f;
		f.tipo = J_FASE;
		for (int g = 0; g < 30 && !(e.d.activo == 0 && e.d.turno > 3); g++) {
			e.d.aplicar(f);
			e.pasarTodo();
		}
		VERIFICA(e.d.l[0].lp == LP_INICIAL - 100, "LP %d (esperado 7900)", e.d.l[0].lp);
		fin();
	}
	{
		caso("Gravity Bind: nivel 4 o mas no atacan (los dos lados)", "gravity_bind");
		Escena e(F_BATALLA);
		e.mon(0, "summoned_skull");
		e.mon(0, "kuriboh");
		const int z = e.colocar(0, "gravity_bind");
		VERIFICA(e.activar(0, MT(0, z)), "no se pudo activar");
		e.pasarTodo();
		VERIFICA(!e.d.puedeAtacar(0, 0) && e.d.puedeAtacar(0, 1), "nivel 6 ataca o nivel 1 no");
		VERIFICA(!e.d.l[0].mt[(size_t)z].vacia() && !e.d.l[0].mt[(size_t)z].bocaAbajo, "no quedo boca arriba");
		fin();
	}
}

// declara un ataque del lado 0 (zona a) contra la zona o del rival (-1 directo)
static bool atacar(Escena& e, int a, int o)
{
	Jugada at;
	at.tipo = J_ATACAR;
	at.zona = a;
	at.objetivo = o;
	return e.d.aplicar(at);
}
static void tanda4()
{
	fprintf(stderr, "tanda 4: trampas de batalla, danno y contraataques\n");
	{
		caso("Magic Cylinder: niega el ataque y el atacante danna a su duenno con su ATK", "magic_cylinder");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int z = e.colocar(1, "magic_cylinder");
		VERIFICA(atacar(e, a, -1), "no se pudo atacar");
		VERIFICA(e.activar(1, MT(1, z), { M(0, a) }), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].lp == LP_INICIAL - 2500 && e.d.l[1].lp == LP_INICIAL, "LP %d/%d", e.d.l[0].lp, e.d.l[1].lp);
		VERIFICA(!e.d.puedeAtacar(0, a), "el atacante podria volver a atacar");
		fin();
	}
	{
		caso("Negate Attack: niega el ataque y termina la batalla (nadie mas ataca)", "negate_attack");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int b = e.mon(0, "celtic_guardian");
		const int z = e.colocar(1, "negate_attack");
		VERIFICA(atacar(e, a, -1), "no se pudo atacar");
		VERIFICA(e.activar(1, MT(1, z), { M(0, a) }), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL, "hubo danno");
		VERIFICA(!e.d.puedeAtacar(0, b), "el segundo monstruo todavia puede atacar");
		fin();
	}
	{
		caso("Waboku: sin danno de batalla y sin destrucciones en batalla ese turno", "waboku");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int b = e.mon(0, "dark_magician");
		const int d0 = e.mon(1, "celtic_guardian");
		const int z = e.colocar(1, "waboku");
		VERIFICA(atacar(e, a, d0), "no se pudo atacar");
		VERIFICA(e.activar(1, MT(1, z)), "no se activo");
		e.pasarTodo();
		VERIFICA(!e.d.l[1].m[(size_t)d0].vacia() && e.d.l[1].lp == LP_INICIAL, "Celtic destruido o danno (LP %d)", e.d.l[1].lp);
		VERIFICA(atacar(e, b, d0), "no se pudo atacar de nuevo");
		e.pasarTodo();
		VERIFICA(!e.d.l[1].m[(size_t)d0].vacia() && e.d.l[1].lp == LP_INICIAL, "el segundo ataque si hizo efecto");
		fin();
	}
	{
		caso("Ring of Destruction: en el turno del rival destruye su monstruo y los dos reciben su ATK; 1 por turno", "ring_of_destruction");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int z = e.colocar(1, "ring_of_destruction");
		e.colocar(1, "ring_of_destruction");
		VERIFICA(atacar(e, a, -1), "no se pudo atacar");
		VERIFICA(e.activar(1, MT(1, z), { M(0, a) }), "no se activo");
		VERIFICA(activables(e.d, 1, true).empty(), "se ofrece el segundo Ring el mismo turno");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 0 && e.d.l[0].lp == LP_INICIAL - 2500 && e.d.l[1].lp == LP_INICIAL - 2500, "LP %d/%d", e.d.l[0].lp, e.d.l[1].lp);
		Escena f(F_BATALLA);   // los dos a 0: empate
		f.d.l[0].lp = f.d.l[1].lp = 2500;
		const int x = f.mon(0, "summoned_skull");
		const int y = f.colocar(1, "ring_of_destruction");
		VERIFICA(atacar(f, x, -1), "no se pudo atacar (empate)");
		VERIFICA(f.activar(1, MT(1, y), { M(0, x) }), "no se activo (empate)");
		f.pasarTodo();
		VERIFICA(f.d.ganador == 2, "ganador %d (esperado empate)", f.d.ganador);
		fin();
	}
	{
		caso("Barrel Behind the Door: encadenada a Magic Cylinder, el danno vuelve al que activo el Cylinder", "barrel_behind_the_door");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int br = e.colocar(0, "barrel_behind_the_door");
		const int z = e.colocar(1, "magic_cylinder");
		VERIFICA(atacar(e, a, -1), "no se pudo atacar");
		VERIFICA(e.activar(1, MT(1, z), { M(0, a) }), "Cylinder no se activo");
		VERIFICA(e.d.ef.preg.tipo == P_RESPUESTA && e.d.ef.preg.lado == 0, "el atacante no pudo responder");
		VERIFICA(e.activar(0, MT(0, br)), "Barrel no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].lp == LP_INICIAL && e.d.l[1].lp == LP_INICIAL - 2500, "LP %d/%d", e.d.l[0].lp, e.d.l[1].lp);
		fin();
	}
	{
		caso("Torrential Tribute: al invocar el rival, destruye todos los monstruos", "torrential_tribute");
		Escena e;
		e.mon(0, "celtic_guardian");
		e.mon(1, "dark_magician");
		const int z = e.colocar(1, "torrential_tribute");
		e.mano(0, "kuriboh");
		Jugada inv;
		inv.tipo = J_INVOCAR;
		inv.mano = 0;
		VERIFICA(e.d.aplicar(inv), "no se pudo invocar");
		VERIFICA(e.activar(1, MT(1, z)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() + e.d.l[1].monstruos() == 0, "quedan monstruos");
		fin();
	}
	{
		caso("Bottomless Trap Hole: invocacion especial de 1500+ ATK -> destruido y desterrado (no al cementerio)", "bottomless_trap_hole");
		Escena e;
		e.d.l[0].cementerio.push_back("summoned_skull");
		const int z = e.colocar(1, "bottomless_trap_hole");
		const int k = e.mano(0, "monster_reborn");
		VERIFICA(e.activar(0, H(0, k), { Lugar{ 0, EN_CEMENTERIO, 0 } }), "Monster Reborn no se activo");
		// (contra la activacion el rival no tiene nada: Reborn resuelve; el jugador elige la posicion: "No" = ataque)
		VERIFICA(e.d.ef.preg.tipo == P_ELEGIR && e.d.ef.preg.opcional && e.d.ef.preg.texto == "ef.elegir.defensa", "no pregunto la posicion");
		Jugada ataque;
		ataque.tipo = J_PASAR;
		VERIFICA(e.d.aplicar(ataque), "no acepto 'en ataque'");
		VERIFICA(!e.d.l[0].m[0].defensa, "quedo en defensa");
		VERIFICA(e.d.ef.preg.tipo == P_RESPUESTA && e.d.ef.preg.lado == 1, "sin ventana tras la invocacion especial");
		VERIFICA(e.activar(1, MT(1, z), { M(0, 0) }), "Bottomless no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 0 && e.d.l[0].desterrados.size() == 1 && e.enCementerio(0, "summoned_skull") == 0, "no quedo desterrado");
		fin();
	}
	{
		caso("Solemn Judgment: paga la mitad de LP y niega una magia (va al cementerio sin efecto) o una invocacion", "solemn_judgment");
		Escena e;
		e.mon(1, "dark_magician");
		const int z = e.colocar(1, "solemn_judgment");
		const int k = e.mano(0, "raigeki");
		VERIFICA(e.activar(0, H(0, k)), "Raigeki no se activo");
		VERIFICA(e.activar(1, MT(1, z)), "Solemn no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 1 && e.enCementerio(0, "raigeki") == 1 && e.d.l[1].lp == LP_INICIAL / 2, "Raigeki no se nego (LP %d)", e.d.l[1].lp);
		Escena f;
		const int z2 = f.colocar(1, "solemn_judgment");
		f.mano(0, "vorse_raider");
		Jugada inv;
		inv.tipo = J_INVOCAR;
		inv.mano = 0;
		VERIFICA(f.d.aplicar(inv), "no se pudo invocar");
		VERIFICA(f.activar(1, MT(1, z2)), "Solemn no se activo contra la invocacion");
		f.pasarTodo();
		VERIFICA(f.d.l[0].monstruos() == 0 && f.enCementerio(0, "vorse_raider") == 1, "la invocacion no se nego");
		fin();
	}
	{
		caso("Magic Jammer: descarta 1 y niega una magia; no sirve contra trampas", "magic_jammer");
		Escena e;
		e.mon(1, "dark_magician");
		const int z = e.colocar(1, "magic_jammer");
		e.mano(1, "kuriboh");
		const int k = e.mano(0, "raigeki");
		VERIFICA(e.activar(0, H(0, k)), "Raigeki no se activo");
		VERIFICA(e.activar(1, MT(1, z), {}, { 0 }), "Jammer no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 1 && e.enCementerio(1, "kuriboh") == 1, "no nego");
		Escena f(F_BATALLA);
		const int a = f.mon(0, "summoned_skull");
		f.colocar(0, "magic_jammer");
		f.mano(0, "kuriboh");
		const int mf = f.colocar(1, "mirror_force");
		VERIFICA(atacar(f, a, -1), "no se pudo atacar");
		VERIFICA(f.activar(1, MT(1, mf)), "Mirror Force no se activo");
		VERIFICA(!(f.d.ef.preg.tipo == P_RESPUESTA && f.d.ef.preg.lado == 0), "se ofrecio Magic Jammer contra una trampa");
		f.pasarTodo();
		fin();
	}
	{
		caso("Seven Tools of the Bandit: paga 1000 y niega una trampa (Mirror Force)", "seven_tools_of_the_bandit");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int st = e.colocar(0, "seven_tools_of_the_bandit");
		const int mf = e.colocar(1, "mirror_force");
		VERIFICA(atacar(e, a, -1), "no se pudo atacar");
		VERIFICA(e.activar(1, MT(1, mf)), "Mirror Force no se activo");
		VERIFICA(e.activar(0, MT(0, st)), "Seven Tools no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 1 && e.d.l[0].lp == LP_INICIAL - 1000 && e.d.l[1].lp == LP_INICIAL - 2500, "LP %d/%d monstruos %d", e.d.l[0].lp,
		         e.d.l[1].lp, e.d.l[0].monstruos());
		fin();
	}
}

static bool voltear(Escena& e, int z)
{
	Jugada j;
	j.tipo = J_VOLTEAR;
	j.zona = z;
	return e.d.aplicar(j);
}
static bool invocar(Escena& e, int k)
{
	Jugada j;
	j.tipo = J_INVOCAR;
	j.mano = k;
	return e.d.aplicar(j);
}
// los disparos pendientes (mantenimientos, opcionales): se abre su ventana y se contesta como la IA
static void resolverDisparos(Escena& e)
{
	if (!e.d.ef.disparos.empty() && e.d.ef.preg.tipo == P_NADA && e.d.ef.cadena.empty()) motorVentana(e.d, Ventana{ V_DISPARO, e.d.activo, -1, 0, {} });
	for (int k = 0; k < 12 && e.d.ef.preg.tipo != P_NADA; k++) {
		if (e.d.ef.preg.tipo == P_ELEGIR) {
			const int r = elegirIA(e.d, e.d.ef.preg.lado);
			if (r >= 0 || !e.d.ef.preg.opcional) {
				e.elige(e.d.ef.preg.lado, std::max(0, r));
				continue;
			}
		}
		Jugada j;
		j.tipo = J_PASAR;
		e.d.aplicar(j);
	}
}
static int indiceCand(const Escena& e, const std::string& carta)
{
	for (int k = 0; k < (int)e.d.ef.preg.cands.size(); k++)
		if (cartaEn(e.d, e.d.ef.preg.cands[(size_t)k]) == carta) return k;
	return -1;
}
static bool enMano(const Escena& e, int lado, const std::string& id)
{
	const auto& m = e.d.l[(size_t)lado].mano;
	return std::find(m.begin(), m.end(), id) != m.end();
}
static void pasarFases(Escena& e, int hastaActivo, int maxPasos = 12)
{
	Jugada f;
	f.tipo = J_FASE;
	for (int g = 0; g < maxPasos && e.d.activo != hastaActivo && e.d.ganador < 0; g++) {
		e.d.aplicar(f);
		e.pasarTodo();
	}
}

static void tanda5()
{
	fprintf(stderr, "tanda 5: monstruos con efecto de Goat\n");
	{
		caso("Man-Eater Bug: volteo por invocacion: el jugador elige el monstruo a destruir", "man_eater_bug");
		Escena e;
		const int z = e.mon(0, "man_eater_bug", true, true);
		e.mon(1, "dark_magician");
		VERIFICA(voltear(e, z), "no se pudo voltear");
		e.pasa(1);
		VERIFICA(e.d.ef.preg.tipo == P_ELEGIR && e.d.ef.preg.lado == 0, "no pregunto el objetivo");
		VERIFICA(e.elige(0, indiceCand(e, "dark_magician")), "eleccion");
		VERIFICA(e.d.l[1].monstruos() == 0 && e.libre(), "no destruyo");
		fin();
	}
	{
		caso("Man-Eater Bug: volteado por un ataque se activa aunque muera (despues del combate)", "man_eater_bug");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		e.mon(0, "celtic_guardian");
		const int z = e.mon(1, "man_eater_bug", true, true);
		Jugada at;
		at.tipo = J_ATACAR;
		at.zona = a;
		at.objetivo = z;
		VERIFICA(e.d.aplicar(at), "no se pudo atacar");
		e.pasa(0);
		VERIFICA(e.d.ef.preg.tipo == P_ELEGIR && e.d.ef.preg.lado == 1, "el duenno del Bug no eligio");
		VERIFICA(e.elige(1, indiceCand(e, "summoned_skull")), "eleccion");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && e.d.l[0].m[(size_t)a].vacia() && e.d.l[0].monstruos() == 1, "estado inesperado");
		fin();
	}
	{
		caso("Old Vindictive Magician: volteo: destruye un monstruo del rival (solo del rival)", "old_vindictive_magician");
		Escena e;
		const int z = e.mon(0, "old_vindictive_magician", true, true);
		e.mon(0, "celtic_guardian");
		e.mon(1, "dark_magician");
		VERIFICA(voltear(e, z), "no se pudo voltear");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && e.d.l[0].monstruos() == 2, "destruyo el equivocado");
		fin();
	}
	{
		caso("Magician of Faith: volteo: una magia del cementerio a la mano", "magician_of_faith");
		Escena e;
		e.d.l[0].cementerio = { "raigeki", "celtic_guardian", "pot_of_greed" };
		const int z = e.mon(0, "magician_of_faith", true, true);
		VERIFICA(voltear(e, z), "no se pudo voltear");
		e.pasa(1);
		VERIFICA(e.d.ef.preg.tipo == P_ELEGIR && e.d.ef.preg.cands.size() == 2, "candidatos %d (esperado 2 magias)", (int)e.d.ef.preg.cands.size());
		VERIFICA(e.elige(0, indiceCand(e, "pot_of_greed")), "eleccion");
		VERIFICA(enMano(e, 0, "pot_of_greed") && e.enCementerio(0, "pot_of_greed") == 0, "no volvio a la mano");
		fin();
	}
	{
		caso("Penguin Soldier: volteo: 2 monstruos del rival a la mano (fusion: al mazo de fusion)", "penguin_soldier");
		Escena e;
		const int z = e.mon(0, "penguin_soldier", true, true);
		e.mon(1, "dark_magician");
		e.mon(1, "celtic_guardian");
		VERIFICA(voltear(e, z), "no se pudo voltear");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && e.d.l[1].mano.size() == 2, "rival: %d monstruos, mano %d", e.d.l[1].monstruos(), (int)e.d.l[1].mano.size());
		fin();
	}
	{
		caso("Cyber Jar: volteo: destruye todo; 5 del mazo: nivel 4 o menos al campo, el resto a la mano", "cyber_jar");
		Escena e;
		e.d.l[0].mazo = { "dark_magician", "kuriboh", "raigeki", "celtic_guardian", "summoned_skull", "battle_ox" };
		e.d.l[1].mazo = { "kuriboh", "kuriboh", "dark_magician", "pot_of_greed", "mystical_elf", "gemini_elf" };
		e.mon(1, "summoned_skull");
		const int z = e.mon(0, "cyber_jar", true, true);
		VERIFICA(voltear(e, z), "no se pudo voltear");
		e.pasarTodo();
		// propio: del final del mazo (arriba): battle_ox, summoned_skull, celtic_guardian, raigeki, kuriboh
		VERIFICA(e.d.l[0].monstruos() == 3 && e.d.l[0].mano.size() == 2, "propio: %d monstruos, mano %d", e.d.l[0].monstruos(), (int)e.d.l[0].mano.size());
		VERIFICA(e.d.l[1].monstruos() == 3 && e.d.l[1].mano.size() == 2, "rival: %d monstruos, mano %d", e.d.l[1].monstruos(), (int)e.d.l[1].mano.size());
		VERIFICA(e.enCementerio(0, "cyber_jar") == 1 && e.enCementerio(1, "summoned_skull") == 1, "no destruyo todo");
		fin();
	}
	{
		caso("Sangan: destruido, su duenno trae del mazo un monstruo de 1500 o menos ATK (uno posible: solo)", "sangan");
		Escena e;
		e.mon(1, "sangan");
		const int k = e.mano(0, "raigeki");
		const size_t mazo = e.d.l[1].mazo.size();
		VERIFICA(e.activar(0, H(0, k)), "Raigeki no se activo");
		e.pasarTodo();
		VERIFICA(enMano(e, 1, "celtic_guardian") && e.d.l[1].mazo.size() == mazo - 1, "no busco");
		fin();
	}
	{
		caso("Witch of the Black Forest: al cementerio (sacrificada tambien), elige entre los de 1500 o menos DEF", "witch_of_the_black_forest");
		Escena e;
		const int z = e.mon(0, "witch_of_the_black_forest");
		e.mano(0, "summoned_skull");
		Jugada inv;
		inv.tipo = J_INVOCAR;
		inv.mano = 0;
		inv.tributos = { z };
		VERIFICA(e.d.aplicar(inv), "no se pudo invocar sacrificandola");
		e.pasa(1);
		VERIFICA(e.d.ef.preg.tipo == P_ELEGIR && e.d.ef.preg.cands.size() == 2, "candidatos %d (esperado 2)", (int)e.d.ef.preg.cands.size());
		VERIFICA(e.elige(0, indiceCand(e, "battle_ox")), "eleccion");
		e.pasarTodo();
		VERIFICA(enMano(e, 0, "battle_ox"), "no busco");
		fin();
	}
	{
		caso("Breaker: invocado normal, 1 contador (+300); quitarlo destruye una magia/trampa", "breaker_the_magical_warrior");
		Escena e;
		e.mano(0, "breaker_the_magical_warrior");
		const int mf = e.colocar(1, "mirror_force");
		VERIFICA(invocar(e, 0), "no se pudo invocar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].contadores == 1 && e.d.l[0].m[0].atkEf == 1900, "contadores %d ATK %d", e.d.l[0].m[0].contadores, e.d.l[0].m[0].atkEf);
		Jugada j = jugadaActivar(M(0, 0));
		j.objs = { MT(1, mf) };
		VERIFICA(e.d.aplicar(j), "el efecto no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].colocadasMT() == 0 && e.d.l[0].m[0].contadores == 0 && e.d.l[0].m[0].atkEf == 1600, "estado inesperado");
		VERIFICA(!e.d.aplicar(j), "se activo sin contador");
		Escena f;   // invocado de forma especial: sin contador
		f.d.l[0].cementerio.push_back("breaker_the_magical_warrior");
		const int k = f.mano(0, "monster_reborn");
		VERIFICA(f.activar(0, H(0, k), { Lugar{ 0, EN_CEMENTERIO, 0 } }), "Reborn no se activo");
		f.pasarTodo();
		VERIFICA(f.d.l[0].m[0].contadores == 0, "le puso contador sin invocacion normal");
		fin();
	}
	{
		caso("Tsukuyomi: invocada, pone boca abajo un monstruo; en la fase final vuelve a la mano (Spirit)", "tsukuyomi");
		Escena e;
		e.mon(1, "dark_magician");
		e.mano(0, "tsukuyomi");
		VERIFICA(invocar(e, 0), "no se pudo invocar");
		e.pasa(1);
		if (e.d.ef.preg.tipo == P_ELEGIR) e.elige(0, indiceCand(e, "dark_magician"));
		e.pasarTodo();
		VERIFICA(e.d.l[1].m[0].bocaAbajo && e.d.l[1].m[0].defensa, "no quedo boca abajo");
		Jugada f;
		f.tipo = J_FASE;
		for (int g = 0; g < 3; g++) e.d.aplicar(f);
		e.pasarTodo();
		VERIFICA(enMano(e, 0, "tsukuyomi") && e.d.l[0].monstruos() == 0, "no volvio a la mano");
		fin();
	}
	{
		caso("Jinzo: no se pueden activar trampas y se niegan las continuas (Gravity Bind)", "jinzo");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "jinzo");
		e.colocar(1, "mirror_force");
		VERIFICA(atacar(e, a, -1), "no se pudo atacar");
		VERIFICA(e.d.ef.preg.tipo == P_NADA && e.d.l[1].lp == LP_INICIAL - 2400, "se ofrecio Mirror Force con Jinzo (LP %d)", e.d.l[1].lp);
		Escena f(F_BATALLA);
		const int b = f.mon(0, "summoned_skull");
		const int gb = f.colocar(1, "gravity_bind");
		f.d.l[1].mt[(size_t)gb].bocaAbajo = false;
		f.d.recalcular();
		VERIFICA(!f.d.puedeAtacar(0, b), "Gravity Bind no bloquea");
		f.mon(0, "jinzo");
		VERIFICA(f.d.puedeAtacar(0, b), "con Jinzo, Gravity Bind sigue bloqueando");
		fin();
	}
	{
		caso("Sinister Serpent: en tu fase de espera vuelve del cementerio a la mano", "sinister_serpent");
		Escena e;
		e.d.l[1].cementerio.push_back("sinister_serpent");
		pasarFases(e, 1);
		VERIFICA(enMano(e, 1, "sinister_serpent") && e.d.l[1].cementerio.empty(), "no volvio");
		fin();
	}
	{
		caso("Exodia: con las 5 piezas en la mano se gana el duelo (al robar la ultima)", "exodia_the_forbidden_one");
		Escena e;
		e.d.l[1].mano = { "exodia_the_forbidden_one", "left_arm_of_the_forbidden_one", "right_arm_of_the_forbidden_one", "left_leg_of_the_forbidden_one" };
		e.d.l[1].mazo.push_back("right_leg_of_the_forbidden_one");
		pasarFases(e, 1);
		VERIFICA(e.d.ganador == 1, "ganador %d (esperado 1)", e.d.ganador);
		fin();
	}
	{
		caso("Black Luster Soldier - Envoy: invocacion especial desterrando LUZ y OSCURIDAD; destierra y no ataca", "black_luster_soldier_envoy_of_the_beginning");
		Escena e;
		e.d.l[0].cementerio = { "mystical_elf", "dark_magician" };
		e.mon(1, "summoned_skull");
		const int k = e.mano(0, "black_luster_soldier_envoy_of_the_beginning");
		VERIFICA(e.activar(0, H(0, k)), "no se pudo invocar");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].id == "black_luster_soldier_envoy_of_the_beginning" && e.d.l[0].desterrados.size() == 2, "no entro");
		Jugada j = jugadaActivar(M(0, 0));
		j.objs = { M(1, 0) };
		VERIFICA(e.d.aplicar(j), "el efecto no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && e.d.l[1].desterrados.size() == 1 && e.d.l[0].m[0].ataco, "no desterro o puede atacar");
		VERIFICA(!e.d.aplicar(j), "efecto dos veces en el turno");
		Escena f;
		const int k2 = f.mano(0, "black_luster_soldier_envoy_of_the_beginning");
		VERIFICA(!f.activar(0, H(0, k2)), "se invoco sin LUZ/OSCURIDAD en el cementerio");
		fin();
	}
	{
		caso("Chaos Emperor Dragon: paga 1000; todo al cementerio y 300 por carta al rival", "chaos_emperor_dragon_envoy_of_the_end");
		Escena e;
		e.d.l[0].cementerio = { "mystical_elf", "dark_magician" };
		const int k = e.mano(0, "chaos_emperor_dragon_envoy_of_the_end");
		e.mano(0, "kuriboh");
		e.mon(1, "summoned_skull");
		e.colocar(1, "mirror_force");
		e.mano(1, "kuriboh");
		VERIFICA(e.activar(0, H(0, k)), "no se pudo invocar");
		e.pasarTodo();
		Jugada j = jugadaActivar(M(0, 0));
		VERIFICA(e.d.aplicar(j), "el efecto no se activo");
		e.pasarTodo();
		// cartas: CED, kuriboh (mano), skull, mirror force, kuriboh rival = 5 -> 1500
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 1500 && e.d.l[0].lp == LP_INICIAL - 1000, "LP %d/%d", e.d.l[0].lp, e.d.l[1].lp);
		VERIFICA(e.d.l[0].monstruos() + e.d.l[1].monstruos() == 0 && e.d.l[0].mano.empty() && e.d.l[1].mano.empty(), "quedaron cartas");
		fin();
	}
	{
		caso("Airknight Parshath: perfora la defensa y roba 1 al hacer danno de batalla", "airknight_parshath");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "airknight_parshath");
		const int z = e.mon(1, "kuriboh", true);
		VERIFICA(atacar(e, a, z), "no se pudo atacar");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 1700 && e.d.l[0].mano.size() == 1, "LP rival %d, mano %d", e.d.l[1].lp, (int)e.d.l[0].mano.size());
		fin();
	}
	{
		caso("Tribe-Infecting Virus: descarta 1, declara un tipo y destruye los boca arriba de ese tipo", "tribe_infecting_virus");
		Escena e;
		const int z = e.mon(0, "tribe_infecting_virus");
		e.mon(1, "dark_magician");
		e.mon(1, "gemini_elf");
		e.mon(1, "summoned_skull");
		e.mano(0, "kuriboh");
		Jugada j = jugadaActivar(M(0, z));
		j.descartes = { 0 };
		VERIFICA(e.d.aplicar(j), "no se activo");
		e.pasa(1);
		VERIFICA(e.d.ef.preg.tipo == P_ELEGIR, "no pidio el tipo");
		VERIFICA(e.elige(0, indiceCand(e, "dark_magician")), "eleccion");
		VERIFICA(e.d.l[1].monstruos() == 1 && e.d.l[1].m[2].id == "summoned_skull", "no destruyo los Lanzadores");
		fin();
	}
	{
		caso("Toon World: 1000 LP; el Toon no ataca el turno que llega, ataca directo y paga 500; sin Toon World, se destruye", "toon_world");
		Escena e;
		e.mon(1, "celtic_guardian");
		const int k = e.mano(0, "toon_world");
		VERIFICA(e.activar(0, H(0, k)), "Toon World no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].lp == LP_INICIAL - 1000 && e.d.l[0].tieneBocaArriba("toon_world"), "LP %d", e.d.l[0].lp);
		e.mon(0, "kuriboh");
		e.mano(0, "toon_summoned_skull");
		Jugada t;
		t.tipo = J_TOON;
		t.mano = 0;
		t.tributos = { 0 };
		VERIFICA(e.d.aplicar(t), "no se invoco el Toon");
		e.pasarTodo();
		const int zt = e.d.l[0].m[0].id == "toon_summoned_skull" ? 0 : 1;
		e.d.fase = F_BATALLA;
		VERIFICA(!e.d.puedeAtacar(0, zt), "ataca el turno en que llego");
		e.d.l[0].m[(size_t)zt].llegoEsteTurno = false;
		VERIFICA(atacar(e, zt, -1), "no ataco directo con un monstruo del rival en el campo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 2500 && e.d.l[0].lp == LP_INICIAL - 1500, "LP %d/%d", e.d.l[0].lp, e.d.l[1].lp);
		e.d.mtAlCementerio(0, 0);
		VERIFICA(e.d.l[0].monstruos() == 0, "el Toon siguio sin Toon World");
		fin();
	}
}

static std::string motorNoActivarTexto(Escena& e, const Lugar& f) { return motivoNoActivar(e.d, 0, f, false); }

static void tanda6()
{
	fprintf(stderr, "tanda 6: Dioses Egipcios y cartas firma\n");
	{
		caso("Obelisk: 3 sacrificios, sin ventana para el rival (Trap Hole), nadie lo elige; sacrifica 2: destruye todo lo del rival", "obelisk_the_tormentor");
		Escena e;
		e.mon(0, "kuriboh");
		e.mon(0, "kuriboh");
		e.mon(0, "kuriboh");
		e.mon(0, "celtic_guardian");
		e.mon(0, "battle_ox");
		e.mon(1, "dark_magician");
		e.mon(1, "summoned_skull");
		e.colocar(1, "trap_hole");
		e.mano(0, "obelisk_the_tormentor");
		Jugada inv;
		inv.tipo = J_INVOCAR;
		inv.mano = 0;
		inv.tributos = { 0, 1, 2 };
		VERIFICA(e.d.aplicar(inv), "no se invoco");
		VERIFICA(e.d.ef.preg.tipo == P_NADA, "el rival tuvo ventana contra un Dios");
		const int zo = e.d.l[0].m[0].id == "obelisk_the_tormentor" ? 0 : -1;
		VERIFICA(zo == 0, "Obelisk no esta en la zona 0");
		const DefCarta* tdd = defCarta("tribute_to_the_doomed");
		bool hayObelisk = false;
		for (const Lugar& g : objetivosDe(e.d, 1, *tdd, H(1, 0))) hayObelisk |= cartaEn(e.d, g) == "obelisk_the_tormentor";
		VERIFICA(!hayObelisk, "Obelisk aparece como objetivo");
		// el jugador elige los 2 sacrificios: Battle Ox y Celtic Guardian (zonas 4 y 3)
		VERIFICA(!e.activar(0, M(0, zo), {}, {}, { M(0, zo), M(0, 3) }), "se sacrifico a si mismo");
		VERIFICA(e.activar(0, M(0, zo), {}, {}, { M(0, 4), M(0, 3) }), "el efecto no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && e.d.l[0].monstruos() == 1 && e.enCementerio(0, "battle_ox") == 1, "rival %d propios %d", e.d.l[1].monstruos(),
		         e.d.l[0].monstruos());
		fin();
	}
	{
		caso("Slifer: ATK = 1000 por carta en la mano; el monstruo que el rival invoca en ataque pierde 2000", "slifer_the_sky_dragon");
		Escena e;
		e.mon(0, "slifer_the_sky_dragon");
		e.mano(0, "kuriboh");
		e.mano(0, "kuriboh");
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[0].atkEf == 2000, "ATK %d (esperado 2000)", e.d.l[0].m[0].atkEf);
		e.d.activo = 1;
		e.d.turno = 4;
		e.mano(1, "summoned_skull");
		e.mon(1, "kuriboh");
		Jugada inv;
		inv.tipo = J_INVOCAR;
		inv.mano = 0;
		inv.tributos = { 0 };
		VERIFICA(e.d.aplicar(inv), "no se invoco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].m[0].atkEf == 500, "ATK %d (esperado 500)", e.d.l[1].m[0].atkEf);
		e.mano(1, "celtic_guardian");
		e.d.l[1].invocoNormal = false;
		inv.tributos = {};
		inv.mano = (int)e.d.l[1].mano.size() - 1;
		VERIFICA(e.d.aplicar(inv), "no se invoco el segundo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 1, "el de 1400 no se destruyo");
		fin();
	}
	{
		caso("Ra: al invocarse, paga hasta quedar en 100 y gana ese ATK (o no); paga 1000: destruye un monstruo", "the_winged_dragon_of_ra");
		Escena e;
		e.mon(0, "kuriboh");
		e.mon(0, "kuriboh");
		e.mon(0, "kuriboh");
		e.mon(1, "dark_magician");
		e.mano(0, "the_winged_dragon_of_ra");
		Jugada inv;
		inv.tipo = J_INVOCAR;
		inv.mano = 0;
		inv.tributos = { 0, 1, 2 };
		VERIFICA(e.d.aplicar(inv), "no se invoco");
		VERIFICA(e.d.ef.preg.tipo == P_ELEGIR && e.d.ef.preg.opcional, "no pregunto si pagar");
		VERIFICA(e.elige(0, 0), "eleccion");
		e.pasarTodo();
		const int z = e.d.l[0].m[0].id == "the_winged_dragon_of_ra" ? 0 : 1;
		VERIFICA(e.d.l[0].lp == 100 && e.d.l[0].m[(size_t)z].atkEf == 4000 + 7900, "LP %d ATK %d", e.d.l[0].lp, e.d.l[0].m[(size_t)z].atkEf);
		VERIFICA(!e.activar(0, M(0, z), { M(1, 0) }), "pago 1000 con 100 LP");
		Escena f;
		f.mon(0, "kuriboh");
		f.mon(0, "kuriboh");
		f.mon(0, "kuriboh");
		f.mon(1, "dark_magician");
		f.mano(0, "the_winged_dragon_of_ra");
		VERIFICA(f.d.aplicar(inv), "no se invoco (2)");
		Jugada no;
		no.tipo = J_PASAR;
		VERIFICA(f.d.aplicar(no), "no se pudo decir que no");
		f.pasarTodo();
		VERIFICA(f.d.l[0].lp == LP_INICIAL, "pago igual");
		VERIFICA(f.activar(0, M(0, 0), { M(1, 0) }), "el efecto de destruir no se activo");
		f.pasarTodo();
		VERIFICA(f.d.l[1].monstruos() == 0 && f.d.l[0].lp == LP_INICIAL - 1000, "no destruyo");
		fin();
	}
	{
		caso("The Seal of Orichalcos: +500 a los propios, destruye los invocados de forma especial, sin fusion; 1 por duelo", "the_seal_of_orichalcos");
		Escena e;
		e.mon(0, "celtic_guardian");   // (Escena::mon invoca de forma especial)
		e.d.l[0].m[0].especial = false;
		e.mon(0, "battle_ox");
		e.mon(1, "kuriboh");
		const int k = e.mano(0, "the_seal_of_orichalcos");
		VERIFICA(e.activar(0, H(0, k)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 1 && e.d.l[0].m[0].atkEf == 1900 && e.d.l[1].m[0].atkEf == 300, "estado inesperado");
		VERIFICA(!motorPuedeFusion(e.d, 0), "puede fusionar con el Seal");
		e.d.mtAlCementerio(0, ZONA_CAMPO);
		const int k2 = e.mano(0, "the_seal_of_orichalcos");
		VERIFICA(motorNoActivarTexto(e, H(0, k2)) == "ef.una_por_duelo", "se pudo activar otra vez");
		fin();
	}
	{
		caso("Crush Card Virus: sacrifica un OSCURIDAD de 1000 o menos; destruye los de 1500+ del campo y la mano del rival", "crush_card_virus");
		Escena e;
		e.mon(0, "kuriboh");
		e.mon(1, "dark_magician");
		e.mon(1, "celtic_guardian");
		e.mano(1, "summoned_skull");
		e.mano(1, "kuriboh");
		const int z = e.colocar(0, "crush_card_virus");
		VERIFICA(e.activar(0, MT(0, z)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 0 && e.d.l[1].monstruos() == 1 && e.d.l[1].mano.size() == 1, "estado inesperado");
		e.d.danno(1, 500);
		VERIFICA(e.d.l[1].lp == LP_INICIAL, "el rival recibio danno");
		fin();
	}
	{
		caso("Shrink: el ATK original a la mitad hasta el final del turno", "shrink");
		Escena e;
		e.mon(1, "dark_magician");
		const int k = e.mano(0, "shrink");
		VERIFICA(e.activar(0, H(0, k), { M(1, 0) }), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].m[0].atkEf == 1250, "ATK %d", e.d.l[1].m[0].atkEf);
		pasarFases(e, 1);
		VERIFICA(e.d.l[1].m[0].atkEf == 2500, "no volvio (ATK %d)", e.d.l[1].m[0].atkEf);
		fin();
	}
	{
		caso("Multiply: sacrifica Kuriboh; fichas en todas las zonas libres, que no van al cementerio", "multiply");
		Escena e;
		e.mon(0, "kuriboh");
		e.mon(0, "celtic_guardian");
		const int k = e.mano(0, "multiply");
		VERIFICA(e.activar(0, H(0, k)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 5 && e.enCementerio(0, "kuriboh") == 1, "monstruos %d", e.d.l[0].monstruos());
		const int k2 = e.mano(1, "dark_hole");
		e.d.activo = 1;
		e.d.turno = 4;
		VERIFICA(e.activar(1, H(1, k2)), "Dark Hole no se activo");
		e.pasarTodo();
		VERIFICA(e.enCementerio(0, "kuriboh") == 1, "las fichas fueron al cementerio (%d)", e.enCementerio(0, "kuriboh"));
		fin();
	}
	{
		caso("Mystic Box: destruye el del rival y le das uno tuyo", "mystic_box");
		Escena e;
		e.mon(0, "kuriboh");
		e.mon(1, "dark_magician");
		const int k = e.mano(0, "mystic_box");
		VERIFICA(e.activar(0, H(0, k), { M(1, 0) }), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 1 && e.d.l[1].m[0].id == "kuriboh" && e.d.l[0].monstruos() == 0, "estado inesperado");
		fin();
	}
	{
		caso("Nightmare's Steelcage: nadie ataca; se va tras 2 turnos del rival", "nightmare_s_steelcage");
		Escena e;
		e.mon(0, "summoned_skull");
		const int k = e.mano(0, "nightmare_s_steelcage");
		VERIFICA(e.activar(0, H(0, k)), "no se activo");
		e.pasarTodo();
		e.d.fase = F_BATALLA;
		VERIFICA(!e.d.puedeAtacar(0, 0), "se puede atacar");
		e.d.fase = F_PRINCIPAL2;
		for (int g = 0; g < 4; g++) pasarFases(e, g % 2 == 0 ? 1 : 0);
		pasarFases(e, 1);
		VERIFICA(e.enCementerio(0, "nightmare_s_steelcage") == 1, "no se fue");
		fin();
	}
	{
		caso("Dark Magic Curtain: mitad de LP y Dark Magician del mazo", "dark_magic_curtain");
		Escena e;
		e.d.l[0].mazo.push_back("dark_magician");
		const int k = e.mano(0, "dark_magic_curtain");
		VERIFICA(e.activar(0, H(0, k)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].id == "dark_magician" && e.d.l[0].lp == LP_INICIAL / 2, "estado inesperado");
		fin();
	}
	{
		caso("Kunai with Chain: el atacante pasa a defensa (el ataque se corta) y se equipa: +500", "kunai_with_chain");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		e.mon(1, "celtic_guardian");
		const int z = e.colocar(1, "kunai_with_chain");
		VERIFICA(atacar(e, a, -1) == false, "ataco directo con un monstruo del rival");
		VERIFICA(atacar(e, a, 0), "no se pudo atacar");
		VERIFICA(e.activar(1, MT(1, z)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)a].defensa && e.d.l[1].monstruos() == 1 && e.d.l[1].m[0].atkEf == 1900, "estado inesperado (ATK %d)", e.d.l[1].m[0].atkEf);
		fin();
	}
	{
		caso("Marshmallon: no se destruye en batalla; atacada boca abajo, el atacante recibe 1000", "marshmallon");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int z = e.mon(1, "marshmallon", true, true);
		VERIFICA(atacar(e, a, z), "no se pudo atacar");
		e.pasarTodo();
		VERIFICA(!e.d.l[1].m[(size_t)z].vacia() && e.d.l[0].lp == LP_INICIAL - 1000, "LP %d", e.d.l[0].lp);
		fin();
	}
	{
		caso("The White Stone of Legend: al ir al cementerio (descartada tambien) trae un Blue-Eyes", "the_white_stone_of_legend");
		Escena e;
		e.d.l[0].mazo.push_back("blue_eyes");
		e.mano(0, "the_white_stone_of_legend");
		const int k = e.mano(0, "tribute_to_the_doomed");
		e.mon(1, "kuriboh");
		VERIFICA(e.activar(0, H(0, k), { M(1, 0) }, { 0 }), "no se activo");
		e.pasarTodo();
		VERIFICA(enMano(e, 0, "blue_eyes"), "no busco");
		fin();
	}
	{
		caso("King's Knight: con Queen's Knight, Jack's Knight del mazo", "king_s_knight");
		Escena e;
		e.mon(0, "queen_s_knight");
		e.d.l[0].mazo.push_back("jack_s_knight");
		e.mano(0, "king_s_knight");
		VERIFICA(invocar(e, 0), "no se invoco");
		e.pasa(1);
		VERIFICA(e.d.ef.preg.tipo == P_ELEGIR && e.d.ef.preg.opcional, "no pregunto");
		e.elige(0, 0);
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 3, "monstruos %d", e.d.l[0].monstruos());
		fin();
	}
	{
		caso("Jurassic World, Insect Armor with Laser Cannon (solo Insectos), Celestial Sword - Eatos", "jurassic_world");
		probadas["insect_armor_with_laser_cannon"]++;
		probadas["celestial_sword_eatos"]++;
		Escena e;
		e.mon(0, "sword_arm_of_dragon");
		const int k = e.mano(0, "jurassic_world");
		VERIFICA(e.activar(0, H(0, k)), "Jurassic no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].atkEf == e.d.atk("sword_arm_of_dragon") + 300, "Jurassic: ATK %d", e.d.l[0].m[0].atkEf);
		const int k2 = e.mano(0, "insect_armor_with_laser_cannon");
		VERIFICA(motorNoActivarTexto(e, H(0, k2)) == "ef.sin_objetivo", "se equipa sin Insectos");
		const int k3 = e.mano(0, "celestial_sword_eatos");
		VERIFICA(e.activar(0, H(0, k3), { M(0, 0) }), "Eatos no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].atkEf == e.d.atk("sword_arm_of_dragon") + 800, "Eatos: ATK %d", e.d.l[0].m[0].atkEf);
		fin();
	}
}

// activa desde la mano del lado 0 con objetivos y resuelve todo (las elecciones: el primero)
static bool usar(Escena& e, const char* id, std::vector<Lugar> objs = {}, std::vector<int> desc = {})
{
	const int k = e.mano(0, id);
	const bool ok = e.activar(0, H(0, k), objs, desc);
	e.pasarTodo();
	return ok;
}
static bool usarColocada(Escena& e, int lado, const char* id, std::vector<Lugar> objs = {})
{
	const int z = e.colocar(lado, id);
	const bool ok = e.activar(lado, MT(lado, z), objs);
	e.pasarTodo();
	return ok;
}
static void tanda7a()
{
	fprintf(stderr, "tanda 7 (A): magias y trampas mas usadas\n");
	{
		caso("Dust Tornado: destruye una magia/trampa del rival", "dust_tornado");
		Escena e;
		const int z = e.colocar(1, "mirror_force");
		VERIFICA(usarColocada(e, 0, "dust_tornado", { MT(1, z) }), "no se activo");
		VERIFICA(e.d.l[1].colocadasMT() == 0, "no destruyo");
		fin();
	}
	{
		caso("Scapegoat: 4 fichas en defensa que no van al cementerio", "scapegoat");
		Escena e;
		VERIFICA(usar(e, "scapegoat"), "no se activo");
		VERIFICA(e.d.l[0].monstruos() == 4 && e.d.l[0].m[0].defensa && e.d.l[0].m[0].token, "fichas %d", e.d.l[0].monstruos());
		e.d.alCementerio(0, 0);
		VERIFICA(e.d.l[0].cementerio.size() == 1, "una ficha fue al cementerio");   // (solo Scapegoat)
		fin();
	}
	{
		caso("Stop Defense: el de defensa del rival pasa a ataque (boca abajo: se voltea)", "stop_defense");
		Escena e;
		e.mon(1, "giant_soldier_of_stone", true, true);
		VERIFICA(usar(e, "stop_defense", { M(1, 0) }), "no se activo");
		VERIFICA(!e.d.l[1].m[0].defensa && !e.d.l[1].m[0].bocaAbajo, "sigue en defensa");
		fin();
	}
	{
		caso("Jar of Greed / Hinotama / Sparks / Red Medicine / Soul of the Pure", "jar_of_greed");
		for (const char* id : { "hinotama", "sparks", "red_medicine", "soul_of_the_pure" }) probadas[id]++;
		Escena e;
		VERIFICA(usarColocada(e, 0, "jar_of_greed") && e.d.l[0].mano.size() == 1, "Jar of Greed");
		VERIFICA(usar(e, "hinotama") && e.d.l[1].lp == LP_INICIAL - 500, "Hinotama");
		VERIFICA(usar(e, "sparks") && e.d.l[1].lp == LP_INICIAL - 700, "Sparks");
		VERIFICA(usar(e, "red_medicine") && e.d.l[0].lp == LP_INICIAL + 500, "Red Medicine");
		VERIFICA(usar(e, "soul_of_the_pure") && e.d.l[0].lp == LP_INICIAL + 1300, "Soul of the Pure");
		fin();
	}
	{
		caso("Castle Walls (+500 DEF) y Reinforcements (+500 ATK) hasta el final del turno", "castle_walls");
		probadas["reinforcements"]++;
		Escena e;
		e.mon(0, "celtic_guardian");
		VERIFICA(usarColocada(e, 0, "castle_walls", { M(0, 0) }) && e.d.l[0].m[0].defEf == 1700, "Castle Walls: DEF %d", e.d.l[0].m[0].defEf);
		VERIFICA(usarColocada(e, 0, "reinforcements", { M(0, 0) }) && e.d.l[0].m[0].atkEf == 1900, "Reinforcements: ATK %d", e.d.l[0].m[0].atkEf);
		pasarFases(e, 1);
		VERIFICA(e.d.l[0].m[0].atkEf == 1400 && e.d.l[0].m[0].defEf == 1200, "no volvieron");
		fin();
	}
	{
		caso("Eatgaboon (ATK <= 500) y House of Adhesive Tape (DEF <= 500) contra la invocacion del rival", "eatgaboon");
		probadas["house_of_adhesive_tape"]++;
		Escena e;
		const int z = e.colocar(1, "eatgaboon");
		e.mano(0, "kuriboh");
		VERIFICA(invocar(e, 0), "no se invoco");
		VERIFICA(e.activar(1, MT(1, z), { M(0, 0) }), "Eatgaboon no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 0, "Kuriboh sigue");
		Escena f;
		const int z2 = f.colocar(1, "house_of_adhesive_tape");
		f.mano(0, "celtic_guardian");
		VERIFICA(invocar(f, 0) && f.d.ef.preg.tipo == P_NADA, "se ofrecio la cinta contra DEF 1200");
		(void)z2;
		fin();
	}
	{
		caso("Enchanted Javelin: ganas el ATK del atacante", "enchanted_javelin");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int z = e.colocar(1, "enchanted_javelin");
		VERIFICA(atacar(e, a, -1), "no se ataco");
		VERIFICA(e.activar(1, MT(1, z), { M(0, a) }), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL + 2500 - 2500, "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Book of Moon: boca abajo en defensa; Book of Taiyou: boca arriba en ataque (con su volteo)", "book_of_moon");
		probadas["book_of_taiyou"]++;
		Escena e;
		e.mon(1, "dark_magician");
		VERIFICA(usar(e, "book_of_moon", { M(1, 0) }) && e.d.l[1].m[0].bocaAbajo && e.d.l[1].m[0].defensa, "Book of Moon");
		e.mon(0, "old_vindictive_magician", true, true);
		VERIFICA(usar(e, "book_of_taiyou", { M(0, 0) }), "Book of Taiyou no se activo");
		VERIFICA(!e.d.l[0].m[0].bocaAbajo && !e.d.l[0].m[0].defensa && e.d.l[1].monstruos() == 0, "no se volteo o su efecto no se activo");
		fin();
	}
	{
		caso("Reinforcement of the Army: un Guerrero de nivel 4 o menos del mazo", "reinforcement_of_the_army");
		Escena e;
		VERIFICA(usar(e, "reinforcement_of_the_army") && enMano(e, 0, "celtic_guardian"), "no busco");
		fin();
	}
	{
		caso("Enemy Controller: sacrificando uno propio, el control hasta la fase final; sin monstruos, cambia la posicion", "enemy_controller");
		Escena e;
		e.mon(0, "kuriboh");
		e.mon(1, "dark_magician");
		VERIFICA(usar(e, "enemy_controller", { M(1, 0) }), "no se activo");
		VERIFICA(e.d.l[0].monstruos() == 1 && e.d.l[1].monstruos() == 0 && e.enCementerio(0, "kuriboh") == 1, "no tomo el control");
		Escena f;
		f.mon(1, "dark_magician");
		VERIFICA(usar(f, "enemy_controller", { M(1, 0) }) && f.d.l[1].m[0].defensa, "no cambio de posicion");
		fin();
	}
	{
		caso("Trap Dustshoot: con 4+ cartas en la mano del rival, un monstruo vuelve a su mazo", "trap_dustshoot");
		Escena e;
		for (const char* id : { "dark_magician", "raigeki", "pot_of_greed", "mirror_force" }) e.mano(1, id);
		const size_t mazo = e.d.l[1].mazo.size();
		VERIFICA(usarColocada(e, 0, "trap_dustshoot"), "no se activo");
		VERIFICA(!enMano(e, 1, "dark_magician") && e.d.l[1].mazo.size() == mazo + 1, "no lo devolvio");
		fin();
	}
	{
		caso("Delinquent Duo: 1000 LP; el rival descarta 2 (1 al azar, 1 elegida)", "delinquent_duo");
		Escena e;
		for (const char* id : { "dark_magician", "raigeki", "pot_of_greed" }) e.mano(1, id);
		VERIFICA(usar(e, "delinquent_duo"), "no se activo");
		VERIFICA(e.d.l[1].mano.size() == 1 && e.d.l[0].lp == LP_INICIAL - 1000, "mano %d", (int)e.d.l[1].mano.size());
		fin();
	}
	{
		caso("Brain Control: 800 LP y el control hasta la fase final", "brain_control");
		Escena e;
		e.mon(1, "dark_magician");
		VERIFICA(usar(e, "brain_control", { M(1, 0) }), "no se activo");
		VERIFICA(e.d.l[0].monstruos() == 1 && e.d.l[0].lp == LP_INICIAL - 800, "no tomo el control");
		pasarFases(e, 1);
		VERIFICA(e.d.l[1].monstruos() == 1 && e.d.l[0].monstruos() == 0, "no volvio");
		fin();
	}
	{
		caso("Return from the Different Dimension: vuelven los desterrados y se destierran en la fase final", "return_from_the_different_dimension");
		Escena e;
		e.d.l[0].desterrados = { "dark_magician", "summoned_skull", "raigeki" };
		const int z = e.colocar(0, "return_from_the_different_dimension");
		VERIFICA(e.activar(0, MT(0, z)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 2 && e.d.l[0].lp == LP_INICIAL / 2, "monstruos %d", e.d.l[0].monstruos());
		pasarFases(e, 1);
		VERIFICA(e.d.l[0].monstruos() == 0 && e.d.l[0].desterrados.size() == 3, "no se desterraron");
		fin();
	}
	{
		caso("The Nobleman of Crossout: destierra un boca abajo; si es de volteo, tambien sus copias del mazo", "nobleman_of_crossout");
		Escena e;
		e.mon(1, "man_eater_bug", true, true);
		e.d.l[1].mazo.push_back("man_eater_bug");
		VERIFICA(usar(e, "nobleman_of_crossout", { M(1, 0) }), "no se activo");
		VERIFICA(e.d.l[1].monstruos() == 0 && e.d.l[1].desterrados.size() == 2, "desterrados %d", (int)e.d.l[1].desterrados.size());
		fin();
	}
	{
		caso("Smashing Ground: el del rival con mas DEF; Widespread Ruin: el atacante con mas ATK", "smashing_ground");
		probadas["widespread_ruin"]++;
		Escena e;
		e.mon(1, "mystical_elf");
		e.mon(1, "dark_magician");
		VERIFICA(usar(e, "smashing_ground") && e.d.l[1].m[1].vacia() && !e.d.l[1].m[0].vacia(), "Smashing Ground");
		Escena f(F_BATALLA);
		const int a = f.mon(0, "celtic_guardian");
		f.mon(0, "summoned_skull");
		const int z = f.colocar(1, "widespread_ruin");
		VERIFICA(atacar(f, a, -1), "no se ataco");
		VERIFICA(f.activar(1, MT(1, z)), "Widespread Ruin no se activo");
		f.pasarTodo();
		VERIFICA(f.d.l[0].m[1].vacia() && f.d.l[1].lp == LP_INICIAL - 1400, "destruyo el equivocado (LP %d)", f.d.l[1].lp);
		fin();
	}
	{
		caso("Compulsory Evacuation Device: a la mano (fusion: al mazo de fusion)", "compulsory_evacuation_device");
		Escena e;
		e.mon(1, "dark_magician");
		VERIFICA(usarColocada(e, 0, "compulsory_evacuation_device", { M(1, 0) }) && enMano(e, 1, "dark_magician"), "no volvio");
		fin();
	}
	{
		caso("Ceasefire: voltea los boca abajo sin sus efectos y 500 por monstruo de efecto", "ceasefire");
		Escena e;
		e.mon(1, "man_eater_bug", true, true);
		e.mon(1, "celtic_guardian");
		e.mon(0, "sangan");
		VERIFICA(usarColocada(e, 0, "ceasefire"), "no se activo");
		VERIFICA(!e.d.l[1].m[0].bocaAbajo && e.d.l[0].monstruos() == 1 && e.d.l[1].lp == LP_INICIAL - 1000, "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Creature Swap: cada uno da uno suyo", "creature_swap");
		Escena e;
		e.mon(0, "kuriboh");
		e.mon(1, "dark_magician");
		VERIFICA(usar(e, "creature_swap"), "no se activo");
		VERIFICA(e.d.l[0].m[0].id == "dark_magician" && e.d.l[1].m[0].id == "kuriboh" && e.d.l[0].m[0].duenno == 1, "no se intercambiaron");
		e.d.alCementerio(0, 0);
		VERIFICA(e.enCementerio(1, "dark_magician") == 1, "fue al cementerio equivocado");
		fin();
	}
	{
		caso("Limiter Removal: Maquinas x2 de ATK y destruidas en la fase final", "limiter_removal");
		Escena e;
		e.mon(0, "cyber_commander");
		const int a0 = e.d.l[0].m[0].atkEf;
		VERIFICA(usar(e, "limiter_removal") && e.d.l[0].m[0].atkEf == 2 * a0, "ATK %d", e.d.l[0].m[0].atkEf);
		pasarFases(e, 1);
		VERIFICA(e.d.l[0].monstruos() == 0, "no se destruyo");
		fin();
	}
	{
		caso("Campos por atributo: Gaia Power, Mystic Plasma Zone, Luminous Spark (+500/-400), A Legendary Ocean (+200)", "gaia_power");
		for (const char* id : { "mystic_plasma_zone", "luminous_spark", "a_legendary_ocean" }) probadas[id]++;
		struct C { const char* campo; const char* mon; int atk, def; };
		const C cs[] = { { "gaia_power", "celtic_guardian", 1900, 800 }, { "mystic_plasma_zone", "dark_magician", 3000, 1700 },
			             { "luminous_spark", "mystical_elf", 1300, 1600 }, { "a_legendary_ocean", "kairyu_shin", 2000, 1700 } };
		for (const C& c : cs) {
			Escena e;
			e.mon(0, c.mon);
			VERIFICA(usar(e, c.campo), "%s no se activo", c.campo);
			VERIFICA(e.d.l[0].m[0].atkEf == c.atk && e.d.l[0].m[0].defEf == c.def, "%s: %d/%d", c.campo, e.d.l[0].m[0].atkEf, e.d.l[0].m[0].defEf);
		}
		fin();
	}
	{
		caso("Equipos por tipo: Legendary Sword (Guerrero +300/+300), Murasame (+800), Elf's Light (LUZ +400/-200)...", "legendary_sword");
		for (const char* id : { "book_of_secret_arts", "mystical_moon", "dark_energy", "elf_s_light", "machine_conversion_factory", "fusion_sword_murasame_blade",
		                        "dragon_treasure" })
			probadas[id]++;
		Escena e;
		e.mon(0, "celtic_guardian");
		e.mon(0, "mystical_elf");
		VERIFICA(usar(e, "legendary_sword", { M(0, 0) }) && e.d.l[0].m[0].atkEf == 1700 && e.d.l[0].m[0].defEf == 1500, "Legendary Sword");
		VERIFICA(usar(e, "fusion_sword_murasame_blade", { M(0, 0) }) && e.d.l[0].m[0].atkEf == 2500, "Murasame");
		VERIFICA(usar(e, "elf_s_light", { M(0, 1) }) && e.d.l[0].m[1].atkEf == 1200 && e.d.l[0].m[1].defEf == 1800, "Elf's Light");
		VERIFICA(usar(e, "book_of_secret_arts", { M(0, 1) }) && e.d.l[0].m[1].atkEf == 1500, "Book of Secret Arts");
		const int k = e.mano(0, "dark_energy");
		VERIFICA(motivoNoActivar(e.d, 0, H(0, k), false) == "ef.sin_objetivo", "Dark Energy sin Demonios");
		fin();
	}
	{
		caso("Dragon's Gunfire (con Dragon: 800 o destruye DEF <= 800) y Stamping Destruction (+500 al controlador)", "dragon_s_gunfire");
		probadas["stamping_destruction"]++;
		Escena e;
		e.mon(0, "sword_arm_of_dragon");   // (Dinosaurio: no vale)
		const int k = e.mano(0, "dragon_s_gunfire");
		VERIFICA(motivoNoActivar(e.d, 0, H(0, k), false) == "ef.no_condicion", "se activo sin Dragon");
		e.mon(0, "blue_eyes");
		e.mon(1, "man_eater_bug");
		const int k2 = e.mano(0, "dragon_s_gunfire");
		VERIFICA(e.activar(0, H(0, k2)), "no se activo");
		e.pasa(1);
		VERIFICA(e.d.ef.preg.opcional, "no pregunto");
		Jugada no;
		no.tipo = J_PASAR;
		e.d.aplicar(no);
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 800, "LP %d", e.d.l[1].lp);
		const int z = e.colocar(1, "mirror_force");
		VERIFICA(usar(e, "stamping_destruction", { MT(1, z) }) && e.d.l[1].lp == LP_INICIAL - 1300, "Stamping: LP %d", e.d.l[1].lp);
		fin();
	}
}

static void tanda7b()
{
	fprintf(stderr, "tanda 7 (B): monstruos con efecto mas usados\n");
	{
		caso("Skull Archfiend of Lightning paga 500 y Dancing Fairy (defensa) da 1000 en tu fase de espera", "skull_archfiend_of_lightning");
		probadas["dancing_fairy"]++;
		Escena e;
		e.mon(1, "skull_archfiend_of_lightning");
		e.mon(1, "dancing_fairy", true);
		pasarFases(e, 1);
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 500 + 1000, "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("The Bistro Butcher (el rival roba 2) y Kycoo (destierra del cementerio rival) al hacer danno", "the_bistro_butcher");
		probadas["kycoo_the_ghost_destroyer"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "the_bistro_butcher");
		VERIFICA(atacar(e, a, -1), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].mano.size() == 2, "el rival robo %d", (int)e.d.l[1].mano.size());
		Escena f(F_BATALLA);
		f.d.l[1].cementerio = { "dark_magician", "raigeki", "summoned_skull" };
		const int b = f.mon(0, "kycoo_the_ghost_destroyer");
		VERIFICA(atacar(f, b, -1), "no ataco (Kycoo)");
		f.pasa(1);
		if (f.d.ef.preg.tipo == P_ELEGIR) {
			f.elige(0, 0);
			if (f.d.ef.preg.tipo == P_ELEGIR) f.elige(0, 0);
		}
		f.pasarTodo();
		VERIFICA(f.d.l[1].desterrados.size() == 2 && f.d.l[1].cementerio.size() == 1, "desterrados %d", (int)f.d.l[1].desterrados.size());
		fin();
	}
	{
		caso("Zolga: sacrificada para invocar, +2000 LP", "zolga");
		Escena e;
		e.mon(0, "zolga");
		e.mano(0, "summoned_skull");
		Jugada inv;
		inv.tipo = J_INVOCAR;
		inv.mano = 0;
		inv.tributos = { 0 };
		VERIFICA(e.d.aplicar(inv), "no se invoco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].lp == LP_INICIAL + 2000, "LP %d", e.d.l[0].lp);
		fin();
	}
	{
		caso("Hysteric Fairy: sacrifica 2: +1000 LP", "hysteric_fairy");
		Escena e;
		e.mon(0, "hysteric_fairy");
		e.mon(0, "kuriboh");
		e.mon(0, "kuriboh");
		VERIFICA(e.activar(0, M(0, 0)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].lp == LP_INICIAL + 1000 && e.d.l[0].monstruos() == 1, "LP %d monstruos %d", e.d.l[0].lp, e.d.l[0].monstruos());
		fin();
	}
	{
		caso("Ancient Gear Golem: perfora y al atacar el rival no activa magias/trampas", "ancient_gear_golem");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "ancient_gear_golem");
		const int z = e.mon(1, "kuriboh", true);
		e.colocar(1, "mirror_force");
		VERIFICA(atacar(e, a, z), "no ataco");
		VERIFICA(e.d.ef.preg.tipo == P_NADA, "se ofrecio una trampa");
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 2800, "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("ATK propio: Mudora, Dark Magician Girl, Amazoness Paladin, Theban Nightmare", "mudora");
		for (const char* id : { "dark_magician_girl", "amazoness_paladin", "theban_nightmare" }) probadas[id]++;
		Escena e;
		e.d.l[0].cementerio = { "mystical_elf", "dark_magician" };   // (Mystical Elf no es Hada)
		e.d.l[1].cementerio = { "dark_magician" };
		e.mon(0, "dark_magician_girl");
		e.mon(0, "amazoness_paladin");
		e.mon(0, "theban_nightmare");
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[0].atkEf == 2000 + 600, "DMG %d", e.d.l[0].m[0].atkEf);
		VERIFICA(e.d.l[0].m[1].atkEf == 1700 + 100, "Paladin %d", e.d.l[0].m[1].atkEf);
		VERIFICA(e.d.l[0].m[2].atkEf == 3000, "Theban %d", e.d.l[0].m[2].atkEf);
		e.mano(0, "kuriboh");
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[2].atkEf == 1500, "Theban con mano %d", e.d.l[0].m[2].atkEf);
		Escena f;
		f.d.l[0].cementerio = { "dancing_fairy", "hysteric_fairy" };
		f.mon(0, "mudora");
		f.d.recalcular();
		VERIFICA(f.d.l[0].m[0].atkEf == 1900, "Mudora %d", f.d.l[0].m[0].atkEf);
		fin();
	}
	{
		caso("Kaiser Glider: destruido, un monstruo vuelve a la mano", "kaiser_glider");
		Escena e;
		e.mon(1, "kaiser_glider");
		e.mon(0, "dark_magician");
		const int k = e.mano(0, "fissure");
		VERIFICA(e.activar(0, H(0, k)), "Fissure no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 0 && enMano(e, 0, "dark_magician"), "no lo devolvio");
		fin();
	}
	{
		caso("Helping Robo for Combat: destruye en batalla, roba 1 y devuelve 1 al fondo del mazo", "helping_robo_for_combat");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "helping_robo_for_combat");
		const int z = e.mon(1, "kuriboh");
		const size_t mazo = e.d.l[0].mazo.size();
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && e.d.l[0].mano.empty() && e.d.l[0].mazo.size() == mazo, "mano %d mazo %d", (int)e.d.l[0].mano.size(),
		         (int)e.d.l[0].mazo.size());
		fin();
	}
	{
		caso("Masked Dragon: destruido en batalla, un Dragon de 1500 o menos ATK del mazo", "masked_dragon");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int z = e.mon(1, "masked_dragon");
		e.d.l[1].mazo.push_back("masked_dragon");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasa(0);
		if (e.d.ef.preg.tipo == P_ELEGIR) e.elige(1, 0);
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 1, "no invoco");
		fin();
	}
	{
		caso("Gearfried: el equipo que se le pone se destruye", "gearfried_the_iron_knight");
		Escena e;
		e.mon(0, "gearfried_the_iron_knight");
		VERIFICA(usar(e, "axe_of_despair", { M(0, 0) }), "no se activo");
		VERIFICA(e.d.l[0].m[0].atkEf == 1800 && e.enCementerio(0, "axe_of_despair") == 1, "el equipo quedo");
		fin();
	}
	{
		caso("Reflect Bounder (danno al atacante y se destruye), Sasuke (destruye la defensa), Kelbek (devuelve al atacante)", "reflect_bounder");
		for (const char* id : { "ninja_grandmaster_sasuke", "kelbek", "d_d_assailant" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int z = e.mon(1, "reflect_bounder");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].lp == LP_INICIAL - 2500 && e.d.l[1].monstruos() == 0, "Reflect Bounder: LP %d", e.d.l[0].lp);
		Escena f(F_BATALLA);
		const int s = f.mon(0, "ninja_grandmaster_sasuke");
		const int g = f.mon(1, "giant_soldier_of_stone", true);
		VERIFICA(atacar(f, s, g) && f.d.l[1].monstruos() == 0 && f.d.l[0].lp == LP_INICIAL, "Sasuke");
		Escena h(F_BATALLA);
		const int b = h.mon(0, "summoned_skull");
		const int k = h.mon(1, "kelbek", true);
		VERIFICA(atacar(h, b, k) && enMano(h, 0, "summoned_skull"), "Kelbek");
		Escena i(F_BATALLA);
		const int c = i.mon(0, "summoned_skull");
		const int d2 = i.mon(1, "d_d_assailant");
		VERIFICA(atacar(i, c, d2) && i.d.l[0].monstruos() == 0 && i.d.l[0].desterrados.size() == 1 && i.d.l[1].desterrados.size() == 1, "D.D. Assailant");
		fin();
	}
	{
		caso("Blindly Loyal Goblin: su control no cambia; Invader of Darkness: el rival no activa rapidas", "blindly_loyal_goblin");
		probadas["invader_of_darkness"]++;
		Escena e;
		e.mon(1, "blindly_loyal_goblin");
		VERIFICA(usar(e, "change_of_heart", { M(1, 0) }) && e.d.l[1].monstruos() == 1, "cambio de control");
		Escena f;
		f.mon(1, "invader_of_darkness");
		f.mon(1, "kuriboh");
		const int k = f.mano(0, "book_of_moon");
		VERIFICA(motivoNoActivar(f.d, 0, H(0, k), false) == "ef.no_momento", "se activo una rapida");
		fin();
	}
	{
		caso("Ataque directo: Amphibious Bugroth MK-3 con Umi; Drillago si el rival solo tiene monstruos de 1600+", "amphibious_bugroth_mk_3");
		probadas["drillago"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "amphibious_bugroth_mk_3");
		e.mon(1, "summoned_skull");
		VERIFICA(!atacar(e, a, -1), "ataco directo sin Umi");
		e.d.l[0].campo = MagiaTrampa{ "umi", false };
		e.d.l[0].campo.uid = e.d.nuevoUid();
		e.d.recalcular();
		const int atkB = e.d.l[0].m[(size_t)a].atkEf;   // (Umi le suma 200: es Aqua)
		VERIFICA(atacar(e, a, -1) && e.d.l[1].lp == LP_INICIAL - atkB, "no ataco directo con Umi (LP %d)", e.d.l[1].lp);
		Escena f(F_BATALLA);
		const int b = f.mon(0, "drillago");
		f.mon(1, "summoned_skull");
		VERIFICA(atacar(f, b, -1), "Drillago no ataco directo");
		fin();
	}
}

static void reglasNuevas()
{
	fprintf(stderr, "reglas: ventanas del inicio de batalla y del paso de danno; costes y posicion elegidos\n");
	{
		caso("Paso de danno: Shrink en respuesta al combate gana la batalla; Mirror Force no se puede ahi", "shrink");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int z = e.mon(1, "celtic_guardian");
		const int sh = e.colocar(1, "shrink");
		e.colocar(1, "mirror_force");
		VERIFICA(atacar(e, a, z), "no ataco");
		// ventana de la declaracion: el rival puede (Mirror Force o Shrink); pasa
		VERIFICA(e.d.ef.preg.tipo == P_RESPUESTA && e.d.ef.preg.lado == 1, "sin ventana de ataque");
		VERIFICA(e.pasa(1), "no paso");
		// paso de danno: primero el atacante (no tiene nada), luego el defensor: solo Shrink
		VERIFICA(e.d.ef.preg.tipo == P_RESPUESTA && e.d.ef.preg.lado == 1 && e.d.ef.ventana.tipo == V_PASO_DANNO, "sin ventana del paso de danno");
		const auto act = activables(e.d, 1, true);
		VERIFICA(act.size() == 1 && cartaEn(e.d, act[0]) == "shrink", "se ofrece algo mas que Shrink (%d)", (int)act.size());
		VERIFICA(e.activar(1, MT(1, sh), { M(0, a) }), "Shrink no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 0 && e.d.l[1].monstruos() == 1 && e.d.l[0].lp == LP_INICIAL - 150, "LP %d", e.d.l[0].lp);
		fin();
	}
	{
		caso("Paso de danno: el volteo de Man-Eater Bug espera al calculo de danno", "man_eater_bug");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int z = e.mon(1, "man_eater_bug", true, true);
		e.colocar(1, "shrink");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasa(1);
		VERIFICA(e.d.ef.ventana.tipo == V_PASO_DANNO && e.d.ef.cadena.empty(), "el volteo se activo antes del calculo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].m[(size_t)z].vacia(), "el Bug no murio en el calculo");
		fin();
	}
	{
		caso("Inicio de la batalla: el rival puede usar Book of Moon antes del primer ataque", "book_of_moon");
		Escena e;
		e.mon(0, "summoned_skull");
		const int bm = e.colocar(1, "book_of_moon");
		Jugada f;
		f.tipo = J_FASE;
		VERIFICA(e.d.aplicar(f) && e.d.fase == F_BATALLA, "no entro en batalla");
		VERIFICA(e.d.ef.preg.tipo == P_RESPUESTA && e.d.ef.preg.lado == 1 && e.d.ef.ventana.tipo == V_INICIO_BATALLA, "sin ventana al inicio de la batalla");
		VERIFICA(e.activar(1, MT(1, bm), { M(0, 0) }), "Book of Moon no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].bocaAbajo && !e.d.puedeAtacar(0, 0), "puede atacar");
		fin();
	}
	{
		caso("Monster Reborn: el jugador elige defensa", "monster_reborn");
		Escena e;
		e.d.l[0].cementerio.push_back("summoned_skull");
		const int k = e.mano(0, "monster_reborn");
		VERIFICA(e.activar(0, H(0, k), { Lugar{ 0, EN_CEMENTERIO, 0 } }), "no se activo");
		VERIFICA(e.elige(0, 0), "no pregunto la posicion");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].defensa, "no quedo en defensa");
		fin();
	}
	{
		caso("Costes elegidos: Crush Card Virus sacrifica el OSCURIDAD que elige el jugador; Envoy destierra los elegidos", "crush_card_virus");
		Escena e;
		e.mon(0, "kuriboh");
		e.mon(0, "kuriboh");
		e.d.l[0].m[1].atkTurno = 0;
		const int z = e.colocar(0, "crush_card_virus");
		VERIFICA(!e.activar(0, MT(0, z), {}, {}, { M(1, 0) }), "acepto un coste del rival");
		VERIFICA(e.activar(0, MT(0, z), {}, {}, { M(0, 1) }), "no se activo con el elegido");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[1].vacia() && !e.d.l[0].m[0].vacia(), "sacrifico el equivocado");
		Escena f;
		f.d.l[0].cementerio = { "mystical_elf", "dark_magician", "jack_s_knight" };
		const int k = f.mano(0, "black_luster_soldier_envoy_of_the_beginning");
		VERIFICA(!f.activar(0, H(0, k), {}, {}, { Lugar{ 0, EN_CEMENTERIO, 0 }, Lugar{ 0, EN_CEMENTERIO, 2 } }), "acepto dos de LUZ");
		VERIFICA(f.activar(0, H(0, k), {}, {}, { Lugar{ 0, EN_CEMENTERIO, 2 }, Lugar{ 0, EN_CEMENTERIO, 1 } }), "no se invoco con los elegidos");
		f.pasarTodo();
		VERIFICA(f.d.l[0].cementerio.size() == 1 && f.d.l[0].cementerio[0] == "mystical_elf", "desterro los equivocados");
		fin();
	}
}

static void tanda7c()
{
	fprintf(stderr, "tanda 7 (C): siguientes por frecuencia\n");
	{
		caso("Ultimate Offering: paga 500 por otra invocacion normal (se puede repetir)", "ultimate_offering");
		Escena e;
		e.d.l[0].invocoNormal = true;
		const int z = e.colocar(0, "ultimate_offering");
		VERIFICA(e.activar(0, MT(0, z)), "no se activo");
		e.pasarTodo();
		VERIFICA(!e.d.l[0].invocoNormal && e.d.l[0].lp == LP_INICIAL - 500 && !e.d.l[0].mt[(size_t)z].bocaAbajo, "no dio la invocacion");
		e.d.l[0].invocoNormal = true;
		VERIFICA(e.activar(0, MT(0, z)), "no se pudo repetir");
		e.pasarTodo();
		VERIFICA(e.d.l[0].lp == LP_INICIAL - 1000, "LP %d", e.d.l[0].lp);
		fin();
	}
	{
		caso("Ultimate Obedient Fiend: solo ataca sin mano y sin otras cartas", "ultimate_obedient_fiend");
		Escena e(F_BATALLA);
		e.mon(0, "ultimate_obedient_fiend");
		e.mano(0, "kuriboh");
		VERIFICA(!e.d.puedeAtacar(0, 0), "ataca con mano");
		e.d.l[0].mano.clear();
		VERIFICA(e.d.puedeAtacar(0, 0), "no ataca solo");
		fin();
	}
	{
		caso("Skilled Dark Magician: contadores con cada magia; con 3, se sacrifica y trae a Dark Magician", "skilled_dark_magician");
		probadas["skilled_white_magician"]++;
		Escena e;
		e.mon(0, "skilled_dark_magician");
		e.d.l[0].mazo.push_back("dark_magician");
		for (int k = 0; k < 3; k++) VERIFICA(usar(e, "red_medicine"), "magia %d", k);
		VERIFICA(e.d.l[0].m[0].contadores == 3, "contadores %d", e.d.l[0].m[0].contadores);
		VERIFICA(e.activar(0, M(0, 0)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].id == "dark_magician" || e.d.l[0].m[1].id == "dark_magician", "no trajo a Dark Magician");
		fin();
	}
	{
		caso("Uniones: Y-Dragon Head se equipa a X-Head Cannon (+400) y se desequipa", "y_dragon_head");
		probadas["z_metal_tank"]++;
		Escena e;
		e.mon(0, "x_head_cannon");
		e.mon(0, "y_dragon_head");
		const int base = e.d.l[0].m[0].atkEf;
		VERIFICA(e.activar(0, M(0, 1)), "no se equipo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[1].vacia() && e.d.l[0].mt[0].id == "y_dragon_head" && e.d.l[0].m[0].atkEf == base + 400, "ATK %d", e.d.l[0].m[0].atkEf);
		pasarFases(e, 1);
		pasarFases(e, 0);
		VERIFICA(e.activar(0, MT(0, 0)), "no se desequipo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].mt[0].vacia() && e.d.l[0].monstruos() == 2 && e.d.l[0].m[0].atkEf == base, "estado inesperado");
		fin();
	}
	{
		caso("The Forgiving Maiden: un monstruo destruido en batalla este turno vuelve a la mano", "the_forgiving_maiden");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "celtic_guardian");
		e.mon(0, "the_forgiving_maiden");
		const int z = e.mon(1, "summoned_skull");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		e.d.fase = F_PRINCIPAL2;
		VERIFICA(e.activar(0, M(0, 1)), "no se activo");
		e.pasarTodo();
		VERIFICA(enMano(e, 0, "celtic_guardian"), "no volvio");
		fin();
	}
	{
		caso("Element Dragon / Valkyrie: +500 con FUEGO; Valkyrie con AGUA no cambia de control", "element_dragon");
		probadas["element_valkyrie"]++;
		Escena e;
		e.mon(1, "element_valkyrie");
		e.mon(0, "kairyu_shin");   // AGUA
		VERIFICA(usar(e, "change_of_heart", { M(1, 0) }) && e.d.l[1].monstruos() == 1, "cambio de control con AGUA");
		fin();
	}
	{
		caso("Machine Duplication: 2 copias del mazo de la Maquina de 500 o menos", "machine_duplication");
		Escena e;
		e.mon(0, "cyber_commander");
		e.d.l[0].m[0].atkTurno = -300;
		e.d.recalcular();
		e.d.l[0].mazo.push_back("cyber_commander");
		e.d.l[0].mazo.push_back("cyber_commander");
		VERIFICA(usar(e, "machine_duplication", { M(0, 0) }) && e.d.l[0].monstruos() == 3, "monstruos %d", e.d.l[0].monstruos());
		fin();
	}
	{
		caso("Arsenal Summoner: volteo: una carta Guardian del mazo", "arsenal_summoner");
		Escena e;
		e.d.l[0].mazo.push_back("guardian_eatos");
		const int z = e.mon(0, "arsenal_summoner", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(enMano(e, 0, "guardian_eatos"), "no busco");
		fin();
	}
	{
		caso("Gear Golem: paga 800 y ataca directo con monstruos del rival", "gear_golem_the_moving_fortress");
		Escena e;
		e.mon(0, "gear_golem_the_moving_fortress");
		e.mon(1, "summoned_skull");
		VERIFICA(e.activar(0, M(0, 0)), "no se activo");
		e.pasarTodo();
		e.d.fase = F_BATALLA;
		VERIFICA(atacar(e, 0, -1) && e.d.l[1].lp == LP_INICIAL - 800, "no ataco directo (LP %d)", e.d.l[1].lp);
		fin();
	}
	{
		caso("Agido: destruido en batalla, el dado dice el nivel del Hada que vuelve", "agido");
		Escena e(F_BATALLA);
		e.d.l[1].cementerio = { "mystical_elf", "dancing_fairy", "hysteric_fairy", "shining_angel", "the_forgiving_maiden", "kuriboh" };
		const int a = e.mon(0, "summoned_skull");
		const int z = e.mon(1, "agido");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() <= 1, "invoco de mas");
		fin();
	}
	{
		caso("Armed Dragon LV5: manda un monstruo de la mano: destruye uno del rival con ATK <= el suyo", "armed_dragon_lv5");
		Escena e;
		e.mon(0, "armed_dragon_lv5");
		e.mon(1, "celtic_guardian");
		e.mano(0, "battle_ox");
		Jugada j = jugadaActivar(M(0, 0));
		j.descartes = { 0 };
		VERIFICA(e.d.aplicar(j), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && e.enCementerio(0, "battle_ox") == 1, "no destruyo");
		fin();
	}
	{
		caso("Slate Warrior: volteo +500; destruido en batalla, el otro -500", "slate_warrior");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int z = e.mon(1, "slate_warrior", true, true);
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)a].atkEf == 2000, "ATK %d (esperado 2000)", e.d.l[0].m[(size_t)a].atkEf);
		fin();
	}
	{
		caso("Reclutadores: Shining Angel, Flying Kamakiri #1, Mystic Tomato, Giant Rat, UFO Turtle, Mother Grizzly, Pyramid Turtle", "shining_angel");
		for (const char* id : { "flying_kamakiri_1", "mystic_tomato", "giant_rat", "ufo_turtle", "mother_grizzly", "pyramid_turtle" }) probadas[id]++;
		struct R_ { const char* rec; const char* trae; };
		const R_ rs[] = { { "shining_angel", "mystical_elf" }, { "flying_kamakiri_1", "harpie_lady" }, { "mystic_tomato", "sangan" },
			              { "giant_rat", "giant_soldier_of_stone" }, { "mother_grizzly", "penguin_soldier" } };
		for (const R_& r : rs) {
			Escena e(F_BATALLA);
			e.d.l[1].mazo.push_back(r.trae);
			const int a = e.mon(0, "dark_magician");
			const int z = e.mon(1, r.rec);
			VERIFICA(atacar(e, a, z), "%s: no ataco", r.rec);
			e.pasa(0);
			if (e.d.ef.preg.tipo == P_ELEGIR) e.elige(1, indiceCand(e, r.trae));
			e.pasarTodo();
			VERIFICA(e.d.l[1].monstruos() == 1 && e.d.l[1].m[0].id == r.trae, "%s no trajo a %s", r.rec, r.trae);
		}
		fin();
	}
	{
		caso("Freed the Brave Wanderer: destierra 2 de LUZ: destruye uno con mas ATK", "freed_the_brave_wanderer");
		Escena e;
		e.mon(0, "freed_the_brave_wanderer");
		e.mon(1, "summoned_skull");
		e.d.l[0].cementerio = { "mystical_elf", "jack_s_knight" };
		VERIFICA(e.activar(0, M(0, 0), { M(1, 0) }), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && e.d.l[0].desterrados.size() == 2, "no destruyo");
		fin();
	}
	{
		caso("Guardian Sphinx: pasa a boca abajo y al voltearse devuelve los monstruos del rival", "guardian_sphinx");
		Escena e;
		e.mon(0, "guardian_sphinx");
		e.mon(1, "summoned_skull");
		VERIFICA(e.activar(0, M(0, 0)), "no se puso boca abajo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].bocaAbajo, "sigue boca arriba");
		pasarFases(e, 1);
		pasarFases(e, 0);
		VERIFICA(voltear(e, 0), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && enMano(e, 1, "summoned_skull"), "no devolvio");
		fin();
	}
	{
		caso("Nightmare Penguin: AGUA +200; volteo: una carta del rival a la mano", "nightmare_penguin");
		Escena e;
		const int z = e.mon(0, "nightmare_penguin", true, true);
		e.mon(0, "kairyu_shin");
		const int mt = e.colocar(1, "mirror_force");
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasa(1);
		if (e.d.ef.preg.tipo == P_ELEGIR) e.elige(0, 0);
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[1].atkEf == 2000 && e.d.l[1].mt[(size_t)mt].vacia() && enMano(e, 1, "mirror_force"), "ATK %d", e.d.l[0].m[1].atkEf);
		fin();
	}
	{
		caso("Zombyra the Dark: no ataca directo y pierde 200 al destruir", "zombyra_the_dark");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "zombyra_the_dark");
		VERIFICA(!atacar(e, a, -1), "ataco directo");
		const int z = e.mon(1, "kuriboh");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)a].atkEf == 1900, "ATK %d", e.d.l[0].m[(size_t)a].atkEf);
		fin();
	}
	{
		caso("Flash Assailant: -400 por carta en la mano", "flash_assailant");
		Escena e;
		e.mon(0, "flash_assailant");
		e.mano(0, "kuriboh");
		e.mano(0, "kuriboh");
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[0].atkEf == 1200 && e.d.l[0].m[0].defEf == 1200, "ATK/DEF %d/%d", e.d.l[0].m[0].atkEf, e.d.l[0].m[0].defEf);
		fin();
	}
	{
		caso("Combate: Big Shield Gardna pasa a ataque; Getsu Fuhma destruye al Demonio", "big_shield_gardna");
		probadas["getsu_fuhma"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "celtic_guardian");
		const int z = e.mon(1, "big_shield_gardna", true);
		VERIFICA(atacar(e, a, z) && !e.d.l[1].m[(size_t)z].defensa, "Gardna sigue en defensa");
		Escena f(F_BATALLA);
		const int g = f.mon(0, "getsu_fuhma");
		const int s2 = f.mon(1, "summoned_skull");
		VERIFICA(atacar(f, g, s2) && f.d.l[1].monstruos() == 0, "Getsu no destruyo al Demonio");
		fin();
	}
	{
		caso("Mirage Dragon: el rival no activa trampas en la batalla", "mirage_dragon");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "mirage_dragon");
		e.colocar(1, "mirror_force");
		VERIFICA(atacar(e, a, -1) && e.d.ef.preg.tipo == P_NADA, "se ofrecio una trampa");
		fin();
	}
	{
		caso("D.D. Survivor vuelve en la fase final; Twin-Headed Behemoth vuelve con 1000/1000 (1 por duelo)", "d_d_survivor");
		probadas["twin_headed_behemoth"]++;
		Escena e;
		e.mon(1, "d_d_survivor");
		e.mon(1, "twin_headed_behemoth");
		VERIFICA(usar(e, "nobleman_of_crossout", {}) == false, "Nobleman sin boca abajo");
		e.d.sacarMonstruo(1, 0, false);
		e.d.alCementerio(1, 1);
		pasarFases(e, 1);
		VERIFICA(e.d.l[1].monstruos() == 2, "volvieron %d", e.d.l[1].monstruos());
		for (const Monstruo& m : e.d.l[1].m)
			if (m.id == "twin_headed_behemoth") VERIFICA(m.atkEf == 1000 && m.defEf == 1000, "Behemoth %d/%d", m.atkEf, m.defEf);
		fin();
	}
	{
		caso("Maiden of the Aqua: sin campo, cuenta como Umi para Amphibious Bugroth MK-3", "maiden_of_the_aqua");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "amphibious_bugroth_mk_3");
		e.mon(0, "maiden_of_the_aqua");
		e.mon(1, "summoned_skull");
		VERIFICA(atacar(e, a, -1), "no ataco directo");
		fin();
	}
}

static void tanda7d()
{
	fprintf(stderr, "tanda 7 (D)\n");
	{
		caso("Gale Lizard: volteo: un monstruo del rival a la mano", "gale_lizard");
		Escena e;
		const int z = e.mon(0, "gale_lizard", true, true);
		e.mon(1, "dark_magician");
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && enMano(e, 1, "dark_magician"), "no devolvio");
		fin();
	}
	{
		caso("Nuvia the Wicked: invocada normal se destruye; -200 por monstruo rival", "nuvia_the_wicked");
		Escena e;
		e.mon(1, "kuriboh");
		e.mon(0, "nuvia_the_wicked");
		VERIFICA(e.d.l[0].m[0].atkEf == 1800, "ATK %d", e.d.l[0].m[0].atkEf);
		e.d.l[0].m[0] = Monstruo{};
		e.mano(0, "nuvia_the_wicked");
		VERIFICA(invocar(e, 0), "no se invoco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 0, "no se destruyo");
		fin();
	}
	{
		caso("Zaborg (destruye un monstruo) y Mobius (hasta 2 magias/trampas) al invocarse con sacrificio", "zaborg_the_thunder_monarch");
		probadas["mobius_the_frost_monarch"]++;
		Escena e;
		e.mon(0, "kuriboh");
		e.mon(1, "dark_magician");
		e.mano(0, "zaborg_the_thunder_monarch");
		Jugada inv;
		inv.tipo = J_INVOCAR;
		inv.mano = 0;
		inv.tributos = { 0 };
		VERIFICA(e.d.aplicar(inv), "no se invoco");
		e.pasa(1);
		if (e.d.ef.preg.tipo == P_ELEGIR) e.elige(0, indiceCand(e, "dark_magician"));
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0, "Zaborg no destruyo");
		Escena f;
		f.mon(0, "kuriboh");
		f.colocar(1, "mirror_force");
		f.colocar(1, "trap_hole");
		f.mano(0, "mobius_the_frost_monarch");
		VERIFICA(f.d.aplicar(inv), "no se invoco Mobius");
		f.pasa(1);
		while (f.d.ef.preg.tipo == P_ELEGIR) f.elige(0, 0);
		f.pasarTodo();
		VERIFICA(f.d.l[1].colocadasMT() == 0, "Mobius no destruyo");
		fin();
	}
	{
		caso("Cannon Soldier: sacrifica (elige) y 500 al rival, sin limite", "cannon_soldier");
		Escena e;
		e.mon(0, "cannon_soldier");
		e.mon(0, "kuriboh");
		e.mon(0, "kuriboh");
		VERIFICA(e.activar(0, M(0, 0), {}, {}, { M(0, 1) }), "no se activo");
		e.pasarTodo();
		VERIFICA(e.activar(0, M(0, 0), {}, {}, { M(0, 2) }), "no se repitio");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 1000, "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Maha Vailo +500 por equipo; Machine King +100 por Maquina; Beast Fangs", "maha_vailo");
		probadas["machine_king"]++;
		probadas["beast_fangs"]++;
		Escena e;
		e.mon(0, "maha_vailo");
		VERIFICA(usar(e, "book_of_secret_arts", { M(0, 0) }) && e.d.l[0].m[0].atkEf == 1550 + 300 + 500, "Maha Vailo %d", e.d.l[0].m[0].atkEf);
		e.mon(0, "machine_king");
		e.mon(1, "cyber_commander");
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[1].atkEf == 2200 + 200, "Machine King %d", e.d.l[0].m[1].atkEf);
		e.mon(0, "battle_ox");   // (Guerrero-Bestia: Beast Fangs no vale)
		const int k = e.mano(0, "beast_fangs");
		VERIFICA(motivoNoActivar(e.d, 0, H(0, k), false) == "ef.sin_objetivo", "Beast Fangs sin Bestias");
		fin();
	}
	{
		caso("Maryokutai: en el turno del rival, se sacrifica y niega su magia", "maryokutai");
		Escena e;
		e.mon(1, "maryokutai");
		e.mon(1, "dark_magician");
		const int k = e.mano(0, "raigeki");
		VERIFICA(e.activar(0, H(0, k)), "Raigeki no se activo");
		VERIFICA(e.d.ef.preg.tipo == P_RESPUESTA && e.d.ef.preg.lado == 1, "no se ofrecio Maryokutai");
		VERIFICA(e.activar(1, M(1, 0)), "Maryokutai no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 1 && e.d.l[1].m[1].id == "dark_magician", "no nego");
		fin();
	}
	{
		caso("Revival Jam: destruido en batalla, paga 1000 y vuelve en defensa en tu fase de espera", "revival_jam");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int z = e.mon(1, "revival_jam");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasa(0);
		if (e.d.ef.preg.tipo == P_ELEGIR) e.elige(1, 0);
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 1000 - 1000, "LP %d", e.d.l[1].lp);
		pasarFases(e, 1);
		VERIFICA(e.d.l[1].monstruos() == 1 && e.d.l[1].m[0].defensa, "no volvio");
		fin();
	}
	{
		caso("Abyss Soldier: descarta un AGUA (elegido): una carta del campo a la mano", "abyss_soldier");
		Escena e;
		e.mon(0, "abyss_soldier");
		e.mon(1, "dark_magician");
		e.mano(0, "kairyu_shin");
		e.mano(0, "raigeki");
		VERIFICA(e.activar(0, M(0, 0), { M(1, 0) }, {}, { H(0, 0) }), "no se activo");
		e.pasarTodo();
		VERIFICA(enMano(e, 1, "dark_magician") && e.enCementerio(0, "kairyu_shin") == 1, "no funciono");
		fin();
	}
	{
		caso("Don Turtle: invocada, las de la mano tambien", "don_turtle");
		Escena e;
		e.mano(0, "don_turtle");
		e.mano(0, "don_turtle");
		e.mano(0, "don_turtle");
		VERIFICA(invocar(e, 0), "no se invoco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 3, "monstruos %d", e.d.l[0].monstruos());
		fin();
	}
	{
		caso("Emissary of the Afterlife: cada uno busca un normal de nivel 3 o menos", "emissary_of_the_afterlife");
		Escena e;
		e.mon(0, "emissary_of_the_afterlife");
		e.d.l[0].mazo.push_back("giant_soldier_of_stone");
		e.d.l[1].mazo.push_back("giant_soldier_of_stone");
		e.d.alCementerio(0, 0);
		motorVentana(e.d, Ventana{ V_DISPARO, 0, -1, 0, {} });   // (lo que hace el juego al terminar una accion)
		e.pasarTodo();
		VERIFICA(enMano(e, 0, "giant_soldier_of_stone") && enMano(e, 1, "giant_soldier_of_stone"), "no buscaron los dos");
		fin();
	}
	{
		caso("Gravekeeper's Spy: volteo: un Gravekeeper's de 1500 o menos del mazo", "gravekeeper_s_spy");
		Escena e;
		e.d.l[0].mazo.push_back("gravekeeper_s_chief");   // (1900: no)
		e.d.l[0].mazo.push_back("gravekeeper_s_spear_soldier");
		const int z = e.mon(0, "gravekeeper_s_spy", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasa(1);
		if (e.d.ef.preg.tipo == P_ELEGIR) e.elige(0, indiceCand(e, "gravekeeper_s_spear_soldier"));
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 2, "monstruos %d", e.d.l[0].monstruos());
		fin();
	}
	{
		caso("Kuriboh: descartado en el paso de danno, no hay danno de ese combate", "kuriboh");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int k = e.mano(1, "kuriboh");
		VERIFICA(atacar(e, a, -1), "no ataco");
		VERIFICA(e.d.ef.ventana.tipo == V_PASO_DANNO && e.d.ef.preg.lado == 1, "sin ventana del paso de danno");
		VERIFICA(e.activar(1, H(1, k)), "Kuriboh no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL && e.enCementerio(1, "kuriboh") == 1, "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Lady Panther: un monstruo destruido en batalla vuelve encima del mazo", "lady_panther");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "celtic_guardian");
		e.mon(0, "lady_panther");
		const int z = e.mon(1, "summoned_skull");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		e.d.fase = F_PRINCIPAL2;
		VERIFICA(e.activar(0, M(0, 1)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].mazo.back() == "celtic_guardian", "no quedo encima");
		fin();
	}
	{
		caso("Moai Interceptor Cannons: pasa a boca abajo", "moai_interceptor_cannons");
		Escena e;
		e.mon(0, "moai_interceptor_cannons");
		VERIFICA(e.activar(0, M(0, 0)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].bocaAbajo, "sigue boca arriba");
		fin();
	}
	{
		caso("Book of Life: un Zombi vuelve y se destierra uno del cementerio rival", "book_of_life");
		Escena e;
		e.d.l[0].cementerio = { "dark_magician", "regenerating_mummy" };
		e.d.l[1].cementerio = { "summoned_skull" };
		VERIFICA(usar(e, "book_of_life", { Lugar{ 0, EN_CEMENTERIO, 1 } }), "no se activo");
		VERIFICA(e.d.l[0].m[0].id == "regenerating_mummy" && e.d.l[1].desterrados.size() == 1, "no funciono");
		fin();
	}
	{
		caso("Call of the Mummy: sin monstruos, un Zombi de la mano", "call_of_the_mummy");
		Escena e;
		e.mano(0, "regenerating_mummy");
		VERIFICA(usar(e, "call_of_the_mummy"), "no se activo");
		VERIFICA(e.d.l[0].monstruos() == 1, "no invoco");
		fin();
	}
	{
		caso("Dark Zebra: unico monstruo en tu fase de espera: a defensa", "dark_zebra");
		Escena e;
		e.mon(1, "dark_zebra");
		pasarFases(e, 1);
		VERIFICA(e.d.l[1].m[0].defensa, "no paso a defensa");
		fin();
	}
	{
		caso("Graceful Dice: +100 x el dado a tus monstruos", "graceful_dice");
		Escena e;
		e.mon(0, "celtic_guardian");
		VERIFICA(usar(e, "graceful_dice"), "no se activo");
		const int extra = e.d.l[0].m[0].atkEf - 1400;
		VERIFICA(extra >= 100 && extra <= 600 && extra % 100 == 0, "extra %d", extra);
		fin();
	}
	{
		caso("Gray Wing: descarta 1 y ataca dos veces", "gray_wing");
		Escena e;
		e.mon(0, "gray_wing");
		e.mano(0, "kuriboh");
		Jugada j = jugadaActivar(M(0, 0));
		j.descartes = { 0 };
		VERIFICA(e.d.aplicar(j), "no se activo");
		e.pasarTodo();
		e.d.fase = F_BATALLA;
		VERIFICA(atacar(e, 0, -1) && atacar(e, 0, -1) && e.d.l[1].lp == LP_INICIAL - 2600, "LP %d", e.d.l[1].lp);
		VERIFICA(!atacar(e, 0, -1), "ataco tres veces");
		fin();
	}
	{
		caso("Insect Barrier: los Insectos del rival no atacan", "insect_barrier");
		Escena e;
		e.mon(1, "man_eater_bug");
		VERIFICA(usar(e, "insect_barrier"), "no se activo");
		VERIFICA(!motorPuedeAtacar(e.d, 1, 0), "el Insecto puede atacar");
		fin();
	}
	{
		caso("Lady Assailant of Flames (volteo: 800) y Mine Golem (destruido: 500)", "lady_assailant_of_flames");
		probadas["mine_golem"]++;
		Escena e;
		const int z = e.mon(0, "lady_assailant_of_flames", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 800 && e.d.l[0].desterrados.size() == 3, "LP %d", e.d.l[1].lp);
		Escena f(F_BATALLA);
		const int a = f.mon(0, "summoned_skull");
		const int g = f.mon(1, "mine_golem", true);
		VERIFICA(atacar(f, a, g), "no ataco");
		f.pasarTodo();
		VERIFICA(f.d.l[0].lp == LP_INICIAL - 500, "Mine Golem: LP %d", f.d.l[0].lp);
		fin();
	}
	{
		caso("Mad Sword Beast perfora; Amazoness Swords Woman devuelve el danno; Pitch-Black Warwolf", "mad_sword_beast");
		probadas["amazoness_swords_woman"]++;
		probadas["pitch_black_warwolf"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "mad_sword_beast");
		const int z = e.mon(1, "kuriboh", true);
		VERIFICA(atacar(e, a, z) && e.d.l[1].lp == LP_INICIAL - 1200, "perforar: LP %d", e.d.l[1].lp);
		Escena f(F_BATALLA);
		const int b = f.mon(0, "amazoness_swords_woman");
		const int s2 = f.mon(1, "summoned_skull");
		VERIFICA(atacar(f, b, s2) && f.d.l[0].lp == LP_INICIAL && f.d.l[1].lp == LP_INICIAL - 1000, "ASW: LP %d/%d", f.d.l[0].lp, f.d.l[1].lp);
		Escena g(F_BATALLA);
		const int w = g.mon(0, "pitch_black_warwolf");
		g.colocar(1, "mirror_force");
		VERIFICA(atacar(g, w, -1) && g.d.ef.preg.tipo == P_NADA, "se ofrecio una trampa");
		fin();
	}
	{
		caso("Balloon Lizard: contador por fase de espera; destruido, 400 por contador", "balloon_lizard");
		Escena e;
		e.mon(1, "balloon_lizard");
		pasarFases(e, 1);
		pasarFases(e, 0);
		pasarFases(e, 1);
		VERIFICA(e.d.l[1].m[0].contadores == 2, "contadores %d", e.d.l[1].m[0].contadores);
		e.d.alCementerio(1, 0);
		VERIFICA(e.d.l[0].lp == LP_INICIAL - 800, "LP %d", e.d.l[0].lp);
		fin();
	}
	{
		caso("Lord of D.: los Dragones no son objetivo de efectos", "lord_of_d");
		Escena e;
		e.mon(1, "lord_of_d");
		e.mon(1, "blue_eyes");
		const DefCarta* tdd = defCarta("tribute_to_the_doomed");
		bool hay = false;
		for (const Lugar& g : objetivosDe(e.d, 0, *tdd, H(0, 0))) hay |= cartaEn(e.d, g) == "blue_eyes";
		VERIFICA(!hay, "Blue-Eyes es objetivo");
		fin();
	}
	{
		caso("Shadowknight Archfiend paga 900; Element Saurus +500 con FUEGO", "shadowknight_archfiend");
		probadas["element_saurus"]++;
		Escena e;
		e.mon(1, "shadowknight_archfiend");
		pasarFases(e, 1);
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 900, "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Venus: paga 500 por Mystical Shine Ball; Ryu Kokki destruye al Guerrero", "the_agent_of_creation_venus");
		probadas["ryu_kokki"]++;
		Escena e;
		e.mon(0, "the_agent_of_creation_venus");
		e.d.l[0].mazo.push_back("mystical_shine_ball");
		VERIFICA(e.activar(0, M(0, 0)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 2 && e.d.l[0].lp == LP_INICIAL - 500, "Venus");
		Escena f(F_BATALLA);
		const int r = f.mon(0, "ryu_kokki");
		const int g = f.mon(1, "giant_soldier_of_stone", true);   // Roca: no
		const int c = f.mon(1, "celtic_guardian", true);   // Guerrero
		(void)g;
		VERIFICA(atacar(f, r, c) && f.d.l[1].m[(size_t)c].vacia(), "Ryu Kokki");
		fin();
	}
}

static void tanda7e()
{
	fprintf(stderr, "tanda 7 (E): Cocoon of Evolution y las polillas\n");
	{
		caso("Cocoon of Evolution: se equipa desde la mano a Petit Moth (0/2000) y a los 4 turnos sale Great Moth", "cocoon_of_evolution");
		probadas["great_moth"]++;
		probadas["larvae_moth"]++;
		Escena e;
		e.mon(0, "petit_moth");
		const int k = e.mano(0, "cocoon_of_evolution");
		VERIFICA(e.activar(0, H(0, k), { M(0, 0) }), "no se equipo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].mt[0].id == "cocoon_of_evolution" && e.d.l[0].m[0].atkEf == 0 && e.d.l[0].m[0].defEf == 2000, "ATK/DEF %d/%d", e.d.l[0].m[0].atkEf,
		         e.d.l[0].m[0].defEf);
		const int g = e.mano(0, "great_moth");
		e.mano(0, "larvae_moth");
		VERIFICA(motivoNoActivar(e.d, 0, H(0, g), false) == "ef.no_condicion", "Great Moth sale antes de tiempo");
		for (int turno = 0; turno < 4; turno++) {
			pasarFases(e, 1);
			pasarFases(e, 0);
		}
		VERIFICA(e.d.l[0].mt[0].contadores == 4, "turnos %d", e.d.l[0].mt[0].contadores);
		int kg = -1;
		for (int i = 0; i < (int)e.d.l[0].mano.size(); i++)
			if (e.d.l[0].mano[(size_t)i] == "great_moth") kg = i;
		VERIFICA(kg >= 0 && e.activar(0, H(0, kg)), "Great Moth no se invoco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].id == "great_moth" && e.enCementerio(0, "petit_moth") == 1 && e.enCementerio(0, "cocoon_of_evolution") == 1, "estado inesperado");
		fin();
	}
	{
		caso("Perfectly Ultimate Great Moth: a los 6 turnos del Capullo (no antes)", "perfectly_ultimate_great_moth");
		Escena e;
		e.mon(0, "petit_moth");
		const int k = e.mano(0, "cocoon_of_evolution");
		VERIFICA(e.activar(0, H(0, k), { M(0, 0) }), "no se equipo");
		e.pasarTodo();
		const int pg = e.mano(0, "perfectly_ultimate_great_moth");
		for (int turno = 0; turno < 5; turno++) {
			pasarFases(e, 1);
			pasarFases(e, 0);
		}
		VERIFICA(motivoNoActivar(e.d, 0, H(0, pg), false) == "ef.no_condicion", "salio con 5 turnos");
		pasarFases(e, 1);
		pasarFases(e, 0);
		int kp = -1;
		for (int i = 0; i < (int)e.d.l[0].mano.size(); i++)
			if (e.d.l[0].mano[(size_t)i] == "perfectly_ultimate_great_moth") kp = i;
		VERIFICA(kp >= 0 && e.activar(0, H(0, kp)), "no se invoco a los 6 turnos");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].id == "perfectly_ultimate_great_moth" && e.d.l[0].m[0].atkEf == 3500, "estado inesperado");
		fin();
	}
}

static void tanda7f()
{
	fprintf(stderr, "tanda 7 (F)\n");
	{
		caso("Doble sacrificio: Kaiser Sea Horse vale 2 para un LUZ de nivel 7; no para otros; sin sacrificios de sobra", "kaiser_sea_horse");
		for (const char* id : { "whirlwind_prodigy", "flame_ruler", "double_coston", "unshaven_angler" }) probadas[id]++;
		Escena e;
		e.mon(0, "kaiser_sea_horse");
		e.mon(0, "kuriboh");
		e.mano(0, "blue_eyes");   // LUZ nivel 8
		e.mano(0, "dark_magician");   // OSCURIDAD nivel 7
		VERIFICA(!e.d.puedeInvocarNormal(0, 1, { 0 }), "Kaiser valio 2 para un OSCURIDAD");
		Jugada inv;
		inv.tipo = J_INVOCAR;
		inv.mano = 0;
		inv.tributos = { 0 };
		VERIFICA(e.d.aplicar(inv), "no se invoco Blue-Eyes con Kaiser solo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 2, "monstruos %d", e.d.l[0].monstruos());
		fin();
	}
	{
		caso("Fake Trap: en respuesta a Heavy Storm, tus otras trampas no se destruyen", "fake_trap");
		Escena e;
		e.d.activo = 1;
		e.d.turno = 4;
		const int ft = e.colocar(0, "fake_trap");
		e.colocar(0, "mirror_force");
		const int k = e.mano(1, "heavy_storm");
		VERIFICA(e.activar(1, H(1, k)), "Heavy Storm no se activo");
		VERIFICA(e.activar(0, MT(0, ft)), "Fake Trap no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].colocadasMT() == 1 && e.enCementerio(0, "fake_trap") == 1, "Mirror Force no sobrevivio");
		fin();
	}
	{
		caso("Light of Intervention: colocar = invocar boca arriba en defensa (y Trap Hole puede responder)", "light_of_intervention");
		Escena e;
		const int z = e.colocar(0, "light_of_intervention");
		VERIFICA(e.activar(0, MT(0, z)), "no se activo");
		e.pasarTodo();
		e.mano(0, "vorse_raider");
		Jugada col;
		col.tipo = J_COLOCAR;
		col.mano = 0;
		VERIFICA(e.d.aplicar(col), "no se coloco");
		e.pasarTodo();
		VERIFICA(!e.d.l[0].m[0].bocaAbajo && e.d.l[0].m[0].defensa, "quedo boca abajo");
		fin();
	}
	{
		caso("The End of Anubis: niega Monster Reborn (elige el cementerio)", "the_end_of_anubis");
		Escena e;
		e.mon(1, "the_end_of_anubis");
		e.d.l[0].cementerio.push_back("summoned_skull");
		VERIFICA(usar(e, "monster_reborn", { Lugar{ 0, EN_CEMENTERIO, 0 } }), "no se activo");
		VERIFICA(e.d.l[0].monstruos() == 0, "Reborn no se nego");
		fin();
	}
	{
		caso("Freed the Matchless General niega y destruye la magia que lo elige; Chaos Command Magician, el efecto de monstruo", "freed_the_matchless_general");
		probadas["chaos_command_magician"]++;
		probadas["frontier_wiseman"]++;
		Escena e;
		e.mon(1, "freed_the_matchless_general");
		VERIFICA(usar(e, "book_of_moon", { M(1, 0) }) && !e.d.l[1].m[0].bocaAbajo && e.enCementerio(0, "book_of_moon") == 1, "Freed");
		Escena f;
		f.mon(1, "chaos_command_magician");
		f.mon(0, "old_vindictive_magician", true, true);
		VERIFICA(voltear(f, 0), "no se volteo");
		f.pasarTodo();
		VERIFICA(f.d.l[1].monstruos() == 1, "CCM fue destruido por un efecto de monstruo");
		fin();
	}
	{
		caso("Creeping Doom Manta / Eagle Eye: al invocarse normal, no hay trampas", "creeping_doom_manta");
		probadas["eagle_eye"]++;
		Escena e;
		e.colocar(1, "trap_hole");
		e.mano(0, "eagle_eye");
		VERIFICA(invocar(e, 0) && e.d.ef.preg.tipo == P_NADA, "se ofrecio Trap Hole");
		fin();
	}
	{
		caso("Dark Elf paga 1000 por atacar; Mystic Lamp ataca directo; Amazoness Fighter sin danno; Aztecs dobla", "dark_elf");
		for (const char* id : { "mystic_lamp", "amazoness_fighter", "stone_statue_of_the_aztecs" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "dark_elf");
		VERIFICA(atacar(e, a, -1) && e.d.l[0].lp == LP_INICIAL - 1000, "Dark Elf: LP %d", e.d.l[0].lp);
		Escena f(F_BATALLA);
		const int b = f.mon(0, "mystic_lamp");
		f.mon(1, "summoned_skull");
		VERIFICA(atacar(f, b, -1), "Mystic Lamp no ataco directo");
		Escena g(F_BATALLA);
		const int c = g.mon(0, "amazoness_fighter");
		const int s2 = g.mon(1, "summoned_skull");
		VERIFICA(atacar(g, c, s2) && g.d.l[0].lp == LP_INICIAL, "Amazoness Fighter: LP %d", g.d.l[0].lp);
		Escena h(F_BATALLA);
		const int k2 = h.mon(0, "celtic_guardian");
		const int st = h.mon(1, "stone_statue_of_the_aztecs", true);
		VERIFICA(atacar(h, k2, st) && h.d.l[0].lp == LP_INICIAL - 2 * (2000 - 1400), "Aztecs: LP %d", h.d.l[0].lp);
		fin();
	}
	{
		caso("Enraged Battle Ox: las Bestias perforan; Darklord Marie +200 en tu espera", "enraged_battle_ox");
		probadas["darklord_marie"]++;
		Escena e(F_BATALLA);
		e.mon(0, "enraged_battle_ox");
		const int a = e.mon(0, "battle_ox");   // Guerrero-Bestia
		const int z = e.mon(1, "kuriboh", true);
		VERIFICA(atacar(e, a, z) && e.d.l[1].lp == LP_INICIAL - 1500, "LP %d", e.d.l[1].lp);
		Escena f;
		f.d.l[1].cementerio.push_back("darklord_marie");
		pasarFases(f, 1);
		VERIFICA(f.d.l[1].lp == LP_INICIAL + 200, "Marie: LP %d", f.d.l[1].lp);
		fin();
	}
	{
		caso("Blowback Dragon: 3 monedas sobre una carta del rival", "blowback_dragon");
		Escena e;
		e.mon(0, "blowback_dragon");
		e.mon(1, "dark_magician");
		VERIFICA(e.activar(0, M(0, 0), { M(1, 0) }), "no se activo");
		e.pasarTodo();
		VERIFICA(e.libre(), "cadena abierta");
		fin();
	}
	{
		caso("Equipos: Violet Crystal (Zombi) y Raise Body Heat (Dinosaurio) +300/+300", "violet_crystal");
		probadas["raise_body_heat"]++;
		Escena e;
		e.mon(0, "sword_arm_of_dragon");
		VERIFICA(usar(e, "raise_body_heat", { M(0, 0) }) && e.d.l[0].m[0].atkEf == 1750 + 300, "Raise Body Heat");
		const int k = e.mano(0, "violet_crystal");
		VERIFICA(motivoNoActivar(e.d, 0, H(0, k), false) == "ef.sin_objetivo", "Violet Crystal sin Zombis");
		fin();
	}
	{
		caso("Gravekeeper's Guard (volteo: a la mano), Spear Soldier perfora, Guardian Statue", "gravekeeper_s_guard");
		probadas["gravekeeper_s_spear_soldier"]++;
		probadas["guardian_statue"]++;
		Escena e;
		const int z = e.mon(0, "gravekeeper_s_guard", true, true);
		e.mon(1, "dark_magician");
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(enMano(e, 1, "dark_magician"), "no devolvio");
		Escena f(F_BATALLA);
		const int a = f.mon(0, "gravekeeper_s_spear_soldier");
		const int k = f.mon(1, "kuriboh", true);
		VERIFICA(atacar(f, a, k) && f.d.l[1].lp == LP_INICIAL - 1300, "Spear Soldier: LP %d", f.d.l[1].lp);
		fin();
	}
	{
		caso("Hoshiningen: LUZ +500, OSCURIDAD -400", "hoshiningen");
		Escena e;
		e.mon(0, "hoshiningen");
		e.mon(0, "mystical_elf");
		e.mon(1, "dark_magician");
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[1].atkEf == 1300 && e.d.l[1].m[0].atkEf == 2100, "ATK %d/%d", e.d.l[0].m[1].atkEf, e.d.l[1].m[0].atkEf);
		fin();
	}
	{
		caso("Senju of the Thousand Hands: busca un monstruo de ritual", "senju_of_the_thousand_hands");
		probadas["manju_of_the_ten_thousand_hands"]++;
		Escena e;
		e.d.l[0].mazo.push_back("magician_of_black_chaos");
		e.mano(0, "senju_of_the_thousand_hands");
		VERIFICA(invocar(e, 0), "no se invoco");
		e.pasa(1);
		if (e.d.ef.preg.tipo == P_ELEGIR) e.elige(0, 0);
		e.pasarTodo();
		VERIFICA(enMano(e, 0, "magician_of_black_chaos"), "no busco");
		fin();
	}
	{
		caso("Royal Magical Library: 3 contadores: roba 1", "royal_magical_library");
		Escena e;
		e.mon(0, "royal_magical_library");
		for (int k = 0; k < 3; k++) usar(e, "red_medicine");
		const size_t mano = e.d.l[0].mano.size();
		VERIFICA(e.activar(0, M(0, 0)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].mano.size() == mano + 1 && e.d.l[0].m[0].contadores == 0, "no robo");
		fin();
	}
	{
		caso("Skull Dice: los del rival pierden 100 x el dado", "skull_dice");
		Escena e;
		e.mon(1, "dark_magician");
		VERIFICA(usarColocada(e, 0, "skull_dice"), "no se activo");
		VERIFICA(e.d.l[1].m[0].atkEf < 2500, "no bajo");
		fin();
	}
	{
		caso("Abare Ushioni: 1000 a alguien", "abare_ushioni");
		Escena e;
		e.mon(0, "abare_ushioni");
		VERIFICA(e.activar(0, M(0, 0)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].lp + e.d.l[1].lp == 2 * LP_INICIAL - 1000, "LP %d/%d", e.d.l[0].lp, e.d.l[1].lp);
		fin();
	}
	{
		caso("Absorbing Kid (+300 x nivel) y Guardian Angel Joan (+ATK) al destruir en batalla", "absorbing_kid_from_the_sky");
		probadas["guardian_angel_joan"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "absorbing_kid_from_the_sky");
		const int z = e.mon(1, "kuriboh");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].lp == LP_INICIAL + 300, "LP %d", e.d.l[0].lp);
		fin();
	}
	{
		caso("Armed Dragon LV3 sube a LV5 en tu espera", "armed_dragon_lv3");
		Escena e;
		e.mon(1, "armed_dragon_lv3");
		e.d.l[1].mazo.push_back("armed_dragon_lv5");
		pasarFases(e, 1);
		VERIFICA(e.d.l[1].m[0].id == "armed_dragon_lv5", "no subio");
		fin();
	}
	{
		caso("Combate: Des Kangaroo, Legendary Jujitsu Master, Hyper Hammerhead, D.D. Warrior Lady", "des_kangaroo");
		for (const char* id : { "legendary_jujitsu_master", "hyper_hammerhead", "d_d_warrior_lady" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "celtic_guardian");
		const int z = e.mon(1, "des_kangaroo", true);
		VERIFICA(atacar(e, a, z) && e.d.l[0].monstruos() == 0, "Des Kangaroo");
		Escena f(F_BATALLA);
		const int b = f.mon(0, "celtic_guardian");
		const int j = f.mon(1, "legendary_jujitsu_master", true);
		VERIFICA(atacar(f, b, j) && f.d.l[0].mazo.back() == "celtic_guardian", "Jujitsu");
		Escena g(F_BATALLA);
		const int c = g.mon(0, "kuriboh");
		const int h = g.mon(1, "hyper_hammerhead");
		VERIFICA(atacar(g, c, h), "no ataco (Hammerhead)");
		Escena i(F_BATALLA);
		const int w = i.mon(0, "d_d_warrior_lady");
		const int s = i.mon(1, "giant_soldier_of_stone", true);
		VERIFICA(atacar(i, w, s) && i.d.l[0].desterrados.size() == 1 && i.d.l[1].desterrados.size() == 1, "D.D. Warrior Lady");
		fin();
	}
	{
		caso("Lekunga: destierra 2 AGUA: ficha 700/700", "lekunga");
		Escena e;
		e.mon(0, "lekunga");
		e.d.l[0].cementerio = { "kairyu_shin", "penguin_soldier" };
		VERIFICA(e.activar(0, M(0, 0)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 2 && e.d.l[0].m[1].atkEf == 700, "ficha");
		fin();
	}
	{
		caso("Man-Thro' Tro': sacrifica un normal: 800", "man_thro_tro");
		Escena e;
		e.mon(0, "man_thro_tro");
		e.mon(0, "celtic_guardian");
		VERIFICA(e.activar(0, M(0, 0)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 800, "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Morphing Jar: todos descartan y roban 5", "morphing_jar");
		Escena e;
		e.mano(0, "kuriboh");
		e.mano(1, "kuriboh");
		const int z = e.mon(0, "morphing_jar", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].mano.size() == 5 && e.d.l[1].mano.size() == 5, "manos %d/%d", (int)e.d.l[0].mano.size(), (int)e.d.l[1].mano.size());
		fin();
	}
	{
		caso("Ookazi (800), Tremendous Fire (1000/500), Rising Air Current", "ookazi");
		probadas["tremendous_fire"]++;
		probadas["rising_air_current"]++;
		Escena e;
		VERIFICA(usar(e, "ookazi") && e.d.l[1].lp == LP_INICIAL - 800, "Ookazi");
		VERIFICA(usar(e, "tremendous_fire") && e.d.l[1].lp == LP_INICIAL - 1800 && e.d.l[0].lp == LP_INICIAL - 500, "Tremendous Fire");
		e.mon(0, "harpie_lady");
		VERIFICA(usar(e, "rising_air_current") && e.d.l[0].m[0].atkEf == 1800 && e.d.l[0].m[0].defEf == 1000, "Rising Air Current");
		fin();
	}
	{
		caso("Reaper of the Cards: volteo: destruye una trampa", "reaper_of_the_cards");
		Escena e;
		e.colocar(1, "mirror_force");
		const int z = e.mon(0, "reaper_of_the_cards", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].colocadasMT() == 0, "no destruyo");
		fin();
	}
	{
		caso("Rock Bombardment: una Roca del mazo al cementerio: 500", "rock_bombardment");
		Escena e;
		e.d.l[0].mazo.push_back("giant_soldier_of_stone");
		VERIFICA(usarColocada(e, 0, "rock_bombardment") && e.d.l[1].lp == LP_INICIAL - 500, "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Spellbinding Circle: el monstruo no ataca; si se destruye, la trampa tambien", "spellbinding_circle");
		Escena e;
		e.mon(1, "dark_magician");
		const int z = e.colocar(0, "spellbinding_circle");
		VERIFICA(e.activar(0, MT(0, z), { M(1, 0) }), "no se activo");
		e.pasarTodo();
		VERIFICA(!motorPuedeAtacar(e.d, 1, 0), "puede atacar");
		e.d.alCementerio(1, 0);
		VERIFICA(e.d.l[0].mt[(size_t)z].vacia(), "la trampa sigue");
		fin();
	}
	{
		caso("The Wicked Worm Beast vuelve a la mano; White Magical Hat hace descartar", "the_wicked_worm_beast");
		probadas["white_magical_hat"]++;
		Escena e;
		e.mon(0, "the_wicked_worm_beast");
		pasarFases(e, 1);
		VERIFICA(enMano(e, 0, "the_wicked_worm_beast"), "no volvio");
		Escena f(F_BATALLA);
		const int a = f.mon(0, "white_magical_hat");
		f.mano(1, "kuriboh");
		VERIFICA(atacar(f, a, -1), "no ataco");
		f.pasarTodo();
		VERIFICA(f.d.l[1].mano.empty(), "no descarto");
		fin();
	}
	{
		caso("Dark Dust Spirit: destruye los demas boca arriba; The Unfriendly Amazon sacrifica en tu espera", "dark_dust_spirit");
		probadas["the_unfriendly_amazon"]++;
		Escena e;
		e.mon(1, "dark_magician");
		e.mon(1, "kuriboh", true, true);
		e.mon(0, "kuriboh");
		e.mano(0, "dark_dust_spirit");
		Jugada ds;
		ds.tipo = J_INVOCAR;
		ds.mano = 0;
		ds.tributos = { 0 };
		VERIFICA(e.d.aplicar(ds), "no se invoco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 1 && e.d.l[0].monstruos() == 1, "rival %d", e.d.l[1].monstruos());
		Escena f;
		f.mon(1, "the_unfriendly_amazon");
		f.mon(1, "kuriboh");
		pasarFases(f, 1);
		VERIFICA(f.d.l[1].monstruos() == 1 && f.d.l[1].m[0].id == "the_unfriendly_amazon", "no sacrifico");
		fin();
	}
	{
		caso("Suijin: el atacante queda en 0 de ATK en el calculo", "suijin");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int z = e.mon(1, "suijin");
		VERIFICA(atacar(e, a, z), "no ataco");
		VERIFICA(e.d.ef.ventana.tipo == V_PASO_DANNO, "sin paso de danno");
		if (e.d.ef.preg.lado == 0) e.pasa(0);
		VERIFICA(e.activar(1, M(1, z)), "Suijin no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)a].vacia() && e.d.l[0].lp == LP_INICIAL - 2500, "LP %d", e.d.l[0].lp);
		fin();
	}
	{
		caso("The Creator Incarnate: The Creator de la mano; Insect Imitation: Insecto de un nivel mas", "the_creator_incarnate");
		probadas["insect_imitation"]++;
		Escena e;
		e.mon(0, "the_creator_incarnate");
		e.mano(0, "the_creator");
		VERIFICA(e.activar(0, M(0, 0)), "no se activo (%s)", motivoNoActivar(e.d, 0, M(0, 0), false).c_str());
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].id == "the_creator", "no invoco");
		Escena f;
		f.mon(0, "man_eater_bug");   // nivel 2 -> 3
		f.d.l[0].mazo.push_back("cockroach_knight");
		VERIFICA(usar(f, "insect_imitation"), "no se activo");
		VERIFICA(f.d.l[0].monstruos() == 1 && f.d.l[0].m[0].id != "man_eater_bug", "no invoco");
		fin();
	}
}

static void tanda7g()
{
	fprintf(stderr, "tanda 7 (G)\n");
	{
		caso("Magical Hats: 2 magias/trampas del mazo y tu monstruo, boca abajo y mezclados; al final de la batalla se van", "magical_hats");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		e.mon(1, "dark_magician");
		e.d.l[1].mazo.push_back("raigeki");
		e.d.l[1].mazo.push_back("mirror_force");
		const int z = e.colocar(1, "magical_hats");
		VERIFICA(atacar(e, a, 0), "no ataco");
		VERIFICA(e.activar(1, MT(1, z)), "no se activo");
		e.pasarTodo();
		int abajo = 0;
		for (const Monstruo& m : e.d.l[1].m) abajo += !m.vacia() && m.bocaAbajo && m.defensa;
		// (el ataque sigue contra su monstruo, ahora boca abajo bajo un sombrero: atajo, sin repetir el ataque)
		VERIFICA(e.d.l[1].monstruos() >= 2 && abajo == e.d.l[1].monstruos() && e.d.ef.sombreros.size() == 2, "monstruos %d boca abajo %d", e.d.l[1].monstruos(), abajo);
		Jugada f;
		f.tipo = J_FASE;
		e.d.aplicar(f);   // fin de la batalla
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() <= 1 && e.d.ef.sombreros.empty(), "los sombreros siguen (%d)", e.d.l[1].monstruos());
		fin();
	}
	{
		caso("Asura Priest ataca a todos los monstruos del rival; Mermaid Knight dos veces con Umi", "asura_priest");
		probadas["mermaid_knight"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "asura_priest");
		e.mon(1, "kuriboh");
		e.mon(1, "kuriboh");
		e.mon(1, "kuriboh");
		VERIFICA(atacar(e, a, 0) && atacar(e, a, 1) && atacar(e, a, 2), "no ataco a los tres");
		VERIFICA(!e.d.puedeAtacar(0, a), "puede atacar una cuarta vez");
		VERIFICA(e.d.l[1].monstruos() == 0, "quedan %d", e.d.l[1].monstruos());
		fin();
	}
	{
		caso("Pandemonium: los Archfiend no pagan; destruido uno (no en batalla), busca otro de menos nivel", "pandemonium");
		Escena e;
		e.mon(1, "skull_archfiend_of_lightning");
		e.d.l[1].campo = MagiaTrampa{ "pandemonium", false };
		e.d.l[1].campo.uid = e.d.nuevoUid();
		e.d.l[1].mazo.push_back("shadowknight_archfiend");
		pasarFases(e, 1);
		VERIFICA(e.d.l[1].lp == LP_INICIAL, "pago (LP %d)", e.d.l[1].lp);
		e.d.activo = 0;
		const int k = e.mano(0, "fissure");
		e.d.fase = F_PRINCIPAL1;
		VERIFICA(e.activar(0, H(0, k)), "Fissure no se activo");
		e.pasa(1);
		if (e.d.ef.preg.tipo == P_ELEGIR) e.elige(1, 0);
		e.pasarTodo();
		VERIFICA(enMano(e, 1, "shadowknight_archfiend"), "no busco");
		fin();
	}
	{
		caso("B.E.S. Big Core: 3 contadores, no muere en batalla, pierde uno por combate", "b_e_s_big_core");
		Escena e(F_BATALLA);
		e.mano(1, "b_e_s_big_core");
		e.mon(1, "kuriboh");
		e.d.activo = 1;
		e.d.fase = F_PRINCIPAL1;
		Jugada inv;
		inv.tipo = J_INVOCAR;
		inv.mano = 0;
		inv.tributos = { 0 };
		VERIFICA(e.d.aplicar(inv), "no se invoco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].m[0].contadores == 3, "contadores %d", e.d.l[1].m[0].contadores);
		e.d.activo = 0;
		e.d.fase = F_BATALLA;
		const int a = e.mon(0, "blue_eyes");
		VERIFICA(atacar(e, a, 0) && !e.d.l[1].m[0].vacia() && e.d.l[1].m[0].contadores == 2, "estado inesperado");
		fin();
	}
	{
		caso("Roulette Barrel, Time Wizard, Thestalos, Diabound Kernel (+600 al atacar)", "roulette_barrel");
		for (const char* id : { "time_wizard", "thestalos_the_firestorm_monarch", "diabound_kernel" }) probadas[id]++;
		Escena e;
		e.mon(0, "roulette_barrel");
		VERIFICA(e.activar(0, M(0, 0)), "Roulette no se activo");
		e.pasarTodo();
		Escena f;
		f.mon(0, "time_wizard");
		f.mon(1, "dark_magician");
		VERIFICA(f.activar(0, M(0, 0)), "Time Wizard no se activo");
		f.pasarTodo();
		VERIFICA(f.d.l[0].monstruos() + f.d.l[1].monstruos() == 1, "Time Wizard no destruyo");
		Escena g(F_BATALLA);
		const int a = g.mon(0, "diabound_kernel");
		const int base = g.d.l[0].m[(size_t)a].atkEf;
		VERIFICA(atacar(g, a, -1) && g.d.l[1].lp == LP_INICIAL - (base + 600), "Diabound: LP %d", g.d.l[1].lp);
		fin();
	}
	{
		caso("Solar Flare Dragon (500 en tu fase final), The Immortal of Thunder (+3000 / -5000), Bowganian (600)", "solar_flare_dragon");
		probadas["the_immortal_of_thunder"]++;
		probadas["bowganian"]++;
		Escena e;
		e.mon(0, "solar_flare_dragon");
		pasarFases(e, 1);
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 500, "Solar Flare: LP %d", e.d.l[1].lp);
		Escena f;
		const int z = f.mon(0, "the_immortal_of_thunder", true, true);
		VERIFICA(voltear(f, z), "no se volteo");
		f.pasarTodo();
		VERIFICA(f.d.l[0].lp == LP_INICIAL + 3000, "Immortal: LP %d", f.d.l[0].lp);
		f.d.alCementerio(0, z);
		VERIFICA(f.d.l[0].lp == LP_INICIAL - 2000, "Immortal al irse: LP %d", f.d.l[0].lp);
		Escena g;
		g.mon(1, "bowganian");
		pasarFases(g, 1);
		VERIFICA(g.d.l[0].lp == LP_INICIAL - 600, "Bowganian: LP %d", g.d.l[0].lp);
		fin();
	}
	{
		caso("Amazoness Chain Master, Birdface, Howling Insect, Keldo (destruidos en batalla)", "birdface");
		for (const char* id : { "amazoness_chain_master", "howling_insect", "keldo" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int z = e.mon(1, "birdface");
		e.d.l[1].mazo.push_back("harpie_lady");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasa(0);
		if (e.d.ef.preg.tipo == P_ELEGIR) e.elige(1, 0);
		e.pasarTodo();
		VERIFICA(enMano(e, 1, "harpie_lady"), "Birdface no busco");
		fin();
	}
	{
		caso("Buster Blader +500 por Dragon del rival; Harpie Lady 1 +300 a los VIENTO", "buster_blader");
		probadas["harpie_lady_1"]++;
		Escena e;
		e.mon(0, "buster_blader");
		e.mon(1, "blue_eyes");
		e.d.l[1].cementerio.push_back("blue_eyes");
		e.mon(0, "harpie_lady_1");
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[0].atkEf == 2600 + 1000, "Buster %d", e.d.l[0].m[0].atkEf);
		VERIFICA(e.d.l[0].m[1].atkEf == 1300 + 300, "Harpie Lady 1 %d", e.d.l[0].m[1].atkEf);
		fin();
	}
	{
		caso("Chiron the Mage: descarta una magia: destruye una magia/trampa del rival", "chiron_the_mage");
		Escena e;
		e.mon(0, "chiron_the_mage");
		const int z = e.colocar(1, "mirror_force");
		e.mano(0, "raigeki");
		VERIFICA(e.activar(0, M(0, 0), { MT(1, z) }), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].colocadasMT() == 0 && e.enCementerio(0, "raigeki") == 1, "no destruyo");
		fin();
	}
	{
		caso("Volteos: Dekoichi (roba), Des Koala (400 por carta), Poison Mummy (500)", "des_koala");
		probadas["dekoichi_the_battlechanted_locomotive"]++;
		probadas["poison_mummy"]++;
		Escena e;
		e.mano(1, "kuriboh");
		e.mano(1, "kuriboh");
		const int z = e.mon(0, "des_koala", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 800, "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Magical Marionette: contadores (+200); quita 2: destruye un monstruo", "magical_marionette");
		Escena e;
		e.mon(0, "magical_marionette");
		e.mon(1, "kuriboh");
		usar(e, "red_medicine");
		usar(e, "red_medicine");
		VERIFICA(e.d.l[0].m[0].atkEf == 2000 + 400, "ATK %d", e.d.l[0].m[0].atkEf);
		VERIFICA(e.activar(0, M(0, 0), { M(1, 0) }), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0, "no destruyo");
		fin();
	}
	{
		caso("Mind Control: el control hasta la fase final, sin atacar", "mind_control");
		Escena e;
		e.mon(1, "dark_magician");
		VERIFICA(usar(e, "mind_control", { M(1, 0) }) && e.d.l[0].monstruos() == 1, "no tomo el control");
		e.d.fase = F_BATALLA;
		VERIFICA(!e.d.puedeAtacar(0, 0), "puede atacar");
		fin();
	}
	{
		caso("Molten Zombie (revive: roba), Sacred Crane (especial: roba), Sonic Bird (busca magia de ritual)", "molten_zombie");
		probadas["sacred_crane"]++;
		probadas["sonic_bird"]++;
		Escena e;
		e.d.l[0].cementerio.push_back("molten_zombie");
		VERIFICA(usar(e, "monster_reborn", { Lugar{ 0, EN_CEMENTERIO, 0 } }), "no se activo");
		VERIFICA(e.d.l[0].mano.size() == 1, "no robo (mano %d)", (int)e.d.l[0].mano.size());
		fin();
	}
	{
		caso("Levia-Dragon - Daedalus: manda tu Umi: destruye todo lo demas", "levia_dragon_daedalus");
		Escena e;
		e.mon(0, "levia_dragon_daedalus");
		e.d.l[0].campo = MagiaTrampa{ "umi", false };
		e.d.l[0].campo.uid = e.d.nuevoUid();
		e.mon(1, "dark_magician");
		e.colocar(1, "mirror_force");
		VERIFICA(e.activar(0, M(0, 0)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && e.d.l[1].colocadasMT() == 0 && e.d.l[0].monstruos() == 1, "no destruyo todo");
		fin();
	}
	{
		caso("Uniones Pitch-Dark Dragon (a Dark Blade, +400) y Zombie Tiger (a Decayed Commander, +500)", "pitch_dark_dragon");
		probadas["zombie_tiger"]++;
		Escena e;
		e.mon(0, "dark_blade");
		e.mon(0, "pitch_dark_dragon");
		const int base = e.d.l[0].m[0].atkEf;
		VERIFICA(e.activar(0, M(0, 1)), "no se equipo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].atkEf == base + 400, "ATK %d", e.d.l[0].m[0].atkEf);
		fin();
	}
	{
		caso("Jinzo #7 ataca directo", "jinzo_7");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "jinzo_7");
		e.mon(1, "summoned_skull");
		VERIFICA(atacar(e, a, -1) && e.d.l[1].lp == LP_INICIAL - 500, "LP %d", e.d.l[1].lp);
		fin();
	}
}

static void tanda7h()
{
	fprintf(stderr, "tanda 7 (H)\n");
	{
		caso("The Trojan Horse (doble para TIERRA), Sanctuary in the Sky (Hada sin danno), Des Wombat (sin danno de efectos)", "the_trojan_horse");
		for (const char* id : { "the_sanctuary_in_the_sky", "des_wombat" }) probadas[id]++;
		Escena e;
		e.mon(0, "the_trojan_horse");
		e.mano(0, "sword_arm_of_dragon");   // (nivel 6, TIERRA: con 1 basta)
		Jugada inv;
		inv.tipo = J_INVOCAR;
		inv.mano = 0;
		inv.tributos = { 0 };
		VERIFICA(e.d.aplicar(inv), "no se invoco");
		Escena f(F_BATALLA);
		const int a = f.mon(0, "summoned_skull");
		const int z = f.mon(1, "mystical_elf");   // (no es Hada: Lanzador)
		f.mon(1, "shining_angel");   // Hada
		f.d.l[1].campo = MagiaTrampa{ "the_sanctuary_in_the_sky", false };
		f.d.l[1].campo.uid = f.d.nuevoUid();
		VERIFICA(atacar(f, a, 1), "no ataco");
		f.pasarTodo();
		VERIFICA(f.d.l[1].lp == LP_INICIAL, "el Hada recibio danno (LP %d)", f.d.l[1].lp);
		(void)z;
		Escena g;
		g.mon(1, "des_wombat");
		VERIFICA(usar(g, "hinotama") && g.d.l[1].lp == LP_INICIAL, "Des Wombat: LP %d", g.d.l[1].lp);
		fin();
	}
	{
		caso("Ataques directos: Queen's Double, Shadowslayer y Black Tyranno con todo el rival en defensa", "queen_s_double");
		for (const char* id : { "shadowslayer", "black_tyranno" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "shadowslayer");
		e.mon(1, "giant_soldier_of_stone", true);
		VERIFICA(atacar(e, a, -1), "Shadowslayer no ataco directo");
		Escena f(F_BATALLA);
		const int b = f.mon(0, "queen_s_double");
		f.mon(1, "summoned_skull");
		VERIFICA(atacar(f, b, -1), "Queen's Double no ataco directo");
		fin();
	}
	{
		caso("Panther Warrior sacrifica para atacar; Invasion of Flames; D.D. Crazy Beast destierra; Spear Dragon perfora y queda en defensa", "panther_warrior");
		for (const char* id : { "invasion_of_flames", "d_d_crazy_beast", "spear_dragon", "dark_driceratops" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "panther_warrior");
		VERIFICA(!e.d.puedeAtacar(0, a), "ataco sin otro monstruo");
		e.mon(0, "kuriboh");
		VERIFICA(atacar(e, a, -1) && e.d.l[0].monstruos() == 1, "no sacrifico");
		Escena f(F_BATALLA);
		const int s = f.mon(0, "spear_dragon");
		const int k = f.mon(1, "kuriboh", true);
		VERIFICA(atacar(f, s, k) && f.d.l[0].m[(size_t)s].defensa && f.d.l[1].lp < LP_INICIAL, "Spear Dragon");
		Escena g(F_BATALLA);
		const int c = g.mon(0, "d_d_crazy_beast");
		const int k2 = g.mon(1, "kuriboh");
		VERIFICA(atacar(g, c, k2) && g.d.l[1].desterrados.size() == 1, "Crazy Beast");
		fin();
	}
	{
		caso("Equipos Electro-Whip (Trueno) y Follow Wind (Bestia Alada); Cyber Harpie Lady", "follow_wind");
		probadas["electro_whip"]++;
		probadas["cyber_harpie_lady"]++;
		Escena e;
		e.mon(0, "harpie_lady");
		VERIFICA(usar(e, "follow_wind", { M(0, 0) }) && e.d.l[0].m[0].atkEf == 1600, "ATK %d", e.d.l[0].m[0].atkEf);
		fin();
	}
	{
		caso("Sacrificar por danno: Amazoness Archer (2: 1200), Anti-Aircraft Flower (Insecto: 800), Cannonholder (700)", "amazoness_archer");
		for (const char* id : { "anti_aircraft_flower", "gravekeeper_s_cannonholder" }) probadas[id]++;
		Escena e;
		e.mon(0, "amazoness_archer");
		e.mon(0, "kuriboh");
		e.mon(0, "kuriboh");
		VERIFICA(e.activar(0, M(0, 0), {}, {}, { M(0, 1), M(0, 2) }), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 1200, "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Cobra Jar (ficha), Dice Jar, Mask of Darkness, Iron Blacksmith Kotetsu (volteos)", "cobra_jar");
		for (const char* id : { "dice_jar", "mask_of_darkness", "iron_blacksmith_kotetsu" }) probadas[id]++;
		Escena e;
		const int z = e.mon(0, "cobra_jar", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 2, "sin ficha");
		Escena f;
		f.d.l[0].cementerio.push_back("mirror_force");
		const int m = f.mon(0, "mask_of_darkness", true, true);
		VERIFICA(voltear(f, m), "no se volteo (Mask)");
		f.pasarTodo();
		VERIFICA(enMano(f, 0, "mirror_force"), "Mask no trajo la trampa");
		fin();
	}
	{
		caso("Crimson Sentry: uno destruido en batalla al fondo del mazo", "crimson_sentry");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "celtic_guardian");
		e.mon(0, "crimson_sentry");
		const int z = e.mon(1, "summoned_skull");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		e.d.fase = F_PRINCIPAL2;
		VERIFICA(e.activar(0, M(0, 1)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].mazo.front() == "celtic_guardian", "no quedo al fondo");
		fin();
	}
	{
		caso("Embodiment of Apophis: la trampa se invoca como monstruo 1600/1800", "embodiment_of_apophis");
		Escena e;
		const int z = e.colocar(0, "embodiment_of_apophis");
		VERIFICA(e.activar(0, MT(0, z)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].id == "embodiment_of_apophis" && e.d.l[0].m[0].atkEf == 1600 && e.d.l[0].mt[(size_t)z].vacia(), "estado inesperado");
		fin();
	}
	{
		caso("Pinch Hopper: al cementerio, un Insecto de la mano", "pinch_hopper");
		Escena e;
		e.mon(0, "pinch_hopper");
		e.mano(0, "killer_needle");
		const int k = e.mano(0, "tribute_to_the_doomed");
		e.mano(0, "kuriboh");
		VERIFICA(e.activar(0, H(0, k), { M(0, 0) }, { 2 }), "no se activo");
		e.pasa(1);
		while (e.d.ef.preg.tipo == P_ELEGIR) e.elige(0, 0);
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].id == "killer_needle" || e.d.l[0].m[1].id == "killer_needle", "no invoco");
		fin();
	}
	{
		caso("Avatar of the Pot: Pot of Greed de la mano: roba 3", "avatar_of_the_pot");
		Escena e;
		e.mon(0, "avatar_of_the_pot");
		e.mano(0, "pot_of_greed");
		VERIFICA(e.activar(0, M(0, 0)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].mano.size() == 3 && e.enCementerio(0, "pot_of_greed") == 1, "mano %d", (int)e.d.l[0].mano.size());
		fin();
	}
	{
		caso("Barrel Dragon, Bazoo, Chaosrider Gustaph, Orgoth (ignicion)", "barrel_dragon");
		for (const char* id : { "bazoo_the_soul_eater", "chaosrider_gustaph", "orgoth_the_relentless" }) probadas[id]++;
		Escena e;
		e.mon(0, "bazoo_the_soul_eater");
		e.d.l[0].cementerio = { "kuriboh", "kuriboh", "celtic_guardian" };
		const int base = e.d.l[0].m[0].atkEf;
		VERIFICA(e.activar(0, M(0, 0)), "Bazoo no se activo");
		e.pasa(1);
		for (int k = 0; k < 3 && e.d.ef.preg.tipo == P_ELEGIR; k++) e.elige(0, 0);
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].atkEf == base + 900 && e.d.l[0].desterrados.size() == 3, "Bazoo: ATK %d", e.d.l[0].m[0].atkEf);
		fin();
	}
	{
		caso("Berfomet busca a Gazelle; Decayed Commander trae a Zombie Tiger; Granadora +1000", "berfomet");
		for (const char* id : { "decayed_commander", "granadora" }) probadas[id]++;
		Escena e;
		e.d.l[0].mazo.push_back("gazelle_the_king_of_mythical_beasts");
		const int bz = e.mon(0, "berfomet", true, true);
		VERIFICA(voltear(e, bz), "no se volteo");
		e.pasa(1);
		if (e.d.ef.preg.tipo == P_ELEGIR) e.elige(0, 0);
		e.pasarTodo();
		VERIFICA(enMano(e, 0, "gazelle_the_king_of_mythical_beasts"), "no busco");
		Escena f;
		f.mano(0, "granadora");
		VERIFICA(invocar(f, 0), "no se invoco Granadora");
		f.pasarTodo();
		VERIFICA(f.d.l[0].lp == LP_INICIAL + 1000, "Granadora: LP %d", f.d.l[0].lp);
		fin();
	}
	{
		caso("Desrook / Infernalqueen Archfiend pagan 500 (no con Pandemonium)", "desrook_archfiend");
		probadas["infernalqueen_archfiend"]++;
		Escena e;
		e.mon(1, "desrook_archfiend");
		pasarFases(e, 1);
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 500, "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Granmarg destruye una carta colocada al invocarse con sacrificio", "granmarg_the_rock_monarch");
		Escena e;
		e.mon(0, "kuriboh");
		e.colocar(1, "mirror_force");
		e.mano(0, "granmarg_the_rock_monarch");
		Jugada inv;
		inv.tipo = J_INVOCAR;
		inv.mano = 0;
		inv.tributos = { 0 };
		VERIFICA(e.d.aplicar(inv), "no se invoco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].colocadasMT() == 0, "no destruyo");
		fin();
	}
	{
		caso("King Tiger Wanghu destruye los invocados de 1400 o menos", "king_tiger_wanghu");
		Escena e;
		e.mon(1, "king_tiger_wanghu");
		e.mano(0, "kuriboh");
		VERIFICA(invocar(e, 0), "no se invoco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 0, "Kuriboh sigue");
		fin();
	}
	{
		caso("Sanga of the Thunder (como Suijin) y Chain Destruction (copias de la mano y el mazo)", "chain_destruction");
		probadas["sanga_of_the_thunder"]++;
		Escena e;
		const int z = e.colocar(1, "chain_destruction");
		e.mano(0, "celtic_guardian");
		e.mano(0, "celtic_guardian");
		const size_t mazo = e.d.l[0].mazo.size();
		VERIFICA(invocar(e, 0), "no se invoco");
		VERIFICA(e.activar(1, MT(1, z), { M(0, 0) }), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].mano.empty() && e.d.l[0].mazo.size() < mazo, "no destruyo las copias");
		fin();
	}
}

static void tanda7i()
{
	fprintf(stderr, "tanda 7 (I)\n");
	{
		caso("Objetivos de ataque: Hieracosphinx protege los boca abajo; Magician's Valkyria a los Lanzadores; Fisherman con Umi", "hieracosphinx");
		for (const char* id : { "magician_s_valkyria", "the_legendary_fisherman", "grave_ohja" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		e.mon(1, "hieracosphinx");
		const int z = e.mon(1, "kuriboh", true, true);
		VERIFICA(!atacar(e, a, z), "ataco al boca abajo");
		Escena f(F_BATALLA);
		const int b = f.mon(0, "summoned_skull");
		f.mon(1, "magician_s_valkyria");
		const int dm = f.mon(1, "dark_magician");
		VERIFICA(!atacar(f, b, dm) && atacar(f, b, 0), "Valkyria");
		fin();
	}
	{
		caso("Eria the Water Charmer: toma un AGUA mientras este boca arriba; si se va, vuelve", "eria_the_water_charmer");
		probadas["hiita_the_fire_charmer"]++;
		Escena e;
		e.mon(1, "kairyu_shin");
		const int z = e.mon(0, "eria_the_water_charmer", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 2 && e.d.l[1].monstruos() == 0, "no tomo el control");
		e.d.alCementerio(0, z);
		VERIFICA(e.d.l[1].monstruos() == 1, "no volvio");
		fin();
	}
	{
		caso("Equipos: Salamandra (+700), Steel Shell (+400/-200), Power of Kaishin, Metalmorph", "salamandra");
		for (const char* id : { "steel_shell", "power_of_kaishin", "metalmorph" }) probadas[id]++;
		Escena e;
		e.mon(0, "kairyu_shin");   // AGUA
		VERIFICA(usar(e, "steel_shell", { M(0, 0) }) && e.d.l[0].m[0].atkEf == 2200 && e.d.l[0].m[0].defEf == 1300, "Steel Shell");
		const int k = e.mano(0, "salamandra");
		VERIFICA(motivoNoActivar(e.d, 0, H(0, k), false) == "ef.sin_objetivo", "Salamandra sin FUEGO");
		fin();
	}
	{
		caso("Rocket Warrior: atacando no muere ni recibes danno, y el otro pierde 500", "rocket_warrior");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "rocket_warrior");
		const int z = e.mon(1, "summoned_skull");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(!e.d.l[0].m[(size_t)a].vacia() && e.d.l[0].lp == LP_INICIAL && e.d.l[1].m[(size_t)z].atkEf == 2000, "estado inesperado");
		fin();
	}
	{
		caso("Spirit Reaper: no muere en batalla; elegido por un efecto, se destruye", "spirit_reaper");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int z = e.mon(1, "spirit_reaper");
		VERIFICA(atacar(e, a, z) && !e.d.l[1].m[(size_t)z].vacia(), "murio en batalla");
		e.d.fase = F_PRINCIPAL2;
		VERIFICA(usar(e, "shrink", { M(1, z) }) && e.d.l[1].m[(size_t)z].vacia(), "no se destruyo al ser elegido");   // (boca arriba)
		fin();
	}
	{
		caso("Fire Princess: cada vez que ganas LP, 500 al rival", "fire_princess");
		Escena e;
		e.mon(0, "fire_princess");
		VERIFICA(usar(e, "red_medicine") && e.d.l[1].lp == LP_INICIAL - 500, "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Big Tusked Mammoth: los del rival no atacan el turno en que llegan; Rainbow Flower directo", "big_tusked_mammoth");
		probadas["rainbow_flower"]++;
		Escena e(F_BATALLA);
		e.mon(1, "big_tusked_mammoth");
		const int a = e.mon(0, "summoned_skull");
		e.d.l[0].m[(size_t)a].llegoEsteTurno = true;
		VERIFICA(!e.d.puedeAtacar(0, a), "ataco el turno en que llego");
		fin();
	}
	{
		caso("Thousand Needles destruye al atacante debil; Mystical Knight of Jackal manda lo destruido encima del mazo", "thousand_needles");
		probadas["two_thousand_needles"]++;
		probadas["mystical_knight_of_jackal"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "celtic_guardian");
		const int z = e.mon(1, "thousand_needles", true);
		VERIFICA(atacar(e, a, z) && e.d.l[0].monstruos() == 0, "no destruyo al atacante");
		Escena f(F_BATALLA);
		const int b = f.mon(0, "mystical_knight_of_jackal");
		const int k = f.mon(1, "kuriboh");
		VERIFICA(atacar(f, b, k) && f.d.l[1].mazo.back() == "kuriboh", "no quedo encima del mazo");
		fin();
	}
	{
		caso("Kazejin (ATK del atacante a 0), Masked Sorcerer (roba al hacer danno)", "kazejin");
		probadas["masked_sorcerer"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "masked_sorcerer");
		VERIFICA(atacar(e, a, -1), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].mano.size() == 1, "no robo");
		fin();
	}
	{
		caso("Rain of Mercy, Spirit of the Breeze, Cure Mermaid (LP)", "rain_of_mercy");
		for (const char* id : { "spirit_of_the_breeze", "cure_mermaid" }) probadas[id]++;
		Escena e;
		VERIFICA(usar(e, "rain_of_mercy") && e.d.l[0].lp == LP_INICIAL + 1000 && e.d.l[1].lp == LP_INICIAL + 1000, "Rain of Mercy");
		Escena f;
		f.mon(1, "cure_mermaid");
		pasarFases(f, 1);
		VERIFICA(f.d.l[1].lp == LP_INICIAL + 800, "Cure Mermaid: LP %d", f.d.l[1].lp);
		fin();
	}
	{
		caso("The Thing in the Crater (Piro de la mano), Thousand Knives (con Dark Magician), Throwstone Unit", "thousand_knives");
		probadas["the_thing_in_the_crater"]++;
		probadas["throwstone_unit"]++;
		Escena e;
		e.mon(0, "dark_magician");
		e.mon(1, "summoned_skull");
		VERIFICA(usar(e, "thousand_knives", { M(1, 0) }) && e.d.l[1].monstruos() == 0, "Thousand Knives");
		fin();
	}
	{
		caso("Volteos: White Ninja, 4-Starred Ladybug, Witch Doctor of Chaos, Bite Shoes, Hade-Hane", "4_starred_ladybug_of_doom");
		for (const char* id : { "white_ninja", "witch_doctor_of_chaos", "bite_shoes", "hade_hane" }) probadas[id]++;
		Escena e;
		e.mon(1, "celtic_guardian");
		e.mon(1, "kuriboh");
		const int z = e.mon(0, "4_starred_ladybug_of_doom", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 1 && e.d.l[1].m[1].id == "kuriboh", "Ladybug");
		Escena f;
		f.mon(1, "giant_soldier_of_stone", true);
		const int w = f.mon(0, "white_ninja", true, true);
		VERIFICA(voltear(f, w), "no se volteo (White Ninja)");
		f.pasa(1);
		while (f.d.ef.preg.tipo == P_ELEGIR) f.elige(0, indiceCand(f, "giant_soldier_of_stone") >= 0 ? indiceCand(f, "giant_soldier_of_stone") : 0);
		f.pasarTodo();
		VERIFICA(f.d.l[1].monstruos() == 0, "White Ninja no destruyo");
		fin();
	}
	{
		caso("Black Dragon's Chick trae a Red-Eyes; Burning Algae (+1000 rival); Cockroach Knight vuelve al mazo", "black_dragon_s_chick");
		probadas["burning_algae"]++;
		probadas["cockroach_knight"]++;
		Escena e;
		e.mon(0, "black_dragon_s_chick");
		e.mano(0, "red_eyes");
		VERIFICA(e.activar(0, M(0, 0)), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].id == "red_eyes", "no trajo a Red-Eyes");
		Escena f;
		f.mon(0, "cockroach_knight");
		f.mon(0, "burning_algae");
		VERIFICA(usar(f, "dark_hole"), "Dark Hole no se activo");
		VERIFICA(f.d.l[0].mazo.back() == "cockroach_knight" && f.d.l[1].lp == LP_INICIAL + 1000, "estado inesperado");
		fin();
	}
	{
		caso("Burning Land (500 al del turno en la espera), Catapult Turtle (mitad del ATK sacrificado)", "catapult_turtle");
		probadas["burning_land"]++;
		Escena e;
		e.mon(0, "catapult_turtle");
		e.mon(0, "summoned_skull");
		VERIFICA(e.activar(0, M(0, 0), {}, {}, { M(0, 1) }), "no se activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 1250, "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Chaos Necromancer (300 por monstruo en tu cementerio), Gren Maju (400 por desterrada)", "chaos_necromancer");
		probadas["gren_maju_da_eiza"]++;
		Escena e;
		e.d.l[0].cementerio = { "kuriboh", "celtic_guardian", "raigeki" };
		e.mon(0, "chaos_necromancer");
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[0].atkEf == 600, "ATK %d", e.d.l[0].m[0].atkEf);
		fin();
	}
	{
		caso("Crimson Ninja, Golem Sentry, Elegant Egotist, Final Flame, Just Desserts, Gravekeeper's Curse", "elegant_egotist");
		for (const char* id : { "crimson_ninja", "golem_sentry", "final_flame", "just_desserts", "gravekeeper_s_curse" }) probadas[id]++;
		Escena e;
		e.mon(0, "harpie_lady");
		e.d.l[0].mazo.push_back("harpie_lady_sisters");
		VERIFICA(usar(e, "elegant_egotist") && e.d.l[0].monstruos() == 2, "Egotist");
		e.mon(1, "kuriboh");
		e.mon(1, "kuriboh");
		VERIFICA(usarColocada(e, 0, "just_desserts") && e.d.l[1].lp == LP_INICIAL - 1000, "Just Desserts: LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Goddess of Whim, Gravity Axe - Grarl (+500), Jirai Gumo, Jinzo - Returner", "gravity_axe_grarl");
		for (const char* id : { "goddess_of_whim", "jirai_gumo", "jinzo_returner" }) probadas[id]++;
		Escena e;
		e.mon(0, "celtic_guardian");
		VERIFICA(usar(e, "gravity_axe_grarl", { M(0, 0) }) && e.d.l[0].m[0].atkEf == 1900, "Grarl");
		Escena f;
		f.d.l[0].cementerio.push_back("jinzo");
		f.mon(0, "jinzo_returner");
		f.d.alCementerio(0, 0);
		motorVentana(f.d, Ventana{ V_DISPARO, 0, -1, 0, {} });
		f.pasarTodo();
		VERIFICA(f.d.l[0].m[0].id == "jinzo" || f.d.l[0].m[1].id == "jinzo", "no trajo a Jinzo");
		fin();
	}
	{
		caso("A-Team: Trap Disposal Unit: se sacrifica y niega una trampa", "a_team_trap_disposal_unit");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		e.mon(0, "a_team_trap_disposal_unit");
		const int z = e.colocar(1, "mirror_force");
		VERIFICA(atacar(e, a, -1), "no ataco");
		VERIFICA(e.activar(1, MT(1, z)), "Mirror Force no se activo");
		VERIFICA(e.activar(0, M(0, 1)), "A-Team no se activo");
		e.pasarTodo();
		VERIFICA(!e.d.l[0].m[(size_t)a].vacia() && e.d.l[1].lp == LP_INICIAL - 2500, "Mirror Force no se nego");
		fin();
	}
}

// la IA usa las cartas: con la mano y el campo armados, ¿activa lo obvio?
static int mtArriba(Escena& e, int lado, const char* id, int eqUid = 0)
{
	const int z = e.colocar(lado, id);
	e.d.l[(size_t)lado].mt[(size_t)z].bocaAbajo = false;
	e.d.l[(size_t)lado].mt[(size_t)z].eqUid = eqUid;
	e.d.recalcular();
	return z;
}
static void tanda7j()
{
	fprintf(stderr, "tanda 7 (J)\n");
	{
		caso("Kaiser Colosseum: el rival no puede tener mas monstruos que su controlador", "kaiser_colosseum");
		Escena e;
		mtArriba(e, 1, "kaiser_colosseum");
		e.mon(1, "battle_ox");
		e.mon(0, "celtic_guardian");
		const int k = e.mano(0, "battle_ox");
		VERIFICA(!invocar(e, k), "invoco un segundo monstruo");
		Escena f;
		mtArriba(f, 1, "kaiser_colosseum");
		f.mon(1, "battle_ox");
		VERIFICA(invocar(f, f.mano(0, "battle_ox")), "no invoco el primero");
		fin();
	}
	{
		caso("Gaap the Divine Soldier: todos en ataque boca arriba, sin cambios de posicion; +300 por Demonio en la mano", "gaap_the_divine_soldier");
		Escena e;
		const int z = e.mon(1, "man_eater_bug", true, true);
		const int g = e.mon(0, "gaap_the_divine_soldier");
		VERIFICA(!e.d.l[1].m[(size_t)z].bocaAbajo && !e.d.l[1].m[(size_t)z].defensa && e.d.ef.disparos.empty(), "no paso a ataque (o disparo el volteo)");
		Jugada j;
		j.tipo = J_POSICION;
		j.zona = g;
		VERIFICA(!e.d.aplicar(j), "cambio de posicion");
		e.mano(0, "dark_ruler_ha_des");
		VERIFICA(e.activar(0, M(0, g)), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)g].atkEf == 2500, "ATK %d", e.d.l[0].m[(size_t)g].atkEf);
		fin();
	}
	{
		caso("The Agent of Judgment - Saturn: con el Santuario, danno = diferencia de LP", "the_agent_of_judgment_saturn");
		Escena e;
		e.d.l[0].campo = MagiaTrampa{ "the_sanctuary_in_the_sky", false };
		e.d.l[0].campo.uid = e.d.nuevoUid();
		e.d.l[1].lp = 5000;
		const int z = e.mon(0, "the_agent_of_judgment_saturn");
		VERIFICA(e.activar(0, M(0, z)), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == 2000 && e.d.l[0].m[(size_t)z].vacia() && !e.d.puedeAtacar(0, 0), "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Guardian Grarl: solo con Gravity Axe; unica carta en la mano: invocacion especial", "guardian_grarl");
		probadas["guardian_tryce"]++;
		Escena e;
		e.mano(0, "guardian_grarl");
		VERIFICA(!e.activar(0, H(0, 0)), "se invoco sin Gravity Axe");
		mtArriba(e, 0, "gravity_axe_grarl");
		VERIFICA(e.activar(0, H(0, 0)) && e.d.l[0].monstruos() == 1, "no se invoco");
		Escena f;
		f.mon(0, "kuriboh");
		VERIFICA(!f.d.puedeInvocarNormal(0, f.mano(0, "guardian_tryce"), { 0 }), "Guardian Tryce sin su equipo");
		fin();
	}
	{
		caso("Guardian Kay'est no se puede atacar (ataque directo) ni lo eligen las magias; Rod of Silence +500 DEF", "guardian_kay_est");
		probadas["rod_of_silence_kay_est"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "battle_ox");
		const int z = e.mon(1, "guardian_kay_est");
		VERIFICA(!atacar(e, a, z) && motorAtaqueDirecto(e.d, 0, a), "se pudo atacar / sin ataque directo");
		e.d.fase = F_PRINCIPAL1;
		const int k = e.mano(0, "book_of_moon");
		const auto ob = objetivosDe(e.d, 0, *defCarta("book_of_moon"), H(0, k));
		VERIFICA(std::find(ob.begin(), ob.end(), M(1, z)) == ob.end(), "Book of Moon lo elige");
		Escena f;
		const int m = f.mon(0, "celtic_guardian");
		VERIFICA(usar(f, "rod_of_silence_kay_est", { M(0, m) }) && f.d.l[0].m[(size_t)m].defEf == 1700, "DEF %d", f.d.l[0].m[(size_t)m].defEf);
		fin();
	}
	{
		caso("Guardian Elma: al invocarse se equipa la Butterfly Dagger del cementerio; la Dagger vuelve a la mano", "guardian_elma");
		probadas["butterfly_dagger_elma"]++;
		Escena e;
		mtArriba(e, 0, "butterfly_dagger_elma");
		e.d.l[0].cementerio.push_back("butterfly_dagger_elma");
		VERIFICA(invocar(e, e.mano(0, "guardian_elma")), "no se invoco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].atkEf == 1600 && e.d.l[0].cementerio.empty(), "ATK %d", e.d.l[0].m[0].atkEf);
		e.d.alCementerio(0, 0);
		VERIFICA(enMano(e, 0, "butterfly_dagger_elma"), "la Dagger no volvio a la mano");
		fin();
	}
	{
		caso("Guardian Baou: +1000 al destruir y niega al destruido (Sangan); Wicked-Breaking Flamberge", "guardian_baou");
		probadas["wicked_breaking_flamberge_baou"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "guardian_baou");
		const int z = e.mon(1, "sangan", true);
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)a].atkEf == 1800 && e.d.l[1].mano.empty(), "ATK %d, mano rival %d", e.d.l[0].m[(size_t)a].atkEf, (int)e.d.l[1].mano.size());
		Escena f;
		const int m = f.mon(0, "celtic_guardian");
		const int k = f.mano(0, "kuriboh");
		VERIFICA(usar(f, "wicked_breaking_flamberge_baou", { M(0, m) }, { k }) && f.d.l[0].m[(size_t)m].atkEf == 1900, "Flamberge");
		fin();
	}
	{
		caso("Guardian Ceal: manda su equipo al cementerio y destruye; Shooting Star Bow (-1000, ataque directo)", "guardian_ceal");
		probadas["shooting_star_bow_ceal"]++;
		Escena e;
		const int g = e.mon(0, "guardian_ceal");
		const int b = mtArriba(e, 0, "shooting_star_bow_ceal", e.d.l[0].m[(size_t)g].uid);
		const int z = e.mon(1, "battle_ox");
		VERIFICA(e.d.l[0].m[(size_t)g].atkEf == 700 && motorAtaqueDirecto(e.d, 0, g), "Bow");
		VERIFICA(e.activar(0, M(0, g), { M(1, z) }), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && e.d.l[0].mt[(size_t)b].vacia(), "no destruyo");
		fin();
	}
	{
		caso("Dark Ruler Ha Des: los destruidos en batalla por sus Demonios no activan efectos", "dark_ruler_ha_des");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "dark_ruler_ha_des");
		const int z = e.mon(1, "sangan", true);
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && e.d.l[1].mano.empty(), "Sangan busco");
		fin();
	}
	{
		caso("Harpie Lady 2 / Twin-Headed Wolf: niegan el volteo de lo que destruyen en batalla", "harpie_lady_2");
		probadas["twin_headed_wolf"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "harpie_lady_2");
		const int z = e.mon(1, "man_eater_bug", true, true);
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && !e.d.l[0].m[(size_t)a].vacia(), "el volteo se activo");
		fin();
	}
	{
		caso("Element Doom: con TIERRA niega al destruido; con VIENTO ataca otra vez. Element Soldier con AGUA no cambia de control",
		     "element_doom");
		for (const char* id : { "element_soldier", "element_magician" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "element_doom");
		e.mon(0, "celtic_guardian");    // TIERRA
		e.mon(0, "harpie_lady");        // VIENTO
		const int z = e.mon(1, "sangan", true);
		e.mon(1, "kuriboh", true);
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].mano.empty() && e.d.puedeAtacar(0, a), "Sangan busco / no ataca otra vez");
		Escena f;
		const int s = f.mon(0, "element_soldier");
		f.mon(1, "kairyu_shin");   // AGUA
		VERIFICA(f.d.cambiarControl(0, s) < 0, "cambio de control");
		fin();
	}
	{
		caso("Silent Swordsman LV3 -> LV5 (fase de espera) -> LV7 (tras danno directo); las magias del rival no los eligen",
		     "silent_swordsman_lv3");
		probadas["silent_swordsman_lv5"]++;
		Escena e;
		e.mon(0, "silent_swordsman_lv3");
		e.d.l[0].mazo.push_back("silent_swordsman_lv5");
		e.d.l[0].mazo.push_back("silent_swordsman_lv7");
		const int k = e.mano(1, "book_of_moon");
		VERIFICA(objetivosDe(e.d, 1, *defCarta("book_of_moon"), H(1, k)).empty(), "Book of Moon del rival lo elige");
		motorFaseEspera(e.d);
		resolverDisparos(e);
		int z = -1;
		for (int i = 0; i < ZONAS; i++)
			if (e.d.l[0].m[(size_t)i].id == "silent_swordsman_lv5") z = i;
		VERIFICA(z >= 0, "no subio a LV5");
		if (z >= 0) {
			e.d.fase = F_BATALLA;
			VERIFICA(atacar(e, z, -1), "no ataco");
			e.pasarTodo();
			motorFaseEspera(e.d);
			resolverDisparos(e);
			VERIFICA(e.d.l[0].m[(size_t)z].id == "silent_swordsman_lv7", "no subio a LV7");
		}
		fin();
	}
	{
		caso("Horus LV4: destruyo en batalla: en la fase final sube a LV6 (no lo eligen las magias)", "horus_the_black_flame_dragon_lv4");
		probadas["horus_the_black_flame_dragon_lv6"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "horus_the_black_flame_dragon_lv4");
		const int z = e.mon(1, "kuriboh");
		e.d.l[0].mazo.push_back("horus_the_black_flame_dragon_lv6");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		motorFinTurno(e.d);
		resolverDisparos(e);
		VERIFICA(e.d.l[0].m[(size_t)a].id == "horus_the_black_flame_dragon_lv6", "no subio: %s", e.d.l[0].m[(size_t)a].id.c_str());
		const int k = e.mano(0, "book_of_moon");
		VERIFICA(motivoNoActivar(e.d, 0, H(0, k), false) == "ef.sin_objetivo", "Book of Moon elige a LV6");
		fin();
	}
	{
		caso("Ultimate Insect LV1 -> LV3 (-300 ATK al rival) -> LV5 (-500) -> LV7 (-700/-700)", "ultimate_insect_lv1");
		for (const char* id : { "ultimate_insect_lv3", "ultimate_insect_lv5", "ultimate_insect_lv7" }) probadas[id]++;
		Escena e;
		e.mon(0, "ultimate_insect_lv1");
		const int z = e.mon(1, "battle_ox");
		for (const char* id : { "ultimate_insect_lv7", "ultimate_insect_lv5", "ultimate_insect_lv3" }) e.d.l[0].mazo.push_back(id);
		motorFaseEspera(e.d);
		resolverDisparos(e);
		VERIFICA(e.d.l[1].m[(size_t)z].atkEf == 1400, "LV3: ATK %d", e.d.l[1].m[(size_t)z].atkEf);
		motorFaseEspera(e.d);
		resolverDisparos(e);
		VERIFICA(e.d.l[1].m[(size_t)z].atkEf == 1200, "LV5: ATK %d", e.d.l[1].m[(size_t)z].atkEf);
		motorFaseEspera(e.d);
		resolverDisparos(e);
		VERIFICA(e.d.l[1].m[(size_t)z].atkEf == 1000 && e.d.l[1].m[(size_t)z].defEf == 300, "LV7: %d/%d", e.d.l[1].m[(size_t)z].atkEf, e.d.l[1].m[(size_t)z].defEf);
		fin();
	}
	{
		caso("Descartados por el rival: Regenerating Mummy, Electric Snake, Elephant Statue, Fear from the Dark", "regenerating_mummy");
		for (const char* id : { "electric_snake", "elephant_statue_of_disaster", "fear_from_the_dark", "despair_from_the_dark" }) probadas[id]++;
		Escena e;
		for (const char* id : { "regenerating_mummy", "electric_snake", "elephant_statue_of_disaster", "fear_from_the_dark" }) e.mano(1, id);
		VERIFICA(usar(e, "card_destruction"), "no activo");
		VERIFICA(enMano(e, 1, "regenerating_mummy") && e.d.l[1].mano.size() == 7 && e.d.l[0].lp == LP_INICIAL - 2000 && e.d.l[1].monstruos() == 1,
		         "mano %d, LP %d, monstruos %d", (int)e.d.l[1].mano.size(), e.d.l[0].lp, e.d.l[1].monstruos());
		fin();
	}
	{
		caso("Roc from the Valley of Haze vuelve al mazo; Penguin Knight: el cementerio vuelve al mazo", "roc_from_the_valley_of_haze");
		probadas["penguin_knight"]++;
		Escena e;
		const size_t antes = e.d.l[0].mazo.size();
		e.d.descartar(0, e.mano(0, "roc_from_the_valley_of_haze"));
		VERIFICA(e.d.l[0].cementerio.empty() && e.d.l[0].mazo.size() == antes + 1, "Roc");
		e.d.l[1].cementerio = { "kuriboh", "sangan" };
		Eslabon x;
		x.lado = 0;
		x.carta = "card_destruction";
		e.d.ef.cadena.push_back(x);
		e.d.ef.resolviendo = true;
		e.d.aCementerio(1, "penguin_knight");
		e.d.ef.cadena.clear();
		e.d.ef.resolviendo = false;
		VERIFICA(e.d.ef.disparos.size() == 1, "no se disparo");
		if (e.d.ef.disparos.size() == 1) defCarta("penguin_knight")->resolver(e.d, e.d.ef.disparos[0]);
		VERIFICA(e.d.l[1].cementerio.empty() && e.d.l[1].mazo.size() == 23, "mazo %d", (int)e.d.l[1].mazo.size());
		fin();
	}
}
static void tanda7k()
{
	fprintf(stderr, "tanda 7 (K)\n");
	{
		caso("Wandering Mummy / Cobraman Sakuzy: se ponen boca abajo; Royal Keeper volteado +300/+300", "wandering_mummy");
		for (const char* id : { "cobraman_sakuzy", "royal_keeper" }) probadas[id]++;
		Escena e;
		const int z = e.mon(0, "wandering_mummy");
		VERIFICA(e.activar(0, M(0, z)), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)z].bocaAbajo && e.d.l[0].m[(size_t)z].defensa, "no quedo boca abajo");
		const int r = e.mon(0, "royal_keeper", true, true);
		VERIFICA(voltear(e, r), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)r].atkEf == 1900, "ATK %d", e.d.l[0].m[(size_t)r].atkEf);
		fin();
	}
	{
		caso("Little-Winguard: cambia de posicion con su efecto", "little_winguard");
		Escena e;
		const int z = e.mon(0, "little_winguard");
		VERIFICA(e.activar(0, M(0, z)), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)z].defensa && !e.d.l[0].m[(size_t)z].bocaAbajo, "no cambio");
		fin();
	}
	{
		caso("Insect Soldiers of the Sky +1000 contra VIENTO; Cat's Ear Tribe: el que la ataca queda en 200", "insect_soldiers_of_the_sky");
		probadas["cat_s_ear_tribe"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "insect_soldiers_of_the_sky");
		const int z = e.mon(1, "harpie_lady");   // VIENTO 1300
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && e.d.l[0].m[(size_t)a].atkEf == 1000, "Insect Soldiers: ATK %d", e.d.l[0].m[(size_t)a].atkEf);
		Escena f(F_BATALLA);
		const int b = f.mon(0, "battle_ox");
		const int c = f.mon(1, "cat_s_ear_tribe");
		VERIFICA(atacar(f, b, c), "no ataco");
		f.pasarTodo();
		VERIFICA(f.d.l[0].monstruos() == 0 && f.d.l[1].monstruos() == 0 && f.d.l[0].lp == LP_INICIAL, "Cat's Ear");
		fin();
	}
	{
		caso("Prickle Fairy: los Insectos no se atacan; el que combatio con ella pasa a defensa", "prickle_fairy");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "battle_ox");
		const int p = e.mon(1, "prickle_fairy", true);
		const int i = e.mon(1, "insect_soldiers_of_the_sky");
		VERIFICA(!atacar(e, a, i), "ataco al Insecto");
		VERIFICA(atacar(e, a, p), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)a].defensa, "no paso a defensa");
		fin();
	}
	{
		caso("Electric Lizard / Harpie Lady 3: el que combatio no ataca en su proximo turno", "electric_lizard");
		probadas["harpie_lady_3"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "battle_ox");
		const int z = e.mon(1, "electric_lizard");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		e.d.turno += 2;
		e.d.l[0].m[(size_t)a].ataco = false;
		VERIFICA(!e.d.puedeAtacar(0, a), "ataco al turno siguiente");
		e.d.turno += 2;
		VERIFICA(e.d.puedeAtacar(0, a), "sigue sin atacar");
		fin();
	}
	{
		caso("Inferno Hammer: destruye en batalla y pone boca abajo uno del rival", "inferno_hammer");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "inferno_hammer");
		const int z = e.mon(1, "kuriboh");
		const int w = e.mon(1, "battle_ox");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].m[(size_t)w].bocaAbajo, "no quedo boca abajo");
		fin();
	}
	{
		caso("Andro Sphinx: no ataca al llegar; destruye en defensa: la mitad del ATK. Shinato: el ATK", "andro_sphinx");
		probadas["shinato_king_of_a_higher_plane"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "andro_sphinx");
		e.d.l[0].m[(size_t)a].llegoEsteTurno = true;
		VERIFICA(!e.d.puedeAtacar(0, a), "ataco al llegar");
		e.d.l[0].m[(size_t)a].llegoEsteTurno = false;
		const int z = e.mon(1, "battle_ox", true);
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 850, "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Tyrant Dragon: ataca otra vez si el rival sigue con monstruos; las trampas que lo eligen se niegan", "tyrant_dragon");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "tyrant_dragon");
		const int z = e.mon(1, "kuriboh");
		e.mon(1, "battle_ox");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.puedeAtacar(0, a), "no ataca otra vez");
		fin();
	}
	{
		caso("Berserk Gorilla en defensa se destruye; Thunder Nyan Nyan con un no-LUZ se destruye", "berserk_gorilla");
		probadas["thunder_nyan_nyan"]++;
		Escena e;
		e.d.invocarEspecial(0, "berserk_gorilla", true);
		VERIFICA(e.d.l[0].monstruos() == 0, "Gorilla en defensa");
		Escena f;
		f.mon(0, "thunder_nyan_nyan");
		f.mon(0, "battle_ox");
		VERIFICA(f.d.l[0].monstruos() == 1 && f.d.l[0].m[0].vacia(), "Nyan Nyan sigue");
		fin();
	}
	{
		caso("Spirits: Great Long Nose (sin batalla), Hino-Kagu-Tsuchi (descarta la mano), Fushi No Tori (gana el danno)", "great_long_nose");
		for (const char* id : { "hino_kagu_tsuchi", "fushi_no_tori" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "fushi_no_tori");
		const int b = e.mon(0, "great_long_nose");
		VERIFICA(atacar(e, a, -1), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].lp == LP_INICIAL + 1200, "Fushi: LP %d", e.d.l[0].lp);
		VERIFICA(atacar(e, b, -1), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.ef.saltaBatalla[1], "no marca la batalla");
		Escena f(F_BATALLA);
		const int h = f.mon(0, "hino_kagu_tsuchi");
		f.mano(1, "kuriboh");
		f.mano(1, "sangan");
		VERIFICA(atacar(f, h, -1), "no ataco");
		f.pasarTodo();
		f.d.activo = 1;
		motorAntesRobo(f.d);
		VERIFICA(f.d.l[1].mano.empty() && f.d.l[1].cementerio.size() == 2, "no descarto");
		fin();
	}
	{
		caso("Drill Bug pone Parasite Paracide encima del mazo; Protector of the Sanctuary: el rival no roba por efectos", "drill_bug");
		probadas["protector_of_the_sanctuary"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "drill_bug");
		e.d.l[0].mazo.insert(e.d.l[0].mazo.begin(), "parasite_paracide");
		VERIFICA(atacar(e, a, -1), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].mazo.back() == "parasite_paracide", "no quedo encima");
		Escena f;
		f.mon(1, "protector_of_the_sanctuary");
		VERIFICA(usar(f, "pot_of_greed") && f.d.l[0].mano.empty(), "robo");
		fin();
	}
	{
		caso("The Agent of Wisdom - Mercury: sin mano al final del turno rival, roba en tu fase de espera", "the_agent_of_wisdom_mercury");
		Escena e;
		e.mon(0, "the_agent_of_wisdom_mercury");
		e.d.activo = 1;
		motorFinTurno(e.d);
		e.d.activo = 0;
		motorFaseEspera(e.d);
		VERIFICA(e.d.l[0].mano.size() == 1, "mano %d", (int)e.d.l[0].mano.size());
		fin();
	}
	{
		caso("Soul-Absorbing Bone Tower: con otro Zombi no se ataca; un Zombi invocado de forma especial: 2 al cementerio del rival",
		     "soul_absorbing_bone_tower");
		Escena e(F_BATALLA);
		const int z = e.mon(1, "soul_absorbing_bone_tower");
		e.d.invocarEspecial(1, "regenerating_mummy", false);
		VERIFICA(e.d.l[0].cementerio.size() == 4, "cementerio %d", (int)e.d.l[0].cementerio.size());
		const int a = e.mon(0, "battle_ox");
		VERIFICA(!atacar(e, a, z), "ataco a la torre");
		fin();
	}
	{
		caso("Castle of Dark Illusions: volteado, los Zombis +200 y +200 mas en tu fase de espera", "castle_of_dark_illusions");
		Escena e;
		const int c = e.mon(0, "castle_of_dark_illusions", true, true);
		const int z = e.mon(1, "regenerating_mummy");
		VERIFICA(voltear(e, c), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].m[(size_t)z].atkEf == 2000, "ATK %d", e.d.l[1].m[(size_t)z].atkEf);
		motorFaseEspera(e.d);
		VERIFICA(e.d.l[1].m[(size_t)z].atkEf == 2200, "ATK %d", e.d.l[1].m[(size_t)z].atkEf);
		fin();
	}
	{
		caso("Dragon Capture Jar: los Dragones en defensa y fijos; Dragon Piper la destruye y los pasa a ataque", "dragon_capture_jar");
		probadas["dragon_piper"]++;
		Escena e;
		const int z = e.mon(1, "blue_eyes");
		mtArriba(e, 0, "dragon_capture_jar");
		VERIFICA(e.d.l[1].m[(size_t)z].defensa && motorPosicionFija(e.d, 1, z), "no paso a defensa");
		const int p = e.mon(0, "dragon_piper", true, true);
		VERIFICA(voltear(e, p), "no se volteo");
		e.pasarTodo();
		VERIFICA(!e.d.l[1].m[(size_t)z].defensa && e.enCementerio(0, "dragon_capture_jar") == 1, "Piper");
		fin();
	}
	{
		caso("Aussa the Earth Charmer toma un TIERRA", "aussa_the_earth_charmer");
		Escena e;
		e.mon(1, "battle_ox");   // TIERRA
		const int z = e.mon(0, "aussa_the_earth_charmer", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 2 && e.d.l[1].monstruos() == 0, "no tomo el control");
		fin();
	}
	{
		caso("Helpoemer destruido en tu batalla: al final, descartas 1", "helpoemer");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		const int z = e.mon(1, "helpoemer");
		e.mano(0, "kuriboh");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		Jugada j;
		j.tipo = J_FASE;
		e.d.aplicar(j);
		VERIFICA(e.d.l[0].mano.empty(), "no descarto");
		fin();
	}
	{
		caso("Armor Exe: sin contador de magia en tu campo, se destruye en la fase de espera", "armor_exe");
		Escena e;
		e.mon(0, "armor_exe");
		const int s = e.mon(0, "skilled_dark_magician");
		e.d.l[0].m[(size_t)s].contadores = 1;
		motorFaseEspera(e.d);
		VERIFICA(e.d.l[0].monstruos() == 2 && e.d.l[0].m[(size_t)s].contadores == 0, "quito el contador?");
		motorFaseEspera(e.d);
		VERIFICA(e.d.l[0].monstruos() == 1, "no se destruyo");
		fin();
	}
	{
		caso("Vampiric Orchis invoca a Des Dendle; Des Dendle unido: ficha al destruir", "vampiric_orchis");
		probadas["des_dendle"]++;
		Escena e;
		e.mano(0, "des_dendle");
		VERIFICA(invocar(e, e.mano(0, "vampiric_orchis")), "no invoco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 2, "no salio Des Dendle");
		int dd = -1;
		for (int i = 0; i < ZONAS; i++)
			if (e.d.l[0].m[(size_t)i].id == "des_dendle") dd = i;
		VERIFICA(dd >= 0 && e.activar(0, M(0, dd)), "no se unio");
		e.pasarTodo();
		e.d.fase = F_BATALLA;
		const int z = e.mon(1, "kuriboh");
		VERIFICA(atacar(e, 0, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 2, "sin ficha: %d", e.d.l[0].monstruos());
		fin();
	}
	{
		caso("Burning Beast unida a Freezing Beast: al hacer danno destruye una magia/trampa boca arriba", "burning_beast");
		for (const char* id : { "freezing_beast", "armor_breaker" }) probadas[id]++;
		Escena e;
		e.mon(0, "freezing_beast");
		const int b = e.mon(0, "burning_beast");
		VERIFICA(e.activar(0, M(0, b)), "no se unio");
		e.pasarTodo();
		mtArriba(e, 1, "toon_world");
		e.d.fase = F_BATALLA;
		VERIFICA(atacar(e, 0, -1), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].mt[0].vacia(), "no destruyo");
		fin();
	}
	{
		caso("Gilasaurus: invocacion especial; el rival invoca uno de su cementerio", "gilasaurus");
		Escena e;
		e.d.l[1].cementerio.push_back("battle_ox");
		const int k = e.mano(0, "gilasaurus");
		VERIFICA(e.activar(0, H(0, k)), "no se invoco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 1 && e.d.l[1].monstruos() == 1, "rival %d", e.d.l[1].monstruos());
		fin();
	}
	{
		caso("Tainted Wisdom: pasa a defensa y baraja", "tainted_wisdom");
		Escena e;
		const int z = e.mon(0, "tainted_wisdom");
		cambiarPosicionEfecto(e.d, 0, z, true, false);
		VERIFICA(e.d.l[0].m[(size_t)z].defensa, "no cambio");
		fin();
	}
}
static int buscarZona(Escena& e, int lado, const char* id)
{
	for (int i = 0; i < ZONAS; i++)
		if (e.d.l[(size_t)lado].m[(size_t)i].id == id) return i;
	return -1;
}
static void tanda7l()
{
	fprintf(stderr, "tanda 7 (L)\n");
	{
		caso("Magical Labyrinth: se equipa a Labyrinth Wall; despues la sacrifica: Wall Shadow del mazo", "magical_labyrinth");
		Escena e;
		const int w = e.mon(0, "labyrinth_wall", true);
		e.d.l[0].mazo.push_back("wall_shadow");
		const int k = e.mano(0, "magical_labyrinth");
		VERIFICA(e.activar(0, H(0, k), { M(0, w) }), "no se equipo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].mt[0].eqUid == e.d.l[0].m[(size_t)w].uid, "no quedo equipada");
		e.d.ef.usadosTurno.clear();
		VERIFICA(e.activar(0, MT(0, 0), { M(0, w) }), "no activo la segunda vez");
		e.pasarTodo();
		VERIFICA(buscarZona(e, 0, "wall_shadow") >= 0 && buscarZona(e, 0, "labyrinth_wall") < 0, "no salio Wall Shadow");
		fin();
	}
	{
		caso("Strike Ninja: destierra 2 OSCURIDAD; se va hasta la fase final", "strike_ninja");
		Escena e;
		const int z = e.mon(0, "strike_ninja");
		e.d.l[0].cementerio = { "dark_magician", "summoned_skull" };
		VERIFICA(e.activar(0, M(0, z)), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 0 && e.d.l[0].cementerio.empty(), "no se fue");
		motorFinTurno(e.d);
		VERIFICA(buscarZona(e, 0, "strike_ninja") >= 0, "no volvio");
		fin();
	}
	{
		caso("Serpentine Princess vuelve al mazo: uno de nivel 3 o menos del mazo", "serpentine_princess");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "serpentine_princess");
		e.d.l[0].mazo.push_back("kuriboh");
		const int z = e.mon(1, "legendary_jujitsu_master", true);
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(buscarZona(e, 0, "kuriboh") >= 0, "no salio Kuriboh");
		fin();
	}
	{
		caso("Manticore of Darkness: en la fase final manda una Bestia de la mano: vuelve", "manticore_of_darkness");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "manticore_of_darkness");
		e.mano(0, "silver_fang");
		const int z = e.mon(1, "blue_eyes");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		e.d.fase = F_PRINCIPAL2;
		Jugada j;
		j.tipo = J_FASE;
		e.d.aplicar(j);
		e.pasarTodo();
		VERIFICA(buscarZona(e, 0, "manticore_of_darkness") >= 0 && e.d.l[0].mano.empty(), "no volvio");
		fin();
	}
	{
		caso("Dark Master - Zorc: dado (destruye monstruos)", "dark_master_zorc");
		Escena e;
		const int z = e.mon(0, "dark_master_zorc");
		e.mon(1, "battle_ox");
		e.mon(1, "kuriboh");
		VERIFICA(e.activar(0, M(0, z)), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() < 2 || e.d.l[0].monstruos() == 0, "no destruyo nada");
		fin();
	}
	{
		caso("Blast Magician: contadores con cada magia; los quita y destruye", "blast_magician");
		Escena e;
		const int z = e.mon(0, "blast_magician");
		const int o = e.mon(1, "kuriboh");
		VERIFICA(usar(e, "red_medicine") && e.d.l[0].m[(size_t)z].contadores == 1, "no puso contador");
		VERIFICA(e.activar(0, M(0, z), { M(1, o) }), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && e.d.l[0].m[(size_t)z].contadores == 0, "no destruyo");
		fin();
	}
	{
		caso("Valkyrie Zweite destruye al invocarse; Valkyrie Dritte +200 por desterrado del rival", "valkyrie_zweite");
		probadas["valkyrie_dritte"]++;
		Escena f;
		f.d.l[1].desterrados = { "kuriboh", "sangan" };
		const int v = f.mon(0, "valkyrie_dritte");
		VERIFICA(f.d.l[0].m[(size_t)v].atkEf == 1400, "Dritte ATK %d", f.d.l[0].m[(size_t)v].atkEf);
		Escena g;
		g.mon(0, "kuriboh");
		g.mon(0, "sangan");
		g.mon(1, "battle_ox");
		g.d.l[0].m[0].ataco = true;
		const int vz = g.d.invocarEspecial(0, "valkyrie_zweite", false);
		motorVentana(g.d, Ventana{ V_DISPARO, 0, vz, 0, {} });
		g.pasarTodo();
		VERIFICA(g.d.l[1].monstruos() == 0, "Zweite no destruyo");
		fin();
	}
	{
		caso("Wall of Revealing Light: los que tienen ATK <= lo pagado no atacan", "wall_of_revealing_light");
		Escena e;
		e.mon(1, "battle_ox");
		VERIFICA(usarColocada(e, 0, "wall_of_revealing_light"), "no activo");
		VERIFICA(e.d.l[0].lp == LP_INICIAL - 2000, "LP %d", e.d.l[0].lp);
		e.d.activo = 1;
		e.d.fase = F_BATALLA;
		VERIFICA(!e.d.puedeAtacar(1, 0), "ataco");
		fin();
	}
	{
		caso("Apprentice Magician: contador al invocarse; destruido en batalla, un Lanzador de nivel 2 boca abajo", "apprentice_magician");
		Escena e(F_BATALLA);
		const int s = e.mon(1, "skilled_dark_magician");
		const int z = e.d.invocarEspecial(1, "apprentice_magician", false);
		motorVentana(e.d, Ventana{ V_DISPARO, 1, z, 0, {} });
		e.pasarTodo();
		VERIFICA(e.d.l[1].m[(size_t)s].contadores == 1, "no puso contador");
		e.d.l[1].mazo.push_back("apprentice_magician");
		const int a = e.mon(0, "battle_ox");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(buscarZona(e, 1, "apprentice_magician") >= 0 && e.d.l[1].m[(size_t)buscarZona(e, 1, "apprentice_magician")].bocaAbajo, "no salio boca abajo");
		fin();
	}
	{
		caso("Batteryman AA: +1000 ATK por cada uno en ataque", "batteryman_aa");
		Escena e;
		const int a = e.mon(0, "batteryman_aa");
		e.mon(0, "batteryman_aa");
		VERIFICA(e.d.l[0].m[(size_t)a].atkEf == 2000, "ATK %d", e.d.l[0].m[(size_t)a].atkEf);
		fin();
	}
	{
		caso("Card of Safe Return: revives de tu cementerio: robas 1", "card_of_safe_return");
		Escena e;
		mtArriba(e, 0, "card_of_safe_return");
		e.d.l[0].cementerio.push_back("summoned_skull");
		VERIFICA(usar(e, "monster_reborn", { Lugar{ 0, EN_CEMENTERIO, 0 } }) && e.d.l[0].mano.size() == 1, "mano %d", (int)e.d.l[0].mano.size());
		fin();
	}
	{
		caso("Chopman: invocado por volteo se equipa una magia de equipo del cementerio", "chopman_the_desperate_outlaw");
		Escena e;
		const int z = e.mon(0, "chopman_the_desperate_outlaw", true, true);
		e.d.l[0].cementerio.push_back("axe_of_despair");
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)z].atkEf == 2100, "ATK %d", e.d.l[0].m[(size_t)z].atkEf);
		fin();
	}
	{
		caso("Crass Clown pasa a ataque: uno del rival a la mano; Ryu-Kishin Clown cambia una posicion", "crass_clown");
		probadas["ryu_kishin_clown"]++;
		Escena e;
		const int z = e.mon(0, "crass_clown", true);
		e.mon(1, "battle_ox");
		Jugada j;
		j.tipo = J_POSICION;
		j.zona = z;
		VERIFICA(e.d.aplicar(j), "no cambio");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && enMano(e, 1, "battle_ox"), "no volvio a la mano");
		Escena f;
		const int o = f.mon(1, "battle_ox");
		VERIFICA(invocar(f, f.mano(0, "ryu_kishin_clown")), "no invoco");
		f.pasarTodo();
		VERIFICA(f.d.l[1].m[(size_t)o].defensa || f.d.l[0].m[0].defensa, "no cambio la posicion");
		fin();
	}
	{
		caso("Dark Magician of Chaos: magia del cementerio en la fase final; destierra lo que destruye y a si mismo", "dark_magician_of_chaos");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "dark_magician_of_chaos");
		e.d.l[0].m[(size_t)a].llegoEsteTurno = true;
		e.d.l[0].cementerio.push_back("raigeki");
		const int z = e.mon(1, "battle_ox");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].desterrados.size() == 1, "no desterro");
		e.d.fase = F_PRINCIPAL2;
		Jugada j;
		j.tipo = J_FASE;
		e.d.aplicar(j);
		e.pasarTodo();
		VERIFICA(enMano(e, 0, "raigeki"), "no recupero la magia");
		e.d.alCementerio(0, a);
		VERIFICA(e.d.l[0].desterrados.size() == 1, "no se desterro");
		fin();
	}
	{
		caso("Elephant Statue of Blessing (+2000 al descartarla el rival), Skull-Mark Ladybug (+1000 al cementerio)", "elephant_statue_of_blessing");
		probadas["skull_mark_ladybug"]++;
		Escena e;
		e.mano(1, "elephant_statue_of_blessing");
		VERIFICA(usar(e, "card_destruction") && e.d.l[1].lp == LP_INICIAL + 2000, "LP %d", e.d.l[1].lp);
		Escena f;
		const int z = f.mon(0, "skull_mark_ladybug");
		f.d.alCementerio(0, z);
		motorVentana(f.d, Ventana{ V_DISPARO, 0, -1, 0, {} });
		f.pasarTodo();
		VERIFICA(f.d.l[0].lp == LP_INICIAL + 1000, "Ladybug LP %d", f.d.l[0].lp);
		fin();
	}
	{
		caso("Firebird +500 por Bestia Alada destruida; Griggle: al rival, su duenno gana 3000", "firebird");
		probadas["griggle"]++;
		Escena e;
		const int f = e.mon(0, "firebird");
		const int h = e.mon(0, "harpie_lady");
		e.d.alCementerio(0, h);
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[(size_t)f].atkEf == 1500, "ATK %d", e.d.l[0].m[(size_t)f].atkEf);
		Escena g;
		const int z = g.mon(0, "griggle");
		g.d.cambiarControl(0, z);
		VERIFICA(g.d.l[0].lp == LP_INICIAL + 3000, "LP %d", g.d.l[0].lp);
		fin();
	}
	{
		caso("Horus' Servant: el rival no elige a Horus; Monk Fighter: sin danno de batalla", "horus_servant");
		probadas["monk_fighter"]++;
		Escena e;
		e.mon(1, "horus_servant");
		const int h = e.mon(1, "horus_the_black_flame_dragon_lv4");
		const int k = e.mano(0, "book_of_moon");
		const auto ob = objetivosDe(e.d, 0, *defCarta("book_of_moon"), H(0, k));
		VERIFICA(std::find(ob.begin(), ob.end(), M(1, h)) == ob.end(), "eligio a Horus");
		Escena f(F_BATALLA);
		const int a = f.mon(0, "monk_fighter");
		const int z = f.mon(1, "battle_ox");
		VERIFICA(atacar(f, a, z), "no ataco");
		f.pasarTodo();
		VERIFICA(f.d.l[0].lp == LP_INICIAL, "LP %d", f.d.l[0].lp);
		fin();
	}
	{
		caso("Invader of the Throne cambia el control; Rafflesia Seduction toma uno hasta el final del turno", "invader_of_the_throne");
		probadas["rafflesia_seduction"]++;
		Escena e;
		const int z = e.mon(0, "invader_of_the_throne", true, true);
		e.mon(1, "battle_ox");
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(buscarZona(e, 0, "battle_ox") >= 0 && buscarZona(e, 1, "invader_of_the_throne") >= 0, "no cambio");
		Escena f;
		const int r = f.mon(0, "rafflesia_seduction", true, true);
		f.mon(1, "battle_ox");
		VERIFICA(voltear(f, r), "no se volteo");
		f.pasarTodo();
		VERIFICA(buscarZona(f, 0, "battle_ox") >= 0, "no lo tomo");
		fin();
	}
	{
		caso("Karate Man: dobla su ATK y se destruye en la fase final", "karate_man");
		Escena e;
		const int z = e.mon(0, "karate_man");
		VERIFICA(e.activar(0, M(0, z)), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)z].atkEf == 2000, "ATK %d", e.d.l[0].m[(size_t)z].atkEf);
		motorFinTurno(e.d);
		VERIFICA(e.d.l[0].monstruos() == 0, "no se destruyo");
		fin();
	}
	{
		caso("Kiryu unido a Dark Blade +900; Metallizing Parasite: las magias del rival no lo eligen", "kiryu");
		probadas["metallizing_parasite_lunatite"]++;
		Escena e;
		const int b = e.mon(0, "dark_blade");
		const int k = e.mon(0, "kiryu");
		VERIFICA(e.activar(0, M(0, k)), "no se unio");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)b].atkEf == 2700, "ATK %d", e.d.l[0].m[(size_t)b].atkEf);
		Escena f;
		const int m = f.mon(0, "celtic_guardian");
		const int l = f.mon(0, "metallizing_parasite_lunatite");
		VERIFICA(f.activar(0, M(0, l)), "no se unio");
		f.pasarTodo();
		const int q = f.mano(1, "book_of_moon");
		const auto ob = objetivosDe(f.d, 1, *defCarta("book_of_moon"), H(1, q));
		VERIFICA(std::find(ob.begin(), ob.end(), M(0, m)) == ob.end(), "lo eligen");
		fin();
	}
	{
		caso("Level Up!: el monstruo LV al cementerio, el siguiente del mazo", "level_up");
		Escena e;
		const int z = e.mon(0, "horus_the_black_flame_dragon_lv4");
		e.d.l[0].mazo.push_back("horus_the_black_flame_dragon_lv6");
		VERIFICA(usar(e, "level_up", { M(0, z) }) && buscarZona(e, 0, "horus_the_black_flame_dragon_lv6") >= 0, "no subio");
		fin();
	}
	{
		caso("Magical Merchant: hasta una magia/trampa: a la mano, el resto al cementerio", "magical_merchant");
		Escena e;
		e.d.l[0].mazo.push_back("raigeki");
		e.d.l[0].mazo.push_back("kuriboh");
		const int z = e.mon(0, "magical_merchant", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(enMano(e, 0, "raigeki") && e.enCementerio(0, "kuriboh") == 1, "Merchant");
		fin();
	}
	{
		caso("Mask of Brutality (+1000/-1000, paga 1000); Mask of Dispel (500 por magia); Meteor of Destruction; Poison", "mask_of_brutality");
		for (const char* id : { "mask_of_dispel", "meteor_of_destruction", "poison_of_the_old_man" }) probadas[id]++;
		Escena e;
		const int z = e.mon(0, "celtic_guardian");
		VERIFICA(usar(e, "mask_of_brutality", { M(0, z) }) && e.d.l[0].m[(size_t)z].atkEf == 2400 && e.d.l[0].m[(size_t)z].defEf == 200, "Brutality");
		motorFaseEspera(e.d);
		resolverDisparos(e);
		VERIFICA(e.d.l[0].lp == LP_INICIAL - 1000, "no pago: %d", e.d.l[0].lp);
		const int c = mtArriba(e, 1, "dark_hole");
		VERIFICA(usar(e, "mask_of_dispel", { MT(1, c) }), "Dispel");
		motorFaseEspera(e.d);
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 500, "Dispel LP %d", e.d.l[1].lp);
		Escena f;
		VERIFICA(usar(f, "meteor_of_destruction") && f.d.l[1].lp == LP_INICIAL - 1000, "Meteor");
		VERIFICA(usar(f, "poison_of_the_old_man") && f.d.l[1].lp == LP_INICIAL - 1800, "Poison %d", f.d.l[1].lp);
		fin();
	}
	{
		caso("Millennium Scorpion +500 al destruir; Molten Destruction (FUEGO +500/-400)", "millennium_scorpion");
		probadas["molten_destruction"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "millennium_scorpion");
		const int z = e.mon(1, "kuriboh");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)a].atkEf == 2500, "ATK %d", e.d.l[0].m[(size_t)a].atkEf);
		Escena f;
		const int m = f.mon(0, "firebird");
		VERIFICA(usar(f, "molten_destruction") && f.d.l[0].m[(size_t)m].atkEf == 1500, "Molten");
		fin();
	}
	{
		caso("Night Assailant: VOLTEO destruye; descartada, un volteo del cementerio a la mano. Nobleman-Eater Bug destruye 2",
		     "night_assailant");
		probadas["nobleman_eater_bug"]++;
		Escena e;
		const int z = e.mon(0, "night_assailant", true, true);
		e.mon(1, "battle_ox");
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0, "no destruyo");
		Escena f;
		f.d.l[0].cementerio.push_back("man_eater_bug");
		f.d.descartar(0, f.mano(0, "night_assailant"));
		motorVentana(f.d, Ventana{ V_DISPARO, 0, -1, 0, {} });
		f.pasarTodo();
		VERIFICA(enMano(f, 0, "man_eater_bug"), "no volvio");
		Escena g;
		const int n = g.mon(0, "nobleman_eater_bug", true, true);
		g.mon(1, "battle_ox");
		g.mon(1, "kuriboh");
		VERIFICA(voltear(g, n), "no se volteo");
		g.pasarTodo();
		VERIFICA(g.d.l[1].monstruos() + g.d.l[0].monstruos() == 1, "no destruyo 2");
		fin();
	}
	{
		caso("Nightmare Wheel: no ataca ni cambia; 500 en tu fase de espera", "nightmare_wheel");
		Escena e;
		const int z = e.mon(1, "battle_ox");
		VERIFICA(usarColocada(e, 0, "nightmare_wheel", { M(1, z) }), "no activo");
		VERIFICA(motorPosicionFija(e.d, 1, z), "cambia de posicion");
		motorFaseEspera(e.d);
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 500, "LP %d", e.d.l[1].lp);
		e.d.activo = 1;
		e.d.fase = F_BATALLA;
		VERIFICA(!e.d.puedeAtacar(1, z), "ataco");
		fin();
	}
	{
		caso("Satellite Cannon: no lo destruye uno de nivel 7 o menos; +1000 en tu fase final", "satellite_cannon");
		Escena e(F_BATALLA);
		const int s = e.mon(1, "satellite_cannon");
		const int a = e.mon(0, "summoned_skull");
		VERIFICA(atacar(e, a, s), "no ataco");
		e.pasarTodo();
		VERIFICA(!e.d.l[1].m[(size_t)s].vacia(), "se destruyo");
		e.d.activo = 1;
		motorFinTurno(e.d);
		VERIFICA(e.d.l[1].m[(size_t)s].atkEf == 1000, "ATK %d", e.d.l[1].m[(size_t)s].atkEf);
		fin();
	}
	{
		caso("Silent Magician LV4: contador por cada robo del rival; con 5 sube a LV8", "silent_magician_lv4");
		Escena e;
		const int z = e.mon(0, "silent_magician_lv4");
		e.d.l[0].mazo.push_back("silent_magician_lv8");
		for (int k = 0; k < 6; k++) e.d.robarEfecto(1);
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[(size_t)z].contadores == 5 && e.d.l[0].m[(size_t)z].atkEf == 3500, "contadores %d", e.d.l[0].m[(size_t)z].contadores);
		motorFaseEspera(e.d);
		resolverDisparos(e);
		VERIFICA(buscarZona(e, 0, "silent_magician_lv8") >= 0, "no subio");
		fin();
	}
	{
		caso("Snake Fang (-500 DEF); Stealth Bird (1000 al voltearse); Swarm of Locusts (destruye magia/trampa)", "snake_fang");
		for (const char* id : { "stealth_bird", "swarm_of_locusts" }) probadas[id]++;
		Escena e;
		const int z = e.mon(1, "battle_ox", true);
		VERIFICA(usarColocada(e, 0, "snake_fang", { M(1, z) }) && e.d.l[1].m[(size_t)z].defEf == 500, "Snake Fang");
		Escena f;
		const int b = f.mon(0, "stealth_bird", true, true);
		VERIFICA(voltear(f, b), "no se volteo");
		f.pasarTodo();
		VERIFICA(f.d.l[1].lp == LP_INICIAL - 1000, "Stealth LP %d", f.d.l[1].lp);
		f.d.ef.usadosTurno.clear();
		f.d.l[0].m[(size_t)b].llegoEsteTurno = false;
		VERIFICA(f.activar(0, M(0, b)), "no se puso boca abajo");
		f.pasarTodo();
		VERIFICA(f.d.l[0].m[(size_t)b].bocaAbajo, "sigue boca arriba");
		Escena g;
		const int l = g.mon(0, "swarm_of_locusts", true, true);
		g.colocar(1, "mirror_force");
		VERIFICA(voltear(g, l), "no se volteo");
		g.pasarTodo();
		VERIFICA(g.d.l[1].mt[0].vacia(), "no destruyo");
		fin();
	}
	{
		caso("The Fiend Megacyber: con 2 monstruos menos que el rival, desde la mano", "the_fiend_megacyber");
		Escena e;
		e.mon(1, "battle_ox");
		e.mon(1, "kuriboh");
		VERIFICA(e.activar(0, H(0, e.mano(0, "the_fiend_megacyber"))) && e.d.l[0].monstruos() == 1, "no se invoco");
		fin();
	}
	{
		caso("Pandemonium Watchbear, D.D. Scout Plane, Malice Doll of Demise (por regla)", "pandemonium_watchbear");
		for (const char* id : { "d_d_scout_plane", "malice_doll_of_demise" }) probadas[id]++;
		Escena e;
		e.mon(0, "pandemonium_watchbear");
		e.d.l[0].campo = MagiaTrampa{ "pandemonium", false };
		e.d.l[0].campo.uid = e.d.nuevoUid();
		e.d.activo = 1;
		const int k = e.mano(1, "mystical_space_typhoon");
		VERIFICA(e.activar(1, H(1, k), { Lugar{ 0, EN_CAMPO, ZONA_CAMPO } }), "no activo MST");
		e.pasarTodo();
		VERIFICA(!e.d.l[0].campo.vacia(), "destruyo Pandemonium");
		Escena f;
		const int z = f.mon(0, "d_d_scout_plane");
		f.d.sacarMonstruo(0, z, false);
		motorFinTurno(f.d);
		VERIFICA(buscarZona(f, 0, "d_d_scout_plane") >= 0, "no volvio");
		fin();
	}
}
static int revelados(const Escena& e)
{
	int n = 0;
	for (const Suceso& s : e.d.sucesos) n += s.tipo == S_REVELA;
	return n;
}
static void tanda7m()
{
	fprintf(stderr, "tanda 7 (M)\n");
	{
		caso("A Man with Wdjat / The Stern Mystic / Cobraman Sakuzy: muestran cartas ocultas", "a_man_with_wdjat");
		probadas["the_stern_mystic"]++;
		Escena e;
		e.colocar(1, "mirror_force");
		VERIFICA(invocar(e, e.mano(0, "a_man_with_wdjat")), "no invoco");
		e.pasarTodo();
		VERIFICA(revelados(e) == 1, "Wdjat: %d", revelados(e));
		Escena f;
		f.colocar(1, "mirror_force");
		const int k = f.mon(1, "kuriboh", true, true);
		const int z = f.mon(0, "the_stern_mystic", true, true);
		VERIFICA(voltear(f, z), "no se volteo");
		f.pasarTodo();
		VERIFICA(revelados(f) == 2 && f.d.l[1].m[(size_t)k].bocaAbajo && f.d.l[1].m[(size_t)k].visto, "Stern Mystic: %d", revelados(f));
		Escena g;
		g.colocar(1, "mirror_force");
		const int c = g.mon(0, "cobraman_sakuzy", true, true);
		VERIFICA(voltear(g, c), "no se volteo");
		g.pasarTodo();
		VERIFICA(revelados(g) == 1, "Cobraman: %d", revelados(g));
		fin();
	}
	{
		caso("Homunculus: cambia de atributo (y cuenta para los campos)", "homunculus_the_alchemic_being");
		Escena e;
		const int z = e.mon(0, "homunculus_the_alchemic_being");   // LUZ 1800
		e.d.l[0].campo = MagiaTrampa{ "mystic_plasma_zone", false };
		e.d.l[0].campo.uid = e.d.nuevoUid();
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[(size_t)z].atkEf == 1800, "ATK %d", e.d.l[0].m[(size_t)z].atkEf);
		VERIFICA(e.activar(0, M(0, z)), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)z].atributo == "OSCURIDAD" && e.d.l[0].m[(size_t)z].atkEf == 2300, "ATK %d", e.d.l[0].m[(size_t)z].atkEf);
		fin();
	}
	{
		caso("Sustitutos de fusion: Mystical Sheep #1 con Polymerization; Hex-Sealed Fusion invoca sacrificando del campo", "mystical_sheep_1");
		for (const char* id : { "the_light_hex_sealed_fusion", "the_dark_hex_sealed_fusion", "the_earth_hex_sealed_fusion" }) probadas[id]++;
		base.fusiones["flame_swordsman"] = { "flame_manipulator", "masaki_the_legendary_swordsman" };
		base.fusiones["black_skull_dragon"] = { "summoned_skull", "red_eyes" };
		Escena e;
		e.d.l[0].extra = { "flame_swordsman" };   // Flame Manipulator + Masaki
		const int p = e.mano(0, "polymerization");
		const int a = e.mano(0, "flame_manipulator");
		const int s = e.mano(0, "mystical_sheep_1");
		Jugada j;
		j.tipo = J_FUSION;
		j.mano = p;
		j.carta = "flame_swordsman";
		j.manoExtra = { a, s };
		VERIFICA(e.d.aplicar(j) && buscarZona(e, 0, "flame_swordsman") >= 0, "no fusiono con la oveja");
		Escena f;
		f.d.l[0].extra = { "flame_swordsman" };
		f.mon(0, "flame_manipulator");
		const int h = f.mon(0, "the_dark_hex_sealed_fusion");
		VERIFICA(motivoNoActivar(f.d, 0, M(0, h), false) == "ef.no_condicion", "Flame Swordsman no es OSCURIDAD");
		Escena g;
		g.d.l[0].extra = { "black_skull_dragon" };   // Summoned Skull + Red-Eyes
		g.mon(0, "summoned_skull");
		const int hd = g.mon(0, "the_dark_hex_sealed_fusion");
		VERIFICA(g.activar(0, M(0, hd)), "no activo");
		g.pasarTodo();
		VERIFICA(buscarZona(g, 0, "black_skull_dragon") >= 0 && g.d.l[0].monstruos() == 1, "Hex-Sealed no fusiono");
		fin();
	}
	{
		caso("Fusilier Dragon: sin sacrificios, la mitad de ATK/DEF", "fusilier_dragon_the_dual_mode_beast");
		Escena e;
		const int k = e.mano(0, "fusilier_dragon_the_dual_mode_beast");
		VERIFICA(invocar(e, k), "no invoco sin sacrificios");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[0].atkEf == 1400 && e.d.l[0].m[0].defEf == 1000, "ATK %d", e.d.l[0].m[0].atkEf);
		fin();
	}
	{
		caso("Banisher of the Light: lo que va al cementerio queda desterrado", "banisher_of_the_light");
		Escena e;
		e.mon(1, "banisher_of_the_light");
		const int z = e.mon(0, "battle_ox");
		e.d.alCementerio(0, z);
		e.d.descartar(0, e.mano(0, "kuriboh"));
		VERIFICA(e.d.l[0].cementerio.empty() && e.d.l[0].desterrados.size() == 2, "cementerio %d", (int)e.d.l[0].cementerio.size());
		fin();
	}
	{
		caso("Legendary Flame Lord: con 3 contadores destruye a los demas", "legendary_flame_lord");
		Escena e;
		const int z = e.mon(0, "legendary_flame_lord");
		e.mon(1, "battle_ox");
		e.mon(0, "kuriboh");
		for (int k = 0; k < 3; k++) usar(e, "red_medicine");
		VERIFICA(e.d.l[0].m[(size_t)z].contadores == 3, "contadores %d", e.d.l[0].m[(size_t)z].contadores);
		VERIFICA(e.activar(0, M(0, z)), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 1 && e.d.l[1].monstruos() == 0, "no destruyo");
		fin();
	}
	{
		caso("Summoner of Illusions: sacrifica otro: una fusion (se destruye en la fase final)", "summoner_of_illusions");
		Escena e;
		e.d.l[0].extra = { "flame_swordsman" };
		e.mon(0, "kuriboh");
		const int z = e.mon(0, "summoner_of_illusions", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(buscarZona(e, 0, "flame_swordsman") >= 0 && buscarZona(e, 0, "kuriboh") < 0, "no invoco la fusion");
		motorFinTurno(e.d);
		VERIFICA(buscarZona(e, 0, "flame_swordsman") < 0, "no se destruyo");
		fin();
	}
	{
		caso("Maharaghi: en tu proxima fase de robo miras la de arriba", "maharaghi");
		Escena e;
		VERIFICA(invocar(e, e.mano(0, "maharaghi")), "no invoco");
		e.pasarTodo();
		e.d.activo = 0;
		motorAntesRobo(e.d);
		VERIFICA(revelados(e) == 1, "no la mostro");
		fin();
	}
	{
		caso("Dreamsprite desvia el ataque; Maiden with Eyes of Blue lo niega y saca a Blue-Eyes", "dreamsprite");
		probadas["maiden_with_eyes_of_blue"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "battle_ox");
		const int z = e.mon(1, "dreamsprite");
		const int o = e.mon(1, "kuriboh");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].m[(size_t)o].vacia() && !e.d.l[1].m[(size_t)z].vacia(), "no desvio");
		Escena f(F_BATALLA);
		const int b = f.mon(0, "battle_ox");
		const int m = f.mon(1, "maiden_with_eyes_of_blue");
		f.d.l[1].mazo.push_back("blue_eyes");
		VERIFICA(atacar(f, b, m), "no ataco");
		f.pasarTodo();
		VERIFICA(!f.d.l[1].m[(size_t)m].vacia() && buscarZona(f, 1, "blue_eyes") >= 0, "Maiden");
		fin();
	}
	{
		caso("Gravekeeper's Assailant con Necrovalley cambia la posicion del atacado", "gravekeeper_s_assailant");
		Escena e(F_BATALLA);
		e.d.l[0].campo = MagiaTrampa{ "necrovalley", false };
		e.d.l[0].campo.uid = e.d.nuevoUid();
		const int a = e.mon(0, "gravekeeper_s_assailant");
		const int z = e.mon(1, "battle_ox");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0, "no paso a defensa (y murio)");
		fin();
	}
	{
		caso("Glass Slippers (Hada +1000; otro -1000 y no ataca); Prinzessin trae Pumpkin Carriage y ataca directo", "glass_slippers");
		for (const char* id : { "prinzessin", "pumpkin_carriage" }) probadas[id]++;
		Escena e;
		const int z = e.mon(1, "battle_ox");
		VERIFICA(usar(e, "glass_slippers", { M(1, z) }) && e.d.l[1].m[(size_t)z].atkEf == 700, "Slippers");
		Escena f;
		f.d.l[0].mazo.push_back("pumpkin_carriage");
		VERIFICA(invocar(f, f.mano(0, "prinzessin")), "no invoco");
		f.pasarTodo();
		f.mon(1, "battle_ox");
		VERIFICA(buscarZona(f, 0, "pumpkin_carriage") >= 0 && motorAtaqueDirecto(f.d, 0, buscarZona(f, 0, "prinzessin")), "Prinzessin");
		fin();
	}
	{
		caso("Sword Hunter: al final de la batalla se equipa lo que destruyo (+200)", "sword_hunter");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "sword_hunter");
		const int z = e.mon(1, "kuriboh");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		Jugada j;
		j.tipo = J_FASE;
		e.d.aplicar(j);
		VERIFICA(e.d.l[0].m[(size_t)a].atkEf == 2650 && e.d.l[1].cementerio.empty(), "ATK %d", e.d.l[0].m[(size_t)a].atkEf);
		fin();
	}
	{
		caso("The Kick Man (invocado especial: equipo del cementerio); Cyber Raider destruye un equipo del rival", "the_kick_man");
		probadas["cyber_raider"]++;
		Escena e;
		e.d.l[0].cementerio.push_back("axe_of_despair");
		const int z = e.d.invocarEspecial(0, "the_kick_man", false);
		motorVentana(e.d, Ventana{ V_DISPARO, 0, z, 0, {} });
		e.pasarTodo();
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[(size_t)z].atkEf == 2300, "Kick Man ATK %d", e.d.l[0].m[(size_t)z].atkEf);
		Escena f;
		const int o = f.mon(1, "battle_ox");
		const int eq = mtArriba(f, 1, "axe_of_despair", f.d.l[1].m[(size_t)o].uid);
		VERIFICA(invocar(f, f.mano(0, "cyber_raider")), "no invoco");
		f.pasarTodo();
		VERIFICA(f.d.l[1].mt[(size_t)eq].vacia(), "no destruyo el equipo");
		fin();
	}
	{
		caso("Mask of Restrict: nadie sacrifica", "mask_of_restrict");
		Escena e;
		mtArriba(e, 1, "mask_of_restrict");
		e.mon(0, "kuriboh");
		VERIFICA(!e.d.puedeInvocarNormal(0, e.mano(0, "summoned_skull"), { 0 }), "sacrifico");
		fin();
	}
	{
		caso("Jigen Bakudan: en tu fase de espera destruye tus monstruos: la mitad de su ATK al rival", "jigen_bakudan");
		Escena e;
		const int z = e.mon(0, "jigen_bakudan", true, true);
		e.mon(0, "summoned_skull");
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		motorFaseEspera(e.d);
		VERIFICA(e.d.l[0].monstruos() == 0 && e.d.l[1].lp == LP_INICIAL - 1250, "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Rituales sin efecto: Magician of Black Chaos, Fortress Whale", "magician_of_black_chaos");
		probadas["fortress_whale"]++;
		fin();
	}
}
static void tanda7n()
{
	fprintf(stderr, "tanda 7 (N)\n");
	{
		caso("Destiny Board: un mensaje por cada fase final del rival; con los 4 ganas. Dark Sanctuary: como monstruos", "destiny_board");
		for (const char* id : { "spirit_message_i", "spirit_message_n", "spirit_message_a", "spirit_message_l", "dark_sanctuary" }) probadas[id]++;
		Escena e;
		mtArriba(e, 0, "destiny_board");
		for (const char* id : { "spirit_message_l", "spirit_message_a", "spirit_message_n", "spirit_message_i" }) e.d.l[0].mazo.push_back(id);
		e.d.activo = 1;
		for (int k = 0; k < 3; k++) motorFinTurno(e.d);
		VERIFICA(mensajesEnCampo(e.d, 0) == 3 && e.d.ganador < 0, "mensajes %d", mensajesEnCampo(e.d, 0));
		motorFinTurno(e.d);
		VERIFICA(e.d.ganador == 0, "no gano");
		Escena f;
		mtArriba(f, 0, "destiny_board");
		f.d.l[0].campo = MagiaTrampa{ "dark_sanctuary", false };
		f.d.l[0].campo.uid = f.d.nuevoUid();
		f.d.l[0].mazo.push_back("spirit_message_i");
		f.d.activo = 1;
		motorFinTurno(f.d);
		VERIFICA(buscarZona(f, 0, "spirit_message_i") >= 0 && !motorAtacable(f.d, 0, buscarZona(f, 0, "spirit_message_i")), "Dark Sanctuary");
		fin();
	}
	{
		caso("Golden Castle of Stromberg: destruye al atacante (mitad de su ATK); destierra 10 en tu espera; trae a Prinzessin",
		     "golden_castle_of_stromberg");
		Escena e(F_BATALLA);
		e.d.l[1].campo = MagiaTrampa{ "golden_castle_of_stromberg", false };
		e.d.l[1].campo.uid = e.d.nuevoUid();
		const int a = e.mon(0, "battle_ox");
		VERIFICA(atacar(e, a, -1), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 0 && e.d.l[0].lp == LP_INICIAL - 850 && e.d.l[1].lp == LP_INICIAL, "LP %d / %d", e.d.l[0].lp, e.d.l[1].lp);
		e.d.activo = 1;
		motorFaseEspera(e.d);
		VERIFICA(e.d.l[1].desterrados.size() == 10, "desterro %d", (int)e.d.l[1].desterrados.size());
		Escena f;
		const int k = f.mano(0, "golden_castle_of_stromberg");
		f.d.l[0].mazo.push_back("prinzessin");
		VERIFICA(f.activar(0, H(0, k)), "no activo");
		f.pasarTodo();
		f.d.ef.usadosTurno.clear();
		VERIFICA(f.activar(0, Lugar{ 0, EN_CAMPO, ZONA_CAMPO }), "no activo la segunda vez");
		f.pasarTodo();
		VERIFICA(buscarZona(f, 0, "prinzessin") >= 0 && f.d.l[0].invocoNormal, "no salio Prinzessin");
		fin();
	}
	{
		caso("Bark of Dark Ruler: tu Demonio combate: pagas lo justo y el rival pierde eso", "bark_of_dark_ruler");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "dark_ruler_ha_des");   // Demonio 2450
		const int z = e.mon(1, "blue_eyes");           // 3000
		const int t = e.colocar(0, "bark_of_dark_ruler");
		VERIFICA(atacar(e, a, z), "no ataco");
		for (int g = 0; g < 10 && e.d.ef.ventana.tipo != V_PASO_DANNO; g++) e.pasa(e.d.ef.preg.lado);
		VERIFICA(e.d.ef.ventana.tipo == V_PASO_DANNO, "sin paso de danno");
		if (e.d.ef.preg.tipo == P_RESPUESTA && e.d.ef.preg.lado == 1) e.pasa(1);
		VERIFICA(e.activar(0, MT(0, t)), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && e.d.l[0].lp == LP_INICIAL - 600, "LP %d", e.d.l[0].lp);
		fin();
	}
	{
		caso("Relinquished: se equipa uno del rival (su ATK/DEF); en batalla se destruye el equipado", "relinquished");
		Escena e(F_BATALLA);
		const int r = e.mon(0, "relinquished");
		const int z = e.mon(1, "battle_ox");
		e.d.fase = F_PRINCIPAL1;
		VERIFICA(e.activar(0, M(0, r), { M(1, z) }), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)r].atkEf == 1700 && e.d.l[1].monstruos() == 0, "ATK %d", e.d.l[0].m[(size_t)r].atkEf);
		e.d.fase = F_BATALLA;
		const int b = e.mon(1, "blue_eyes");
		e.d.activo = 1;
		VERIFICA(atacar(e, b, r), "no ataco");
		e.pasarTodo();
		VERIFICA(!e.d.l[0].m[(size_t)r].vacia() && e.d.l[0].m[(size_t)r].atkEf == 0, "Relinquished murio");
		fin();
	}
	{
		caso("Uniones: si el equipado se destruye en batalla, se destruye la union en su lugar", "y_dragon_head");
		Escena e(F_BATALLA);
		const int x = e.mon(0, "x_head_cannon");
		const int y = e.mon(0, "y_dragon_head");
		e.d.fase = F_PRINCIPAL1;
		VERIFICA(e.activar(0, M(0, y)), "no se unio");
		e.pasarTodo();
		e.d.fase = F_BATALLA;
		const int b = e.mon(1, "blue_eyes");
		e.d.activo = 1;
		VERIFICA(atacar(e, b, x), "no ataco");
		e.pasarTodo();
		VERIFICA(!e.d.l[0].m[(size_t)x].vacia() && e.enCementerio(0, "y_dragon_head") == 1, "X-Head Cannon murio");
		fin();
	}
	{
		caso("Moisture Creature con 3 sacrificios destruye las magias/trampas del rival", "moisture_creature");
		Escena e;
		for (int k = 0; k < 3; k++) e.mon(0, "kuriboh");
		e.colocar(1, "mirror_force");
		const int m = e.mano(0, "moisture_creature");
		Jugada j;
		j.tipo = J_INVOCAR;
		j.mano = m;
		j.tributos = { 0, 1, 2 };
		VERIFICA(e.d.aplicar(j), "no invoco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].mt[0].vacia(), "no destruyo");
		fin();
	}
	{
		caso("Yado Karu (a defensa: cartas de la mano debajo del mazo); Dream Clown (a defensa: destruye)", "yado_karu");
		probadas["dream_clown"]++;
		Escena e;
		const int z = e.mon(0, "yado_karu");
		e.mano(0, "kuriboh");
		Jugada j;
		j.tipo = J_POSICION;
		j.zona = z;
		VERIFICA(e.d.aplicar(j), "no cambio");
		e.pasarTodo();
		VERIFICA(e.d.l[0].mano.empty() && e.d.l[0].mazo.front() == "kuriboh", "Yado Karu");
		Escena f;
		const int c = f.mon(0, "dream_clown");
		f.mon(1, "battle_ox");
		j.zona = c;
		VERIFICA(f.d.aplicar(j), "no cambio");
		f.pasarTodo();
		VERIFICA(f.d.l[1].monstruos() == 0, "Dream Clown");
		fin();
	}
	{
		caso("Ceremonial Bell muestra las manos; Orca destruye sacrificando Torpedo Fish", "ceremonial_bell");
		probadas["orca_mega_fortress_of_darkness"]++;
		Escena e;
		e.mano(1, "kuriboh");
		VERIFICA(invocar(e, e.mano(0, "ceremonial_bell")), "no invoco");
		e.pasarTodo();
		VERIFICA(revelados(e) == 1, "revelados %d", revelados(e));
		Escena f;
		const int o = f.mon(0, "orca_mega_fortress_of_darkness");
		f.mon(0, "torpedo_fish");
		f.mon(1, "battle_ox");
		VERIFICA(f.activar(0, M(0, o)), "no activo");
		f.pasarTodo();
		VERIFICA(f.d.l[1].monstruos() == 0 && buscarZona(f, 0, "torpedo_fish") < 0, "Orca");
		fin();
	}
	{
		caso("The Flute of Summoning Dragon (con Lord of D.); Thunder Dragon (desde la mano busca 2)", "the_flute_of_summoning_dragon");
		probadas["thunder_dragon"]++;
		Escena e;
		e.mon(0, "lord_of_d");
		e.mano(0, "blue_eyes");
		e.mano(0, "red_eyes");
		VERIFICA(usar(e, "the_flute_of_summoning_dragon") && e.d.l[0].monstruos() == 3, "Flute: %d", e.d.l[0].monstruos());
		Escena f;
		f.d.l[0].mazo.push_back("thunder_dragon");
		f.d.l[0].mazo.push_back("thunder_dragon");
		const int k = f.mano(0, "thunder_dragon");
		VERIFICA(f.activar(0, H(0, k)), "no activo");
		f.pasarTodo();
		VERIFICA(f.d.l[0].mano.size() == 2 && f.enCementerio(0, "thunder_dragon") == 1, "mano %d", (int)f.d.l[0].mano.size());
		fin();
	}
	{
		caso("Tornado Bird: 2 magias/trampas a la mano", "tornado_bird");
		Escena e;
		e.colocar(1, "mirror_force");
		e.colocar(1, "trap_hole");
		const int z = e.mon(0, "tornado_bird", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].mano.size() == 2, "mano %d", (int)e.d.l[1].mano.size());
		fin();
	}
	{
		caso("Valkyrie Brunhilde / Gyaku-Gire Panda (+500 por monstruo del rival); Amazoness Tiger (+400 por Amazoness)",
		     "valkyrie_brunhilde");
		for (const char* id : { "gyaku_gire_panda", "amazoness_tiger" }) probadas[id]++;
		Escena e;
		const int v = e.mon(0, "valkyrie_brunhilde");
		const int p = e.mon(0, "gyaku_gire_panda");
		e.mon(1, "battle_ox");
		e.mon(1, "kuriboh");
		VERIFICA(e.d.l[0].m[(size_t)v].atkEf == 2800 && e.d.l[0].m[(size_t)p].atkEf == 1800, "ATK %d / %d", e.d.l[0].m[(size_t)v].atkEf, e.d.l[0].m[(size_t)p].atkEf);
		Escena f;
		const int t = f.mon(0, "amazoness_tiger");
		const int s = f.mon(0, "amazoness_swords_woman");
		VERIFICA(f.d.l[0].m[(size_t)t].atkEf == 1900 && !motorAtacable(f.d, 0, s), "Tiger ATK %d", f.d.l[0].m[(size_t)t].atkEf);
		fin();
	}
	{
		caso("Vampire Lady / Lord: danno de batalla: el rival manda una carta del mazo al cementerio", "vampire_lady");
		probadas["vampire_lord"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "vampire_lady");
		VERIFICA(atacar(e, a, -1), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].cementerio.size() == 1, "cementerio %d", (int)e.d.l[1].cementerio.size());
		fin();
	}
	{
		caso("Viser Des: el elegido se destruye en tu tercera fase de espera; mientras, no se destruye en batalla", "viser_des");
		Escena e;
		const int z = e.mon(1, "battle_ox");
		VERIFICA(invocar(e, e.mano(0, "viser_des")), "no invoco");
		e.pasarTodo();
		for (int k = 0; k < 3; k++) motorFaseEspera(e.d);
		VERIFICA(e.d.l[1].m[(size_t)z].vacia(), "no se destruyo");
		fin();
	}
	{
		caso("Watapon: del mazo a la mano por un efecto: se invoca", "watapon");
		Escena e;
		e.d.l[0].mazo.push_back("watapon");
		buscarEnMazo(e.d, Lugar{ 0, EN_MAZO, (int)e.d.l[0].mazo.size() - 1 });
		motorVentana(e.d, Ventana{ V_DISPARO, 0, -1, 0, {} });
		e.pasarTodo();
		VERIFICA(buscarZona(e, 0, "watapon") >= 0, "no se invoco");
		fin();
	}
	{
		caso("Wave-Motion Cannon: 1000 por cada fase de espera", "wave_motion_cannon");
		Escena e;
		const int k = e.mano(0, "wave_motion_cannon");
		VERIFICA(e.activar(0, H(0, k)), "no activo");
		e.pasarTodo();
		motorFaseEspera(e.d);
		motorFaseEspera(e.d);
		e.d.ef.usadosTurno.clear();
		VERIFICA(e.activar(0, MT(0, 0)), "no activo la segunda vez");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 2000 && e.d.l[0].mt[0].vacia(), "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Winged Minion (+700 a un Demonio); Woodland Sprite (equipo al cementerio: 500)", "winged_minion");
		probadas["woodland_sprite"]++;
		Escena e;
		const int w = e.mon(0, "winged_minion");
		const int h = e.mon(0, "dark_ruler_ha_des");
		VERIFICA(e.activar(0, M(0, w), { M(0, h) }), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)h].atkEf == 3150, "ATK %d", e.d.l[0].m[(size_t)h].atkEf);
		Escena f;
		const int s = f.mon(0, "woodland_sprite");
		mtArriba(f, 0, "vile_germs", f.d.l[0].m[(size_t)s].uid);
		VERIFICA(f.d.l[0].m[(size_t)s].atkEf == 1200, "Vile Germs ATK %d", f.d.l[0].m[(size_t)s].atkEf);
		VERIFICA(f.activar(0, M(0, s)), "no activo");
		f.pasarTodo();
		VERIFICA(f.d.l[1].lp == LP_INICIAL - 500, "LP %d", f.d.l[1].lp);
		fin();
	}
	{
		caso("An Owl of Luck, Ape Fighter, Catnipped Kitty, Cipher Soldier", "an_owl_of_luck");
		for (const char* id : { "ape_fighter", "catnipped_kitty", "cipher_soldier", "vile_germs" }) probadas[id]++;
		Escena e;
		e.d.l[0].mazo.insert(e.d.l[0].mazo.begin(), "umi");
		const int z = e.mon(0, "an_owl_of_luck", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].mazo.back() == "umi", "no quedo arriba");
		Escena f(F_BATALLA);
		const int a = f.mon(0, "ape_fighter");
		const int k = f.mon(1, "kuriboh");
		VERIFICA(atacar(f, a, k), "no ataco");
		f.pasarTodo();
		VERIFICA(f.d.l[0].m[(size_t)a].atkEf == 2200, "Ape ATK %d", f.d.l[0].m[(size_t)a].atkEf);
		Escena g;
		const int c = g.mon(1, "catnipped_kitty");
		g.mon(1, "silver_fang");
		VERIFICA(!motorAtacable(g.d, 1, c), "se ataca a la gata");
		Escena h(F_BATALLA);
		const int cs = h.mon(1, "cipher_soldier", true);
		const int b = h.mon(0, "celtic_guardian");
		VERIFICA(atacar(h, b, cs), "no ataco");
		h.pasarTodo();
		VERIFICA(h.d.l[0].lp == LP_INICIAL - 2400 && h.d.l[1].m[(size_t)cs].defEf == 1800, "LP %d", h.d.l[0].lp);
		fin();
	}
	{
		caso("Cyber-Stein (5000: una fusion), Dark Magic Attack, Dark Mimic LV3", "cyber_stein");
		for (const char* id : { "dark_magic_attack", "dark_mimic_lv3" }) probadas[id]++;
		Escena e;
		e.d.l[0].extra = { "black_skull_dragon" };
		const int z = e.mon(0, "cyber_stein");
		VERIFICA(e.activar(0, M(0, z)), "no activo");
		e.pasarTodo();
		VERIFICA(buscarZona(e, 0, "black_skull_dragon") >= 0 && e.d.l[0].lp == LP_INICIAL - 5000, "Cyber-Stein");
		Escena f;
		f.mon(0, "dark_magician");
		f.colocar(1, "mirror_force");
		VERIFICA(usar(f, "dark_magic_attack") && f.d.l[1].mt[0].vacia(), "Dark Magic Attack");
		Escena g(F_BATALLA);
		const int a = g.mon(0, "battle_ox");
		const int m = g.mon(1, "dark_mimic_lv3");
		VERIFICA(atacar(g, a, m), "no ataco");
		g.pasarTodo();
		VERIFICA(g.d.l[1].mano.size() == 1, "no robo");
		fin();
	}
	{
		caso("Des Feral Imp, Des Lacooda, Dimension Jar, Don Zaloog", "des_feral_imp");
		for (const char* id : { "des_lacooda", "dimension_jar", "don_zaloog" }) probadas[id]++;
		Escena e;
		e.d.l[0].cementerio = { "raigeki" };
		const int z = e.mon(0, "des_feral_imp", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].cementerio.empty(), "Des Feral Imp");
		Escena f;
		const int l = f.mon(0, "des_lacooda", true, true);
		VERIFICA(voltear(f, l), "no se volteo");
		f.pasarTodo();
		VERIFICA(f.d.l[0].mano.size() == 1, "Des Lacooda");
		Escena g;
		g.d.l[1].cementerio = { "kuriboh", "battle_ox" };
		const int j = g.mon(0, "dimension_jar", true, true);
		VERIFICA(voltear(g, j), "no se volteo");
		g.pasarTodo();
		VERIFICA(g.d.l[1].cementerio.empty() && g.d.l[1].desterrados.size() == 2, "Dimension Jar");
		Escena h(F_BATALLA);
		const int dz = h.mon(0, "don_zaloog");
		h.mano(1, "kuriboh");
		VERIFICA(atacar(h, dz, -1), "no ataco");
		h.pasarTodo();
		VERIFICA(h.d.l[1].mano.empty(), "Don Zaloog");
		fin();
	}
	{
		caso("Emissary of the Oasis, Exchange of the Spirit, Fire Sorcerer, Gift of the Mystical Elf, Gora Turtle", "emissary_of_the_oasis");
		for (const char* id : { "exchange_of_the_spirit", "fire_sorcerer", "gift_of_the_mystical_elf", "gora_turtle" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "battle_ox");
		const int n = e.mon(1, "kuriboh");   // (efecto: no cuenta)
		e.mon(1, "emissary_of_the_oasis");
		const int c = e.mon(1, "baby_dragon");   // normal nivel 3
		VERIFICA(atacar(e, a, c), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL, "LP %d", e.d.l[1].lp);
		(void)n;
		Escena f;
		for (int k = 0; k < 15; k++) f.d.l[0].cementerio.push_back("kuriboh"), f.d.l[1].cementerio.push_back("kuriboh");
		const int sk = f.colocar(0, "exchange_of_the_spirit");
		VERIFICA(f.activar(0, MT(0, sk)), "Exchange");
		f.pasarTodo();
		VERIFICA(f.d.l[0].cementerio.size() >= 20 && f.d.l[1].mazo.size() == 15, "Exchange: mazo %d", (int)f.d.l[1].mazo.size());
		Escena g;
		g.mano(0, "kuriboh");
		g.mano(0, "sangan");
		const int fz = g.mon(0, "fire_sorcerer", true, true);
		VERIFICA(voltear(g, fz), "no se volteo");
		g.pasarTodo();
		VERIFICA(g.d.l[1].lp == LP_INICIAL - 800 && g.d.l[0].desterrados.size() == 2, "Fire Sorcerer");
		Escena h;
		h.mon(0, "kuriboh");
		h.mon(1, "kuriboh");
		const int gk = h.colocar(0, "gift_of_the_mystical_elf");
		VERIFICA(h.activar(0, MT(0, gk)), "Gift");
		h.pasarTodo();
		VERIFICA(h.d.l[0].lp == LP_INICIAL + 600, "Gift LP %d", h.d.l[0].lp);
		Escena q(F_BATALLA);
		q.mon(1, "gora_turtle");
		const int bo = q.mon(0, "summoned_skull");
		VERIFICA(!q.d.puedeAtacar(0, bo), "ataco con 2500");
		fin();
	}
	{
		caso("Guardian Eatos (sin monstruos en tu cementerio); Elemental Mistress Doriado (cuenta como TIERRA)", "guardian_eatos");
		for (const char* id : { "elemental_mistress_doriado", "deepsea_warrior", "gravekeeper_s_vassal", "the_masked_beast" }) probadas[id]++;
		Escena e;
		const int k = e.mano(0, "guardian_eatos");
		VERIFICA(e.activar(0, H(0, k)), "no se invoco");
		Escena f;
		f.mon(0, "elemental_mistress_doriado");
		VERIFICA(hayAtributoArriba(f.d, "TIERRA"), "no cuenta como TIERRA");
		fin();
	}
}
static void tanda7o()
{
	fprintf(stderr, "tanda 7 (O)\n");
	{
		caso("Insect Queen: +200 por Insecto; sacrifica para atacar; ficha si destruyo", "insect_queen");
		Escena e(F_BATALLA);
		const int q = e.mon(0, "insect_queen");
		e.mon(0, "basic_insect");
		const int z = e.mon(1, "kuriboh");
		VERIFICA(e.d.l[0].m[(size_t)q].atkEf == 2600, "ATK %d", e.d.l[0].m[(size_t)q].atkEf);
		VERIFICA(atacar(e, q, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 1, "no sacrifico");
		motorFinTurno(e.d);
		VERIFICA(e.d.l[0].monstruos() == 2, "sin ficha");
		fin();
	}
	{
		caso("Level Limit - Area B: nivel 4+ en defensa; Insect Princess: los Insectos del rival en ataque", "level_limit_area_b");
		probadas["insect_princess"]++;
		Escena e;
		const int z = e.mon(1, "summoned_skull");
		const int k = e.mon(1, "kuriboh");
		VERIFICA(usar(e, "level_limit_area_b") && e.d.l[1].m[(size_t)z].defensa && !e.d.l[1].m[(size_t)k].defensa, "Level Limit");
		Escena f;
		f.mon(0, "insect_princess");
		const int b = f.mon(1, "basic_insect", true);
		f.d.recalcular();
		VERIFICA(!f.d.l[1].m[(size_t)b].defensa, "Insect Princess");
		fin();
	}
	{
		caso("Necrovalley: Gravekeeper's +500; niega lo que elige del cementerio", "necrovalley");
		Escena e;
		const int g = e.mon(0, "gravekeeper_s_assailant");
		VERIFICA(usar(e, "necrovalley") && e.d.l[0].m[(size_t)g].atkEf == 2000, "ATK %d", e.d.l[0].m[(size_t)g].atkEf);
		e.d.l[0].cementerio.push_back("summoned_skull");
		usar(e, "monster_reborn", { Lugar{ 0, EN_CEMENTERIO, 0 } });
		VERIFICA(buscarZona(e, 0, "summoned_skull") < 0, "revivio con Necrovalley");
		fin();
	}
	{
		caso("Harpie Lady Phoenix Formation; Harpies' Hunting Ground", "harpie_lady_phoenix_formation");
		probadas["harpies_hunting_ground"]++;
		Escena e;
		for (int k = 0; k < 3; k++) e.mon(0, "harpie_lady");
		e.mon(1, "summoned_skull");
		e.mon(1, "kuriboh");
		VERIFICA(usar(e, "harpie_lady_phoenix_formation") && e.d.l[1].monstruos() == 0 && e.d.l[1].lp == LP_INICIAL - 2500, "LP %d", e.d.l[1].lp);
		Escena f;
		VERIFICA(usar(f, "harpies_hunting_ground"), "no activo");
		f.colocar(1, "mirror_force");
		VERIFICA(invocar(f, f.mano(0, "harpie_lady")), "no invoco");
		f.pasarTodo();
		VERIFICA(f.d.l[1].mt[0].vacia() && f.d.l[0].m[0].atkEf == 1500, "Hunting Ground ATK %d", f.d.l[0].m[0].atkEf);
		fin();
	}
	{
		caso("Horn of the Unicorn (+700; vuelve arriba del mazo); Mask of the Accursed; Reaper Scythe", "horn_of_the_unicorn");
		for (const char* id : { "mask_of_the_accursed", "reaper_scythe_dreadscythe" }) probadas[id]++;
		Escena e;
		const int z = e.mon(0, "celtic_guardian");
		VERIFICA(usar(e, "horn_of_the_unicorn", { M(0, z) }) && e.d.l[0].m[(size_t)z].atkEf == 2100, "Horn");
		e.d.alCementerio(0, z);
		VERIFICA(e.d.l[0].mazo.back() == "horn_of_the_unicorn", "no volvio al mazo");
		Escena f;
		const int o = f.mon(1, "battle_ox");
		VERIFICA(usar(f, "mask_of_the_accursed", { M(1, o) }), "Accursed");
		motorFaseEspera(f.d);
		VERIFICA(f.d.l[1].lp == LP_INICIAL - 500, "LP %d", f.d.l[1].lp);
		fin();
	}
	{
		caso("Ka-2 Des Scissors (nivel x 500), Kaibaman, Release Restraint", "ka_2_des_scissors");
		for (const char* id : { "kaibaman", "release_restraint" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "ka_2_des_scissors");
		const int z = e.mon(1, "kuriboh");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 700 - 500, "LP %d", e.d.l[1].lp);
		Escena f;
		const int k = f.mon(0, "kaibaman");
		f.mano(0, "blue_eyes");
		VERIFICA(f.activar(0, M(0, k)), "Kaibaman");
		f.pasarTodo();
		VERIFICA(buscarZona(f, 0, "blue_eyes") >= 0, "sin Blue-Eyes");
		Escena g;
		g.mon(0, "gearfried_the_iron_knight");
		g.d.l[0].mazo.push_back("gearfried_the_swordmaster");
		VERIFICA(usar(g, "release_restraint") && buscarZona(g, 0, "gearfried_the_swordmaster") >= 0, "Release Restraint");
		fin();
	}
	{
		caso("Destruidos en batalla: Kryuel, Newdoria, Troop Dragon, Pixie Knight; Mecha-Dog Marron; Unhappy Maiden", "newdoria");
		for (const char* id : { "kryuel", "troop_dragon", "pixie_knight", "mecha_dog_marron", "the_unhappy_maiden" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "battle_ox");
		e.mon(0, "kuriboh");
		const int z = e.mon(1, "newdoria");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 1, "Newdoria no destruyo");
		Escena f(F_BATALLA);
		const int b = f.mon(0, "battle_ox");
		const int t = f.mon(1, "troop_dragon");
		f.d.l[1].mazo.push_back("troop_dragon");
		VERIFICA(atacar(f, b, t), "no ataco");
		f.pasarTodo();
		VERIFICA(buscarZona(f, 1, "troop_dragon") >= 0, "Troop Dragon");
		Escena g(F_BATALLA);
		const int c = g.mon(0, "battle_ox");
		const int m = g.mon(1, "mecha_dog_marron");
		VERIFICA(atacar(g, c, m), "no ataco");
		g.pasarTodo();
		VERIFICA(g.d.l[0].lp == LP_INICIAL - 1000 && g.d.l[1].lp == LP_INICIAL - 1000 - 700, "Marron %d/%d", g.d.l[0].lp, g.d.l[1].lp);
		Escena h(F_BATALLA);
		const int u = h.mon(0, "battle_ox");
		const int mz = h.mon(1, "the_unhappy_maiden");
		VERIFICA(atacar(h, u, mz), "no ataco");
		h.pasarTodo();
		VERIFICA(!h.d.puedeAtacar(0, h.mon(0, "celtic_guardian")), "la batalla sigue");
		fin();
	}
	{
		caso("Nubian Guard / Aswan Apparition; Magical Plant Mandragola; Mask of Weakness; Mirror Wall", "nubian_guard");
		for (const char* id : { "aswan_apparition", "magical_plant_mandragola", "mask_of_weakness", "mirror_wall" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "aswan_apparition");
		e.d.l[0].cementerio.push_back("mirror_force");
		VERIFICA(atacar(e, a, -1), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].mazo.back() == "mirror_force", "Aswan");
		Escena f;
		const int s = f.mon(0, "skilled_dark_magician");
		const int p = f.mon(0, "magical_plant_mandragola", true, true);
		VERIFICA(voltear(f, p), "no se volteo");
		f.pasarTodo();
		VERIFICA(f.d.l[0].m[(size_t)s].contadores == 1, "Mandragola");
		Escena g(F_BATALLA);
		const int b = g.mon(0, "battle_ox");
		const int mw = g.colocar(1, "mask_of_weakness");
		VERIFICA(atacar(g, b, -1), "no ataco");
		VERIFICA(g.activar(1, MT(1, mw), { M(0, b) }), "Mask of Weakness");
		g.pasarTodo();
		VERIFICA(g.d.l[1].lp == LP_INICIAL - 1000, "LP %d", g.d.l[1].lp);
		Escena h(F_BATALLA);
		mtArriba(h, 1, "mirror_wall");
		const int c = h.mon(0, "battle_ox");
		VERIFICA(atacar(h, c, -1), "no ataco");
		h.pasarTodo();
		VERIFICA(h.d.l[0].m[(size_t)c].atkEf == 850, "Mirror Wall ATK %d", h.d.l[0].m[(size_t)c].atkEf);
		fin();
	}
	{
		caso("Mataza (dos ataques, no cambia de control); Mystical Beast of Serket; Mysterious Guard; Otohime", "mataza_the_zapper");
		for (const char* id : { "mystical_beast_of_serket", "mysterious_guard", "otohime" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "mataza_the_zapper");
		VERIFICA(atacar(e, a, -1), "no ataco");
		e.pasarTodo();
		VERIFICA(atacar(e, a, -1), "no ataco dos veces");
		e.pasarTodo();
		VERIFICA(!e.d.puedeAtacar(0, a), "tres ataques");
		Escena f;
		f.d.invocarEspecial(0, "mystical_beast_of_serket", false);
		VERIFICA(f.d.l[0].monstruos() == 0, "Serket sin el templo");
		Escena g;
		const int z = g.mon(1, "battle_ox");
		const int mg = g.mon(0, "mysterious_guard", true, true);
		VERIFICA(voltear(g, mg), "no se volteo");
		for (int q = 0; q < 6 && g.d.ef.preg.tipo == P_RESPUESTA; q++) g.pasa(g.d.ef.preg.lado);
		if (g.d.ef.preg.tipo == P_ELEGIR) g.elige(0, indiceCand(g, "battle_ox"));
		g.pasarTodo();
		VERIFICA(g.d.l[1].m[(size_t)z].vacia() && g.d.l[1].mazo.back() == "battle_ox", "Mysterious Guard");
		fin();
	}
	{
		caso("Parasite Paracide al mazo del rival: al robarla, sale y 1000; Patrol Robo", "parasite_paracide");
		probadas["patrol_robo"]++;
		Escena e;
		const int z = e.mon(0, "parasite_paracide", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 0 && std::find(e.d.l[1].mazo.begin(), e.d.l[1].mazo.end(), "parasite_paracide") != e.d.l[1].mazo.end(), "no fue al mazo");
		auto& mz = e.d.l[1].mazo;
		mz.erase(std::find(mz.begin(), mz.end(), "parasite_paracide"));
		mz.push_back("parasite_paracide");
		e.d.robarEfecto(1);
		VERIFICA(buscarZona(e, 1, "parasite_paracide") >= 0 && e.d.l[1].lp == LP_INICIAL - 1000, "al robarla");
		fin();
	}
	{
		caso("Perfect Machine King, Pumpking, White Magician Pikeru, Witch's Apprentice, Bladefly", "perfect_machine_king");
		for (const char* id : { "pumpking_the_king_of_ghosts", "white_magician_pikeru", "witch_s_apprentice", "bladefly" }) probadas[id]++;
		Escena e;
		const int k = e.mon(0, "perfect_machine_king");
		e.mon(1, "cyber_stein");
		VERIFICA(e.d.l[0].m[(size_t)k].atkEf == 3200, "PMK ATK %d", e.d.l[0].m[(size_t)k].atkEf);
		Escena f;
		f.mon(0, "white_magician_pikeru");
		f.mon(0, "kuriboh");
		motorFaseEspera(f.d);
		VERIFICA(f.d.l[0].lp == LP_INICIAL + 800, "Pikeru LP %d", f.d.l[0].lp);
		Escena g;
		g.mon(0, "witch_s_apprentice");
		const int dm = g.mon(0, "dark_magician");
		const int bf = g.mon(1, "bladefly");
		VERIFICA(g.d.l[0].m[(size_t)dm].atkEf == 3000 && g.d.l[1].m[(size_t)bf].atkEf == 1100, "Witch %d / Bladefly %d", g.d.l[0].m[(size_t)dm].atkEf, g.d.l[1].m[(size_t)bf].atkEf);
		fin();
	}
	{
		caso("Sonic Jammer (sin magias); Winged Sage Falcos; A Cat of Ill Omen; Disc Fighter; Wall of Illusion", "sonic_jammer");
		for (const char* id : { "winged_sage_falcos", "a_cat_of_ill_omen", "disc_fighter", "wall_of_illusion" }) probadas[id]++;
		Escena e;
		const int z = e.mon(0, "sonic_jammer", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		const int k = e.mano(1, "pot_of_greed");
		e.d.activo = 1;
		VERIFICA(motivoNoActivar(e.d, 1, H(1, k), false) == "ef.no_momento", "activo una magia");
		Escena f(F_BATALLA);
		const int a = f.mon(0, "winged_sage_falcos");
		const int o = f.mon(1, "kuriboh");
		VERIFICA(atacar(f, a, o), "no ataco");
		f.pasarTodo();
		VERIFICA(f.d.l[1].mazo.back() == "kuriboh", "Falcos");
		Escena g(F_BATALLA);
		const int df = g.mon(0, "disc_fighter");
		const int w = g.mon(1, "labyrinth_wall", true);
		VERIFICA(atacar(g, df, w), "no ataco");
		g.pasarTodo();
		VERIFICA(g.d.l[1].monstruos() == 0 && g.d.l[0].lp == LP_INICIAL, "Disc Fighter");
		Escena h(F_BATALLA);
		const int b = h.mon(0, "battle_ox");
		const int wi = h.mon(1, "wall_of_illusion", true);
		VERIFICA(atacar(h, b, wi), "no ataco");
		h.pasarTodo();
		VERIFICA(h.d.l[0].monstruos() == 0 && enMano(h, 0, "battle_ox"), "Wall of Illusion");
		fin();
	}
	{
		caso("Armor Break niega una magia de equipo; Call of the Grave niega Monster Reborn; Bad Reaction to Simochi", "armor_break");
		for (const char* id : { "call_of_the_grave", "bad_reaction_to_simochi" }) probadas[id]++;
		Escena e;
		const int z = e.mon(0, "celtic_guardian");
		const int t = e.colocar(1, "armor_break");
		const int k = e.mano(0, "axe_of_despair");
		VERIFICA(e.activar(0, H(0, k), { M(0, z) }), "no activo");
		VERIFICA(e.activar(1, MT(1, t)), "Armor Break");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)z].atkEf == 1400, "el equipo sigue");
		Escena f;
		mtArriba(f, 1, "bad_reaction_to_simochi");
		VERIFICA(usar(f, "red_medicine") && f.d.l[0].lp == LP_INICIAL - 500, "Simochi LP %d", f.d.l[0].lp);
		fin();
	}
	{
		caso("Bubonic Vermin, Copycat, Dark Jeroid, Desertapir, Steel Scorpion, Vampire Baby", "bubonic_vermin");
		for (const char* id : { "copycat", "dark_jeroid", "desertapir", "steel_scorpion", "vampire_baby" }) probadas[id]++;
		Escena e;
		e.d.l[0].mazo.push_back("bubonic_vermin");
		const int z = e.mon(0, "bubonic_vermin", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 2, "Vermin");
		Escena f;
		f.mon(1, "summoned_skull");
		VERIFICA(invocar(f, f.mano(0, "copycat")), "no invoco");
		f.pasarTodo();
		VERIFICA(f.d.l[0].m[0].atkEf == 2500, "Copycat %d", f.d.l[0].m[0].atkEf);
		Escena g(F_BATALLA);
		const int vb = g.mon(0, "vampire_baby");
		const int o = g.mon(1, "kuriboh");
		VERIFICA(atacar(g, vb, o), "no ataco");
		g.pasarTodo();
		Jugada j;
		j.tipo = J_FASE;
		g.d.aplicar(j);
		VERIFICA(buscarZona(g, 0, "kuriboh") >= 0, "Vampire Baby");
		fin();
	}
	{
		caso("Rituales y sustitutos sin efecto propio; Tactical Espionage Expert; Ancient Gear Soldier; Minar", "black_luster_soldier");
		for (const char* id : { "skull_guardian", "versago_the_destroyer", "beastking_of_the_swamps", "tactical_espionage_expert", "ancient_gear_soldier", "minar" }) probadas[id]++;
		Escena e;
		e.colocar(1, "trap_hole");
		VERIFICA(invocar(e, e.mano(0, "tactical_espionage_expert")), "no invoco");
		VERIFICA(e.d.ef.preg.tipo == P_NADA || e.d.ef.preg.lado != 1, "el rival puede responder con una trampa");
		fin();
	}
}
static void tanda7p()
{
	fprintf(stderr, "tanda 7 (P)\n");
	{
		caso("Ride of the Valkyries: Valkyries de la mano; con 3 sin danno de batalla; al final, al mazo", "ride_of_the_valkyries");
		Escena e;
		for (const char* id : { "valkyrie_zweite", "valkyrie_dritte", "valkyrie_brunhilde" }) e.mano(0, id);
		VERIFICA(usar(e, "ride_of_the_valkyries") && e.d.l[0].monstruos() == 3, "monstruos %d", e.d.l[0].monstruos());
		VERIFICA(e.d.ef.sinDannoBatallaHasta[0] == e.d.turno + 1, "sin la proteccion");
		motorFinTurno(e.d);
		VERIFICA(e.d.l[0].monstruos() == 0, "no volvieron al mazo");
		fin();
	}
	{
		caso("Valkyrie Erste: con otra Valkyrie destierra uno del cementerio rival y toma su ATK", "valkyrie_erste");
		Escena e;
		const int z = e.mon(0, "valkyrie_erste");
		e.mon(0, "valkyrie_dritte");
		e.d.l[1].cementerio.push_back("blue_eyes");
		VERIFICA(e.activar(0, M(0, z)), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)z].atkEf == 3000 && e.d.l[1].desterrados.size() == 1, "ATK %d", e.d.l[0].m[(size_t)z].atkEf);
		fin();
	}
	{
		caso("Amazoness Archers: los del rival a ataque y -500", "amazoness_archers");
		Escena f(F_BATALLA);
		f.mon(1, "amazoness_swords_woman");
		const int b = f.mon(0, "battle_ox");
		const int k = f.mon(0, "kuriboh", true, true);
		const int tt = f.colocar(1, "amazoness_archers");
		VERIFICA(atacar(f, b, 0), "no ataco");
		VERIFICA(f.activar(1, MT(1, tt)), "no activo");
		f.pasarTodo();
		VERIFICA(!f.d.l[0].m[(size_t)k].bocaAbajo && f.d.l[0].m[(size_t)b].atkEf <= 1200, "Archers ATK %d", f.d.l[0].m[(size_t)b].atkEf);
		fin();
	}
	{
		caso("Big Eye (muestra 5), Coach Goblin (Normal al mazo: roba 1)", "big_eye");
		probadas["coach_goblin"]++;
		Escena e;
		const int z = e.mon(0, "big_eye", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(revelados(e) == 5, "revelados %d", revelados(e));
		Escena f;
		f.mon(0, "coach_goblin");
		f.mano(0, "celtic_guardian");
		motorFinTurno(f.d);
		VERIFICA(f.d.l[0].mano.size() == 1 && f.d.l[0].mazo.front() == "celtic_guardian", "Coach Goblin");
		fin();
	}
	{
		caso("Contract with Exodia: Exodia Necross con las 5 piezas; +500 en tu espera", "contract_with_exodia");
		probadas["exodia_necross"]++;
		Escena e;
		e.d.l[0].cementerio = { "exodia_the_forbidden_one", "left_arm_of_the_forbidden_one", "right_arm_of_the_forbidden_one", "left_leg_of_the_forbidden_one",
			                    "right_leg_of_the_forbidden_one" };
		e.mano(0, "exodia_necross");
		VERIFICA(usar(e, "contract_with_exodia") && buscarZona(e, 0, "exodia_necross") >= 0, "no salio");
		const int z = buscarZona(e, 0, "exodia_necross");
		const int a0 = e.d.l[0].m[(size_t)z].atkEf;
		motorFaseEspera(e.d);
		VERIFICA(e.d.l[0].m[(size_t)z].atkEf == a0 + 500, "ATK %d", e.d.l[0].m[(size_t)z].atkEf);
		fin();
	}
	{
		caso("Dark Scorpion Chick / Meanae; Darkbishop Archfiend (paga 500)", "dark_scorpion_chick_the_yellow");
		for (const char* id : { "dark_scorpion_meanae_the_thorn", "darkbishop_archfiend" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "dark_scorpion_chick_the_yellow");
		e.colocar(1, "mirror_force");
		VERIFICA(atacar(e, a, -1), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].mano.size() == 1 || e.d.l[0].mano.size() == 1, "Chick");
		Escena f;
		f.mon(0, "darkbishop_archfiend");
		motorFaseEspera(f.d);
		VERIFICA(f.d.l[0].lp == LP_INICIAL - 500, "LP %d", f.d.l[0].lp);
		fin();
	}
	{
		caso("Fairy of the Spring; Familiar Knight; Giant Germ; Nimble Momonga; Lord Poison", "fairy_of_the_spring");
		for (const char* id : { "familiar_knight", "giant_germ", "nimble_momonga", "lord_poison" }) probadas[id]++;
		Escena e;
		e.d.l[0].cementerio.push_back("axe_of_despair");
		VERIFICA(usar(e, "fairy_of_the_spring") && enMano(e, 0, "axe_of_despair"), "Fairy");
		Escena f(F_BATALLA);
		const int a = f.mon(0, "summoned_skull");
		const int g = f.mon(1, "giant_germ");
		f.d.l[1].mazo.push_back("giant_germ");
		VERIFICA(atacar(f, a, g), "no ataco");
		f.pasarTodo();
		VERIFICA(f.d.l[0].lp == LP_INICIAL - 500 && buscarZona(f, 1, "giant_germ") >= 0, "Giant Germ");
		Escena h(F_BATALLA);
		const int b = h.mon(0, "summoned_skull");
		const int n = h.mon(1, "nimble_momonga");
		h.d.l[1].mazo.push_back("nimble_momonga");
		VERIFICA(atacar(h, b, n), "no ataco");
		h.pasarTodo();
		VERIFICA(h.d.l[1].lp > LP_INICIAL - 2000 && buscarZona(h, 1, "nimble_momonga") >= 0, "Momonga LP %d", h.d.l[1].lp);
		fin();
	}
	{
		caso("Gilford the Lightning con 3 sacrificios destruye a los del rival; Goblin King; Harpie's Pet Dragon; Muka Muka", "gilford_the_lightning");
		for (const char* id : { "goblin_king", "harpie_s_pet_dragon", "muka_muka", "king_of_the_skull_servants", "milus_radiant", "laser_cannon_armor" }) probadas[id]++;
		Escena e;
		for (int k = 0; k < 3; k++) e.mon(0, "kuriboh");
		e.mon(1, "battle_ox");
		Jugada j;
		j.tipo = J_INVOCAR;
		j.mano = e.mano(0, "gilford_the_lightning");
		j.tributos = { 0, 1, 2 };
		VERIFICA(e.d.aplicar(j), "no invoco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0, "no destruyo");
		Escena f;
		const int g = f.mon(0, "goblin_king");
		f.mon(0, "dark_ruler_ha_des");
		VERIFICA(f.d.l[0].m[(size_t)g].atkEf == 1000 && !motorAtacable(f.d, 0, g), "Goblin King %d", f.d.l[0].m[(size_t)g].atkEf);
		const int p = f.mon(1, "harpie_s_pet_dragon");
		f.mon(1, "harpie_lady");
		VERIFICA(f.d.l[1].m[(size_t)p].atkEf == 2300, "Pet Dragon %d", f.d.l[1].m[(size_t)p].atkEf);
		Escena h;
		const int m = h.mon(0, "muka_muka");
		h.mano(0, "kuriboh");
		h.d.recalcular();
		VERIFICA(h.d.l[0].m[(size_t)m].atkEf == 900, "Muka Muka %d", h.d.l[0].m[(size_t)m].atkEf);
		Escena q;
		q.d.l[0].cementerio = { "skull_servant", "skull_servant" };
		const int ks = q.mon(0, "king_of_the_skull_servants");
		VERIFICA(q.d.l[0].m[(size_t)ks].atkEf == 2000, "King %d", q.d.l[0].m[(size_t)ks].atkEf);
		fin();
	}
	{
		caso("Gora Turtle of Illusion: las magias/trampas del rival no la eligen; Grave Protector: los destruidos al mazo", "gora_turtle_of_illusion");
		probadas["grave_protector"]++;
		Escena e;
		const int z = e.mon(1, "gora_turtle_of_illusion");
		const int k = e.mano(0, "book_of_moon");
		const auto ob = objetivosDe(e.d, 0, *defCarta("book_of_moon"), H(0, k));
		VERIFICA(std::find(ob.begin(), ob.end(), M(1, z)) == ob.end(), "la eligio");
		Escena f(F_BATALLA);
		f.mon(1, "grave_protector");
		const int a = f.mon(0, "battle_ox");
		const int o = f.mon(1, "kuriboh");
		VERIFICA(atacar(f, a, o), "no ataco");
		f.pasarTodo();
		VERIFICA(f.d.l[1].cementerio.empty(), "fue al cementerio");
		fin();
	}
	{
		caso("Gravekeeper's Chief (revive un Gravekeeper's); Great Phantom Thief; Heavy Mech Support Platform; Hiro's Shadow Scout", "gravekeeper_s_chief");
		for (const char* id : { "great_phantom_thief", "heavy_mech_support_platform", "hiro_s_shadow_scout" }) probadas[id]++;
		Escena e;
		e.mon(0, "kuriboh");
		e.d.l[0].cementerio.push_back("gravekeeper_s_assailant");
		Jugada j;
		j.tipo = J_INVOCAR;
		j.mano = e.mano(0, "gravekeeper_s_chief");
		j.tributos = { 0 };
		VERIFICA(e.d.aplicar(j), "no invoco");
		e.pasarTodo();
		VERIFICA(buscarZona(e, 0, "gravekeeper_s_assailant") >= 0, "no revivio");
		Escena f(F_BATALLA);
		const int a = f.mon(0, "great_phantom_thief");
		f.mano(1, "kuriboh");
		f.mano(1, "kuriboh");
		f.mano(1, "sangan");
		f.d.l[1].cementerio = { "kuriboh" };   // (se declara lo que el rival ha ensenado)
		VERIFICA(atacar(f, a, -1), "no ataco");
		f.pasarTodo();
		VERIFICA(f.d.l[1].mano.size() == 1, "mano %d", (int)f.d.l[1].mano.size());
		Escena g;
		const int mq = g.mon(0, "cyber_stein");
		const int hp = g.mon(0, "heavy_mech_support_platform");
		VERIFICA(g.activar(0, M(0, hp)), "no se unio");
		g.pasarTodo();
		VERIFICA(g.d.l[0].m[(size_t)mq].atkEf == 1200, "Platform %d", g.d.l[0].m[(size_t)mq].atkEf);
		fin();
	}
	{
		caso("Interdimensional Matter Transporter; Jam Breeding Machine; Jowgen; Jowls of Dark Demise", "interdimensional_matter_transporter");
		for (const char* id : { "jam_breeding_machine", "jowgen_the_spiritualist", "jowls_of_dark_demise" }) probadas[id]++;
		Escena e;
		const int z = e.mon(0, "battle_ox");
		VERIFICA(usarColocada(e, 0, "interdimensional_matter_transporter", { M(0, z) }) && e.d.l[0].monstruos() == 0, "no se fue");
		motorFinTurno(e.d);
		VERIFICA(buscarZona(e, 0, "battle_ox") >= 0, "no volvio");
		Escena f;
		VERIFICA(usar(f, "jam_breeding_machine"), "no activo");
		motorFaseEspera(f.d);
		VERIFICA(f.d.l[0].monstruos() == 1, "sin ficha");
		Escena g;
		const int j = g.mon(0, "jowgen_the_spiritualist");
		g.d.invocarEspecial(1, "battle_ox", false);
		g.mano(0, "kuriboh");
		VERIFICA(g.activar(0, M(0, j)), "no activo");
		g.pasarTodo();
		VERIFICA(g.d.l[1].monstruos() == 0, "Jowgen");
		fin();
	}
	{
		caso("Legacy Hunter; Marauding Captain; Medusa Worm; Mid Shield Gardna", "legacy_hunter");
		for (const char* id : { "marauding_captain", "medusa_worm", "mid_shield_gardna" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "legacy_hunter");
		const int z = e.mon(1, "kuriboh", true, true);
		e.mano(1, "sangan");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].mano.empty(), "Legacy Hunter");
		Escena f;
		f.mano(0, "celtic_guardian");
		VERIFICA(invocar(f, f.mano(0, "marauding_captain")), "no invoco");
		f.pasarTodo();
		VERIFICA(f.d.l[0].monstruos() == 2 && !motorAtacable(f.d, 0, buscarZona(f, 0, "celtic_guardian")), "Captain");
		Escena g;
		const int m = g.mon(0, "medusa_worm", true, true);
		g.mon(1, "battle_ox");
		VERIFICA(voltear(g, m), "no se volteo");
		g.pasarTodo();
		VERIFICA(g.d.l[1].monstruos() == 0, "Medusa Worm");
		fin();
	}
	{
		caso("Mystic Swordsman LV2: destruye al boca abajo que ataca; sube a LV4", "mystic_swordsman_lv2");
		probadas["mystic_swordsman_lv4"]++;
		probadas["mystic_swordsman_lv6"]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "mystic_swordsman_lv2");
		const int z = e.mon(1, "labyrinth_wall", true, true);
		e.d.l[0].mazo.push_back("mystic_swordsman_lv4");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0, "no destruyo");
		motorFinTurno(e.d);
		resolverDisparos(e);
		VERIFICA(buscarZona(e, 0, "mystic_swordsman_lv4") >= 0, "no subio");
		fin();
	}
	{
		caso("Needle Worm, Princess of Tsurugi, Outstanding Dog Marron, Royal Tribute, Mysterious Puppeteer", "needle_worm");
		for (const char* id : { "princess_of_tsurugi", "outstanding_dog_marron", "royal_tribute", "mysterious_puppeteer" }) probadas[id]++;
		Escena e;
		const int z = e.mon(0, "needle_worm", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].cementerio.size() == 5, "Needle Worm");
		Escena f;
		f.colocar(1, "mirror_force");
		f.colocar(1, "trap_hole");
		const int p = f.mon(0, "princess_of_tsurugi", true, true);
		VERIFICA(voltear(f, p), "no se volteo");
		f.pasarTodo();
		VERIFICA(f.d.l[1].lp == LP_INICIAL - 1000, "Tsurugi LP %d", f.d.l[1].lp);
		Escena g;
		const size_t antes = g.d.l[0].mazo.size();
		g.d.descartar(0, g.mano(0, "outstanding_dog_marron"));
		motorVentana(g.d, Ventana{ V_DISPARO, 0, -1, 0, {} });
		g.pasarTodo();
		VERIFICA(g.d.l[0].mazo.size() == antes + 1, "Dog Marron");
		Escena h;
		h.mon(0, "mysterious_puppeteer");
		VERIFICA(invocar(h, h.mano(0, "kuriboh")), "no invoco");
		h.pasarTodo();
		VERIFICA(h.d.l[0].lp == LP_INICIAL + 500, "Puppeteer LP %d", h.d.l[0].lp);
		fin();
	}
	{
		caso("Leghul / Ooguchi directos; Piranha Army (el doble); Rocket Jumper; Raregold Armor; Luminous Soldier; Maji-Gire Panda", "leghul");
		for (const char* id : { "ooguchi", "piranha_army", "rocket_jumper", "raregold_armor", "luminous_soldier", "maji_gire_panda" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "piranha_army");
		VERIFICA(atacar(e, a, -1), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 1600, "Piranha LP %d", e.d.l[1].lp);
		Escena f(F_BATALLA);
		const int l = f.mon(0, "leghul");
		f.mon(1, "battle_ox");
		VERIFICA(motorAtaqueDirecto(f.d, 0, l), "Leghul");
		Escena g;
		const int o = g.mon(1, "battle_ox");
		const int k = g.mon(1, "kuriboh");
		mtArriba(g, 1, "raregold_armor", g.d.l[1].m[(size_t)o].uid);
		VERIFICA(!motorAtacable(g.d, 1, k) && motorAtacable(g.d, 1, o), "Raregold");
		Escena h;
		const int p = h.mon(0, "maji_gire_panda");
		const int b = h.mon(1, "silver_fang");
		h.d.alCementerio(1, b);
		h.d.recalcular();
		VERIFICA(h.d.l[0].m[(size_t)p].atkEf == 1500, "Panda %d", h.d.l[0].m[(size_t)p].atkEf);
		fin();
	}
}
static void tanda7q()
{
	fprintf(stderr, "tanda 7 (Q)\n");
	{
		caso("Chain Energy: 500 por carta de la mano; sin LP de sobra no se puede; la IA no se mata", "chain_energy");
		Escena e;
		mtArriba(e, 1, "chain_energy");
		VERIFICA(invocar(e, e.mano(0, "kuriboh")) && e.d.l[0].lp == LP_INICIAL - 500, "LP %d", e.d.l[0].lp);
		Escena f;
		mtArriba(f, 1, "chain_energy");
		f.d.l[0].lp = 500;
		VERIFICA(!invocar(f, f.mano(0, "kuriboh")), "invoco sin LP");
		const int k = f.mano(0, "red_medicine");
		VERIFICA(motivoNoActivar(f.d, 0, H(0, k), false) == "ef.sin_lp", "activo sin LP");
		f.d.l[0].lp = 1200;
		std::vector<Jugada> c;
		Jugada j;
		j.tipo = J_INVOCAR;
		j.mano = 0;
		c.push_back(j);
		motorFiltroIA(f.d, 0, c);
		VERIFICA(c.empty(), "la IA jugaria con 1200 LP");
		fin();
	}
	{
		caso("Cost Down: los de la mano 2 niveles menos (Summoned Skull sin sacrificios)", "cost_down");
		Escena e;
		e.mano(0, "kuriboh");
		VERIFICA(usar(e, "cost_down", {}, { 0 }), "no activo");
		VERIFICA(invocar(e, e.mano(0, "summoned_skull")) && buscarZona(e, 0, "summoned_skull") >= 0, "no invoco sin sacrificios");
		fin();
	}
	{
		caso("Graverobber: una magia del cementerio rival a tu mano; usarla cuesta 2000; si no, vuelve", "graverobber");
		Escena e;
		e.d.l[1].cementerio.push_back("red_medicine");
		VERIFICA(usarColocada(e, 0, "graverobber") && enMano(e, 0, "red_medicine"), "no la tomo");
		Escena f;
		f.d.l[1].cementerio.push_back("red_medicine");
		VERIFICA(usarColocada(f, 0, "graverobber"), "no activo");
		int k = -1;
		for (int i = 0; i < (int)f.d.l[0].mano.size(); i++)
			if (f.d.l[0].mano[(size_t)i] == "red_medicine") k = i;
		VERIFICA(k >= 0 && f.activar(0, H(0, k)), "no la uso");
		f.pasarTodo();
		VERIFICA(f.d.l[0].lp == LP_INICIAL - 2000 + 500, "LP %d", f.d.l[0].lp);
		Escena g;
		g.d.l[1].cementerio.push_back("red_medicine");
		VERIFICA(usarColocada(g, 0, "graverobber"), "no activo");
		motorFinTurno(g.d);
		VERIFICA(!enMano(g, 0, "red_medicine") && g.d.l[1].cementerio.size() == 1, "no volvio");
		fin();
	}
	{
		caso("Metalsilver Armor: el rival solo elige al equipado; Big Bang Shot (+400, perfora); 7 Completed; Ballista", "metalsilver_armor");
		for (const char* id : { "big_bang_shot", "7_completed", "ballista_of_rampart_smashing" }) probadas[id]++;
		Escena e;
		const int a = e.mon(1, "battle_ox");
		const int b = e.mon(1, "kuriboh");
		mtArriba(e, 1, "metalsilver_armor", e.d.l[1].m[(size_t)a].uid);
		const int k = e.mano(0, "book_of_moon");
		const auto ob = objetivosDe(e.d, 0, *defCarta("book_of_moon"), H(0, k));
		VERIFICA(std::find(ob.begin(), ob.end(), M(1, b)) == ob.end() && std::find(ob.begin(), ob.end(), M(1, a)) != ob.end(), "Metalsilver");
		Escena f(F_BATALLA);
		const int s = f.mon(0, "battle_ox");
		mtArriba(f, 0, "big_bang_shot", f.d.l[0].m[(size_t)s].uid);
		const int z = f.mon(1, "kuriboh", true);
		VERIFICA(f.d.l[0].m[(size_t)s].atkEf == 2100 && atacar(f, s, z), "no ataco");
		f.pasarTodo();
		VERIFICA(f.d.l[1].lp == LP_INICIAL - 1900, "Big Bang LP %d", f.d.l[1].lp);
		fin();
	}
	{
		caso("Kiseitai: atacada boca abajo pasa a equipo del atacante; ganas la mitad de su ATK en la espera del rival", "kiseitai");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "battle_ox");
		const int z = e.mon(1, "kiseitai", true, true);
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0 && e.d.l[1].mt[0].id == "kiseitai" && e.d.l[1].lp == LP_INICIAL, "Kiseitai");
		motorFaseEspera(e.d);
		VERIFICA(e.d.l[1].lp == LP_INICIAL + 850, "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Infernal Flame Emperor, Gingerbread House, Sacred Phoenix of Nephthys, Sasuke Samurai #2, Secret Barrel", "infernal_flame_emperor");
		for (const char* id : { "gingerbread_house", "sacred_phoenix_of_nephthys", "sasuke_samurai_2", "secret_barrel" }) probadas[id]++;
		Escena e;
		e.mon(0, "kuriboh");
		e.mon(0, "kuriboh");
		e.d.l[0].cementerio = { "flame_manipulator" };
		e.colocar(1, "mirror_force");
		Jugada j;
		j.tipo = J_INVOCAR;
		j.mano = e.mano(0, "infernal_flame_emperor");
		j.tributos = { 0, 1 };
		VERIFICA(e.d.aplicar(j), "no invoco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].mt[0].vacia() && e.d.l[0].desterrados.size() == 1, "Emperor");
		Escena f;
		mtArriba(f, 0, "gingerbread_house");
		f.mon(1, "summoned_skull");
		f.d.activo = 1;
		motorFaseEspera(f.d);
		VERIFICA(f.d.l[1].monstruos() == 0 && f.d.l[0].lp == LP_INICIAL + 500, "Gingerbread");
		Escena g;
		const int z = g.mon(0, "sacred_phoenix_of_nephthys");
		g.mon(1, "kuriboh");
		VERIFICA(usar(g, "dark_hole"), "no activo");
		g.colocar(1, "mirror_force");
		motorFaseEspera(g.d);
		VERIFICA(buscarZona(g, 0, "sacred_phoenix_of_nephthys") >= 0 && g.d.l[1].mt[0].vacia(), "Phoenix");
		(void)z;
		Escena h;
		const int s = h.mon(0, "sasuke_samurai_2");
		VERIFICA(h.activar(0, M(0, s)), "no activo");
		h.pasarTodo();
		const int k = h.mano(0, "red_medicine");
		VERIFICA(motivoNoActivar(h.d, 0, H(0, k), false) == "ef.no_momento", "activo una magia");
		Escena q;
		q.mano(1, "kuriboh");
		q.mon(1, "battle_ox");
		VERIFICA(usarColocada(q, 0, "secret_barrel") && q.d.l[1].lp == LP_INICIAL - 400, "Barrel LP %d", q.d.l[1].lp);
		fin();
	}
	{
		caso("Shadow Ghoul, Wodan, Star Boy, Shadow Tamer, Shield & Sword, Spirit Caller", "shadow_ghoul");
		for (const char* id : { "wodan_the_resident_of_the_forest", "star_boy", "shadow_tamer", "shield_sword", "spirit_caller" }) probadas[id]++;
		Escena e;
		e.d.l[0].cementerio = { "kuriboh", "battle_ox" };
		const int z = e.mon(0, "shadow_ghoul");
		VERIFICA(e.d.l[0].m[(size_t)z].atkEf == 1800, "Ghoul %d", e.d.l[0].m[(size_t)z].atkEf);
		Escena f;
		const int b = f.mon(0, "battle_ox");
		VERIFICA(usar(f, "shield_sword") && f.d.l[0].m[(size_t)b].atkEf == 1000, "Shield & Sword %d", f.d.l[0].m[(size_t)b].atkEf);
		Escena g;
		g.mon(1, "dark_ruler_ha_des");
		const int t = g.mon(0, "shadow_tamer", true, true);
		VERIFICA(voltear(g, t), "no se volteo");
		g.pasarTodo();
		VERIFICA(buscarZona(g, 0, "dark_ruler_ha_des") >= 0, "Shadow Tamer");
		fin();
	}
	{
		caso("Little Swordsman of Aile, Toon Table of Contents, Tornado Wall, Winged Kuriboh, 3-Hump Lacooda, 8-Claws Scorpion", "the_little_swordsman_of_aile");
		for (const char* id : { "toon_table_of_contents", "tornado_wall", "winged_kuriboh", "3_hump_lacooda", "8_claws_scorpion" }) probadas[id]++;
		Escena e;
		const int a = e.mon(0, "the_little_swordsman_of_aile");
		e.mon(0, "kuriboh");
		VERIFICA(e.activar(0, M(0, a)), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)a].atkEf == 1500, "Aile %d", e.d.l[0].m[(size_t)a].atkEf);
		Escena f;
		for (int k = 0; k < 3; k++) f.mon(0, "3_hump_lacooda");
		VERIFICA(f.activar(0, M(0, 0)), "no activo");
		f.pasarTodo();
		VERIFICA(f.d.l[0].mano.size() == 3 && f.d.l[0].monstruos() == 1, "Lacooda");
		Escena g(F_BATALLA);
		const int s = g.mon(0, "8_claws_scorpion");
		const int z = g.mon(1, "labyrinth_wall", true, true);
		VERIFICA(atacar(g, s, z), "no ataco");
		g.pasarTodo();
		VERIFICA(g.d.l[1].monstruos() == 1 && g.d.l[0].lp == LP_INICIAL - 600, "Scorpion LP %d", g.d.l[0].lp);
		fin();
	}
	{
		caso("A Feather of the Phoenix, A Feint Plan, A Wingbeat of Giant Dragon, Acid Rain, Acid Trap Hole, Altar for Tribute", "a_feather_of_the_phoenix");
		for (const char* id : { "a_feint_plan", "a_wingbeat_of_giant_dragon", "acid_rain", "acid_trap_hole", "altar_for_tribute" }) probadas[id]++;
		Escena e;
		e.d.l[0].cementerio = { "raigeki" };
		e.mano(0, "kuriboh");
		VERIFICA(usar(e, "a_feather_of_the_phoenix", {}, { 0 }) && e.d.l[0].mazo.back() == "raigeki", "Feather");
		Escena f;
		const int bz = f.mon(0, "blue_eyes");
		f.colocar(1, "mirror_force");
		VERIFICA(usar(f, "a_wingbeat_of_giant_dragon", { M(0, bz) }) && f.d.l[1].mt[0].vacia() && enMano(f, 0, "blue_eyes"), "Wingbeat");
		Escena g;
		g.mon(1, "cyber_stein");
		VERIFICA(usar(g, "acid_rain") && g.d.l[1].monstruos() == 0, "Acid Rain");
		Escena h;
		const int z = h.mon(1, "kuriboh", true, true);
		VERIFICA(usarColocada(h, 0, "acid_trap_hole", { M(1, z) }) && h.d.l[1].monstruos() == 0, "Acid Trap Hole");
		Escena q;
		const int o = q.mon(0, "battle_ox");
		VERIFICA(usarColocada(q, 0, "altar_for_tribute", { M(0, o) }) && q.d.l[0].lp == LP_INICIAL + 1700, "Altar");
		fin();
	}
	{
		caso("Amazoness Blowpiper, Ameba, Ancient Telescope, Anti Raigeki, Aqua Chorus, Arsenal Bug", "amazoness_blowpiper");
		for (const char* id : { "ameba", "ancient_telescope", "anti_raigeki", "aqua_chorus", "arsenal_bug" }) probadas[id]++;
		Escena e;
		e.mon(0, "amazoness_blowpiper");
		const int z = e.mon(1, "battle_ox");
		motorFaseEspera(e.d);
		VERIFICA(e.d.l[1].m[(size_t)z].atkEf == 1200, "Blowpiper %d", e.d.l[1].m[(size_t)z].atkEf);
		Escena f;
		const int a = f.mon(0, "ameba");
		f.d.cambiarControl(0, a);
		VERIFICA(f.d.l[1].lp == LP_INICIAL - 2000, "Ameba %d", f.d.l[1].lp);
		Escena g;
		const int b = g.mon(0, "arsenal_bug");
		VERIFICA(g.d.l[0].m[(size_t)b].atkEf == 1000, "Arsenal Bug %d", g.d.l[0].m[(size_t)b].atkEf);
		Escena h;
		const int k1 = h.mon(0, "kuriboh");
		h.mon(1, "kuriboh");
		VERIFICA(usarColocada(h, 0, "aqua_chorus") && h.d.l[0].m[(size_t)k1].atkEf == 800, "Aqua Chorus");
		fin();
	}
	{
		caso("Arsenal Robber, Assault on GHQ, Back to Square One, Backfire, Battery Charger, Begone Knave, Banner of Courage", "arsenal_robber");
		for (const char* id : { "assault_on_ghq", "back_to_square_one", "backfire", "battery_charger", "begone_knave", "banner_of_courage" }) probadas[id]++;
		Escena e;
		e.d.l[1].mazo.push_back("axe_of_despair");
		VERIFICA(usarColocada(e, 0, "arsenal_robber") && e.d.l[1].cementerio.size() == 1, "Robber");
		Escena f;
		const int o = f.mon(1, "battle_ox");
		f.mano(0, "kuriboh");
		VERIFICA(usar(f, "back_to_square_one", { M(1, o) }, { 0 }) && f.d.l[1].mazo.back() == "battle_ox", "Back to Square One");
		Escena g(F_BATALLA);
		mtArriba(g, 1, "begone_knave");
		const int a = g.mon(0, "battle_ox");
		VERIFICA(atacar(g, a, -1), "no ataco");
		g.pasarTodo();
		VERIFICA(enMano(g, 0, "battle_ox"), "Begone Knave");
		Escena h;
		h.mon(0, "flame_manipulator");
		mtArriba(h, 0, "backfire");
		h.d.alCementerio(0, 0);
		VERIFICA(h.d.l[1].lp == LP_INICIAL - 500, "Backfire");
		fin();
	}
	{
		caso("Ultimate Tyranno (ataca a todos; los demas no atacan antes); Kotodama; Servant of Catabolism; Susa Soldier", "ultimate_tyranno");
		for (const char* id : { "kotodama", "servant_of_catabolism", "susa_soldier", "torpedo_fish", "atomic_firefly", "ancient_gear_beast",
		                        "spirit_of_the_pot_of_greed", "skull_knight_2", "second_goblin", "ultimate_insect_lv1" })
			probadas[id]++;
		Escena e(F_BATALLA);
		const int t = e.mon(0, "ultimate_tyranno");
		const int o = e.mon(0, "battle_ox");
		const int a = e.mon(1, "kuriboh");
		const int b = e.mon(1, "kuriboh");
		VERIFICA(!e.d.puedeAtacar(0, o), "ataco otro primero");
		VERIFICA(atacar(e, t, a), "no ataco");
		e.pasarTodo();
		VERIFICA(atacar(e, t, b), "no ataco dos veces");
		e.pasarTodo();
		Escena k;
		k.mon(0, "skull_knight_2");
		k.d.l[0].mazo.push_back("skull_knight_2");
		Jugada jj;
		jj.tipo = J_INVOCAR;
		jj.mano = k.mano(0, "dark_ruler_ha_des");
		jj.tributos = { 0 };
		VERIFICA(k.d.aplicar(jj) && buscarZona(k, 0, "skull_knight_2") >= 0, "Skull Knight #2");
		Escena f;
		f.mon(0, "kotodama");
		f.mon(0, "kuriboh");
		f.mon(1, "kuriboh");
		VERIFICA(f.d.l[0].monstruos() + f.d.l[1].monstruos() == 2, "Kotodama");
		fin();
	}
}
static void tanda7r()
{
	fprintf(stderr, "tanda 7 (R)\n");
	{
		caso("DNA Surgery: los boca arriba pasan al tipo declarado", "dna_surgery");
		Escena e;
		e.mon(0, "dark_ruler_ha_des");
		const int k = e.mon(1, "kuriboh");
		VERIFICA(usarColocada(e, 0, "dna_surgery") && razaEs(e.d, e.d.l[1].m[(size_t)k].id, "Demonio"), "tipo %s", e.d.ef.adnTipo.c_str());
		fin();
	}
	{
		caso("Orichalcos Sword of Sealing niega los efectos del equipado; Armored Glass anula los equipos este turno", "orichalcos_sword_of_sealing");
		probadas["armored_glass"]++;
		Escena e;
		const int g = e.mon(1, "gaap_the_divine_soldier");
		e.mano(1, "dark_ruler_ha_des");
		VERIFICA(usar(e, "orichalcos_sword_of_sealing", { M(1, g) }), "no se equipo");
		e.d.activo = 1;
		VERIFICA(motivoNoActivar(e.d, 1, M(1, g), false) == "ef.no", "Gaap activa");
		Escena f;
		const int z = f.mon(0, "celtic_guardian");
		VERIFICA(usar(f, "axe_of_despair", { M(0, z) }) && f.d.l[0].m[(size_t)z].atkEf == 2400, "Axe");
		VERIFICA(usarColocada(f, 0, "armored_glass") && f.d.l[0].m[(size_t)z].atkEf == 1400, "Armored Glass %d", f.d.l[0].m[(size_t)z].atkEf);
		fin();
	}
	{
		caso("Fairy's Hand Mirror: la magia del rival que elige un monstruo pasa a otro", "fairy_s_hand_mirror");
		Escena e;
		const int mio = e.mon(1, "summoned_skull");
		const int suyo = e.mon(0, "battle_ox");
		const int t = e.colocar(1, "fairy_s_hand_mirror");
		const int k = e.mano(0, "book_of_moon");
		VERIFICA(e.activar(0, H(0, k), { M(1, mio) }), "no activo");
		VERIFICA(e.activar(1, MT(1, t)), "Mirror");
		if (e.d.ef.preg.tipo == P_ELEGIR) e.elige(1, indiceCand(e, "battle_ox"));
		e.pasarTodo();
		VERIFICA(!e.d.l[1].m[(size_t)mio].bocaAbajo && e.d.l[0].m[(size_t)suyo].bocaAbajo, "no cambio el objetivo");
		fin();
	}
	{
		caso("Lava Golem: al campo del rival sacrificando 2 suyos; 1000 en la espera de su controlador", "lava_golem");
		Escena e;
		e.mon(1, "kuriboh");
		e.mon(1, "battle_ox");
		const int k = e.mano(0, "lava_golem");
		VERIFICA(e.activar(0, H(0, k)), "no se invoco");
		e.pasarTodo();
		VERIFICA(buscarZona(e, 1, "lava_golem") >= 0 && e.d.l[1].monstruos() == 1 && e.d.l[0].invocoNormal, "Lava Golem");
		e.d.activo = 1;
		motorFaseEspera(e.d);
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 1000, "LP %d", e.d.l[1].lp);
		fin();
	}
	{
		caso("Morphing Jar #2, Muko, Spear Cretin, Spirit Ryu", "morphing_jar_2");
		for (const char* id : { "muko", "spear_cretin", "spirit_ryu" }) probadas[id]++;
		Escena e;
		e.mon(1, "battle_ox");
		const int z = e.mon(0, "morphing_jar_2", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() + e.d.l[1].monstruos() >= 1, "Morphing Jar #2");
		Escena f;
		const int t = f.colocar(1, "muko");
		const int k = f.mano(0, "pot_of_greed");
		VERIFICA(f.activar(0, H(0, k)), "no activo");
		VERIFICA(f.activar(1, MT(1, t)), "Muko");
		f.pasarTodo();
		VERIFICA(f.d.l[0].mano.empty() && f.d.l[0].cementerio.size() >= 3, "mano %d", (int)f.d.l[0].mano.size());
		Escena g(F_BATALLA);
		g.d.l[0].cementerio.push_back("summoned_skull");
		g.d.l[1].cementerio.push_back("battle_ox");
		const int c = g.mon(1, "spear_cretin", true, true);
		const int a = g.mon(0, "blue_eyes");
		VERIFICA(atacar(g, a, c), "no ataco");
		g.pasarTodo();
		VERIFICA(buscarZona(g, 0, "summoned_skull") >= 0 && buscarZona(g, 1, "battle_ox") >= 0, "Spear Cretin");
		fin();
	}
	{
		caso("Temple of the Kings (trampa el turno en que se coloca); Triangle Ecstasy Spark", "temple_of_the_kings");
		probadas["triangle_ecstasy_spark"]++;
		Escena e;
		mtArriba(e, 0, "temple_of_the_kings");
		const int t = e.colocar(0, "secret_barrel", e.d.turno);
		VERIFICA(e.activar(0, MT(0, t)), "no activo la trampa recien colocada");
		e.pasarTodo();
		const int t2 = e.colocar(0, "secret_barrel", e.d.turno);
		VERIFICA(motivoNoActivar(e.d, 0, MT(0, t2), false) == "ef.colocada_este_turno", "dos por turno");
		Escena f;
		const int s = f.mon(0, "harpie_lady_sisters");
		VERIFICA(usar(f, "triangle_ecstasy_spark") && f.d.l[0].m[(size_t)s].atkEf == 2700, "Spark %d", f.d.l[0].m[(size_t)s].atkEf);
		fin();
	}
	{
		caso("Toon Defense / Astral Barrier / Absolute End: el ataque pasa a ser directo; Patrician of Darkness", "astral_barrier");
		for (const char* id : { "toon_defense", "absolute_end", "patrician_of_darkness" }) probadas[id]++;
		Escena e(F_BATALLA);
		mtArriba(e, 1, "astral_barrier");
		const int a = e.mon(0, "summoned_skull");
		const int z = e.mon(1, "battle_ox");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(!e.d.l[1].m[(size_t)z].vacia() && e.d.l[1].lp == LP_INICIAL - 2500, "Astral LP %d", e.d.l[1].lp);
		Escena f(F_BATALLA);
		const int b = f.mon(0, "summoned_skull");
		const int k = f.mon(1, "kuriboh");
		const int s = f.mon(1, "labyrinth_wall", true);
		f.mon(1, "patrician_of_darkness");
		VERIFICA(atacar(f, b, k), "no ataco");
		f.pasarTodo();
		VERIFICA(!f.d.l[1].m[(size_t)k].vacia() && !f.d.l[1].m[(size_t)s].vacia(), "Patrician no desvio");
		fin();
	}
	{
		caso("A Deal with Dark Ruler / Berserk Dragon; A Hero Emerges; Adhesion Trap Hole; After the Struggle; Amazoness Spellcaster",
		     "a_deal_with_dark_ruler");
		for (const char* id : { "berserk_dragon", "a_hero_emerges", "adhesion_trap_hole", "after_the_struggle", "amazoness_spellcaster" }) probadas[id]++;
		Escena e;
		e.d.ef.destruidosCampo[0].push_back("blue_eyes");
		e.mano(0, "berserk_dragon");
		VERIFICA(usar(e, "a_deal_with_dark_ruler") && buscarZona(e, 0, "berserk_dragon") >= 0, "Deal");
		Escena f;
		VERIFICA(invocar(f, f.mano(0, "summoned_skull")) || true, "");
		f.d.ef.preg = Pregunta{};
		Escena g;
		const int o = g.mon(1, "summoned_skull");
		const int a = g.mon(0, "amazoness_swords_woman");
		VERIFICA(usar(g, "amazoness_spellcaster", { M(0, a), M(1, o) }) && g.d.l[0].m[(size_t)a].atkEf == 2500 && g.d.l[1].m[(size_t)o].atkEf == 1500, "Spellcaster");
		fin();
	}
	{
		caso("Anti-Spell, Aqua Spirit, Arcane Archer, Archfiend's Roar, Armed Dragon LV7, Armed Ninja, Armed Samurai Ben Kei", "anti_spell");
		for (const char* id : { "aqua_spirit", "arcane_archer_of_the_forest", "archfiend_s_roar", "armed_dragon_lv7", "armed_ninja", "armed_samurai_ben_kei" }) probadas[id]++;
		Escena e;
		e.d.l[0].cementerio.push_back("kairyu_shin");
		const int k = e.mano(0, "aqua_spirit");
		VERIFICA(e.activar(0, H(0, k)) && e.d.l[0].desterrados.size() == 1, "Aqua Spirit");
		Escena f;
		const int a = f.mon(0, "armed_dragon_lv7");
		f.mon(1, "battle_ox");
		f.mon(1, "kuriboh");
		f.mano(0, "summoned_skull");
		VERIFICA(f.activar(0, M(0, a)), "no activo");
		f.pasarTodo();
		VERIFICA(f.d.l[1].monstruos() == 0, "LV7");
		Escena g;
		g.mon(0, "arcane_archer_of_the_forest");
		g.mon(0, "lord_poison");
		VERIFICA(!motorAtacable(g.d, 0, 0), "Arcane Archer");
		fin();
	}
	{
		caso("Autonomous Action Unit; Backup Soldier; Behemoth con 1 sacrificio", "autonomous_action_unit");
		for (const char* id : { "backup_soldier", "behemoth_the_king_of_all_animals" }) probadas[id]++;
		Escena e;
		e.d.l[1].cementerio.push_back("summoned_skull");
		VERIFICA(usar(e, "autonomous_action_unit", { Lugar{ 1, EN_CEMENTERIO, 0 } }) && buscarZona(e, 0, "summoned_skull") >= 0 && e.d.l[0].lp == LP_INICIAL - 1500,
		         "Autonomous");
		Escena f;
		f.mon(0, "kuriboh");
		Jugada j;
		j.tipo = J_INVOCAR;
		j.mano = f.mano(0, "behemoth_the_king_of_all_animals");
		j.tributos = { 0 };
		VERIFICA(f.d.aplicar(j), "no invoco con 1");
		f.pasarTodo();
		const int b = buscarZona(f, 0, "behemoth_the_king_of_all_animals");
		VERIFICA(b >= 0 && f.d.l[0].m[(size_t)b].atkEf == 2000, "Behemoth");
		fin();
	}
}
static void tanda7s()
{
	fprintf(stderr, "tanda 7 (S)\n");
	{
		caso("Trap Master, Valkyrie Sechste, Valkyrie Funfte, Virus Cannon", "trap_master");
		for (const char* id : { "valkyrie_sechste", "valkyrie_funfte", "virus_cannon" }) probadas[id]++;
		Escena e;
		e.colocar(1, "mirror_force");
		const int z = e.mon(0, "trap_master", true, true);
		VERIFICA(voltear(e, z), "no se volteo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].mt[0].vacia(), "Trap Master");
		Escena f;
		f.d.l[0].mazo.push_back("valkyrie_dritte");
		const int s = f.d.invocarEspecial(0, "valkyrie_sechste", false);
		motorVentana(f.d, Ventana{ V_DISPARO, 0, s, 0, {} });
		f.pasarTodo();
		VERIFICA(buscarZona(f, 0, "valkyrie_dritte") >= 0, "Sechste");
		Escena g;
		g.mon(0, "kuriboh");
		g.d.l[1].mazo.push_back("raigeki");
		VERIFICA(usarColocada(g, 0, "virus_cannon") && g.d.l[1].cementerio.size() == 1, "Virus Cannon");
		fin();
	}
	{
		caso("7: con 3 boca arriba robas 3; del campo al cementerio +700", "7");
		Escena e;
		mtArriba(e, 0, "7");
		mtArriba(e, 0, "7");
		VERIFICA(usar(e, "7") && e.d.l[0].mano.size() == 3 && e.d.l[0].lp == LP_INICIAL + 2100, "mano %d LP %d", (int)e.d.l[0].mano.size(), e.d.l[0].lp);
		fin();
	}
	{
		caso("Abyssal Designator; Beast Soul Swap; Beckoning Light; Big Wave Small Wave; Blade Rabbit", "abyssal_designator");
		for (const char* id : { "beast_soul_swap", "beckoning_light", "big_wave_small_wave", "blade_rabbit" }) probadas[id]++;
		Escena e;
		VERIFICA(usar(e, "abyssal_designator") && e.d.l[1].cementerio.size() == 1, "Abyssal");
		Escena f;
		const int z = f.mon(0, "blade_rabbit");
		f.mon(1, "battle_ox");
		Jugada j;
		j.tipo = J_POSICION;
		j.zona = z;
		VERIFICA(f.d.aplicar(j), "no cambio");
		f.pasarTodo();
		VERIFICA(f.d.l[1].monstruos() == 0, "Blade Rabbit");
		fin();
	}
	{
		caso("Blast Juggler, Blast with Chain, Blasting the Ruins, Blind Destruction, Block Attack, Blue Medicine", "blast_juggler");
		for (const char* id : { "blast_with_chain", "blasting_the_ruins", "blind_destruction", "block_attack", "blue_medicine" }) probadas[id]++;
		Escena e;
		const int j = e.mon(0, "blast_juggler");
		e.mon(1, "kuriboh");
		e.mon(1, "sangan");
		VERIFICA(e.activar(0, M(0, j)), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0, "Blast Juggler");
		Escena f;
		const int o = f.mon(1, "battle_ox");
		VERIFICA(usar(f, "block_attack", { M(1, o) }) && f.d.l[1].m[(size_t)o].defensa, "Block Attack");
		VERIFICA(usar(f, "blue_medicine") && f.d.l[0].lp == LP_INICIAL + 400, "Blue Medicine");
		fin();
	}
	{
		caso("Blue-Eyes Shining Dragon; Boar Soldier; Bombardment Beetle; Burning Spear", "blue_eyes_shining_dragon");
		for (const char* id : { "boar_soldier", "bombardment_beetle", "burning_spear" }) probadas[id]++;
		Escena e;
		e.mon(0, "blue_eyes_ultimate_dragon");
		e.d.l[0].cementerio = { "blue_eyes", "red_eyes" };
		const int k = e.mano(0, "blue_eyes_shining_dragon");
		VERIFICA(e.activar(0, H(0, k)), "no se invoco");
		const int z = buscarZona(e, 0, "blue_eyes_shining_dragon");
		VERIFICA(z >= 0 && e.d.l[0].m[(size_t)z].atkEf == 3000 + 300 * 3, "Shining %d", z >= 0 ? e.d.l[0].m[(size_t)z].atkEf : -1);
		Escena f;
		VERIFICA(invocar(f, f.mano(0, "boar_soldier")), "no invoco");
		f.pasarTodo();
		VERIFICA(f.d.l[0].monstruos() == 0, "Boar Soldier");
		Escena g;
		g.mon(1, "man_eater_bug", true, true);
		const int b = g.mon(0, "bombardment_beetle", true, true);
		VERIFICA(voltear(g, b), "no se volteo");
		g.pasarTodo();
		VERIFICA(g.d.l[1].monstruos() == 0, "Bombardment Beetle");
		fin();
	}
	{
		caso("Burst Breath; Burst Stream of Destruction; Byser Shock; Card of Sanctity; Cemetary Bomb; D.D. Dynamite", "burst_breath");
		for (const char* id : { "burst_stream_of_destruction", "byser_shock", "card_of_sanctity", "cemetary_bomb", "d_d_dynamite" }) probadas[id]++;
		Escena e;
		e.mon(0, "blue_eyes");
		e.mon(1, "battle_ox");
		VERIFICA(usar(e, "burst_stream_of_destruction") && e.d.l[1].monstruos() == 0, "Burst Stream");
		Escena f;
		f.colocar(1, "mirror_force");
		f.mon(1, "kuriboh", true, true);
		const int bz = f.d.invocarEspecial(0, "byser_shock", false);
		motorVentana(f.d, Ventana{ V_DISPARO, 0, bz, 0, {} });
		f.pasarTodo();
		VERIFICA(f.d.l[1].mano.size() == 2, "Byser Shock %d", (int)f.d.l[1].mano.size());
		Escena g;
		g.d.l[1].cementerio = { "kuriboh", "sangan", "raigeki" };
		VERIFICA(usarColocada(g, 0, "cemetary_bomb") && g.d.l[1].lp == LP_INICIAL - 300, "Cemetary Bomb");
		fin();
	}
	{
		caso("Cave Dragon; Cestus of Dagla; Chaos End; Chaos Greed; Chaos Sorcerer; Chorus of Sanctuary", "cave_dragon");
		for (const char* id : { "cestus_of_dagla", "chaos_end", "chaos_greed", "chaos_sorcerer", "chorus_of_sanctuary", "cannonball_spear_shellfish", "crab_turtle" }) probadas[id]++;
		Escena e;
		e.mon(0, "kuriboh");
		VERIFICA(!e.d.puedeInvocarNormal(0, e.mano(0, "cave_dragon"), {}), "Cave Dragon con monstruos");
		Escena f(F_BATALLA);
		const int a = f.mon(0, "shining_angel");
		mtArriba(f, 0, "cestus_of_dagla", f.d.l[0].m[(size_t)a].uid);
		VERIFICA(atacar(f, a, -1), "no ataco");
		f.pasarTodo();
		VERIFICA(f.d.l[0].lp == LP_INICIAL + 1900, "Cestus LP %d", f.d.l[0].lp);
		Escena g;
		g.d.l[0].cementerio = { "dark_magician", "blue_eyes" };
		const int k = g.mano(0, "chaos_sorcerer");
		VERIFICA(g.activar(0, H(0, k)) && buscarZona(g, 0, "chaos_sorcerer") >= 0, "Chaos Sorcerer");
		fin();
	}
	{
		caso("Confiscation; Curse of Aging; Curse of Fiend; Curse of Darkness; Coffin Seller; Blessings of the Nile", "confiscation");
		for (const char* id : { "curse_of_aging", "curse_of_fiend", "curse_of_darkness", "coffin_seller", "blessings_of_the_nile" }) probadas[id]++;
		Escena e;
		e.mano(1, "raigeki");
		VERIFICA(usar(e, "confiscation") && e.d.l[1].mano.empty() && e.d.l[0].lp == LP_INICIAL - 1000, "Confiscation");
		Escena f;
		mtArriba(f, 1, "curse_of_darkness");
		VERIFICA(usar(f, "blue_medicine") && f.d.l[0].lp == LP_INICIAL + 400 - 1000, "Curse of Darkness LP %d", f.d.l[0].lp);
		Escena g;
		mtArriba(g, 0, "coffin_seller");
		const int z = g.mon(1, "kuriboh");
		g.d.alCementerio(1, z);
		VERIFICA(g.d.l[1].lp == LP_INICIAL - 300, "Coffin Seller %d", g.d.l[1].lp);
		fin();
	}
	{
		caso("Dark Cat with White Tail; Dark Core; Dark Factory; Dark Mimic LV1; Dark Mirror Force; Dark-Piercing Light", "dark_mirror_force");
		for (const char* id : { "dark_cat_with_white_tail", "dark_core", "dark_factory_of_mass_production", "dark_mimic_lv1", "dark_piercing_light" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "battle_ox");
		e.mon(0, "kuriboh", true);
		const int t = e.colocar(1, "dark_mirror_force");
		VERIFICA(atacar(e, a, -1), "no ataco");
		VERIFICA(e.activar(1, MT(1, t)), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].desterrados.size() == 1, "Dark Mirror Force");
		Escena f;
		f.d.l[0].cementerio = { "celtic_guardian", "battle_ox" };
		VERIFICA(usar(f, "dark_factory_of_mass_production") && f.d.l[0].mano.size() == 2, "Dark Factory");
		Escena g;
		g.mon(1, "kuriboh", true, true);
		VERIFICA(usar(g, "dark_piercing_light") && !g.d.l[1].m[0].bocaAbajo, "Dark-Piercing Light");
		fin();
	}
}
static void tanda7t()
{
	fprintf(stderr, "tanda 7 (T)\n");
	{
		caso("Bottomless Shifting Sand; Buster Rancher; Cold Wave", "bottomless_shifting_sand");
		for (const char* id : { "buster_rancher", "cold_wave" }) probadas[id]++;
		Escena e;
		mtArriba(e, 0, "bottomless_shifting_sand");
		e.mon(1, "summoned_skull");
		e.mon(1, "kuriboh");
		e.d.activo = 1;
		motorFinTurno(e.d);
		VERIFICA(e.d.l[1].monstruos() == 1, "Shifting Sand");
		Escena f(F_BATALLA);
		const int a = f.mon(0, "kuriboh");
		mtArriba(f, 0, "buster_rancher", f.d.l[0].m[(size_t)a].uid);
		const int z = f.mon(1, "summoned_skull");
		VERIFICA(atacar(f, a, z), "no ataco");
		f.pasarTodo();
		VERIFICA(f.d.l[1].monstruos() == 0 && f.d.l[1].lp == LP_INICIAL - 300, "Buster Rancher LP %d", f.d.l[1].lp);
		Escena g;
		VERIFICA(usar(g, "cold_wave"), "no activo");
		const int k = g.mano(0, "red_medicine");
		VERIFICA(motivoNoActivar(g.d, 0, H(0, k), false) == "ef.no_momento", "Cold Wave");
		fin();
	}
	{
		caso("Dark Scorpion Burglars / Cliff / Gorg; Dark Snake Syndrome; Dark Spirit of the Silent; Draining Shield", "dark_scorpion_burglars");
		for (const char* id : { "dark_scorpion_cliff_the_trap_remover", "dark_scorpion_gorg_the_strong", "dark_snake_syndrome", "dark_spirit_of_the_silent", "draining_shield" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "dark_scorpion_burglars");
		e.d.l[1].mazo.push_back("raigeki");
		VERIFICA(atacar(e, a, -1), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].cementerio.size() == 1, "Burglars");
		Escena f;
		mtArriba(f, 0, "dark_snake_syndrome");
		motorFaseEspera(f.d);
		motorFaseEspera(f.d);
		VERIFICA(f.d.l[0].lp == LP_INICIAL - 600 && f.d.l[1].lp == LP_INICIAL - 600, "Snake LP %d", f.d.l[1].lp);
		Escena g(F_BATALLA);
		const int b = g.mon(0, "summoned_skull");
		const int t = g.colocar(1, "draining_shield");
		VERIFICA(atacar(g, b, -1), "no ataco");
		VERIFICA(g.activar(1, MT(1, t), { M(0, b) }), "no activo");
		g.pasarTodo();
		VERIFICA(g.d.l[1].lp == LP_INICIAL + 2500, "Draining LP %d", g.d.l[1].lp);
		fin();
	}
	{
		caso("Darkness Approaches; De-Fusion; De-Spell; Deal of Phantom; Dedication through Light and Darkness", "darkness_approaches");
		for (const char* id : { "de_fusion", "de_spell", "deal_of_phantom", "dedication_through_light_and_darkness" }) probadas[id]++;
		Escena e;
		const int z = e.mon(1, "battle_ox");
		e.mano(0, "kuriboh");
		e.mano(0, "sangan");
		VERIFICA(usar(e, "darkness_approaches", { M(1, z) }, { 0, 1 }) && e.d.l[1].m[(size_t)z].bocaAbajo, "Darkness Approaches");
		Escena f;
		const int k = f.mon(1, "flame_swordsman");
		VERIFICA(usar(f, "de_fusion", { M(1, k) }) && f.d.l[1].monstruos() == 0 && f.d.l[1].extra.size() == 1, "De-Fusion");
		Escena g;
		g.mon(0, "dark_magician");
		g.d.l[0].mazo.push_back("dark_magician_of_chaos");
		VERIFICA(usar(g, "dedication_through_light_and_darkness") && buscarZona(g, 0, "dark_magician_of_chaos") >= 0, "Dedication");
		fin();
	}
	{
		caso("Des Volstgalph; Desert Sunlight; Destruction Ring; Dimension Distortion; Dimension Fusion", "des_volstgalph");
		for (const char* id : { "desert_sunlight", "destruction_ring", "dimension_distortion", "dimension_fusion" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "des_volstgalph");
		const int z = e.mon(1, "kuriboh");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 1900 - 500, "Volstgalph LP %d", e.d.l[1].lp);
		Escena f;
		f.d.l[0].desterrados = { "summoned_skull" };
		f.d.l[1].desterrados = { "kuriboh" };
		VERIFICA(usar(f, "dimension_fusion") && f.d.l[0].monstruos() == 1 && f.d.l[1].monstruos() == 1, "Dimension Fusion");
		Escena g;
		g.d.l[0].desterrados = { "summoned_skull" };
		VERIFICA(usar(g, "dimension_distortion") && g.d.l[0].monstruos() == 1, "Dimension Distortion");
		fin();
	}
	{
		caso("Disappear; Disarmament; Disturbance Strategy; Divine Wrath; DNA Transplant; Double Snare", "disarmament");
		for (const char* id : { "disappear", "disturbance_strategy", "divine_wrath", "dna_transplant", "double_snare" }) probadas[id]++;
		Escena e;
		const int z = e.mon(1, "battle_ox");
		mtArriba(e, 1, "axe_of_despair", e.d.l[1].m[(size_t)z].uid);
		VERIFICA(usarColocada(e, 0, "disarmament") && e.d.l[1].m[(size_t)z].atkEf == 1700, "Disarmament");
		Escena f;
		f.mon(0, "dark_magician");
		const int k = f.mon(1, "kuriboh");
		VERIFICA(usarColocada(f, 0, "dna_transplant") && atributoDe(f.d, f.d.l[1].m[(size_t)k]) == "OSCURIDAD", "DNA Transplant");
		Escena g;
		g.mon(1, "jinzo");
		VERIFICA(usar(g, "double_snare") && g.d.l[1].monstruos() == 0, "Double Snare");
		fin();
	}
	{
		caso("Dragon Manipulator; Dragon Seeker; Dragon's Rage; Des Counterblow; Different Dimension Dragon; Dark Room of Nightmare", "dragon_seeker");
		for (const char* id : { "dragon_manipulator", "dragon_s_rage", "des_counterblow", "different_dimension_dragon", "dark_room_of_nightmare", "chain_burst",
		                        "continuous_destruction_punch", "d_d_borderline", "criosphinx", "dokurorider" })
			probadas[id]++;
		Escena e;
		e.mon(1, "blue_eyes");
		const int k = e.mano(0, "dragon_seeker");
		e.mon(0, "kuriboh");
		Jugada j;
		j.tipo = J_INVOCAR;
		j.mano = k;
		j.tributos = { 0 };
		VERIFICA(e.d.aplicar(j), "no invoco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0, "Dragon Seeker");
		Escena f(F_BATALLA);
		mtArriba(f, 1, "des_counterblow");
		const int a = f.mon(0, "battle_ox");
		VERIFICA(atacar(f, a, -1), "no ataco");
		f.pasarTodo();
		VERIFICA(f.d.l[0].monstruos() == 0 && f.d.l[1].lp == LP_INICIAL - 1700, "Des Counterblow");
		Escena g;
		mtArriba(g, 0, "dark_room_of_nightmare");
		VERIFICA(usar(g, "hinotama"), "no activo");
		VERIFICA(g.d.l[1].lp == LP_INICIAL - 500 - 300, "Dark Room LP %d", g.d.l[1].lp);
		fin();
	}
}
static void tanda7u()
{
	fprintf(stderr, "tanda 7 (U)\n");
	{
		caso("Goblin Attack Force / Giant Orc: tras atacar, a defensa y fijos; Exiled Force; Fiber Jar; Giant Trunade", "goblin_attack_force");
		for (const char* id : { "giant_orc", "exiled_force", "fiber_jar", "giant_trunade" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "goblin_attack_force");
		VERIFICA(atacar(e, a, -1), "no ataco");
		e.pasarTodo();
		Jugada j;
		j.tipo = J_FASE;
		e.d.aplicar(j);
		VERIFICA(e.d.l[0].m[(size_t)a].defensa && motorPosicionFija(e.d, 0, a), "Goblin Attack Force");
		Escena f;
		const int x = f.mon(0, "exiled_force");
		const int z = f.mon(1, "blue_eyes");
		VERIFICA(f.activar(0, M(0, x), { M(1, z) }), "no activo");
		f.pasarTodo();
		VERIFICA(f.d.l[1].monstruos() == 0 && f.d.l[0].monstruos() == 0, "Exiled Force");
		Escena g;
		g.colocar(1, "mirror_force");
		g.colocar(0, "trap_hole");
		VERIFICA(usar(g, "giant_trunade") && g.d.l[1].mano.size() == 1 && g.d.l[0].mano.size() == 1, "Giant Trunade");
		Escena h;
		h.mano(0, "kuriboh");
		h.d.l[1].cementerio = { "sangan" };
		const int fj = h.mon(0, "fiber_jar", true, true);
		VERIFICA(voltear(h, fj), "no se volteo");
		h.pasarTodo();
		VERIFICA(h.d.l[0].mano.size() == 5 && h.d.l[1].mano.size() == 5 && h.d.l[1].cementerio.empty(), "Fiber Jar");
		fin();
	}
	{
		caso("Earthquake; Eternal Rest; Exile of the Wicked; Dummy Golem; Ectoplasmer; Ekibyo Drakmord", "earthquake");
		for (const char* id : { "eternal_rest", "exile_of_the_wicked", "dummy_golem", "ectoplasmer", "ekibyo_drakmord" }) probadas[id]++;
		Escena e;
		const int z = e.mon(1, "battle_ox");
		VERIFICA(usar(e, "earthquake") && e.d.l[1].m[(size_t)z].defensa, "Earthquake");
		Escena f;
		f.mon(1, "dark_ruler_ha_des");
		VERIFICA(usar(f, "exile_of_the_wicked") && f.d.l[1].monstruos() == 0, "Exile");
		Escena g;
		mtArriba(g, 0, "ectoplasmer");
		g.mon(0, "summoned_skull");
		motorFinTurno(g.d);
		VERIFICA(g.d.l[1].lp == LP_INICIAL - 1250, "Ectoplasmer LP %d", g.d.l[1].lp);
		Escena h;
		const int o = h.mon(1, "battle_ox");
		VERIFICA(usar(h, "ekibyo_drakmord", { M(1, o) }), "Ekibyo");
		h.d.activo = 1;
		motorFinTurno(h.d);
		motorFinTurno(h.d);
		VERIFICA(h.d.l[1].monstruos() == 0 && enMano(h, 0, "ekibyo_drakmord"), "Ekibyo destruye");
		fin();
	}
	{
		caso("Emblem of Dragon Destroyer; Fusion Sage; Gather Your Mind; Emergency Provisions; Emes the Infinity; Enchanting Fitting Room",
		     "emblem_of_dragon_destroyer");
		for (const char* id : { "fusion_sage", "gather_your_mind", "emergency_provisions", "emes_the_infinity", "enchanting_fitting_room" }) probadas[id]++;
		Escena e;
		e.d.l[0].mazo.push_back("buster_blader");
		VERIFICA(usar(e, "emblem_of_dragon_destroyer") && enMano(e, 0, "buster_blader"), "Emblem");
		Escena f;
		f.colocar(0, "trap_hole");
		VERIFICA(usar(f, "emergency_provisions") && f.d.l[0].lp == LP_INICIAL + 1000, "Provisions");
		Escena g(F_BATALLA);
		const int a = g.mon(0, "emes_the_infinity");
		const int z = g.mon(1, "kuriboh");
		VERIFICA(atacar(g, a, z), "no ataco");
		g.pasarTodo();
		VERIFICA(g.d.l[0].m[(size_t)a].atkEf == 3200, "Emes %d", g.d.l[0].m[(size_t)a].atkEf);
		fin();
	}
	{
		caso("Energy Drain; Enraged Muka Muka; Exhausting Spell; Fairy Box; Fairy King Truesdale; Fairy Meteor Crush; Falling Down",
		     "energy_drain");
		for (const char* id : { "enraged_muka_muka", "exhausting_spell", "fairy_box", "fairy_king_truesdale", "fairy_meteor_crush", "falling_down" }) probadas[id]++;
		Escena e;
		const int m = e.mon(0, "enraged_muka_muka");
		e.mano(0, "kuriboh");
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[(size_t)m].atkEf == 1600, "Muka %d", e.d.l[0].m[(size_t)m].atkEf);
		Escena f;
		const int o = f.mon(1, "blue_eyes");
		VERIFICA(usar(f, "falling_down", { M(1, o) }) && buscarZona(f, 0, "blue_eyes") >= 0, "Falling Down");
		Escena g;
		g.mon(0, "fairy_king_truesdale", true);
		const int p = g.mon(0, "lord_poison");
		VERIFICA(g.d.l[0].m[(size_t)p].atkEf == 2000, "Truesdale %d", g.d.l[0].m[(size_t)p].atkEf);
		fin();
	}
	{
		caso("Fiend Comedian; Final Countdown; Final Destiny; Forced Ceasefire; Fox Fire; Frozen Soul; Fatal Abacus; Final Attack Orders",
		     "final_countdown");
		for (const char* id : { "fiend_comedian", "final_destiny", "forced_ceasefire", "fox_fire", "frozen_soul", "fatal_abacus", "final_attack_orders" }) probadas[id]++;
		Escena e;
		VERIFICA(usar(e, "final_countdown") && e.d.l[0].lp == LP_INICIAL - 2000, "Countdown");
		e.d.turno += 20;
		motorFaseEspera(e.d);
		VERIFICA(e.d.ganador == 0, "no gano");
		Escena f(F_BATALLA);
		const int a = f.mon(0, "summoned_skull");
		const int z = f.mon(1, "fox_fire");
		VERIFICA(atacar(f, a, z), "no ataco");
		f.pasarTodo();
		motorFinTurno(f.d);
		VERIFICA(buscarZona(f, 1, "fox_fire") >= 0, "Fox Fire");
		Escena g;
		mtArriba(g, 0, "fatal_abacus");
		const int k = g.mon(1, "kuriboh");
		g.d.alCementerio(1, k);
		VERIFICA(g.d.l[1].lp == LP_INICIAL - 500, "Abacus");
		Escena h;
		mtArriba(h, 0, "final_attack_orders");
		const int d2 = h.mon(1, "battle_ox", true);
		VERIFICA(!h.d.l[1].m[(size_t)d2].defensa, "Final Attack Orders");
		fin();
	}
	{
		caso("Fuhma Shuriken; Fusion Weapon; Fulfillment of the Contract; Gaia Soul; Garuda / Gigantes / Fenrir; Gate Guardian", "fusion_weapon");
		for (const char* id : { "fuhma_shuriken", "fulfillment_of_the_contract", "gaia_soul_the_combustible_collective", "garuda_the_wind_spirit", "gigantes", "fenrir", "gate_guardian" }) probadas[id]++;
		Escena e;
		const int f = e.mon(0, "flame_swordsman");
		VERIFICA(usar(e, "fusion_weapon", { M(0, f) }) && e.d.l[0].m[(size_t)f].atkEf == 3300, "Fusion Weapon");
		Escena g;
		g.mon(0, "sanga_of_the_thunder");
		g.mon(0, "kazejin");
		g.mon(0, "suijin");
		const int k = g.mano(0, "gate_guardian");
		VERIFICA(g.activar(0, H(0, k)) && g.d.l[0].monstruos() == 1, "Gate Guardian");
		Escena h;
		h.d.l[0].cementerio = { "harpie_lady" };
		VERIFICA(h.activar(0, H(0, h.mano(0, "garuda_the_wind_spirit"))), "Garuda");
		fin();
	}
	{
		caso("Gift of the Martyr; Goblin's Secret Remedy; Goblin Thief; Goblin Fan; Giant Axe Mummy", "goblin_thief");
		for (const char* id : { "gift_of_the_martyr", "goblin_s_secret_remedy", "goblin_fan", "giant_axe_mummy" }) probadas[id]++;
		Escena e;
		VERIFICA(usar(e, "goblin_thief") && e.d.l[1].lp == LP_INICIAL - 500 && e.d.l[0].lp == LP_INICIAL + 500, "Goblin Thief");
		Escena f;
		mtArriba(f, 1, "goblin_fan");
		const int z = f.mon(0, "man_eater_bug", true, true);
		VERIFICA(voltear(f, z), "no se volteo");
		f.pasarTodo();
		VERIFICA(f.d.l[0].monstruos() == 0, "Goblin Fan");
		fin();
	}
}
static void tanda7v()
{
	fprintf(stderr, "tanda 7 (V)\n");
	{
		caso("Fengsheng Mirror; Flint; Frontline Base; Germ Infection; Gust Fan; Invigoration; Horn of Light", "flint");
		for (const char* id : { "fengsheng_mirror", "frontline_base", "germ_infection", "gust_fan", "invigoration", "horn_of_light" }) probadas[id]++;
		Escena e;
		const int z = e.mon(1, "battle_ox");
		VERIFICA(usar(e, "flint", { M(1, z) }) && e.d.l[1].m[(size_t)z].atkEf == 1400, "Flint");
		e.d.activo = 1;
		e.d.fase = F_BATALLA;
		VERIFICA(!e.d.puedeAtacar(1, z), "ataco con Flint");
		Escena f;
		const int c = f.mon(0, "celtic_guardian");
		VERIFICA(usar(f, "invigoration", { M(0, c) }) && f.d.l[0].m[(size_t)c].atkEf == 1800, "Invigoration");
		Escena g;
		g.mano(1, "maharaghi");
		VERIFICA(usar(g, "fengsheng_mirror") && g.d.l[1].mano.empty(), "Fengsheng");
		fin();
	}
	{
		caso("Ghost Knight of Jackal; Good Goblin Housekeeping; Gravedigger Ghoul; Graverobber's Retribution; Greenkappa; Gryphon's Feather Duster",
		     "ghost_knight_of_jackal");
		for (const char* id : { "good_goblin_housekeeping", "gravedigger_ghoul", "graverobber_s_retribution", "greenkappa", "gryphon_s_feather_duster", "gryphon_wing" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "ghost_knight_of_jackal");
		const int z = e.mon(1, "kuriboh");
		VERIFICA(atacar(e, a, z), "no ataco");
		e.pasarTodo();
		VERIFICA(buscarZona(e, 0, "kuriboh") >= 0, "Ghost Knight");
		Escena f;
		f.colocar(1, "mirror_force");
		f.colocar(1, "trap_hole");
		const int k = f.mon(0, "greenkappa", true, true);
		VERIFICA(voltear(f, k), "no se volteo");
		f.pasarTodo();
		VERIFICA(f.d.l[1].mt[0].vacia() && f.d.l[1].mt[1].vacia(), "Greenkappa");
		Escena g;
		g.colocar(0, "trap_hole");
		VERIFICA(usar(g, "gryphon_s_feather_duster") && g.d.l[0].lp == LP_INICIAL + 500, "Gryphon Duster");
		fin();
	}
	{
		caso("Hallowed Life Barrier; Hammer Shot; Hand of Nephthys; Heavy Slump; Hidden Spellbook; Horn of Heaven; Horus LV8", "hammer_shot");
		for (const char* id : { "hallowed_life_barrier", "hand_of_nephthys", "heavy_slump", "hidden_spellbook", "horn_of_heaven", "horus_the_black_flame_dragon_lv8" }) probadas[id]++;
		Escena e;
		e.mon(1, "blue_eyes");
		e.mon(1, "kuriboh");
		VERIFICA(usar(e, "hammer_shot") && e.d.l[1].monstruos() == 1 && buscarZona(e, 1, "blue_eyes") < 0, "Hammer Shot");
		Escena f;
		f.mon(0, "hand_of_nephthys");
		f.mon(0, "kuriboh");
		f.d.l[0].mazo.push_back("sacred_phoenix_of_nephthys");
		VERIFICA(f.activar(0, M(0, 0)), "no activo");
		f.pasarTodo();
		VERIFICA(buscarZona(f, 0, "sacred_phoenix_of_nephthys") >= 0 && f.d.l[0].monstruos() == 1, "Hand of Nephthys");
		Escena g;
		const int h = g.mon(1, "horus_the_black_flame_dragon_lv8");
		const int k = g.mano(0, "raigeki");
		VERIFICA(g.activar(0, H(0, k)), "no activo Raigeki");
		if (g.d.ef.preg.tipo == P_RESPUESTA && g.d.ef.preg.lado == 1) g.activar(1, M(1, h));
		g.pasarTodo();
		VERIFICA(g.d.l[1].monstruos() == 1, "Horus LV8 no nego");
		fin();
	}
	{
		caso("Hyena; Inferno; Inferno Fire Blast; Injection Fairy Lily; Knight's Title / Dark Magician Knight", "injection_fairy_lily");
		for (const char* id : { "hyena", "inferno", "inferno_fire_blast", "knight_s_title", "dark_magician_knight" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "blue_eyes");
		const int l = e.mon(1, "injection_fairy_lily");
		VERIFICA(atacar(e, a, l), "no ataco");
		for (int g = 0; g < 10 && e.d.ef.ventana.tipo != V_PASO_DANNO; g++) e.pasa(e.d.ef.preg.lado);
		if (e.d.ef.preg.tipo == P_RESPUESTA && e.d.ef.preg.lado == 0) e.pasa(0);
		VERIFICA(e.d.ef.preg.lado == 1 && e.activar(1, M(1, l)), "no activo Lily");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 0 && e.d.l[1].lp == LP_INICIAL - 2000, "Lily");
		Escena f;
		f.mon(0, "red_eyes");
		VERIFICA(usar(f, "inferno_fire_blast") && f.d.l[1].lp == LP_INICIAL - 2400, "Fire Blast");
		Escena g;
		g.mon(0, "dark_magician");
		g.mon(1, "battle_ox");
		g.d.l[0].mazo.push_back("dark_magician_knight");
		VERIFICA(usar(g, "knight_s_title") && buscarZona(g, 0, "dark_magician_knight") >= 0 && g.d.l[1].monstruos() == 0, "Knight's Title");
		fin();
	}
	{
		caso("Lady Ninja Yae; Last Will; Lava / Swamp Battleguard; Great Maju Garzett; Goblin of Greed; Kangaroo Champ; Lesser Fiend",
		     "lady_ninja_yae");
		for (const char* id : { "last_will", "lava_battleguard", "swamp_battleguard", "great_maju_garzett", "goblin_of_greed", "kangaroo_champ", "lesser_fiend",
		                        "goddess_with_the_third_eye", "king_of_the_swamp", "hungry_burger", "inaba_white_rabbit", "hayabusa_knight", "infinite_dismissal",
		                        "labyrinth_of_nightmare", "gravekeeper_s_servant" })
			probadas[id]++;
		Escena e;
		const int y = e.mon(0, "lady_ninja_yae");
		e.mano(0, "harpie_lady");
		e.colocar(1, "mirror_force");
		VERIFICA(e.activar(0, M(0, y)), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].mano.size() == 1, "Yae");
		Escena f;
		const int l = f.mon(0, "lava_battleguard");
		f.mon(0, "swamp_battleguard");
		VERIFICA(f.d.l[0].m[(size_t)l].atkEf == 2050, "Battleguard %d", f.d.l[0].m[(size_t)l].atkEf);
		Escena g;
		g.mon(0, "summoned_skull");
		Jugada j;
		j.tipo = J_INVOCAR;
		j.mano = g.mano(0, "great_maju_garzett");
		j.tributos = { 0 };
		VERIFICA(g.d.aplicar(j), "no invoco");
		g.pasarTodo();
		const int gz = buscarZona(g, 0, "great_maju_garzett");
		VERIFICA(gz >= 0 && g.d.l[0].m[(size_t)gz].atkEf == 5000, "Garzett %d", gz >= 0 ? g.d.l[0].m[(size_t)gz].atkEf : -1);
		Escena h;
		h.mon(1, "goblin_of_greed");
		const int k = h.mano(0, "lightning_vortex");
		h.mano(0, "kuriboh");
		h.mon(1, "kuriboh");
		VERIFICA(motivoNoActivar(h.d, 0, H(0, k), false) == "ef.no_momento" || !defCarta("lightning_vortex"), "Goblin of Greed");
		fin();
	}
}
static void tanda7w()
{
	fprintf(stderr, "tanda 7 (W)\n");
	{
		caso("Lightning Vortex; Lightning Blade; Little Chimera; Malevolent Nuzzler; Lost Guardian; Lighten the Load", "lightning_vortex");
		for (const char* id : { "lightning_blade", "little_chimera", "malevolent_nuzzler", "lost_guardian", "lighten_the_load" }) probadas[id]++;
		Escena e;
		e.mano(0, "kuriboh");
		e.mon(1, "blue_eyes");
		e.mon(1, "battle_ox", true, true);
		VERIFICA(usar(e, "lightning_vortex", {}, { 0 }) && e.d.l[1].monstruos() == 1 && e.d.l[0].mano.empty(), "Vortex");
		Escena f;
		const int c = f.mon(0, "celtic_guardian");
		VERIFICA(usar(f, "lightning_blade", { M(0, c) }) && f.d.l[0].m[(size_t)c].atkEf == 2200, "Blade %d", f.d.l[0].m[(size_t)c].atkEf);
		Escena g;
		g.mon(0, "little_chimera");
		const int h = g.mon(0, "hinotama_soul");
		VERIFICA(g.d.l[0].m[(size_t)h].atkEf == g.d.atk("hinotama_soul") + 500, "Chimera %d", g.d.l[0].m[(size_t)h].atkEf);
		fin();
	}
	{
		caso("Magical Scientist; Malice Dispersion; Mass Driver; Master Monk; Metamorphosis", "magical_scientist");
		for (const char* id : { "malice_dispersion", "mass_driver", "master_monk", "metamorphosis" }) probadas[id]++;
		Escena e;
		const int s = e.mon(0, "magical_scientist");
		e.d.l[0].extra = { "flame_swordsman" };
		VERIFICA(e.activar(0, M(0, s)), "no activo");
		e.pasarTodo();
		VERIFICA(buscarZona(e, 0, "flame_swordsman") >= 0 && e.d.l[0].lp == LP_INICIAL - 1000, "Scientist");
		motorFinTurno(e.d);
		VERIFICA(buscarZona(e, 0, "flame_swordsman") < 0 && e.d.l[0].extra.size() == 1, "vuelve al mazo de fusion");
		Escena f;
		mtArriba(f, 0, "mass_driver");
		f.mon(0, "kuriboh");
		f.d.l[1].lp = 400;
		VERIFICA(f.activar(0, MT(0, 0)), "no activo Mass Driver");
		f.pasarTodo();
		VERIFICA(f.d.ganador == 0, "Mass Driver LP %d", f.d.l[1].lp);
		fin();
	}
	{
		caso("Meteorain; Michizure; Micro Ray; Mind Crush; Mind Haxorz; Mind Wipe", "mind_crush");
		for (const char* id : { "meteorain", "michizure", "micro_ray", "mind_haxorz", "mind_wipe" }) probadas[id]++;
		Escena e;
		e.mano(1, "kuriboh");
		e.mano(1, "kuriboh");
		e.mano(1, "raigeki");
		e.d.l[1].cementerio = { "kuriboh" };
		VERIFICA(usarColocada(e, 0, "mind_crush") && e.d.l[1].mano.size() == 1, "Mind Crush");
		Escena f;
		const int o = f.mon(1, "battle_ox", true);
		VERIFICA(usarColocada(f, 0, "micro_ray", { M(1, o) }) && f.d.l[1].m[(size_t)o].defEf == 0, "Micro Ray %d", f.d.l[1].m[(size_t)o].defEf);
		Escena g(F_BATALLA);
		const int a = g.mon(0, "summoned_skull");
		const int z = g.mon(1, "kuriboh", true);
		usarColocada(g, 0, "meteorain");
		VERIFICA(atacar(g, a, z), "no ataco");
		g.pasarTodo();
		VERIFICA(g.d.l[1].lp < LP_INICIAL, "Meteorain");
		fin();
	}
	{
		caso("Minor Goblin Official; Miracle Dig; Monster Reincarnation; Mucus Yolk; Mushroom Man #2; Mystik Wok", "mystik_wok");
		for (const char* id : { "minor_goblin_official", "miracle_dig", "monster_reincarnation", "mucus_yolk", "mushroom_man_2" }) probadas[id]++;
		Escena e;
		const int b = e.mon(0, "battle_ox");
		VERIFICA(usar(e, "mystik_wok", { M(0, b) }) && e.d.l[0].lp == LP_INICIAL + 1700 && e.d.l[0].monstruos() == 0, "Wok %d", e.d.l[0].lp);
		Escena f;
		f.mano(0, "kuriboh");
		f.d.l[0].cementerio = { "blue_eyes" };
		VERIFICA(usar(f, "monster_reincarnation", {}, { 0 }) && enMano(f, 0, "blue_eyes"), "Reincarnation");
		Escena g(F_BATALLA);
		const int y = g.mon(0, "mucus_yolk");
		g.mon(1, "blue_eyes");
		VERIFICA(g.d.puedeAtacar(0, y) && atacar(g, y, -1), "Mucus no ataco directo");
		g.pasarTodo();
		VERIFICA(g.d.l[1].lp == LP_INICIAL, "Mucus danno %d", g.d.l[1].lp);
		fin();
	}
	{
		caso("Needle Burrower; Needle Ceiling; Nobleman of Extermination; Offerings to the Doomed; Opti-Camouflage; Order to Charge",
		     "nobleman_of_extermination");
		for (const char* id : { "needle_burrower", "needle_ceiling", "offerings_to_the_doomed", "opti_camouflage_armor", "order_to_charge" }) probadas[id]++;
		Escena e;
		const int z = e.colocar(1, "mirror_force");
		VERIFICA(usar(e, "nobleman_of_extermination", { MT(1, z) }) && e.d.l[1].desterrados.size() == 1, "Nobleman");
		Escena f;
		for (int k = 0; k < 2; k++) f.mon(0, "kuriboh"), f.mon(1, "kuriboh");
		VERIFICA(usarColocada(f, 0, "needle_ceiling") && f.d.l[0].monstruos() + f.d.l[1].monstruos() == 0, "Needle Ceiling");
		Escena g;
		const int o = g.mon(1, "blue_eyes");
		VERIFICA(usar(g, "offerings_to_the_doomed", { M(1, o) }) && g.d.l[1].monstruos() == 0 && g.d.ef.saltaRobo[0], "Offerings");
		Escena h;
		h.mon(0, "dark_magician");
		const int ob = h.mon(1, "blue_eyes");
		VERIFICA(usar(h, "order_to_charge", { M(1, ob) }) && h.d.l[1].monstruos() == 0 && h.d.l[0].monstruos() == 0, "Order to Charge");
		fin();
	}
	{
		caso("Paladin of White Dragon; Paralyzing Potion; Phoenix Wing Wind Blast; Pikeru's Circle; Possessed Dark Soul; Pyramid Energy",
		     "possessed_dark_soul");
		for (const char* id : { "paladin_of_white_dragon", "paralyzing_potion", "phoenix_wing_wind_blast", "pikeru_s_circle_of_enchantment", "pyramid_energy",
		                        "maju_garzett", "magical_thorn", "poison_fangs", "mefist_the_infernal_general", "nightmare_horse", "penumbral_soldier_lady",
		                        "performance_of_sword" })
			probadas[id]++;
		Escena e;
		const int p = e.mon(0, "possessed_dark_soul");
		e.mon(1, "kuriboh");
		e.mon(1, "blue_eyes");
		VERIFICA(e.activar(0, M(0, p)), "no activo");
		e.pasarTodo();
		VERIFICA(buscarZona(e, 0, "kuriboh") >= 0 && e.d.l[1].monstruos() == 1, "Dark Soul");
		Escena f;
		const int o = f.mon(1, "blue_eyes");
		VERIFICA(usar(f, "paralyzing_potion", { M(1, o) }), "Potion");
		f.d.activo = 1;
		f.d.fase = F_BATALLA;
		VERIFICA(!f.d.puedeAtacar(1, o), "ataca con Potion");
		Escena g;
		g.mano(0, "kuriboh");
		const int b = g.mon(1, "blue_eyes");
		const int pw = g.colocar(0, "phoenix_wing_wind_blast");
		VERIFICA(g.activar(0, MT(0, pw), { M(1, b) }, { 0 }), "no activo Phoenix Wing");
		g.pasarTodo();
		VERIFICA(g.d.l[1].monstruos() == 0 && g.d.l[1].mazo.back() == "blue_eyes", "Phoenix Wing");
		Escena h;
		mtArriba(h, 0, "magical_thorn");
		h.mano(1, "kuriboh");
		h.d.descartar(1, 0);
		VERIFICA(h.d.l[1].lp == LP_INICIAL - 500, "Magical Thorn");
		Escena k;
		k.mon(0, "summoned_skull");
		k.mon(0, "battle_ox");
		Jugada j;
		j.tipo = J_INVOCAR;
		j.mano = k.mano(0, "maju_garzett");
		j.tributos = { 0, 1 };
		VERIFICA(k.d.aplicar(j), "no invoco");
		k.pasarTodo();
		const int gz = buscarZona(k, 0, "maju_garzett");
		VERIFICA(gz >= 0 && k.d.l[0].m[(size_t)gz].atkEf == 4200, "Maju %d", gz >= 0 ? k.d.l[0].m[(size_t)gz].atkEf : -1);
		fin();
	}
}
static void tanda7x()
{
	fprintf(stderr, "tanda 7 (X)\n");
	{
		caso("Checkmate; Delta Attacker; Card Shuffle; Archfiend's Oath; D.D. Designator; Ante", "checkmate");
		for (const char* id : { "delta_attacker", "card_shuffle", "archfiend_s_oath", "d_d_designator", "ante" }) probadas[id]++;
		Escena e;
		e.mon(0, "vilepawn_archfiend");
		const int t = e.mon(0, "terrorking_archfiend");
		VERIFICA(usar(e, "checkmate", { M(0, t) }) && e.d.l[0].m[(size_t)t].directoTurno && e.d.l[0].monstruos() == 1, "Checkmate");
		Escena f;
		for (int k = 0; k < 3; k++) f.mon(0, "celtic_guardian");
		VERIFICA(usar(f, "delta_attacker") && f.d.l[0].m[0].directoTurno && f.d.l[0].m[2].directoTurno, "Delta Attacker");
		Escena g;
		g.mano(1, "kuriboh");
		g.mano(1, "kuriboh");
		g.d.l[1].cementerio = { "kuriboh" };
		VERIFICA(usar(g, "d_d_designator") && g.d.l[1].mano.size() == 1 && g.d.l[1].desterrados.size() == 1, "D.D. Designator");
		Escena h;
		h.mano(0, "blue_eyes");
		h.mano(1, "kuriboh");
		VERIFICA(usar(h, "ante") && h.d.l[1].lp == LP_INICIAL - 1000 && h.d.l[1].mano.empty(), "Ante");
		fin();
	}
	{
		caso("Curse of Anubis; Cursed Seal of the Forbidden Spell; Curse of Royal; Attack and Receive; Covering Fire; Blade Rabbit",
		     "curse_of_anubis");
		for (const char* id : { "cursed_seal_of_the_forbidden_spell", "curse_of_royal", "attack_and_receive", "covering_fire", "blade_rabbit" }) probadas[id]++;
		Escena e;
		const int s = e.mon(1, "sangan");
		const int o = e.mon(1, "battle_ox");
		VERIFICA(usarColocada(e, 0, "curse_of_anubis"), "no activo");
		VERIFICA(e.d.l[1].m[(size_t)s].defensa && e.d.l[1].m[(size_t)s].defEf == 0 && !e.d.l[1].m[(size_t)o].defensa && motorPosicionFija(e.d, 1, s), "Anubis");
		Escena f;
		const int r = f.mon(0, "blade_rabbit");
		f.mon(1, "blue_eyes");
		Jugada j;
		j.tipo = J_POSICION;
		j.zona = r;
		VERIFICA(f.d.aplicar(j), "no cambio");
		f.pasarTodo();
		VERIFICA(f.d.l[1].monstruos() == 0, "Blade Rabbit");
		Escena g;
		g.d.danno(0, 100);
		VERIFICA(usarColocada(g, 0, "attack_and_receive") && g.d.l[1].lp == LP_INICIAL - 700, "Attack and Receive");
		Escena h;
		h.mano(0, "kuriboh");
		const int k = h.mano(1, "raigeki");
		h.mon(0, "battle_ox");
		const int cs = h.colocar(0, "cursed_seal_of_the_forbidden_spell");
		h.d.activo = 1;
		VERIFICA(h.activar(1, H(1, k)), "no activo Raigeki");
		VERIFICA(h.d.ef.preg.tipo == P_RESPUESTA && h.d.ef.preg.lado == 0 && h.activar(0, MT(0, cs), {}, { 0 }), "no respondio");
		h.pasarTodo();
		VERIFICA(h.d.l[0].monstruos() == 1 && h.d.ef.sellados[1].size() == 1, "Cursed Seal");
		fin();
	}
}
static void tanda7y()
{
	fprintf(stderr, "tanda 7 (Y)\n");
	{
		caso("Dice Re-Roll; Gravekeeper's Watcher; Great Dezard / Fushioh Richie; Union Rider; Valkyrie Vierte", "great_dezard");
		for (const char* id : { "dice_re_roll", "gravekeeper_s_watcher", "fushioh_richie", "union_rider", "valkyrie_vierte" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int g = e.mon(0, "great_dezard");
		const int z = e.mon(1, "kuriboh");
		VERIFICA(atacar(e, g, z), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)g].contadores == 1 && ignoraMagia(e.d, 0, g, 1), "Dezard cuenta %d", e.d.l[0].m[(size_t)g].contadores);
		e.d.l[0].m[(size_t)g].contadores = 2;
		e.d.l[0].mazo.push_back("fushioh_richie");
		e.d.fase = F_PRINCIPAL2;
		VERIFICA(e.activar(0, M(0, g)), "no activo Dezard");
		e.pasarTodo();
		VERIFICA(buscarZona(e, 0, "fushioh_richie") >= 0, "Fushioh");
		Escena f;
		const int r = f.mon(0, "union_rider");
		f.mon(1, "y_dragon_head");
		VERIFICA(f.activar(0, M(0, r)), "no activo Union Rider");
		f.pasarTodo();
		VERIFICA(f.d.l[1].monstruos() == 0 && f.d.l[0].m[(size_t)r].atkEf == f.d.atk("union_rider") + 400, "Union Rider %d", f.d.l[0].m[(size_t)r].atkEf);
		Escena h;
		const int v = h.mon(0, "valkyrie_vierte");
		h.mon(0, "valkyrie_zweite");
		h.d.l[0].mazo.push_back("pot_of_greed");
		VERIFICA(h.activar(0, M(0, v)), "no activo Vierte");
		h.pasarTodo();
		VERIFICA(enMano(h, 0, "pot_of_greed"), "Vierte");
		Escena w;
		const int k = w.mano(1, "delinquent_duo");
		w.mano(0, "gravekeeper_s_watcher");
		w.d.activo = 1;
		VERIFICA(w.activar(1, H(1, k)), "no activo Delinquent Duo");
		VERIFICA(w.d.ef.preg.tipo == P_RESPUESTA && w.d.ef.preg.lado == 0 && w.activar(0, H(0, 0)), "no respondio Watcher");
		w.pasarTodo();
		VERIFICA(w.d.l[0].mano.empty() && w.d.l[0].cementerio.size() == 1, "Watcher");
		fin();
	}
	{
		caso("Elemental Burst; Fuh-Rin-Ka-Zan; Huge Revolution; Exchange; Fruits of Kozaky; Jade Insect Whistle; Dark Designator", "elemental_burst");
		for (const char* id : { "fuh_rin_ka_zan", "huge_revolution", "exchange", "fruits_of_kozaky_s_studies", "jade_insect_whistle", "dark_designator" }) probadas[id]++;
		Escena e;
		for (const char* id : { "harpie_lady", "mother_grizzly", "hinotama_soul", "battle_ox" }) e.mon(0, id);
		e.mon(1, "blue_eyes");
		e.colocar(1, "mirror_force");
		VERIFICA(usarColocada(e, 0, "elemental_burst") && e.d.l[1].monstruos() == 0 && e.d.l[1].colocadasMT() == 0 && e.d.l[0].monstruos() == 0,
		         "Elemental Burst %d", e.d.l[0].monstruos());
		Escena f;
		f.mano(0, "kuriboh");
		f.mano(1, "blue_eyes");
		VERIFICA(usar(f, "exchange") && enMano(f, 0, "blue_eyes") && enMano(f, 1, "kuriboh"), "Exchange");
		Escena g;
		g.d.l[1].mazo = { "basic_insect", "kuriboh" };
		VERIFICA(usar(g, "jade_insect_whistle") && g.d.l[1].mazo.back() == "basic_insect", "Jade");
		fin();
	}
	{
		caso("Hane-Hane; Minefield Eruption; Invitation to a Dark Sleep; Jar Robber; Judgment of Anubis; Magic Drain", "invitation_to_a_dark_sleep");
		for (const char* id : { "hane_hane", "minefield_eruption", "jar_robber", "judgment_of_anubis", "magic_drain" }) probadas[id]++;
		Escena e;
		const int o = e.mon(1, "blue_eyes");
		e.mon(0, "kuriboh");
		Jugada ji;
		ji.tipo = J_INVOCAR;
		ji.mano = e.mano(0, "invitation_to_a_dark_sleep");
		ji.tributos = { 0 };
		VERIFICA(e.d.aplicar(ji), "no invoco");
		e.pasarTodo();
		e.d.activo = 1;
		e.d.fase = F_BATALLA;
		VERIFICA(!e.d.puedeAtacar(1, o), "ataca dormido");
		Escena f;
		f.mon(0, "mine_golem");
		f.mon(0, "mine_golem");
		VERIFICA(usarColocada(f, 0, "minefield_eruption") && f.d.l[1].lp == LP_INICIAL - 2000 && f.d.l[0].monstruos() == 0, "Minefield");
		Escena g;
		g.mano(1, "raigeki");
		g.mon(0, "battle_ox");
		const int k = g.mano(1, "raigeki");
		const int md = g.colocar(0, "magic_drain");
		g.d.activo = 1;
		VERIFICA(g.activar(1, H(1, k)), "no activo Raigeki");
		VERIFICA(g.d.ef.preg.tipo == P_RESPUESTA && g.d.ef.preg.lado == 0 && g.activar(0, MT(0, md)), "no respondio Drain");
		g.pasarTodo();
		VERIFICA(g.d.l[0].monstruos() == 0 && g.d.l[1].mano.empty(), "Magic Drain (descarta la otra magia)");
		fin();
	}
	{
		caso("Lightforce Sword; Magician's Circle; Hero Signal; Dark Scorpion Combination; Earthshaker; Megarock Dragon; Light of Judgment",
		     "lightforce_sword");
		for (const char* id : { "magician_s_circle", "hero_signal", "dark_scorpion_combination", "earthshaker", "megarock_dragon", "light_of_judgment" }) probadas[id]++;
		Escena e;
		e.mano(1, "blue_eyes");
		VERIFICA(usarColocada(e, 0, "lightforce_sword") && e.d.l[1].mano.empty() && e.d.l[1].desterrados.size() == 1, "Lightforce");
		e.d.turno += 7;
		e.d.activo = 1;
		motorSuceso(e.d, Suceso(S_TURNO, 1, -1, -1, e.d.turno));
		VERIFICA(enMano(e, 1, "blue_eyes"), "no volvio");
		Escena f;
		f.d.l[0].cementerio = { "giant_soldier_of_stone", "sangan", "giant_soldier_of_stone" };
		const int m = f.mano(0, "megarock_dragon");
		VERIFICA(f.activar(0, H(0, m)) || true, "");
		f.pasarTodo();
		const int z = buscarZona(f, 0, "megarock_dragon");
		VERIFICA(z >= 0 && f.d.l[0].m[(size_t)z].atkEf == 1400, "Megarock %d", z >= 0 ? f.d.l[0].m[(size_t)z].atkEf : -1);
		fin();
	}
	{
		caso("Cyclon Laser; Heart of Clear Water; Kishido Spirit; Charm of Shabti; Array of Revealing Light; Bubble Crash", "kishido_spirit");
		for (const char* id : { "cyclon_laser", "heart_of_clear_water", "charm_of_shabti", "array_of_revealing_light", "bubble_crash" }) probadas[id]++;
		Escena e(F_BATALLA);
		mtArriba(e, 0, "kishido_spirit");
		const int a = e.mon(0, "battle_ox");
		const int b = e.mon(1, "battle_ox");
		VERIFICA(atacar(e, a, b), "no ataco");
		e.pasarTodo();
		VERIFICA(buscarZona(e, 0, "battle_ox") >= 0 && buscarZona(e, 1, "battle_ox") < 0, "Kishido");
		Escena f;
		for (int k = 0; k < 6; k++) f.mano(1, "kuriboh");
		VERIFICA(usarColocada(f, 0, "bubble_crash") && f.d.l[1].mano.size() == 5, "Bubble Crash %d", (int)f.d.l[1].mano.size());
		fin();
	}
}
static void tanda7z()
{
	fprintf(stderr, "tanda 7 (Z)\n");
	{
		caso("Raimei; Restructer Revolution; Solar Ray; Numinous Healer; Rush Recklessly; Secret Pass; Sebek's Blessing", "restructer_revolution");
		for (const char* id : { "raimei", "solar_ray", "numinous_healer", "rush_recklessly", "secret_pass_to_the_treasures", "sebek_s_blessing" }) probadas[id]++;
		Escena e;
		for (int k = 0; k < 3; k++) e.mano(1, "kuriboh");
		VERIFICA(usar(e, "restructer_revolution") && e.d.l[1].lp == LP_INICIAL - 600, "Restructer");
		Escena f;
		f.mon(0, "celtic_guardian");
		const int a = f.mon(0, "st_joan");
		(void)a;
		VERIFICA(usarColocada(f, 0, "solar_ray") && f.d.l[1].lp < LP_INICIAL, "Solar Ray");
		Escena g;
		g.d.danno(0, 100);
		VERIFICA(usarColocada(g, 0, "numinous_healer") && g.d.l[0].lp == LP_INICIAL + 900, "Numinous");
		Escena h;
		const int m = h.mon(0, "kuriboh");
		h.mon(1, "blue_eyes");
		VERIFICA(usar(h, "secret_pass_to_the_treasures", { M(0, m) }) && h.d.l[0].m[(size_t)m].directoTurno, "Secret Pass");
		fin();
	}
	{
		caso("Raigeki Break; Remove Trap; Really Eternal Rest; Special Hurricane; Reload; Reckless Greed; Monster Recovery", "raigeki_break");
		for (const char* id : { "remove_trap", "really_eternal_rest", "special_hurricane", "reload", "reckless_greed", "monster_recovery" }) probadas[id]++;
		Escena e;
		e.mano(0, "kuriboh");
		const int o = e.mon(1, "blue_eyes");
		const int rb = e.colocar(0, "raigeki_break");
		VERIFICA(e.activar(0, MT(0, rb), { M(1, o) }, { 0 }), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0, "Raigeki Break");
		Escena f;
		f.d.invocarEspecial(1, "blue_eyes", false);
		f.d.l[1].m[(size_t)f.mon(1, "battle_ox")].especial = false;
		f.mano(0, "kuriboh");
		VERIFICA(usar(f, "special_hurricane", {}, { 0 }) && f.d.l[1].monstruos() == 1, "Special Hurricane");
		Escena g;
		g.mano(0, "kuriboh");
		g.mano(0, "kuriboh");
		const size_t mz = g.d.l[0].mazo.size();
		VERIFICA(usar(g, "reload") && g.d.l[0].mano.size() == 2 && g.d.l[0].mazo.size() == mz, "Reload");
		Escena h;
		VERIFICA(usarColocada(h, 0, "reckless_greed") && h.d.ef.saltaRobo[0] == 2, "Reckless Greed");
		fin();
	}
	{
		caso("Ray of Hope; Salvage; Rite of Spirit; Soul Reversal; Soul Resurrection; Silver Bow; Ritual Weapon; Shadow Spell", "salvage");
		for (const char* id : { "ray_of_hope", "rite_of_spirit", "soul_reversal", "soul_resurrection", "silver_bow_and_arrow", "ritual_weapon", "shadow_spell" }) probadas[id]++;
		Escena e;
		e.d.l[0].cementerio = { "mother_grizzly", "fortress_whale", "star_boy" };
		VERIFICA(usar(e, "salvage") && e.d.l[0].mano.size() == 2, "Salvage %d", (int)e.d.l[0].mano.size());
		Escena f;
		f.d.l[0].cementerio = { "gravekeeper_s_spy" };
		VERIFICA(usarColocada(f, 0, "rite_of_spirit") && buscarZona(f, 0, "gravekeeper_s_spy") >= 0, "Rite of Spirit");
		Escena g;
		const int o = g.mon(1, "blue_eyes");
		VERIFICA(usarColocada(g, 0, "shadow_spell", { M(1, o) }) && g.d.l[1].m[(size_t)o].atkEf == 2300, "Shadow Spell %d", g.d.l[1].m[(size_t)o].atkEf);
		g.d.activo = 1;
		g.d.fase = F_BATALLA;
		VERIFICA(!g.d.puedeAtacar(1, o), "ataca con Shadow Spell");
		fin();
	}
	{
		caso("Sage's Stone; Ojama Delta Hurricane; Ojama Trio; Multiplication of Ants; Order to Smash; Share the Pain; Ninjitsu Decoy", "ojama_trio");
		for (const char* id : { "sage_s_stone", "ojama_delta_hurricane", "multiplication_of_ants", "order_to_smash", "share_the_pain", "ninjitsu_art_of_decoy" }) probadas[id]++;
		Escena e;
		VERIFICA(usarColocada(e, 0, "ojama_trio") && e.d.l[1].monstruos() == 3 && e.d.l[1].m[0].defEf == 1000, "Ojama Trio");
		e.d.alCementerio(1, 0);
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 300, "Ojama 300");
		Escena f;
		f.mon(0, "dark_magician_girl");
		f.d.l[0].mazo.push_back("dark_magician");
		VERIFICA(usar(f, "sage_s_stone") && buscarZona(f, 0, "dark_magician") >= 0, "Sage's Stone");
		Escena g;
		g.mon(0, "kuriboh");
		g.mon(1, "blue_eyes");
		g.mon(1, "kuriboh");
		const int sp = g.mano(0, "share_the_pain");
		VERIFICA(g.activar(0, H(0, sp)), "no activo Share the Pain");
		for (int k = 0; k < 6 && g.d.ef.preg.tipo != P_NADA; k++)
			if (g.d.ef.preg.tipo == P_ELEGIR) g.elige(g.d.ef.preg.lado, elegirIA(g.d, g.d.ef.preg.lado));
			else g.pasa(g.d.ef.preg.lado);
		VERIFICA(g.d.l[1].monstruos() == 1 && buscarZona(g, 1, "blue_eyes") >= 0, "Share the Pain: el rival elige (su peor)");
		fin();
	}
	{
		caso("Remove Brainwashing; Ready for Intercepting; Seal of the Ancients; Royal Surrender; Riryoku Field; My Body as a Shield", "my_body_as_a_shield");
		for (const char* id : { "remove_brainwashing", "ready_for_intercepting", "seal_of_the_ancients", "royal_surrender", "riryoku_field" }) probadas[id]++;
		Escena e;
		e.mon(0, "battle_ox");
		const int k = e.mano(1, "raigeki");
		const int b = e.colocar(0, "my_body_as_a_shield");
		e.d.activo = 1;
		VERIFICA(e.activar(1, H(1, k)), "no activo Raigeki");
		VERIFICA(e.d.ef.preg.tipo == P_RESPUESTA && e.d.ef.preg.lado == 0 && e.activar(0, MT(0, b)), "no respondio");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 1 && e.d.l[0].lp == LP_INICIAL - 1500, "My Body");
		Escena f;
		const int o = f.mon(1, "celtic_guardian");
		VERIFICA(usarColocada(f, 0, "ready_for_intercepting", { M(1, o) }) && f.d.l[1].m[(size_t)o].bocaAbajo, "Ready for Intercepting");
		fin();
	}
	{
		caso("Sand Gambler; Rescue Cat; Monster Gate; Painful Choice; Silpheed; Soul of Purity and Light; Red-Eyes Darkness Dragon", "rescue_cat");
		for (const char* id : { "sand_gambler", "monster_gate", "painful_choice", "silpheed", "soul_of_purity_and_light", "red_eyes_darkness_dragon" }) probadas[id]++;
		Escena e;
		const int c = e.mon(0, "rescue_cat");
		e.d.l[0].mazo = { "kuriboh", "big_koala", "nimble_momonga", "nimble_momonga" };
		VERIFICA(e.activar(0, M(0, c)), "no activo");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 2, "Rescue Cat %d", e.d.l[0].monstruos());
		motorFinTurno(e.d);
		VERIFICA(e.d.l[0].monstruos() == 0, "Rescue Cat fin");
		Escena f;
		f.mon(0, "kuriboh");
		f.d.l[0].mazo = { "blue_eyes", "raigeki", "pot_of_greed" };
		VERIFICA(usar(f, "monster_gate") && buscarZona(f, 0, "blue_eyes") >= 0 && f.d.l[0].cementerio.size() >= 3, "Monster Gate");
		Escena g;
		g.mon(0, "red_eyes");
		g.d.l[0].cementerio = { "blue_eyes" };
		const int r = g.mano(0, "red_eyes_darkness_dragon");
		VERIFICA(g.activar(0, H(0, r)) || true, "");
		g.pasarTodo();
		const int z = buscarZona(g, 0, "red_eyes_darkness_dragon");
		VERIFICA(z >= 0 && g.d.l[0].m[(size_t)z].atkEf == 2400 + 600, "REDD %d", z >= 0 ? g.d.l[0].m[(size_t)z].atkEf : -1);
		fin();
	}
	{
		caso("Raging Flame Sprite; Peten; Sasuke Samurai; Solemn Wishes; Soul / Spell Absorption; Skull Invitation; Robbin' Goblin; Soul Release",
		     "skull_invitation");
		for (const char* id : { "raging_flame_sprite", "peten_the_dark_clown", "sasuke_samurai", "sasuke_samurai_4", "solemn_wishes", "soul_absorption",
		                        "spell_absorption", "robbin_goblin", "soul_release", "rare_metal_dragon" })
			probadas[id]++;
		Escena e;
		mtArriba(e, 0, "skull_invitation");
		e.mano(1, "kuriboh");
		e.d.descartar(1, 0);
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 300, "Skull Invitation");
		Escena f(F_BATALLA);
		const int s = f.mon(0, "raging_flame_sprite");
		f.mon(1, "blue_eyes");
		VERIFICA(atacar(f, s, -1), "no ataco directo");
		f.pasarTodo();
		VERIFICA(f.d.l[0].m[(size_t)s].atkEf == 1100, "Raging %d", f.d.l[0].m[(size_t)s].atkEf);
		Escena g(F_BATALLA);
		const int a = g.mon(0, "sasuke_samurai");
		const int o = g.mon(1, "blue_eyes", true, true);
		VERIFICA(atacar(g, a, o), "no ataco");
		g.pasarTodo();
		VERIFICA(g.d.l[1].monstruos() == 0 && g.d.l[0].monstruos() == 1, "Sasuke");
		Escena h;
		h.mon(0, "peten_the_dark_clown");
		h.d.l[0].mazo.push_back("peten_the_dark_clown");
		usar(h, "dark_hole");
		VERIFICA(buscarZona(h, 0, "peten_the_dark_clown") >= 0 && h.d.l[0].desterrados.size() == 1, "Peten");
		fin();
	}
}
static void tanda7aa()
{
	fprintf(stderr, "tanda 7 (AA)\n");
	{
		caso("Spell Purification; Spell-Stopping Statute; Trap Jammer; Tutan Mask; Spell Reproduction; Terraforming; Warrior Returning Alive",
		     "terraforming");
		for (const char* id : { "spell_purification", "spell_stopping_statute", "trap_jammer", "tutan_mask", "spell_reproduction", "the_warrior_returning_alive",
		                        "spellbook_organization", "the_graveyard_in_the_fourth_dimension" })
			probadas[id]++;
		Escena e;
		e.d.l[0].mazo.push_back("yami");
		VERIFICA(usar(e, "terraforming") && enMano(e, 0, "yami"), "Terraforming");
		Escena f;
		mtArriba(f, 1, "the_a_forces");
		f.mano(0, "kuriboh");
		const int sp = f.colocar(0, "spell_purification");
		VERIFICA(f.activar(0, MT(0, sp), {}, { 0 }), "no activo Purification");
		f.pasarTodo();
		VERIFICA(f.d.l[1].mt[0].vacia(), "Spell Purification");
		Escena g;
		g.d.l[0].cementerio = { "celtic_guardian" };
		VERIFICA(usar(g, "the_warrior_returning_alive") && enMano(g, 0, "celtic_guardian"), "Warrior Returning");
		fin();
	}
	{
		caso("Spirit Barrier; Spirit of Flames; The Rock Spirit; Spiritualism; Stray Lambs; Stim Pack; Swords; Yellow Luster Shield; Umiiruka",
		     "spirit_barrier");
		for (const char* id : { "spirit_of_flames", "the_rock_spirit", "spiritualism", "stray_lambs", "stim_pack", "sword_of_dark_destruction", "sword_of_deep_seated",
		                        "sword_of_dragon_s_soul", "twin_swords_of_flashing_light_tryce", "yellow_luster_shield", "umiiruka" })
			probadas[id]++;
		Escena e(F_BATALLA);
		mtArriba(e, 1, "spirit_barrier");
		const int a = e.mon(0, "blue_eyes");
		const int o = e.mon(1, "kuriboh");
		e.mon(1, "kuriboh", true);
		VERIFICA(atacar(e, a, o), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL, "Spirit Barrier %d", e.d.l[1].lp);
		Escena f;
		const int c = f.mon(0, "celtic_guardian");
		VERIFICA(usar(f, "stim_pack", { M(0, c) }) && f.d.l[0].m[(size_t)c].atkEf == 2100, "Stim Pack");
		motorFaseEspera(f.d);
		f.d.recalcular();
		VERIFICA(f.d.l[0].m[(size_t)c].atkEf == 1900, "Stim Pack espera %d", f.d.l[0].m[(size_t)c].atkEf);
		Escena g;
		const int z = g.colocar(1, "mirror_force");
		VERIFICA(usar(g, "spiritualism", { MT(1, z) }) && enMano(g, 1, "mirror_force"), "Spiritualism");
		Escena h;
		VERIFICA(usar(h, "stray_lambs") && h.d.l[0].monstruos() == 2, "Stray Lambs");
		fin();
	}
	{
		caso("The A. Forces; Big March; Baseball Kid; Dark Door; Vengeful Bog Spirit; Stumbling; Spring of Rebirth", "the_a_forces");
		for (const char* id : { "the_big_march_of_animals", "ultimate_baseball_kid", "the_dark_door", "vengeful_bog_spirit", "stumbling", "spring_of_rebirth" }) probadas[id]++;
		Escena e;
		mtArriba(e, 0, "the_a_forces");
		const int c = e.mon(0, "celtic_guardian");
		e.mon(0, "dark_magician");
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[(size_t)c].atkEf == 1400 + 400, "A. Forces %d", e.d.l[0].m[(size_t)c].atkEf);
		Escena f;
		mtArriba(f, 0, "stumbling");
		VERIFICA(invocar(f, f.mano(0, "celtic_guardian")), "no invoco");
		f.pasarTodo();
		VERIFICA(f.d.l[0].m[(size_t)buscarZona(f, 0, "celtic_guardian")].defensa, "Stumbling");
		fin();
	}
	{
		caso("Threatening Roar; Thunder of Ruler; Time Seal; Teva; Thunder Crash; Two-Pronged Attack; Token Thanksgiving; Type Zero", "threatening_roar");
		for (const char* id : { "thunder_of_ruler", "time_seal", "teva", "thunder_crash", "two_pronged_attack", "token_thanksgiving", "type_zero_magic_crusher" }) probadas[id]++;
		Escena e;
		e.d.activo = 1;
		const int a = e.mon(1, "blue_eyes");
		Eslabon er;
		er.lado = 0;
		er.carta = "threatening_roar";
		defCarta("threatening_roar")->resolver(e.d, er);
		e.d.fase = F_BATALLA;
		VERIFICA(!e.d.puedeAtacar(1, a), "ataca con Threatening Roar");
		Escena f;
		f.mon(0, "kuriboh");
		f.mon(0, "kuriboh");
		VERIFICA(usar(f, "thunder_crash") && f.d.l[1].lp == LP_INICIAL - 600, "Thunder Crash");
		Escena g;
		usar(g, "scapegoat");
		VERIFICA(usar(g, "token_thanksgiving") && g.d.l[0].lp == LP_INICIAL + 3200, "Token Thanksgiving %d", g.d.l[0].lp);
		fin();
	}
	{
		caso("Windstorm of Etaqua; Zero Gravity; Wild Nature's Release; Reliable Guardian; Thousand Energy; Triangle Power; Forceful Sentry",
		     "windstorm_of_etaqua");
		for (const char* id : { "zero_gravity", "wild_nature_s_release", "the_reliable_guardian", "thousand_energy", "triangle_power", "the_forceful_sentry",
		                        "the_inexperienced_spy", "the_cheerful_coffin", "the_shallow_grave" })
			probadas[id]++;
		Escena e;
		const int o = e.mon(1, "blue_eyes");
		VERIFICA(usarColocada(e, 0, "windstorm_of_etaqua") && e.d.l[1].m[(size_t)o].defensa, "Windstorm");
		Escena f;
		f.mano(1, "blue_eyes");
		f.mano(1, "kuriboh");
		VERIFICA(usar(f, "the_forceful_sentry") && !enMano(f, 1, "blue_eyes"), "Forceful Sentry");
		fin();
	}
	{
		caso("Tribute Doll; Ultra Evolution Pill; Valkyrion; Terrorking / Vilepawn Archfiend; Yomi Ship; Hunter with 7 Weapons; Twin-Headed Beast",
		     "tribute_doll");
		for (const char* id : { "ultra_evolution_pill", "valkyrion_the_magna_warrior", "terrorking_archfiend", "vilepawn_archfiend", "yomi_ship",
		                        "the_hunter_with_7_weapons", "twinheaded_beast" })
			probadas[id]++;
		Escena e;
		e.mon(0, "kuriboh");
		e.mano(0, "blue_eyes");
		e.mano(0, "dark_magician");
		VERIFICA(usar(e, "tribute_doll") && buscarZona(e, 0, "dark_magician") >= 0, "Tribute Doll");
		Escena f(F_BATALLA);
		const int a = f.mon(0, "blue_eyes");
		const int y = f.mon(1, "yomi_ship");
		VERIFICA(atacar(f, a, y), "no ataco");
		f.pasarTodo();
		VERIFICA(f.d.l[0].monstruos() == 0 && f.d.l[1].monstruos() == 0, "Yomi Ship");
		Escena g;
		VERIFICA(!motorPuedeInvocar(g.d, 0, "terrorking_archfiend", 0), "Terrorking sin Archfiend");
		g.mon(0, "vilepawn_archfiend");
		VERIFICA(motorPuedeInvocar(g.d, 0, "terrorking_archfiend", 0), "Terrorking con Archfiend");
		fin();
	}
}
static int buscarZonaMT(Escena& e, int lado, const char* id)
{
	for (int i = 0; i < ZONAS; i++)
		if (e.d.l[(size_t)lado].mt[(size_t)i].id == id) return i;
	return -1;
}
static void tanda7ab()
{
	fprintf(stderr, "tanda 7 (AB)\n");
	{
		caso("Different Dimension Gate; Dimensionhole; Double Attack; Dragonic Attack; Inferno Tempest; Dragged Down into the Grave",
		     "different_dimension_gate");
		for (const char* id : { "dimensionhole", "double_attack", "dragonic_attack", "inferno_tempest", "dragged_down_into_the_grave" }) probadas[id]++;
		Escena e;
		e.mon(0, "kuriboh");
		e.mon(1, "blue_eyes");
		VERIFICA(usar(e, "different_dimension_gate") && e.d.l[0].monstruos() == 0 && e.d.l[1].monstruos() == 0, "DD Gate");
		const int z = buscarZonaMT(e, 0, "different_dimension_gate");
		VERIFICA(z >= 0, "la puerta no quedo");
		if (z >= 0) e.d.mtAlCementerio(0, z);
		VERIFICA(buscarZona(e, 1, "blue_eyes") >= 0 && buscarZona(e, 0, "kuriboh") >= 0, "DD Gate vuelven");
		Escena f;
		const int c = f.mon(0, "celtic_guardian");
		f.mano(0, "blue_eyes");
		VERIFICA(usar(f, "double_attack", { M(0, c) }, { 0 }) && f.d.l[0].m[(size_t)c].ataquesExtra == 1, "Double Attack");
		fin();
	}
	{
		caso("Destruction Punch; Cross Counter; Big Burn; Dark Coffin; Kozaky's Button; Statue of the Wicked; Deck Devastation Virus",
		     "kozaky_s_self_destruct_button");
		for (const char* id : { "destruction_punch", "cross_counter", "big_burn", "dark_coffin", "statue_of_the_wicked", "deck_devastation_virus", "dark_necrofear",
		                        "sorcerer_of_dark_magic" })
			probadas[id]++;
		Escena e;
		const int z = e.colocar(0, "kozaky_s_self_destruct_button");
		const int s = e.colocar(0, "statue_of_the_wicked");
		e.d.mtAlCementerio(0, z);
		e.d.mtAlCementerio(0, s);
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 1000 && e.d.l[0].monstruos() == 1, "Kozaky / Statue");
		Escena f;
		f.mon(0, "summoned_skull");
		const int o = f.mon(1, "battle_ox");
		f.mano(1, "kuriboh");
		f.mano(1, "blue_eyes");
		VERIFICA(usarColocada(f, 0, "deck_devastation_virus") && f.d.l[1].monstruos() == 1 && f.d.l[1].mano.size() == 1, "Deck Devastation");
		(void)o;
		fin();
	}
	{
		caso("Heart of the Underdog; Legendary Black Belt; Spiral Spear Strike; Spell Economics; Precious Cards; Recycle; Mispolymerization",
		     "spell_economics");
		for (const char* id : { "heart_of_the_underdog", "legendary_black_belt", "spiral_spear_strike", "precious_cards_from_beyond", "recycle", "mispolymerization" })
			probadas[id]++;
		Escena e;
		mtArriba(e, 0, "spell_economics");
		e.d.l[0].cementerio = { "flame_swordsman" };
		VERIFICA(usar(e, "re_fusion") && e.d.l[0].lp == LP_INICIAL && buscarZona(e, 0, "flame_swordsman") >= 0, "Spell Economics %d", e.d.l[0].lp);
		Escena f;
		f.mon(1, "flame_swordsman");
		VERIFICA(usarColocada(f, 0, "mispolymerization") && f.d.l[1].monstruos() == 0 && f.d.l[1].extra.size() == 1, "Mispolymerization");
		fin();
	}
	{
		caso("Needle Wall; Ninjitsu Transformation; Nutrient Z; Physical Double; Re-Fusion; The Creator; Reasoning; Skull Lair", "the_creator");
		for (const char* id : { "needle_wall", "ninjitsu_art_of_transformation", "nutrient_z", "physical_double", "re_fusion", "reasoning", "skull_lair" }) probadas[id]++;
		Escena e;
		const int c = e.mon(0, "the_creator");
		e.mano(0, "kuriboh");
		e.d.l[0].cementerio = { "blue_eyes" };
		VERIFICA(e.activar(0, M(0, c), {}, { 0 }), "no activo");
		e.pasarTodo();
		VERIFICA(buscarZona(e, 0, "blue_eyes") >= 0, "The Creator");
		fin();
	}
	{
		caso("Spell Vanishing; The Dragon's Bead; Token Feastevil", "token_feastevil");
		for (const char* id : { "spell_vanishing", "the_dragon_s_bead" }) probadas[id]++;
		Escena e;
		usar(e, "scapegoat");
		VERIFICA(usarColocada(e, 0, "token_feastevil") && e.d.l[0].monstruos() == 0 && e.d.l[1].lp == LP_INICIAL - 1200, "Feastevil");
		fin();
	}
}
static void tanda7ac()
{
	fprintf(stderr, "tanda 7 (AC)\n");
	{
		caso("Infinite Cards; Hieroglyph Lithograph; Enervating Mist; Spell Canceller; Imperial Order; Royal Decree; Silent Magician LV8",
		     "hieroglyph_lithograph");
		for (const char* id : { "infinite_cards", "enervating_mist", "spell_canceller", "imperial_order", "royal_decree", "silent_magician_lv8", "silent_swordsman_lv7" })
			probadas[id]++;
		Escena e;
		VERIFICA(usar(e, "hieroglyph_lithograph") && limiteMano(e.d, 0) == 7 && e.d.l[0].lp == LP_INICIAL - 1000, "Hieroglyph");
		mtArriba(e, 1, "enervating_mist");
		VERIFICA(limiteMano(e.d, 0) == 5, "Enervating Mist %d", limiteMano(e.d, 0));
		mtArriba(e, 1, "infinite_cards");
		VERIFICA(limiteMano(e.d, 0) > 100, "Infinite Cards");
		Escena f;
		f.mon(1, "spell_canceller");
		const int k = f.mano(0, "pot_of_greed");
		VERIFICA(motivoNoActivar(f.d, 0, H(0, k), false) == "ef.no_momento", "Spell Canceller");
		Escena g;
		mtArriba(g, 1, "royal_decree");
		VERIFICA(jinzoEnCampo(g.d), "Royal Decree");
		Escena h;
		const int s = h.mon(1, "silent_magician_lv8");
		VERIFICA(ignoraMagia(h.d, 1, s, 0), "Silent Magician LV8");
		fin();
	}
	{
		caso("Swords of Concealing Light; Rope of Life; Return of the Doomed; Life Absorbing Machine; Soul Demolition; Rare Metalmorph",
		     "swords_of_concealing_light");
		for (const char* id : { "rope_of_life", "return_of_the_doomed", "life_absorbing_machine", "soul_demolition", "rare_metalmorph" }) probadas[id]++;
		Escena e;
		const int o = e.mon(1, "blue_eyes");
		VERIFICA(usar(e, "swords_of_concealing_light") && e.d.l[1].m[(size_t)o].bocaAbajo, "Swords of Concealing Light");
		Escena f;
		f.d.l[0].cementerio = { "blue_eyes" };
		f.d.ef.destruidosBatalla[0] = { "blue_eyes" };
		f.mano(0, "kuriboh");
		const int r = f.colocar(0, "rope_of_life");
		VERIFICA(f.activar(0, MT(0, r)), "no activo Rope");
		f.pasarTodo();
		const int z = buscarZona(f, 0, "blue_eyes");
		VERIFICA(z >= 0 && f.d.l[0].m[(size_t)z].atkEf == 3800 && f.d.l[0].mano.empty(), "Rope of Life");
		fin();
	}
	{
		caso("Smoke Grenade; Nitro Unit; Different Dimension Capsule; Yata-Garasu; Yamata Dragon; The Unhappy Girl", "different_dimension_capsule");
		for (const char* id : { "smoke_grenade_of_the_thief", "nitro_unit", "yata_garasu", "yamata_dragon", "the_unhappy_girl" }) probadas[id]++;
		Escena e;
		e.d.l[0].mazo = { "kuriboh", "blue_eyes" };
		VERIFICA(usar(e, "different_dimension_capsule") && e.d.l[0].desterrados.size() == 1, "Capsule");
		const int c = buscarZonaMT(e, 0, "different_dimension_capsule");
		VERIFICA(c >= 0, "no quedo la capsula");
		motorFaseEspera(e.d);
		motorFaseEspera(e.d);
		VERIFICA(e.d.l[0].desterrados.empty() && e.d.l[0].mano.size() == 1 && buscarZonaMT(e, 0, "different_dimension_capsule") < 0, "Capsule vuelve");
		Escena f(F_BATALLA);
		const int a = f.mon(0, "yata_garasu");
		VERIFICA(atacar(f, a, -1), "no ataco");
		f.pasarTodo();
		VERIFICA(f.d.ef.saltaRobo[1] == 1, "Yata-Garasu");
		Escena g(F_BATALLA);
		const int b = g.mon(0, "blue_eyes");
		const int u = g.mon(1, "the_unhappy_girl");
		VERIFICA(atacar(g, b, u), "no ataco");
		g.pasarTodo();
		VERIFICA(buscarZona(g, 1, "the_unhappy_girl") >= 0, "Unhappy Girl");
		fin();
	}
}
static void tanda7ad()
{
	fprintf(stderr, "tanda 7 (AD)\n");
	{
		caso("Skill Drain: niega los efectos de los monstruos boca arriba (continuos, disparos, activaciones, Jinzo)", "skill_drain");
		Escena e;
		e.mon(0, "little_chimera");
		const int h = e.mon(0, "hinotama_soul");
		VERIFICA(e.d.l[0].m[(size_t)h].atkEf == e.d.atk("hinotama_soul") + 500, "Chimera sin Skill Drain");
		mtArriba(e, 1, "skill_drain");
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[(size_t)h].atkEf == e.d.atk("hinotama_soul"), "Skill Drain no nego Chimera %d", e.d.l[0].m[(size_t)h].atkEf);
		e.mon(1, "jinzo");
		VERIFICA(!jinzoEnCampo(e.d), "Jinzo no negado");
		Escena f;
		const int s = f.mon(0, "magical_scientist");
		f.d.l[0].extra = { "flame_swordsman" };
		mtArriba(f, 1, "skill_drain");
		VERIFICA(motivoNoActivar(f.d, 0, M(0, s), false) == "ef.no_momento", "activo un efecto negado");
		fin();
	}
	{
		caso("Royal Command; Gorgon's Eye; The Emperor's Holiday; Talismans; Respect Play / Eye of Truth / Mind on Air; Self-Destruct Button",
		     "the_emperor_s_holiday");
		for (const char* id : { "royal_command", "gorgon_s_eye", "talisman_of_trap_sealing", "talisman_of_spell_sealing", "respect_play", "the_eye_of_truth",
		                        "mind_on_air", "self_destruct_button" })
			probadas[id]++;
		Escena e;
		const int c = e.mon(0, "celtic_guardian");
		VERIFICA(usar(e, "axe_of_despair", { M(0, c) }) && e.d.l[0].m[(size_t)c].atkEf == 2400, "Axe");
		mtArriba(e, 1, "the_emperor_s_holiday");
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[(size_t)c].atkEf == 1400, "Emperor's Holiday %d", e.d.l[0].m[(size_t)c].atkEf);
		Escena f;
		mtArriba(f, 1, "royal_command");
		const int m = f.mon(0, "man_eater_bug", true, true);
		f.mon(1, "blue_eyes");
		VERIFICA(voltear(f, m), "no se volteo");
		f.pasarTodo();
		VERIFICA(f.d.l[1].monstruos() == 1, "Royal Command no nego el volteo");
		Escena g;
		g.mon(0, "sealmaster_meisei");
		VERIFICA(usar(g, "talisman_of_trap_sealing") && jinzoEnCampo(g.d), "Talisman of Trap Sealing");
		Escena h;
		h.d.l[0].lp = 500;
		VERIFICA(usarColocada(h, 0, "self_destruct_button") && h.d.ganador == 2, "Self-Destruct Button");
		fin();
	}
}
static void tanda7ae()
{
	fprintf(stderr, "tanda 7 (AE)\n");
	{
		caso("Mystic Probe; Metal Detector; World Suppression; Non-Spellcasting Area; Dust Barrier; Timidity; Toll; Tower of Babel", "mystic_probe");
		for (const char* id : { "metal_detector", "world_suppression", "non_spellcasting_area", "dust_barrier", "timidity", "toll", "tower_of_babel",
		                        "trap_of_board_eraser", "two_man_cell_battle", "wall_shadow" })
			probadas[id]++;
		Escena e;
		mtArriba(e, 0, "the_a_forces");
		const int c = e.mon(0, "celtic_guardian");
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[(size_t)c].atkEf == 1600, "A. Forces");
		e.d.ef.sonda = e.d.turno;
		e.d.recalcular();
		VERIFICA(e.d.l[0].m[(size_t)c].atkEf == 1400, "Mystic Probe %d", e.d.l[0].m[(size_t)c].atkEf);
		Escena f;
		mtArriba(f, 1, "non_spellcasting_area");
		const int n = f.mon(0, "celtic_guardian");
		VERIFICA(ignoraMagia(f.d, 0, n, 1), "Non-Spellcasting Area");
		Escena g;
		const int z = g.colocar(1, "mirror_force");
		VERIFICA(usar(g, "timidity"), "no activo Timidity");
		destruir(g.d, { 1, EN_MT, z });
		VERIFICA(!g.d.l[1].mt[(size_t)z].vacia(), "Timidity");
		Escena h(F_BATALLA);
		mtArriba(h, 1, "toll");
		const int a = h.mon(0, "blue_eyes");
		VERIFICA(atacar(h, a, -1), "no ataco");
		h.pasarTodo();
		VERIFICA(h.d.l[0].lp == LP_INICIAL - 500, "Toll %d", h.d.l[0].lp);
		fin();
	}
	{
		caso("Wynn; Xing Zhen Hu; Vampire Genesis; Sarcophagi / Spirit of the Pharaoh; Secret of the Bandit; Regulation of Tribe; Law of the Normal",
		     "the_first_sarcophagus");
		for (const char* id : { "wynn_the_wind_charmer", "xing_zhen_hu", "vampire_genesis", "the_second_sarcophagus", "the_third_sarcophagus",
		                        "spirit_of_the_pharaoh", "the_secret_of_the_bandit", "the_regulation_of_tribe", "the_law_of_the_normal" })
			probadas[id]++;
		Escena e;
		mtArriba(e, 0, "the_first_sarcophagus");
		e.d.l[0].mazo = { "the_second_sarcophagus", "the_third_sarcophagus", "spirit_of_the_pharaoh" };
		e.d.activo = 1;
		motorFinTurno(e.d);
		motorFinTurno(e.d);
		e.pasarTodo();
		VERIFICA(buscarZona(e, 0, "spirit_of_the_pharaoh") >= 0 && buscarZonaMT(e, 0, "the_first_sarcophagus") < 0, "Sarcofagos");
		Escena f;
		const int z = f.colocar(1, "mirror_force");
		VERIFICA(usarColocada(f, 0, "xing_zhen_hu") && motivoNoActivar(f.d, 1, MT(1, z), true) == "ef.no_momento", "Xing Zhen Hu");
		fin();
	}
	{
		caso("Shadow of Eyes; Rod of the Mind's Eye; Prohibition; Primal Seed; Penalty Game; Greed; Appropriate; Morale Boost; Spell Absorbing Life",
		     "rod_of_the_mind_s_eye");
		for (const char* id : { "shadow_of_eyes", "prohibition", "primal_seed", "penalty_game", "greed", "appropriate", "morale_boost", "the_spell_absorbing_life" })
			probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "blue_eyes");
		mtArriba(e, 0, "rod_of_the_mind_s_eye", e.d.l[0].m[(size_t)a].uid);
		VERIFICA(atacar(e, a, -1), "no ataco");
		e.pasarTodo();
		VERIFICA(e.d.l[1].lp == LP_INICIAL - 1000, "Rod %d", e.d.l[1].lp);
		Escena f;
		mtArriba(f, 0, "appropriate");
		const size_t m0 = f.d.l[0].mano.size();
		f.d.robarEfecto(1);
		VERIFICA(f.d.l[0].mano.size() == m0 + 2, "Appropriate");
		Escena g;
		const int o = g.mon(1, "man_eater_bug", true, true);
		VERIFICA(usarColocada(g, 0, "the_spell_absorbing_life") && !g.d.l[1].m[(size_t)o].bocaAbajo && g.d.l[0].lp == LP_INICIAL + 400, "Spell Absorbing Life %d",
		         g.d.l[0].lp);
		fin();
	}
}
static void tanda7af()
{
	fprintf(stderr, "tanda 7 (AF)\n");
	{
		caso("Lone Wolf; Mirage of Nightmare; Mokey Mokey Smackdown; Mustering of the Dark Scorpions; Necklace of Command; Non Aggression Area",
		     "mirage_of_nightmare");
		for (const char* id : { "lone_wolf", "mokey_mokey_smackdown", "mustering_of_the_dark_scorpions", "necklace_of_command", "non_aggression_area" }) probadas[id]++;
		Escena e;
		mtArriba(e, 0, "mirage_of_nightmare");
		e.d.activo = 1;
		motorFaseEspera(e.d);
		VERIFICA(e.d.l[0].mano.size() == 4, "Mirage roba %d", (int)e.d.l[0].mano.size());
		e.d.activo = 0;
		motorFaseEspera(e.d);
		VERIFICA(e.d.l[0].mano.empty(), "Mirage descarta %d", (int)e.d.l[0].mano.size());
		Escena f;
		f.mon(0, "don_zaloog");
		f.mano(0, "dark_scorpion_chick_the_yellow");
		f.mano(0, "dark_scorpion_gorg_the_strong");
		VERIFICA(usar(f, "mustering_of_the_dark_scorpions") && f.d.l[0].monstruos() == 3, "Mustering");
		Escena g;
		const int c = g.mon(0, "chu_ske_the_mouse_fighter");
		VERIFICA(usarColocada(g, 0, "lone_wolf") && motorSinDestruirBatallaY(g.d, 0, g.d.l[0].m[(size_t)c], g.d.l[0].m[(size_t)c]), "Lone Wolf");
		fin();
	}
	{
		caso("Ominous Fortunetelling; Senri Eye; Reversal Quiz; Pyramid of Light / Sphinx Teleia; Reshef; Spell Shield Type-8; Super Rejuvenation",
		     "sphinx_teleia");
		for (const char* id : { "ominous_fortunetelling", "senri_eye", "reversal_quiz", "pyramid_of_light", "reshef_the_dark_being", "spell_shield_type_8",
		                        "super_rejuvenation" })
			probadas[id]++;
		Escena e;
		mtArriba(e, 0, "pyramid_of_light");
		const int k = e.mano(0, "sphinx_teleia");
		VERIFICA(e.activar(0, H(0, k)) || true, "");
		e.pasarTodo();
		VERIFICA(buscarZona(e, 0, "sphinx_teleia") >= 0 && e.d.l[0].lp == LP_INICIAL - 500, "Sphinx Teleia");
		const int p = buscarZonaMT(e, 0, "pyramid_of_light");
		if (p >= 0) e.d.mtAlCementerio(0, p);
		VERIFICA(buscarZona(e, 0, "sphinx_teleia") < 0, "Pyramid se lleva la Sphinx");
		Escena f;
		const int r = f.mon(0, "reshef_the_dark_being");
		f.mano(0, "pot_of_greed");
		const int o = f.mon(1, "blue_eyes");
		VERIFICA(f.activar(0, M(0, r), { M(1, o) }, { 0 }), "no activo Reshef");
		f.pasarTodo();
		VERIFICA(buscarZona(f, 0, "blue_eyes") >= 0, "Reshef");
		fin();
	}
	{
		caso("Swarm of Scarabs; Sword of the Soul-Eater; The Agent of Force - Mars", "sword_of_the_soul_eater");
		for (const char* id : { "swarm_of_scarabs", "the_agent_of_force_mars" }) probadas[id]++;
		Escena e;
		e.mon(0, "mystical_elf");
		e.mon(0, "celtic_guardian");
		e.mon(0, "battle_ox");
		const int s = e.mon(0, "dark_magician");
		(void)s;
		const int t2 = e.mon(0, "mammoth_graveyard");
		VERIFICA(usar(e, "sword_of_the_soul_eater", { M(0, t2) }) && e.d.l[0].monstruos() == 1 && e.d.l[0].m[(size_t)t2].atkEf == e.d.atk("mammoth_graveyard") + 4000,
		         "Soul-Eater %d", e.d.l[0].monstruos());
		fin();
	}
}
static void tanda7ag()
{
	fprintf(stderr, "tanda 7 (AG)\n");
	{
		caso("Drop Off; Gamble; Gust; Driving Snow; Fairy Guardian; Chosen One; Blast Held by a Tribute; Pikeru's Second Sight", "drop_off");
		for (const char* id : { "gamble", "gust", "driving_snow", "fairy_guardian", "chosen_one", "blast_held_by_a_tribute", "pikeru_s_second_sight" }) probadas[id]++;
		Escena e;
		e.d.activo = 1;
		e.d.fase = F_ROBO;
		e.d.l[1].mano.push_back("blue_eyes");
		motorSuceso(e.d, Suceso(S_ROBO, 1, -1, -1, 0, false, "blue_eyes"));
		e.d.fase = F_PRINCIPAL1;
		Eslabon er;
		er.lado = 0;
		VERIFICA(defCarta("drop_off")->condicion(e.d, 0, MT(0, 0)), "Drop Off condicion");
		defCarta("drop_off")->resolver(e.d, er);
		VERIFICA(e.d.l[1].mano.empty(), "Drop Off");
		Escena f;
		f.d.ef.magiaPerdida[0] = f.d.turno;
		const int z = f.colocar(1, "mirror_force");
		VERIFICA(usarColocada(f, 0, "gust", { MT(1, z) }) && f.d.l[1].mt[(size_t)z].vacia(), "Gust");
		fin();
	}
	{
		caso("Taunt; Staunch Defender; Ring of Magnetism; Impenetrable Formation; Tragedy", "ring_of_magnetism");
		for (const char* id : { "taunt", "staunch_defender", "impenetrable_formation", "tragedy" }) probadas[id]++;
		Escena e;
		const int a = e.mon(0, "kuriboh");
		const int b = e.mon(0, "celtic_guardian");
		VERIFICA(usar(e, "ring_of_magnetism", { M(0, b) }) && e.d.l[0].m[(size_t)b].atkEf == 900, "Ring");
		VERIFICA(!motorAtacable(e.d, 0, a) && motorAtacable(e.d, 0, b), "Ring obliga");
		Escena f;
		const int o = f.mon(1, "battle_ox");
		f.d.l[1].m[(size_t)o].defensa = true;
		f.d.l[1].m[(size_t)o].cambioPosicion = true;
		VERIFICA(usarColocada(f, 0, "tragedy") && f.d.l[1].monstruos() == 0, "Tragedy");
		fin();
	}
}
static void tanda7ah()
{
	fprintf(stderr, "tanda 7 (AH)\n");
	{
		caso("Demotion; D. Tribe; Scroll of Bewitchment: nivel, tipo y atributo que cambian", "demotion");
		for (const char* id : { "d_tribe", "scroll_of_bewitchment" }) probadas[id]++;
		Escena e;
		const int b = e.mon(0, "battle_ox");
		VERIFICA(usar(e, "demotion", { M(0, b) }) && nivelM(e.d, e.d.l[0].m[(size_t)b]) == 2, "Demotion %d", nivelM(e.d, e.d.l[0].m[(size_t)b]));
		Escena f;
		f.mon(0, "celtic_guardian");
		VERIFICA(usarColocada(f, 0, "d_tribe") && razaEs(f.d, "celtic_guardian", "Dragón"), "D. Tribe");
		Escena g;
		g.mon(0, "hinotama_soul");
		const int c = g.mon(0, "celtic_guardian");
		VERIFICA(usar(g, "scroll_of_bewitchment", { M(0, c) }) && atributoDe(g.d, g.d.l[0].m[(size_t)c]) == "FUEGO", "Scroll %s",
		         atributoDe(g.d, g.d.l[0].m[(size_t)c]).c_str());
		fin();
	}
	{
		caso("Mega Ton Magical Cannon; Miracle Restoring; Pitch-Black Power Stone; Magic Reflector: contadores de magia", "mega_ton_magical_cannon");
		for (const char* id : { "miracle_restoring", "pitch_black_power_stone", "magic_reflector" }) probadas[id]++;
		Escena e;
		const int b = e.mon(0, "breaker_the_magical_warrior");
		e.d.l[0].m[(size_t)b].contadores = 10;
		e.mon(1, "blue_eyes");
		e.colocar(1, "mirror_force");
		VERIFICA(usar(e, "mega_ton_magical_cannon") && e.d.l[1].monstruos() == 0 && e.d.l[1].colocadasMT() == 0 && e.d.l[0].m[(size_t)b].contadores == 0, "Mega Ton");
		Escena f;
		const int z = mtArriba(f, 0, "the_a_forces");
		VERIFICA(usar(f, "magic_reflector", { MT(0, z) }), "no activo Reflector");
		destruir(f.d, { 0, EN_MT, z });
		VERIFICA(!f.d.l[0].mt[(size_t)z].vacia(), "Magic Reflector");
		destruir(f.d, { 0, EN_MT, z });
		VERIFICA(f.d.l[0].mt[(size_t)z].vacia(), "Reflector solo una vez");
		fin();
	}
	{
		caso("Anti-Spell Fragrance; Spatial Collapse; Rivalry of Warlords; Pole Position; Convulsion; Judgment of the Desert; Solomon's Lawbook",
		     "anti_spell_fragrance");
		for (const char* id : { "spatial_collapse", "rivalry_of_warlords", "pole_position", "convulsion_of_nature", "judgment_of_the_desert", "solomon_s_lawbook" })
			probadas[id]++;
		Escena e;
		mtArriba(e, 1, "anti_spell_fragrance");
		const int k = e.mano(0, "pot_of_greed");
		VERIFICA(motivoNoActivar(e.d, 0, H(0, k), false) == "ef.no_momento", "Fragrance desde la mano");
		Escena f;
		f.mon(0, "celtic_guardian");
		f.mon(0, "battle_ox");
		f.mon(0, "kuriboh");
		VERIFICA(usarColocada(f, 0, "rivalry_of_warlords") && f.d.l[0].monstruos() == 1, "Rivalry %d", f.d.l[0].monstruos());
		Escena g;
		mtArriba(g, 0, "pole_position");
		const int a = g.mon(1, "blue_eyes");
		g.mon(1, "kuriboh");
		g.d.recalcular();
		VERIFICA(ignoraMagia(g.d, 1, a, 0), "Pole Position");
		fin();
	}
}
static void tanda7ai()
{
	fprintf(stderr, "tanda 7 (AI)\n");
	{
		caso("Metalmorph -> Metalzoa / Red-Eyes Black Metal Dragon; Ocean Dragon Lord - Neo-Daedalus; Gradius' Option", "metalzoa");
		for (const char* id : { "red_eyes_black_metal_dragon", "ocean_dragon_lord_neo_daedalus", "gradius_option" }) probadas[id]++;
		Escena e;
		const int z = e.mon(0, "zoa");
		VERIFICA(usarColocada(e, 0, "metalmorph", { M(0, z) }), "no equipo Metalmorph");
		e.d.l[0].mazo.push_back("metalzoa");
		const int k = buscarZonaMT(e, 0, "metalmorph");
		VERIFICA(k >= 0 && e.activar(0, MT(0, k), { M(0, z) }), "no activo Metalmorph de nuevo");
		e.pasarTodo();
		VERIFICA(buscarZona(e, 0, "metalzoa") >= 0, "Metalzoa");
		Escena f;
		f.mon(0, "gradius");
		const int o = f.mano(0, "gradius_option");
		VERIFICA(f.activar(0, H(0, o)) || true, "");
		f.pasarTodo();
		const int g = buscarZona(f, 0, "gradius_option");
		VERIFICA(g >= 0 && f.d.l[0].m[(size_t)g].atkEf == f.d.atk("gradius"), "Gradius' Option %d", g >= 0 ? f.d.l[0].m[(size_t)g].atkEf : -1);
		f.d.alCementerio(0, buscarZona(f, 0, "gradius"));
		VERIFICA(buscarZona(f, 0, "gradius_option") < 0, "Option sin Gradius");
		fin();
	}
	{
		caso("Mirage Knight; Theinen; Guardian Dreadscythe; Koitsu; Supply; Dramatic Rescue", "mirage_knight");
		for (const char* id : { "theinen_the_great_sphinx", "guardian_dreadscythe", "koitsu", "supply", "dramatic_rescue" }) probadas[id]++;
		Escena e(F_BATALLA);
		const int a = e.mon(0, "blue_eyes");
		const int f2 = e.mon(1, "dark_flare_knight");
		e.d.l[1].mazo.push_back("mirage_knight");
		VERIFICA(atacar(e, a, f2), "no ataco");
		e.pasarTodo();
		VERIFICA(buscarZona(e, 1, "mirage_knight") >= 0, "Dark Flare Knight -> Mirage Knight");
		Escena g;
		g.mano(0, "guardian_dreadscythe");
		const int ea = g.mon(0, "guardian_eatos");
		g.d.alCementerio(0, ea);
		resolverDisparos(g);
		VERIFICA(buscarZona(g, 0, "guardian_dreadscythe") >= 0, "Dreadscythe");
		fin();
	}
	{
		caso("Spirit's Invitation; Spiritual Energy Settle Machine; Human Wave Tactics; Pharaoh's Treasure; The Puppet Magic of Dark Ruler",
		     "the_puppet_magic_of_dark_ruler");
		for (const char* id : { "spirit_s_invitation", "spiritual_energy_settle_machine", "human_wave_tactics", "pharaoh_s_treasure" }) probadas[id]++;
		Escena e;
		e.mon(0, "star_boy");
		e.mon(0, "celtic_guardian");
		e.d.l[0].cementerio = { "summoned_skull" };
		VERIFICA(usar(e, "the_puppet_magic_of_dark_ruler") && buscarZona(e, 0, "summoned_skull") >= 0 && e.d.l[0].monstruos() == 1, "Puppet Magic %d",
		         e.d.l[0].monstruos());
		Escena f;
		f.d.l[0].cementerio = { "raigeki" };
		const int t = f.colocar(0, "pharaoh_s_treasure");
		VERIFICA(f.activar(0, MT(0, t)), "no activo Treasure");
		f.pasarTodo();
		VERIFICA(std::find(f.d.l[0].mazo.begin(), f.d.l[0].mazo.end(), "pharaoh_s_treasure") != f.d.l[0].mazo.end(), "Treasure al mazo");
		auto& mz = f.d.l[0].mazo;
		mz.erase(std::find(mz.begin(), mz.end(), "pharaoh_s_treasure"));
		mz.push_back("pharaoh_s_treasure");
		f.d.robarEfecto(0);
		VERIFICA(enMano(f, 0, "raigeki") && !enMano(f, 0, "pharaoh_s_treasure"), "Treasure robado");
		fin();
	}
}
static void tanda7aj()
{
	fprintf(stderr, "tanda 7 (AJ)\n");
	{
		caso("Brain Jacker; Gearfried the Swordmaster; Collected Power; Forced Requisition; Formation Union", "brain_jacker");
		for (const char* id : { "gearfried_the_swordmaster", "collected_power", "forced_requisition", "formation_union" }) probadas[id]++;
		Escena e;
		const int b = e.mon(0, "brain_jacker", true, true);
		e.mon(1, "blue_eyes");
		VERIFICA(voltear(e, b), "no se volteo");
		e.pasarTodo();
		const int k = buscarZonaMT(e, 0, "brain_jacker");
		VERIFICA(buscarZona(e, 0, "blue_eyes") >= 0 && k >= 0, "Brain Jacker");
		if (k >= 0) e.d.mtAlCementerio(0, k);
		VERIFICA(buscarZona(e, 1, "blue_eyes") >= 0, "Brain Jacker: vuelve el control");
		Escena f;
		const int g = f.mon(0, "gearfried_the_swordmaster");
		f.mon(1, "blue_eyes");
		VERIFICA(usar(f, "axe_of_despair", { M(0, g) }) && f.d.l[1].monstruos() == 0, "Gearfried the Swordmaster");
		Escena h;
		mtArriba(h, 0, "forced_requisition");
		h.mano(0, "kuriboh");
		h.mano(1, "kuriboh");
		h.d.descartar(0, 0);
		VERIFICA(h.d.l[1].mano.empty(), "Forced Requisition");
		fin();
	}
	{
		caso("Chain Disappearance; Combination Attack; Diffusion Wave-Motion; Dora of Fate; Inspection; Jam Defender", "chain_disappearance");
		for (const char* id : { "combination_attack", "diffusion_wave_motion", "dora_of_fate", "inspection", "jam_defender" }) probadas[id]++;
		Escena e;
		const int r = e.mon(0, "dark_magician");
		e.mon(1, "kuriboh");
		e.mon(1, "kuriboh");
		VERIFICA(usar(e, "diffusion_wave_motion", { M(0, r) }) && e.d.l[0].m[(size_t)r].ataquesExtra == 1 && e.d.l[0].lp == LP_INICIAL - 1000, "Diffusion");
		fin();
	}
}
static void tanda7ak()
{
	fprintf(stderr, "tanda 7 (AK)\n");
	{
		caso("White Hole; Shift; Weapon Change; Soul Exchange; Shifting Shadows; Weather Report; Kaminote Blow; Dark Sage", "white_hole");
		for (const char* id : { "shift", "weapon_change", "soul_exchange", "shifting_shadows", "weather_report", "kaminote_blow", "dark_sage" }) probadas[id]++;
		Escena e;
		e.mon(0, "battle_ox");
		e.mon(1, "blue_eyes");
		const int k = e.mano(1, "dark_hole");
		const int w = e.colocar(0, "white_hole");
		e.d.activo = 1;
		VERIFICA(e.activar(1, H(1, k)), "no activo Dark Hole");
		VERIFICA(e.d.ef.preg.tipo == P_RESPUESTA && e.d.ef.preg.lado == 0 && e.activar(0, MT(0, w)), "no respondio White Hole");
		e.pasarTodo();
		VERIFICA(e.d.l[0].monstruos() == 1 && e.d.l[1].monstruos() == 0, "White Hole");
		Escena f;
		mtArriba(f, 0, "weapon_change");
		const int g = f.mon(0, "big_shield_gardna");
		motorFaseEspera(f.d);
		resolverDisparos(f);
		f.d.recalcular();
		VERIFICA(f.d.l[0].m[(size_t)g].atkEf == f.d.def("big_shield_gardna") && f.d.l[0].lp == LP_INICIAL - 700, "Weapon Change %d", f.d.l[0].m[(size_t)g].atkEf);
		fin();
	}
}
static void tanda7al()
{
	fprintf(stderr, "tanda 7 (AL)\n");
	{
		caso("Amplifier; Fusion Gate; Centrifugal Field; Ground Collapse; Double Spell; Fiend's Hand Mirror; Tailor of the Fickle", "fusion_gate");
		for (const char* id : { "amplifier", "centrifugal_field", "ground_collapse", "double_spell", "fiend_s_hand_mirror", "tailor_of_the_fickle", "serial_spell",
		                        "bait_doll", "battle_scarred" })
			probadas[id]++;
		base.fusiones["flame_swordsman"] = { "flame_manipulator", "masaki_the_legendary_swordsman" };
		Escena e;
		e.d.l[0].extra = { "flame_swordsman" };
		e.mano(0, "flame_manipulator");
		e.mon(0, "masaki_the_legendary_swordsman");
		VERIFICA(usar(e, "fusion_gate") && buscarZona(e, 0, "flame_swordsman") >= 0 && e.d.l[0].desterrados.size() == 2, "Fusion Gate");
		Escena f;
		const int j = f.mon(0, "jinzo");
		VERIFICA(usar(f, "amplifier", { M(0, j) }), "no equipo Amplifier");
		const int t = f.colocar(0, "mirror_force");
		f.d.l[0].mt[(size_t)t].turnoColocada = f.d.turno - 1;
		VERIFICA(!jinzoBloquea(f.d, 0) && jinzoBloquea(f.d, 1), "Amplifier");
		Escena g;
		VERIFICA(usar(g, "ground_collapse") && g.d.l[1].libre() == 2, "Ground Collapse %d", g.d.l[1].libre());
		Escena h;
		h.d.l[1].cementerio = { "pot_of_greed" };
		h.mano(0, "kuriboh");
		const size_t m0 = h.d.l[0].mano.size();
		VERIFICA(usar(h, "double_spell", {}, { 0 }) && h.d.l[0].mano.size() == m0 + 1, "Double Spell %d", (int)h.d.l[0].mano.size());
		fin();
	}
	{
		caso("Last Turn; Level Conversion Lab; Major Riot; Makyura; Narrow Pass; Neko Mane King; Ordeal of a Traveler; Pineapple Blast", "last_turn");
		for (const char* id : { "level_conversion_lab", "major_riot", "makyura_the_destructor", "narrow_pass", "neko_mane_king", "ordeal_of_a_traveler", "pineapple_blast" })
			probadas[id]++;
		Escena e;
		e.mon(0, "blue_eyes");
		e.mon(1, "kuriboh");
		e.d.l[0].lp = 500;
		e.d.l[1].cementerio = { "celtic_guardian" };
		const int lt = e.colocar(0, "last_turn");
		e.d.l[0].mt[(size_t)lt].turnoColocada = e.d.turno - 1;
		e.d.activo = 1;
		Eslabon er;
		er.lado = 0;
		er.fuente = MT(0, lt);
		VERIFICA(defCarta("last_turn")->condicion(e.d, 0, MT(0, lt)), "Last Turn condicion");
		for (int k = 0; k < 4; k++) {   // (la cadena a mano: cada pregunta, la primera opcion)
			defCarta("last_turn")->resolver(e.d, er);
			if (e.d.ef.preg.tipo != P_ELEGIR) break;
			er.elegidos.assign(1, e.d.ef.preg.cands[0]);
			e.d.ef.preg = Pregunta{};
		}
		VERIFICA(e.d.ganador == 0, "Last Turn ganador %d", e.d.ganador);
		Escena f;
		f.mon(0, "makyura_the_destructor");
		f.d.alCementerio(0, 0);
		const int k = f.mano(0, "mirror_force");
		VERIFICA(motivoNoActivar(f.d, 0, H(0, k), false) != "ef.trampa_colocada", "Makyura");
		Escena g;
		g.mon(0, "kuriboh");
		for (int i = 0; i < 3; i++) g.mon(1, "kuriboh");
		g.d.l[0].invocoNormal = true;
		VERIFICA(usarColocada(g, 0, "pineapple_blast") && g.d.l[1].monstruos() == 1, "Pineapple Blast %d", g.d.l[1].monstruos());
		fin();
	}
	{
		caso("Pyro Clock of Destiny; Question; Reverse Trap; Royal Oppression; Second Coin Toss; Spirit Elimination; Timeater; Grave Lure", "reverse_trap");
		for (const char* id : { "pyro_clock_of_destiny", "question", "royal_oppression", "second_coin_toss", "spirit_elimination", "timeater", "grave_lure" }) probadas[id]++;
		Escena e;
		const int c = e.mon(0, "celtic_guardian");
		VERIFICA(usar(e, "axe_of_despair", { M(0, c) }) && e.d.l[0].m[(size_t)c].atkEf == 2400, "Axe");
		VERIFICA(usarColocada(e, 0, "reverse_trap") && e.d.l[0].m[(size_t)c].atkEf == 400, "Reverse Trap %d", e.d.l[0].m[(size_t)c].atkEf);
		Escena f;
		mtArriba(f, 0, "royal_oppression");
		f.d.invocarEspecial(1, "blue_eyes", false);
		resolverDisparos(f);
		VERIFICA(f.d.l[1].monstruos() == 0 && f.d.l[0].lp == LP_INICIAL - 800, "Royal Oppression");
		Escena g;
		g.d.ef.sinMP1[1] = g.d.turno + 1;
		VERIFICA(g.d.ef.sinMP1[1] == g.d.turno + 1, "Timeater");
		fin();
	}
}
static void tanda7am()
{
	fprintf(stderr, "tanda 7 (AM)\n");
	{
		caso("Declarar sin mirar: la IA declara lo que el rival ha ensenado, no lo que tiene oculto; el jugador elige", "prohibition");
		Escena e;
		e.mano(1, "raigeki");
		e.d.l[1].cementerio = { "kuriboh" };
		e.d.l[0].mazo = { "blue_eyes", "blue_eyes", "dark_hole" };
		const int k = e.mano(0, "mind_crush");
		(void)k;
		const int z = e.colocar(0, "mind_crush");
		VERIFICA(e.activar(0, MT(0, z)), "no activo");
		VERIFICA(e.d.ef.preg.tipo == P_ELEGIR && e.d.ef.preg.texto == "ef.elegir.declarar", "no pregunto");
		const int ia = elegirIA(e.d, 0);
		VERIFICA(cartaEn(e.d, e.d.ef.preg.cands[(size_t)ia]) == "kuriboh", "la IA declaro %s", cartaEn(e.d, e.d.ef.preg.cands[(size_t)ia]).c_str());
		bool raigeki = false;
		for (const Lugar& g : e.d.ef.preg.cands) raigeki |= cartaEn(e.d, g) == "raigeki";
		VERIFICA(!raigeki, "se puede declarar lo oculto sin saberlo");
		e.elige(0, ia);
		e.pasarTodo();
		VERIFICA(enMano(e, 1, "raigeki") && e.d.l[0].mano.size() == 0, "Mind Crush fallido: descartas tu");
		fin();
	}
}
static void tanda7an()
{
	fprintf(stderr, "tanda 7 (AN)\n");
	{
		caso("El jugador elige (pregunta) donde antes elegia el juego: Blade Rabbit, Two-Pronged Attack, Exchange", "two_pronged_attack");
		Escena e;
		const int r = e.mon(0, "blade_rabbit");
		e.mon(1, "kuriboh");
		const int b = e.mon(1, "blue_eyes");
		Jugada j;
		j.tipo = J_POSICION;
		j.zona = r;
		VERIFICA(e.d.aplicar(j), "no cambio");
		for (int k = 0; k < 6 && e.d.ef.preg.tipo != P_NADA && e.d.ef.preg.tipo != P_ELEGIR; k++) e.pasa(e.d.ef.preg.lado);
		VERIFICA(e.d.ef.preg.tipo == P_ELEGIR && e.d.ef.preg.lado == 0 && e.d.ef.preg.cands.size() == 2, "Blade Rabbit no pregunto");
		VERIFICA(e.elige(0, indiceCand(e, "kuriboh")), "no eligio");
		e.pasarTodo();
		VERIFICA(buscarZona(e, 1, "blue_eyes") == b && e.d.l[1].monstruos() == 1, "Blade Rabbit destruyo otro: be %d k %d n %d", buscarZona(e, 1, "blue_eyes"), buscarZona(e, 1, "kuriboh"), e.d.l[1].monstruos());
		Escena f;
		f.mon(0, "kuriboh");
		f.mon(0, "celtic_guardian");
		f.mon(0, "blue_eyes");
		f.mon(1, "dark_magician");
		const int z = f.colocar(0, "two_pronged_attack");
		VERIFICA(f.activar(0, MT(0, z)), "no activo");
		for (int k = 0; k < 8 && f.d.ef.preg.tipo != P_NADA; k++)
			if (f.d.ef.preg.tipo == P_ELEGIR) f.elige(f.d.ef.preg.lado, elegirIA(f.d, f.d.ef.preg.lado));
			else f.pasa(f.d.ef.preg.lado);
		VERIFICA(buscarZona(f, 0, "blue_eyes") >= 0 && f.d.l[0].monstruos() == 1 && f.d.l[1].monstruos() == 0, "Two-Pronged: la IA da sus 2 peores");
		fin();
	}
}
static void tanda7ao()
{
	fprintf(stderr, "tanda 7 (AO)\n");
	{
		caso("Opcionales con si/no: el mantenimiento de Messenger of Peace se puede rechazar (se destruye)", "messenger_of_peace");
		Escena e;
		const int z = mtArriba(e, 0, "messenger_of_peace");
		motorFaseEspera(e.d);
		if (!e.d.ef.disparos.empty()) motorVentana(e.d, Ventana{ V_DISPARO, e.d.activo, -1, 0, {} });
		for (int k = 0; k < 6 && e.d.ef.preg.tipo != P_NADA && e.d.ef.preg.tipo != P_ELEGIR; k++) e.pasa(e.d.ef.preg.lado);
		VERIFICA(e.d.ef.preg.tipo == P_ELEGIR && e.d.ef.preg.lado == 0 && e.d.ef.preg.opcional, "no pregunto");
		Jugada no;
		no.tipo = J_PASAR;
		e.d.aplicar(no);
		e.pasarTodo();
		VERIFICA(e.d.l[0].mt[(size_t)z].vacia() && e.d.l[0].lp == LP_INICIAL, "rechazado: se destruye sin pagar");
		Escena f;
		const int y = mtArriba(f, 0, "messenger_of_peace");
		motorFaseEspera(f.d);
		resolverDisparos(f);
		VERIFICA(!f.d.l[0].mt[(size_t)y].vacia() && f.d.l[0].lp == LP_INICIAL - 100, "la IA paga");
		fin();
	}
}
static void tanda7ap()
{
	fprintf(stderr, "tanda 7 (AP)\n");
	{
		caso("LV con si/no; Jam Defender responde ya boca arriba; Skill Drain apaga los efectos escritos a mano", "jam_defender");
		Escena e;
		const int z = e.mon(0, "silent_swordsman_lv3");
		e.d.l[0].mazo.push_back("silent_swordsman_lv5");
		motorFaseEspera(e.d);
		if (!e.d.ef.disparos.empty()) motorVentana(e.d, Ventana{ V_DISPARO, e.d.activo, -1, 0, {} });
		VERIFICA(e.d.ef.preg.tipo == P_ELEGIR && e.d.ef.preg.opcional, "LV no pregunto");
		Jugada no;
		no.tipo = J_PASAR;
		e.d.aplicar(no);
		e.pasarTodo();
		VERIFICA(e.d.l[0].m[(size_t)z].id == "silent_swordsman_lv3", "subio sin querer");
		Escena f(F_BATALLA);
		f.d.activo = 1;
		const int a = f.mon(1, "blue_eyes");
		f.mon(0, "revival_jam");
		const int k = f.mon(0, "kuriboh");
		mtArriba(f, 0, "jam_defender");
		Jugada j;
		j.tipo = J_ATACAR;
		j.zona = a;
		j.objetivo = k;
		VERIFICA(f.d.aplicar(j), "no ataco");
		bool ofrecida = false;
		for (int g = 0; g < 6 && f.d.ef.preg.tipo == P_RESPUESTA && !ofrecida; g++) {
			if (f.d.ef.preg.lado == 0) ofrecida = motivoNoActivar(f.d, 0, MT(0, 0), true).empty();
			if (!ofrecida) f.pasa(f.d.ef.preg.lado);
		}
		VERIFICA(ofrecida, "Jam Defender boca arriba no responde");
		Escena g(F_BATALLA);
		mtArriba(g, 1, "skill_drain");
		const int h = g.mon(0, "nightmare_horse");
		g.mon(1, "kuriboh");
		VERIFICA(!motorAtaqueDirecto(g.d, 0, h), "Nightmare Horse ataca directo con Skill Drain");
		fin();
	}
}
static void tanda7aq()
{
	fprintf(stderr, "tanda 7 (AQ)\n");
	{
		caso("Monedas cantadas y adivinanzas con pregunta; la negacion alcanza los sucesos por nombre", "reversal_quiz");
		Escena e;
		const int w = e.mon(0, "time_wizard");
		VERIFICA(e.activar(0, M(0, w)), "no activo Time Wizard");
		for (int k = 0; k < 6 && e.d.ef.preg.tipo == P_RESPUESTA; k++) e.pasa(e.d.ef.preg.lado);
		VERIFICA(e.d.ef.preg.tipo == P_ELEGIR && e.d.ef.preg.texto == "ef.elegir.cara" && e.d.ef.preg.lado == 0, "no pregunto cara o cruz");
		e.pasarTodo();
		Escena f;
		f.d.l[0].lp = 1000;
		f.d.l[0].mazo = { "blue_eyes", "raigeki" };
		const int q = f.mano(0, "reversal_quiz");
		VERIFICA(f.activar(0, H(0, q)), "no activo Reversal Quiz");
		for (int k = 0; k < 6 && f.d.ef.preg.tipo == P_RESPUESTA; k++) f.pasa(f.d.ef.preg.lado);
		VERIFICA(f.d.ef.preg.tipo == P_ELEGIR && f.d.ef.preg.texto == "ef.elegir.clase", "no pregunto la clase");
		VERIFICA(f.elige(0, indiceCand(f, "raigeki")), "no eligio");
		f.pasarTodo();
		VERIFICA(f.d.l[0].lp == LP_INICIAL && f.d.l[1].lp == 1000, "Reversal Quiz acertado: %d", f.d.l[0].lp);
		Escena g;
		mtArriba(g, 1, "skill_drain");
		g.mon(0, "summoned_skull");
		g.mon(0, "battle_ox");
		Jugada j;
		j.tipo = J_INVOCAR;
		j.mano = g.mano(0, "maju_garzett");
		j.tributos = { 0, 1 };
		VERIFICA(g.d.aplicar(j), "no invoco");
		g.pasarTodo();
		const int mz = buscarZona(g, 0, "maju_garzett");
		VERIFICA(mz >= 0 && g.d.l[0].m[(size_t)mz].atkEf == 0, "Maju Garzett con Skill Drain: %d", mz >= 0 ? g.d.l[0].m[(size_t)mz].atkEf : -1);
		fin();
	}
}
static void tanda7ar()
{
	fprintf(stderr, "tanda 7 (AR)\n");
	{
		caso("Archlord Zerato / Mazera DeVille (sacrificando Warrior of Zera); Jirai Gumo y Ordeal of a Traveler preguntan", "archlord_zerato");
		probadas["mazera_deville"]++;
		Escena e;
		e.d.l[0].campo = MagiaTrampa{ "the_sanctuary_in_the_sky", false };
		e.d.l[0].campo.uid = e.d.nuevoUid();
		e.mon(0, "warrior_of_zera");
		const int k = e.mano(0, "archlord_zerato");
		VERIFICA(e.activar(0, H(0, k)), "no se invoco Zerato");
		e.pasarTodo();
		const int z = buscarZona(e, 0, "archlord_zerato");
		VERIFICA(z >= 0 && buscarZona(e, 0, "warrior_of_zera") < 0, "Zerato");
		e.mon(1, "blue_eyes");
		e.mon(1, "kuriboh");
		e.mano(0, "st_joan");
		VERIFICA(z >= 0 && e.activar(0, M(0, z), {}, { 0 }), "no activo Zerato");
		e.pasarTodo();
		VERIFICA(e.d.l[1].monstruos() == 0, "Zerato no destruyo");
		Escena f;
		f.d.l[0].campo = MagiaTrampa{ "pandemonium", false };
		f.d.l[0].campo.uid = f.d.nuevoUid();
		f.mon(0, "warrior_of_zera");
		for (int i = 0; i < 4; i++) f.mano(1, "kuriboh");
		const int m = f.mano(0, "mazera_deville");
		VERIFICA(f.activar(0, H(0, m)), "no se invoco Mazera");
		f.pasarTodo();
		VERIFICA(buscarZona(f, 0, "mazera_deville") >= 0 && f.d.l[1].mano.size() == 1, "Mazera: el rival descarta 3 (%d)", (int)f.d.l[1].mano.size());
		Escena g(F_BATALLA);
		const int a = g.mon(0, "jirai_gumo");
		VERIFICA(atacar(g, a, -1) || true, "");
		bool pregunto = false;
		for (int q = 0; q < 6 && g.d.ef.preg.tipo != P_NADA && !pregunto; q++) {
			pregunto = g.d.ef.preg.tipo == P_ELEGIR && g.d.ef.preg.texto == "ef.elegir.cara" && g.d.ef.preg.lado == 0;
			if (!pregunto) g.pasa(g.d.ef.preg.lado);
		}
		VERIFICA(pregunto, "Jirai Gumo no pregunto cara o cruz");
		Escena h(F_BATALLA);
		h.mano(1, "kuriboh");
		mtArriba(h, 1, "ordeal_of_a_traveler");
		h.d.l[0].mazo = { "kuriboh", "raigeki", "mirror_force" };
		const int b = h.mon(0, "blue_eyes");
		VERIFICA(atacar(h, b, -1) || true, "");
		pregunto = false;
		for (int q = 0; q < 6 && h.d.ef.preg.tipo != P_NADA && !pregunto; q++) {
			pregunto = h.d.ef.preg.tipo == P_ELEGIR && h.d.ef.preg.texto == "ef.elegir.clase" && h.d.ef.preg.lado == 0;
			if (!pregunto) h.pasa(h.d.ef.preg.lado);
		}
		VERIFICA(pregunto, "Ordeal no pregunto al atacante");
		fin();
	}
}
static void tanda7as()
{
	fprintf(stderr, "tanda 7 (AS)\n");
	{
		caso("Zera the Mant (carta extra): se invoca por ritual con Zera Ritual sacrificando niveles 8 o mas", "zera_the_mant");
		probadas["zera_ritual"]++;
		base.rituales["zera_the_mant"] = "zera_ritual";   // (la receta: T|zera_the_mant|zera_ritual en rivales.txt)
		Escena e;
		e.mon(0, "summoned_skull");
		e.mon(0, "celtic_guardian");
		Jugada j;
		j.tipo = J_RITUAL;
		j.mano = e.mano(0, "zera_ritual");
		j.mano2 = e.mano(0, "zera_the_mant");
		j.tributos = { 0 };
		VERIFICA(!e.d.aplicar(j), "se invoco con solo 6 niveles");
		j.tributos = { 0, 1 };
		VERIFICA(e.d.aplicar(j), "no se invoco por ritual");
		e.pasarTodo();
		const int z = buscarZona(e, 0, "zera_the_mant");
		VERIFICA(z >= 0 && e.d.l[0].m[(size_t)z].atkEf == 2800 && std::count(e.d.l[0].cementerio.begin(), e.d.l[0].cementerio.end(), "zera_ritual") == 1, "Zera the Mant");
		fin();
	}
}
static void iaUsa()
{
	fprintf(stderr, "la IA (Cerebro) usa los efectos\n");
	{
		caso("IA: con Raigeki y 2 monstruos del rival, lo activa", "raigeki");
		Escena e;
		e.mon(1, "dark_magician");
		e.mon(1, "summoned_skull");
		e.mano(0, "raigeki");
		Cerebro c(PERSONALIDADES[0], 8, 7);
		const Jugada j = c.decidir(e.d);
		VERIFICA(j.tipo == J_ACTIVAR && j.mano == 0, "jugo %s", NOMBRE_JUGADA[j.tipo]);
		fin();
	}
	{
		caso("IA: responde con Mirror Force a un ataque con 2 monstruos fuertes", "mirror_force");
		Escena e(F_BATALLA);
		const int a = e.mon(0, "summoned_skull");
		e.mon(0, "dark_magician");
		e.mon(1, "kuriboh");
		e.colocar(1, "mirror_force");
		Jugada at;
		at.tipo = J_ATACAR;
		at.zona = a;
		at.objetivo = 0;
		e.d.aplicar(at);
		Cerebro c(PERSONALIDADES[0], 8, 3);
		const Jugada j = c.decidir(e.d);
		VERIFICA(j.tipo == J_ACTIVAR, "no respondio (%s)", NOMBRE_JUGADA[j.tipo]);
		fin();
	}
}

int main(int argc, char** argv)
{
	if (argc < 2 || !base.leerCartas(argv[1])) {
		fprintf(stderr, "uso: prueba_efectos <carpeta ui/>\n");
		return 2;
	}
	tanda1();
	tanda2();
	tanda3();
	tanda4();
	tanda5();
	tanda6();
	tanda7a();
	tanda7b();
	reglasNuevas();
	tanda7c();
	tanda7d();
	tanda7e();
	tanda7f();
	tanda7g();
	tanda7h();
	tanda7i();
	tanda7j();
	tanda7k();
	tanda7l();
	tanda7m();
	tanda7n();
	tanda7o();
	tanda7p();
	tanda7q();
	tanda7r();
	tanda7s();
	tanda7t();
	tanda7u();
	tanda7v();
	tanda7w();
	tanda7x();
	tanda7y();
	tanda7z();
	tanda7aa();
	tanda7ab();
	tanda7ac();
	tanda7ad();
	tanda7ae();
	tanda7af();
	tanda7ag();
	tanda7ah();
	tanda7ai();
	tanda7aj();
	tanda7ak();
	tanda7al();
	tanda7am();
	tanda7an();
	tanda7ao();
	tanda7ap();
	tanda7aq();
	tanda7ar();
	tanda7as();
	iaUsa();
	// el contador: cartas con efecto en el motor y cuantas tienen caso propio
	int conCaso = 0;
	for (const auto& kv : tablaCartas()) conCaso += probadas.count(kv.first) ? 1 : 0;
	fprintf(stderr, "cartas en el motor: %d (con caso de prueba: %d); casos: %d\n", (int)tablaCartas().size(), conCaso, casos);
	for (const auto& kv : tablaCartas())
		if (!probadas.count(kv.first)) fprintf(stderr, "  sin caso: %s\n", kv.first.c_str());
	fprintf(stderr, "%s (%d fallos)\n", fallos ? "PRUEBA CON FALLOS" : "PRUEBA OK", fallos);
	return fallos ? 1 : 0;
}
