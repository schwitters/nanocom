#!/bin/bash

# export_project_to_single_file.sh
# Sammelt alle relevanten Source-Dateien in einer einzigen Textdatei.
# Formatierung ist so gewählt, dass LLMs (Gemini/ChatGPT) die Dateigrenzen erkennen.
VERSION="080"
PROJECT="nanocom"
OUTPUT_FILE="${PROJECT}_complete_code_v${VERSION}.txt"

# Datei leeren/erstellen
echo "${PROJECT} Project Export (Version ${VERSION})" > "$OUTPUT_FILE"
echo "Generated on: $(date)" >> "$OUTPUT_FILE"
echo "==================================================================" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

# Funktion zum Anhängen einer Datei
append_file() {
    local filepath="$1"
    if [ -f "$filepath" ]; then
        echo "Processing: $filepath"
        echo "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv" >> "$OUTPUT_FILE"
        echo "START OF FILE: $filepath" >> "$OUTPUT_FILE"
        echo "--------------------------------------------------" >> "$OUTPUT_FILE"
        cat "$filepath" >> "$OUTPUT_FILE"
        echo "" >> "$OUTPUT_FILE"
        echo "--------------------------------------------------" >> "$OUTPUT_FILE"
        echo "END OF FILE: $filepath" >> "$OUTPUT_FILE"
        echo "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" >> "$OUTPUT_FILE"
        echo "" >> "$OUTPUT_FILE"
        echo "" >> "$OUTPUT_FILE"
    else
        echo "Warning: File $filepath not found."
    fi
}

echo -e "\033[0;36mExportiere Projekt in $OUTPUT_FILE ...\033[0m"

# 3. Source Code (Headers & CC) - Rekursiv
# Wir nutzen 'find', um durch alle Unterordner von hosts zu gehen
find idl -type f  | sort | while read file; do
    append_file "$file"
done
find include -type f  | sort | while read file; do
    append_file "$file"
done
find src -type f  | sort | while read file; do
    append_file "$file"
done
find tests -type f  | sort | while read file; do
    append_file "$file"
done
find tools -type f  | sort | while read file; do
    append_file "$file"
done
echo -e "\033[0;32mFertig! Lade die Datei '$OUTPUT_FILE' in den neuen Chat hoch.\033[0m"
