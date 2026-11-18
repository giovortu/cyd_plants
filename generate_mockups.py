from PIL import Image, ImageDraw, ImageFont
import numpy as np

# Screen dimensions for CYD
width, height = 320, 240

# Colors
TFT_BLACK = (0, 0, 0)
TFT_WHITE = (255, 255, 255)
TFT_DARKGREY = (100, 100, 100)
TFT_LIGHTGREY = (192, 192, 192)
TFT_BLUE = (0, 0, 255)
TFT_GREEN = (0, 255, 0)
TFT_RED = (255, 0, 0)

plants = ["Pomodori", "Peperoni", "Pothos", "Patate", "Dieffenbachia"]
FONT_HEIGHT = 12 

def draw_tabs(draw, current_tab, first_visible=0):
    tab_height = 30
    arrow_width = 30
    settings_width = 60
    available_width = width - (2 * arrow_width) - settings_width
    plant_tab_width = available_width // 3
    
    draw.polygon([(arrow_width - 5, 15), (5, 8), (5, 22)], fill=TFT_WHITE)
    
    for i in range(3):
        plant_idx = first_visible + i
        x_pos = arrow_width + (i * plant_tab_width)
        if plant_idx < len(plants):
            bg_color = TFT_DARKGREY if current_tab == plant_idx else TFT_BLACK
            text_color = TFT_WHITE if current_tab == plant_idx else TFT_LIGHTGREY
            draw.rectangle([x_pos, 0, x_pos + plant_tab_width - 2, tab_height], fill=bg_color, outline=TFT_DARKGREY if current_tab != plant_idx else TFT_WHITE)
            name = plants[plant_idx]
            if len(name) > 7: name = name[:4] + "..."
            text_w = len(name) * 6
            draw.text((x_pos + (plant_tab_width - text_w)//2, (tab_height - FONT_HEIGHT)//2), name, fill=text_color)
        else:
            draw.rectangle([x_pos, 0, x_pos + plant_tab_width, tab_height], fill=TFT_BLACK)

    settings_x = arrow_width + (3 * plant_tab_width)
    bg_color = TFT_DARKGREY if current_tab == len(plants) else TFT_BLACK
    text_color = TFT_WHITE if current_tab == len(plants) else TFT_LIGHTGREY
    draw.rectangle([settings_x, 0, settings_x + settings_width - 2, tab_height], fill=bg_color, outline=TFT_DARKGREY if current_tab != len(plants) else TFT_WHITE)
    draw.text((settings_x + (settings_width - 30)//2, (tab_height - FONT_HEIGHT)//2), "Set", fill=text_color)
    draw.polygon([(width - arrow_width + 5, 15), (width - 5, 8), (width - 5, 22)], fill=TFT_WHITE)

img_data = Image.new('RGB', (width, height), color='black')
draw_data = ImageDraw.Draw(img_data)
draw_tabs(draw_data, 0, 0)

label_width = 110
bar_x = label_width + 10
bar_w = width - bar_x - 20
bar_h = 25
gauges = [
    (60, 22.5, "Temperature", "C", TFT_BLUE),
    (100, 45.2, "Moisture", "%", TFT_BLUE),
    (140, 1250, "Luminosity", "lx", TFT_BLUE),
    (180, 88.0, "Battery", "%", TFT_GREEN)
]

for y, val, lbl, unit, color in gauges:
    text_w = len(lbl) * 6
    draw_data.text((label_width - text_w, y + (bar_h - FONT_HEIGHT)//2), lbl, fill=TFT_WHITE)
    draw_data.rectangle([bar_x, y, bar_x + bar_w, y + bar_h], outline=TFT_WHITE)
    pct = 0.5 if "Temp" in lbl else (0.45 if "Moist" in lbl else (0.25 if "Lum" in lbl else 0.88))
    draw_data.rectangle([bar_x+1, y+1, bar_x + int(bar_w * pct), y + bar_h - 1], fill=color)
    val_str = f"{val} {unit}"
    val_text_w = len(val_str) * 6
    draw_data.text((bar_x + (bar_w - val_text_w)//2, y + (bar_h - FONT_HEIGHT)//2), val_str, fill=TFT_WHITE)

img_data.save('/home/gortu/projects/display/interface_mockup.png')

img_set = Image.new('RGB', (width, height), color='black')
draw_set = ImageDraw.Draw(img_set)
draw_tabs(draw_set, len(plants), 0)
draw_set.text((20, 60), "--- Network Settings ---", fill=TFT_WHITE)
draw_set.text((20, 80), "SSID: Your_WiFi_SSID", fill=TFT_WHITE)
draw_set.text((20, 100), "MQTT: 192.168.1.50", fill=TFT_WHITE)
draw_set.rectangle([20, 140, 160, 180], fill=TFT_DARKGREY, outline=TFT_WHITE)
draw_set.text((20 + (140 - 80)//2, 155), "Reload Config", fill=TFT_WHITE)
draw_set.rectangle([170, 140, 310, 180], fill=TFT_RED, outline=TFT_WHITE)
draw_set.text((170 + (140 - 90)//2, 155), "Reset Default", fill=TFT_WHITE)

img_set.save('/home/gortu/projects/display/interface_settings_mockup.png')
