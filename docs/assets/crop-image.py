import sys
from pathlib import Path
from PIL import Image

parts = [
    ("keep", 355),
    ("remove", 276),
    ("keep", 35),
    ("remove", 362),
    ("keep", 135),
]

if len(sys.argv) != 2:
    print(f"Usage: python {Path(sys.argv[0]).name} image.png")
    sys.exit(1)

input_path = Path(sys.argv[1])

img = Image.open(input_path)
w, h = img.size

total_width = sum(width for _, width in parts)

if total_width > w:
    print(
        f"Erreur : les morceaux demandés font {total_width}px, "
        f"mais l'image ne fait que {w}px de large."
    )
    sys.exit(1)

kept_parts = []
x = 0

for action, width in parts:
    if x + width > w:
        print("Erreur : les découpes dépassent la largeur de l'image.")
        sys.exit(1)

    if action == "keep":
        kept_parts.append(img.crop((x, 0, x + width, h)))

    x += width

output_width = sum(part.width for part in kept_parts)
result = Image.new(img.mode, (output_width, h))

x = 0
for part in kept_parts:
    result.paste(part, (x, 0))
    x += part.width

output_path = input_path.with_name(
    input_path.stem + ".crop" + input_path.suffix
)

result.save(output_path)
print(f"Créé : {output_path}")