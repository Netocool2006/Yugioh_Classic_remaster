#!/usr/bin/env bash
# El probador automatico de efectos (rama classic-probador), dentro del contenedor arch-ps5:
#   bash /compartido/wt/<rama>/remake-ps5/src/pruebas/certificar.sh [solo_id]
# Arma una carpeta ui/ con las cartas (classic-assets) y los datos de la rama, compila prueba_certificacion.cpp (si
# cambio algo) y escribe docs/certificacion_efectos.md y docs/certificacion_efectos.csv. Con un id: solo esa carta
# (plantilla y front, al log) sin tocar el reporte.
set -euo pipefail
rama=$(cd "$(dirname "$0")/../.." && pwd)
assets=${YUGI_ASSETS:-/compartido/classic-assets}
ui=/tmp/cert/ui
mkdir -p "$ui"
cp "$assets"/ui/cartas_ui.txt "$assets"/ui/cartas_ui_en.txt "$assets"/ui/cartas_classic.txt "$ui/"
cp "$rama"/datos/*.txt "$ui/"
bin=/tmp/prueba_certificacion
if [[ ! -x $bin || -n "$(find "$rama/src" \( -name '*.h' -o -name '*.cpp' \) -newer $bin)" ]]; then
	g++ -std=c++17 -O1 -w -I "$rama/src" "$rama/src/pruebas/prueba_certificacion.cpp" -o $bin
fi
rc=0
export CERT_CASOS="$rama/src/pruebas/prueba_efectos.cpp"   # (que cartas usa cada caso a mano)
export CERT_NOTAS="$rama/src/pruebas/certificacion_notas.md"   # (la revision escrita a mano, va al reporte)
$bin "$ui" "$rama/docs" "$@" 2> /tmp/certificacion.log || rc=$?
grep -E "^CERTIFICACION|^casos a mano:|: plantilla " /tmp/certificacion.log || tail -20 /tmp/certificacion.log
exit $rc
