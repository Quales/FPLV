import sys
from pathlib import Path
from PIL import Image

left_width = 355
right_width = 150

if len(sys.argv) != 2:
    print(f"Usage: python {Path(sys.argv[0]).name} image.png")
    sys.exit(1)

input_path = Path(sys.argv[1])

img = Image.open(input_path)
w, h = img.size

if left_width + right_width > w:
    print("Erreur : les largeurs demandées dépassent la largeur de l'image.")
    sys.exit(1)

left = img.crop((0, 0, left_width, h))
right = img.crop((w - right_width, 0, w, h))
result = Image.new(img.mode, (left_width + right_width, h))
result.paste(left, (0, 0))
result.paste(right, (left_width, 0))
output_path = input_path.with_name(
    input_path.stem + ".crop" + input_path.suffix
)
result.save(output_path)
print(f"Créé : {output_path}")