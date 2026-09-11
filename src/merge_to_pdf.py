import os
import re

# Папки для сбора
TARGET_DIRS = ['Features', 'Hook', 'SDK', 'Utils']
# Расширения файлов (можно оставить только ['.hpp'], если хочешь вариант 2)
EXTENSIONS = ['.cpp', '.hpp', '.h']

OUTPUT_HTML = "project_code.html"

def clean_code(code):
    """Удаляет лишние пробелы и пустые строки для экономии места"""
    # Удаляем однострочные комментарии // ... (раскомментируй строку ниже, если нужно еще сильнее ужать)
    # code = re.sub(r'//.*', '', code)
    
    lines = code.splitlines()
    cleaned = []
    for line in lines:
        stripped = line.rstrip()
        # Пропускаем подряд идущие пустые строки
        if not stripped and cleaned and not cleaned[-1]:
            continue
        cleaned.append(stripped)
    return "\n".join(cleaned)

html_content = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<style>
    @page {
        size: A4 landscape;
        margin: 6mm 6mm 6mm 6mm;
    }
    body {
        font-family: "Consolas", "Courier New", monospace;
        font-size: 4.8pt;
        line-height: 1.15;
        margin: 0;
        padding: 0;
        column-count: 3;
        column-gap: 5mm;
        column-rule: 0.5px solid #ccc;
    }
    .file-block {
        break-inside: avoid-column;
        margin-bottom: 6px;
    }
    .file-header {
        font-weight: bold;
        background: #2b2b2b;
        color: #ffffff;
        padding: 1px 4px;
        border-radius: 2px;
        margin-bottom: 2px;
        font-size: 5.5pt;
    }
    pre {
        margin: 0;
        white-space: pre-wrap;
        word-break: break-all;
    }
</style>
</head>
<body>
"""

files_count = 0
total_lines = 0

for target_dir in TARGET_DIRS:
    if not os.path.exists(target_dir):
        continue
    for root, _, files in os.walk(target_dir):
        for file in files:
            ext = os.path.splitext(file)[1].lower()
            if ext in EXTENSIONS:
                filepath = os.path.join(root, file)
                rel_path = os.path.relpath(filepath)
                try:
                    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                except Exception as e:
                    continue

                cleaned = clean_code(content)
                lines = cleaned.count('\n') + 1
                total_lines += lines
                files_count += 1

                # Экранирование HTML
                safe_code = (cleaned
                             .replace("&", "&amp;")
                             .replace("<", "&lt;")
                             .replace(">", "&gt;"))

                html_content += f"""
                <div class="file-block">
                    <div class="file-header">[{rel_path}] ({lines} lines)</div>
                    <pre>{safe_code}</pre>
                </div>
                """

html_content += """
</body>
</html>
"""

with open(OUTPUT_HTML, 'w', encoding='utf-8') as f:
    f.write(html_content)

print(f"Готово! Обработано файлов: {files_count}, всего строк: {total_lines}")
print(f"Результат сохранен в {OUTPUT_HTML}")
