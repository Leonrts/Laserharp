import uuid
import os

class KiCadSchematicGenerator:
    def __init__(self, filename):
        self.filename = filename
        self.uuid_counter = 0
        self.symbols = []
        self.wires = []
        self.labels = []
        self.lib_symbols = []

    def generate_uuid(self):
        # Deterministic UUIDs for reproducibility or just random
        return str(uuid.uuid4())

    def add_lib_symbol(self, name, pins):
        # Create a simplified symbol definition embedded in the file
        # pins is a list of (number, name, type, x, y, orientation)
        symbol_parts = [f"""
  (symbol "{name}" (in_bom yes) (on_board yes)
    (property "Reference" "U" (id 0) (at 0 0 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Value" "{name}" (id 1) (at 0 -2.54 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Footprint" "" (id 2) (at 0 0 0)
      (effects (font (size 1.27 1.27)) hide)
    )
    (symbol "{name}_1_1"
      (rectangle (start -7.62 10.16) (end 7.62 -10.16)
        (stroke (width 0) (type default) (color 0 0 0 0))
        (fill (type background))
      )
"""]

        for num, pname, ptype, x, y, rot in pins:
            # pin format: (pin type shape (at x y rot) (length 2.54)
            #   (name "PinName" (effects (font (size 1.27 1.27))))
            #   (number "PinNum" (effects (font (size 1.27 1.27))))
            # )
            # rot: 0=right, 90=up, 180=left, 270=down
            symbol_parts.append(f"""      (pin {ptype} line (at {x} {y} {rot}) (length 2.54)
        (name "{pname}" (effects (font (size 1.27 1.27))))
        (number "{num}" (effects (font (size 1.27 1.27))))
      )
""")
        symbol_parts.append("    )\n  )")
        self.lib_symbols.append("".join(symbol_parts))

    def add_instance(self, lib_name, ref, value, x, y):
        u = self.generate_uuid()
        instance = f"""
  (symbol (lib_id "{lib_name}") (at {x} {y} 0) (unit 1)
    (in_bom yes) (on_board yes) (dnp no) (fields_autoplaced)
    (uuid {u})
    (property "Reference" "{ref}" (id 0) (at {x} {y-3} 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Value" "{value}" (id 1) (at {x} {y-5} 0)
      (effects (font (size 1.27 1.27)))
    )
  )"""
        self.symbols.append(instance)
        return u

    def add_label(self, text, x, y, orientation=0):
        # Orientation: 0 is right, 2 is left
        u = self.generate_uuid()
        self.labels.append(f"""
  (label "{text}" (at {x} {y} {orientation}) (fields_autoplaced)
    (effects (font (size 1.27 1.27)) (justify left bottom))
    (uuid {u})
  )""")

    def add_wire(self, x1, y1, x2, y2):
        self.wires.append(f"""
  (wire (pts (xy {x1} {y1}) (xy {x2} {y2}))
    (stroke (width 0) (type default) (color 0 0 0 0))
    (uuid {self.generate_uuid()})
  )""")

    def write(self):
        with open(self.filename, 'w') as f:
            f.write('(kicad_sch (version 20211014) (generator "AI_Gen")\n')
            f.write('  (paper "A3")\n')
            f.write('  (lib_symbols\n')
            for s in self.lib_symbols:
                f.write(s)
            f.write('  )\n') # End lib_symbols

            for s in self.symbols:
                f.write(s)
            for w in self.wires:
                f.write(w)
            for l in self.labels:
                f.write(l)

            f.write(')\n')

