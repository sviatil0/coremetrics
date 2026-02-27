#!/bin/bash
# Compiles uml/classes.puml and places the output PNG into assets/
# Run from anywhere — paths are resolved relative to this script's location.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UML_FILE="$SCRIPT_DIR/uml/classes.puml"
OUT_DIR="$SCRIPT_DIR/assets"

mkdir -p "$OUT_DIR"
plantuml -o "$OUT_DIR" "$UML_FILE"

echo "Diagram written to $OUT_DIR/classes.png"