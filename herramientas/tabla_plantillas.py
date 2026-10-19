# Genera remake-ps5/src/reglas/tabla_plantillas.h: la PLANTILLA (arquetipo con el que se hizo la figura: guerrero_espada,
# guerrero_arma_larga, tirador, maquina, luchador...) de cada carta, desde remake/diseno/plantillas.json (las ~500
# cartas usables: cada lote nuevo de la serie ya esta ahi). El motor la usa como una de las pistas del ESCANEO del arma
# al cargar cada modelo (reglas/escaneo_arma.h): la plantilla dice si la figura se hizo con espada, arma larga, arco o
# canon; el nombre de la carta y la geometria del .glb (la punta del arma, la boquilla, el puno) hacen el resto.
# Corre solo en sincronizar.sh (si encuentra plantillas.json) para cada compilacion; no hay listas a mano.
# Uso: python -I tabla_plantillas.py [salida.h] [ruta/a/cartas.json]
import json, os, re, sys

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
salida = sys.argv[1] if len(sys.argv) > 1 else os.path.join(BASE, "remake-ps5", "src", "reglas", "tabla_plantillas.h")
ruta_cartas = sys.argv[2] if len(sys.argv) > 2 else os.path.join(BASE, "remake", "recursos", "cartas", "cartas.json")
PLANT = json.load(open(os.path.join(BASE, "remake", "diseno", "plantillas.json"), encoding="utf-8"))["cartas"]


def slug(s):
    return re.sub(r"[^a-z0-9]+", "_", s.lower()).strip("_")


filas = {k: (c.get("plantilla", ""), c.get("nombre", k)) for k, c in PLANT.items() if c.get("plantilla")}
# los id del remake distintos del nombre en ingles (red_eyes, blue_eyes, gaia...), si esta cartas.json
for ruta in (ruta_cartas, r"D:\Yugi-Reload\remake\recursos\cartas\cartas.json"):
    if os.path.exists(ruta):
        cartas = json.load(open(ruta, encoding="utf-8"))
        cartas = cartas if isinstance(cartas, list) else cartas.get("cartas", [])
        for c in cartas:
            k = slug(c.get("nombre_en", c["id"]))
            if c["id"] not in filas and k in filas:
                filas[c["id"]] = filas[k]
        break

lineas = ['\t{ "%s", "%s" },   // %s' % (k, v[0], v[1].replace("*/", "")) for k, v in sorted(filas.items())]
open(salida, "w", encoding="utf-8", newline="\n").write(
    "// GENERADO por herramientas/tabla_plantillas.py desde remake/diseno/plantillas.json: no editar a mano.\n"
    "// La plantilla con la que se hizo la figura de cada carta: una pista del escaneo del arma (escaneo_arma.h).\n"
    "#pragma once\n#include <cstring>\n\nnamespace combate {\n\n"
    "struct PlantillaCarta { const char* carta; const char* plantilla; };\n\n"
    "const PlantillaCarta TABLA_PLANTILLAS[] = {\n" + "\n".join(lineas) + "\n};\n\n"
    "// la plantilla de una carta (por su id del remake o su nombre en ingles); \"\" si no esta\n"
    "inline const char* plantillaDe(const char* carta)\n{\n"
    "\tfor (const PlantillaCarta& p : TABLA_PLANTILLAS) {\n\t\tif (strcmp(p.carta, carta) == 0) {\n\t\t\treturn p.plantilla;\n\t\t}\n\t}\n"
    "\treturn \"\";\n}\n\n} // namespace combate\n")
print(len(filas), "cartas ->", salida)
