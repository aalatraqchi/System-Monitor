"""Regenerate the Windows icon (Pillow needed only when editing this asset)."""
from pathlib import Path
from PIL import Image, ImageDraw

scale = 4
image = Image.new("RGBA", (256 * scale, 256 * scale))
draw = ImageDraw.Draw(image)
def box(values):
    return tuple(value * scale for value in values)
draw.rounded_rectangle(box((8, 8, 248, 248)), radius=52 * scale, fill="#111827")
draw.rounded_rectangle(box((38, 44, 218, 188)), radius=20 * scale, fill="#1f2937",
                       outline="#38bdf8", width=9 * scale)
draw.line([(x * scale, y * scale) for x, y in [(56, 132), (88, 132), (110, 89),
          (139, 155), (162, 120), (201, 120)]], fill="#38bdf8", width=12 * scale, joint="curve")
draw.line([(100 * scale, 214 * scale), (156 * scale, 214 * scale)], fill="#e5e7eb", width=10 * scale)
draw.line([(128 * scale, 190 * scale), (128 * scale, 214 * scale)], fill="#e5e7eb", width=10 * scale)
image.resize((256, 256), Image.Resampling.LANCZOS).save(Path(__file__).with_name("app.ico"),
    sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