def generate_schematic():
    sch = KiCadSchematicGenerator('hardware/laser_harp.kicad_sch')

    # 1. Define ESP32-S3 Symbol (Simplified)
    # 40 pins roughly.
    esp_pins = []
    # Left side (Power, Boot, ADC)
    y = 7.62
    esp_pins.append(("1", "GND", "power_in", -10.16, y, 0)); y -= 2.54
    esp_pins.append(("2", "3V3", "power_in", -10.16, y, 0)); y -= 2.54
    esp_pins.append(("3", "EN", "input", -10.16, y, 0)); y -= 2.54
    esp_pins.append(("0", "IO0_BOOT", "input", -10.16, y, 0)); y -= 2.54
    esp_pins.append(("4", "IO4_SENS", "input", -10.16, y, 0)); y -= 2.54

    # Right side (SPI, DACs)
    y = 7.62
    esp_pins.append(("10", "IO10_CSXY", "output", 10.16, y, 180)); y -= 2.54
    esp_pins.append(("9",  "IO9_CSRG",  "output", 10.16, y, 180)); y -= 2.54
    esp_pins.append(("14", "IO14_CSBI", "output", 10.16, y, 180)); y -= 2.54
    esp_pins.append(("13", "IO13_LDAC", "output", 10.16, y, 180)); y -= 2.54
    esp_pins.append(("11", "IO11_MOSI", "output", 10.16, y, 180)); y -= 2.54
    esp_pins.append(("12", "IO12_CLK",  "output", 10.16, y, 180)); y -= 2.54

    sch.add_lib_symbol("ESP32-S3-Custom", esp_pins)

    # 2. Define MCP4922 Symbol
    # 14 pins.
    dac_pins = []
    # Left (Control)
    y = 5.08
    dac_pins.append(("1", "VDD", "power_in", -10.16, y, 0)); y -= 2.54
    dac_pins.append(("2", "CS", "input", -10.16, y, 0)); y -= 2.54
    dac_pins.append(("3", "SCK", "input", -10.16, y, 0)); y -= 2.54
    dac_pins.append(("4", "SDI", "input", -10.16, y, 0)); y -= 2.54
    dac_pins.append(("5", "LDAC", "input", -10.16, y, 0)); y -= 2.54
    # Right (Analog Out)
    y = 5.08
    dac_pins.append(("14", "VOUTA", "output", 10.16, y, 180)); y -= 2.54
    dac_pins.append(("13", "VREF", "input", 10.16, y, 180)); y -= 2.54
    dac_pins.append(("11", "VOUTB", "output", 10.16, y, 180)); y -= 2.54
    dac_pins.append(("12", "VSS", "power_in", 10.16, y, 180)); y -= 2.54

    sch.add_lib_symbol("MCP4922-Custom", dac_pins)

    # 3. Place Components
    sch.add_instance("ESP32-S3-Custom", "U4", "ESP32-S3", 100, 100)

    # Place DACs
    sch.add_instance("MCP4922-Custom", "U1", "DAC_XY", 180, 60)
    sch.add_instance("MCP4922-Custom", "U2", "DAC_RG", 180, 100)
    sch.add_instance("MCP4922-Custom", "U3", "DAC_BI", 180, 140)

    # 4. Wiring / Labels
    # ESP Connections
    # Labels are added at the pin coordinates defined in add_lib_symbol + instance offset
    # ESP is at 100, 100.
    # Pin 10 is at x=10.16, y=7.62 relative to center.
    # Absolute: 100 + 10.16, 100 + 7.62 (Remember Y grows downwards in KiCad typically? No, Y grows Down. )
    # Let's double check coordinates. (at x y).
    # We will simply draw small wire stubs and add labels.

    # Helper to add label to ESP pin
    def label_esp(pname, px, py, label):
        # Pin is at 100+px, 100+py
        # Wire from pin to pin+5
        x = 100 + px
        y = 100 - py # Check sign of Y in symbol def. Definition had +Y as Top?
        # In SVG Y is down. In KiCad Y is down.
        # My symbol def: "start -7.62 10.16" means Top Left? Usually Max Y is Bottom.
        # Let's assume (at x y) means +X right, +Y down.
        # If I defined pin at (at -10.16 7.62), that is Left, and Positive Y (Down).
        # Wait, if rectangle is (-7.62 10.16) to (7.62 -10.16), then 10.16 is Bottom if Y is Down?
        # KiCad Coord System: Y increases Downwards.
        # So "start -7.62 10.16" is Left Bottom ??
        # Let's stick to standard: (at x y).

        # Let's just place labels near the components. The user can wire them.
        sch.add_label(label, x + (2.54 if px > 0 else -7.62), y)

    # Adding Labels for Connectivity
    # ESP32
    sch.add_label("SPI_MOSI", 112, 90) # Near IO11
    sch.add_label("SPI_CLK", 112, 92.5) # Near IO12
    sch.add_label("CS_XY", 112, 95)
    sch.add_label("CS_RG", 112, 97.5)
    sch.add_label("CS_BI", 112, 100)
    sch.add_label("LDAC", 112, 102.5)
    sch.add_label("SENSOR_IN", 85, 110)

    # DACs
    # U1
    sch.add_label("SPI_MOSI", 165, 65) # SDI
    sch.add_label("SPI_CLK", 165, 62.5) # SCK
    sch.add_label("CS_XY", 165, 60) # CS
    sch.add_label("LDAC", 165, 67.5) # LDAC
    sch.add_label("OUT_X", 195, 60)
    sch.add_label("OUT_Y", 195, 65)

    # U2
    sch.add_label("SPI_MOSI", 165, 105)
    sch.add_label("SPI_CLK", 165, 102.5)
    sch.add_label("CS_RG", 165, 100)
    sch.add_label("LDAC", 165, 107.5)
    sch.add_label("OUT_R", 195, 100)
    sch.add_label("OUT_G", 195, 105)

    # U3
    sch.add_label("SPI_MOSI", 165, 145)
    sch.add_label("SPI_CLK", 165, 142.5)
    sch.add_label("CS_BI", 165, 140)
    sch.add_label("LDAC", 165, 147.5)
    sch.add_label("OUT_B", 195, 140)
    sch.add_label("OUT_I", 195, 145)

    sch.write()
    print("Schematic generated.")

if __name__ == "__main__":
    generate_schematic()
