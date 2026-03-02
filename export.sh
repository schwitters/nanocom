#!/bin/bash

# export_project_to_single_file.sh
# Collects all relevant source files into a single text file.
# The formatting is chosen so that LLMs (Gemini/ChatGPT) can recognize file boundaries.
VERSION="080"
PROJECT="nanocom"
OUTPUT_FILE="${PROJECT}_complete_code_v${VERSION}.txt"

# Clear/create output file
echo "${PROJECT} Project Export (Version ${VERSION})" > "$OUTPUT_FILE"
echo "Generated on: $(date)" >> "$OUTPUT_FILE"
echo "==================================================================" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

# Function to append a file
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

echo -e "\033[0;36mExporting project to $OUTPUT_FILE ...\033[0m"

# 3. Source code (headers & sources) - recursive
# We use 'find' to traverse all subdirectories
find cmake -type f  | sort | while read file; do
    append_file "$file"
done
find include -type f  | sort | while read file; do
    append_file "$file"
done
find samples -type f  | sort | while read file; do
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
append_file "CMakeLists.txt"
append_file "README.md"

echo -e "\033[0;32mDone! Upload the file '$OUTPUT_FILE' into the new chat.\033[0m"