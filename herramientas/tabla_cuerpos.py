# Genera remake-ps5/src/reglas/tabla_cuerpos.h: el tipo de CUERPO de cada carta (bipedo, cuadrupedo, alado,
# compacto) desde su plantilla (remake/diseno/plantillas.json), para la postura de defensa (reglas/postura_defensa.h),
# y su FORMA y FAMILIA para morir (reglas/formas.h, muertes.h): ninguna muere como estatua; los que tienen piernas o
# patas se desploman doblandolas. Una carta nueva de la serie solo necesita su plantilla en plantillas.json.
# Trae las ~500 cartas de plantillas.json (por su nombre en ingles: red_eyes_b_dragon...) y ademas el id del remake
# cuando es distinto (red_eyes, blue_eyes, gaia...), sacado de cartas.json.
# Uso: python -I tabla_cuerpos.py [ruta/a/cartas.json]   (por defecto remake/recursos/cartas/cartas.json)
import json, os, re, sys

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PLANT = json.load(open(os.path.join(BASE, "remake", "diseno", "plantillas.json"), encoding="utf-8"))["cartas"]
ruta_cartas = sys.argv[1] if len(sys.argv) > 1 else os.path.join(BASE, "remake", "recursos", "cartas", "cartas.json")

# regla del usuario (2026-10-16): en defensa los de 2 piernas y los de 4 patas se agachan y se cubren; los alados
# bajan al suelo con las alas envueltas; los demas (maquinas, objetos, criaturas pequenas...) se compactan
CUERPO = {
    "lanzador_baculo": "Bipedo", "lanzador_manos": "Bipedo", "guerrero_espada": "Bipedo",
    "guerrero_arma_larga": "Bipedo", "tirador": "Bipedo", "luchador": "Bipedo",
    "cuadrupedo": "Cuadrupedo", "jinete": "Cuadrupedo", "bicho_rastrero": "Cuadrupedo",
    "volador_alado": "Alado",
    "criatura_pequena": "Compacto", "maquina": "Compacto", "objeto": "Compacto", "espiritu": "Compacto",
    "serpentino": "Compacto", "acuatico": "Compacto",
}


# regla del usuario (2026-10-17): "ninguna carta cuando sea atacada y muere cae como estatua; tiene que caer como
# personaje vivo, mucho menos si posee piernas o patas". Forma del cuerpo al morir, por plantilla (formas.h)
FORMA = {
    "lanzador_baculo": "Bipedo", "lanzador_manos": "Bipedo", "guerrero_espada": "Bipedo",
    "guerrero_arma_larga": "Bipedo", "tirador": "Bipedo", "luchador": "Bipedo",
    "cuadrupedo": "Cuadrupedo", "jinete": "Cuadrupedo", "bicho_rastrero": "Cuadrupedo",
    "volador_alado": "Alado", "criatura_pequena": "Pequeno", "maquina": "Maquina", "objeto": "Objeto",
    "espiritu": "Espiritu", "serpentino": "Serpiente", "acuatico": "Pez",
}
# por el nombre (pisan a la plantilla): gelatinosos que se derriten y sirenas (cola en vez de piernas)
FORMA_NOMBRE = [(r"slime|jam|ooze|germ|ameba|amoeba|jelly", "Blob"), (r"mermaid", "Serpiente")]
# familia de la muerte por el tipo de la carta (muertes.h); el resto, Generica
FAMILIA_TIPO = {"Fiend": "Demonio", "Zombie": "Zombi", "Pyro": "Piro", "Thunder": "Trueno", "Plant": "Planta",
                "Rock": "Objeto", "Machine": "Trueno"}


def forma_y_familia(k, plantilla, tipo):
    forma = FORMA[plantilla]
    for patron, f in FORMA_NOMBRE:
        if re.search(patron, k):
            forma = f
    familia = FAMILIA_TIPO.get(tipo, "Generica")
    if forma == "Espiritu":
        familia = "Espiritu"
    elif forma == "Objeto" and familia == "Generica":
        familia = "Objeto"
    elif forma == "Maquina" and familia == "Generica":
        familia = "Trueno"
    return forma, familia


def slug(s):
    return re.sub(r"[^a-z0-9]+", "_", s.lower()).strip("_")


filas = {}
for k, c in PLANT.items():
    p = c.get("plantilla")
    if p not in CUERPO:
        print("plantilla sin cuerpo:", k, p)
        continue
    filas[k] = (CUERPO[p], p, c.get("nombre", k)) + forma_y_familia(k, p, c.get("tipo"))
alias = 0
if os.path.exists(ruta_cartas):
    cartas = json.load(open(ruta_cartas, encoding="utf-8"))
    cartas = cartas if isinstance(cartas, list) else cartas.get("cartas", [])
    for c in cartas:
        k = slug(c.get("nombre_en", c["id"]))
        if c["id"] not in filas and k in filas:
            filas[c["id"]] = filas[k]
            alias += 1
else:
    print("AVISO: sin", ruta_cartas, "(no se agregan los id del remake distintos del nombre: red_eyes, blue_eyes...)")

lineas = ['\t{ "%s", Cuerpo::%s, Forma::%s, Familia::%s },   // %s (%s)' % (k, v[0], v[3], v[4], v[2].replace("*/", ""), v[1])
          for k, v in sorted(filas.items())]
salida = os.path.join(BASE, "remake-ps5", "src", "reglas", "tabla_cuerpos.h")
open(salida, "w", encoding="utf-8", newline="\n").write(
    "// GENERADO por herramientas/tabla_cuerpos.py desde remake/diseno/plantillas.json: no editar a mano.\n"
    "// Tipo de cuerpo de cada carta (de su plantilla) para la postura de defensa (postura_defensa.h), y su forma y\n"
    "// familia para morir (formas.h, muertes.h): ninguna muere como estatua.\n"
    "#pragma once\n#include <cstring>\n#include \"formas.h\"\n\nnamespace combate {\n\n"
    "enum class Cuerpo : unsigned char { Bipedo, Cuadrupedo, Alado, Compacto };\n\n"
    "struct CuerpoCarta { const char* carta; Cuerpo cuerpo; Forma forma; Familia familia; };\n\n"
    "const CuerpoCarta TABLA_CUERPOS[] = {\n" + "\n".join(lineas) + "\n};\n\n"
    "// la fila de una carta (por su id del remake o su nombre en ingles); nullptr si no esta en la tabla\n"
    "inline const CuerpoCarta* filaCuerpo(const char* carta)\n{\n"
    "\tfor (const CuerpoCarta& c : TABLA_CUERPOS) {\n\t\tif (strcmp(c.carta, carta) == 0) {\n\t\t\treturn &c;\n\t\t}\n\t}\n"
    "\treturn nullptr;\n}\n\n"
    "// el cuerpo de una carta; \"siNo\" si no esta en la tabla\n"
    "inline Cuerpo cuerpoDe(const char* carta, Cuerpo siNo)\n{\n"
    "\tconst CuerpoCarta* c = filaCuerpo(carta);\n"
    "\treturn c ? c->cuerpo : siNo;\n}\n\n} // namespace combate\n")
print(len(filas), "cartas (", alias, "alias del remake ) ->", salida)
