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

        # Maps symbol_name -> { pin_name: (x, y, orientation) }
        self.symbol_pin_map = {}
        # Maps ref -> { lib_name, x, y }
        self.instance_map = {}

    def generate_uuid(self):
        # Deterministic UUIDs for reproducibility or just random
        return str(uuid.uuid4())

    def add_lib_symbol(self, name, pins):
        # Store pin definitions for later lookup
        self.symbol_pin_map[name] = {}

        # Create a simplified symbol definition embedded in the file
        # pins is a list of (number, name, type, x, y, orientation)
        symbol_def = f"""
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
"""
        for num, pname, ptype, x, y, rot in pins:
            self.symbol_pin_map[name][pname] = (x, y, rot)

            # pin format: (pin type shape (at x y rot) (length 2.54)
            #   (name "PinName" (effects (font (size 1.27 1.27))))
            #   (number "PinNum" (effects (font (size 1.27 1.27))))
            # )
            # rot: 0=right, 90=up, 180=left, 270=down
            symbol_def += f"""      (pin {ptype} line (at {x} {y} {rot}) (length 2.54)
        (name "{pname}" (effects (font (size 1.27 1.27))))
        (number "{num}" (effects (font (size 1.27 1.27))))
      )
"""
        symbol_def += "    )\n  )"
        self.lib_symbols.append(symbol_def)

    def add_instance(self, lib_name, ref, value, x, y):
        # Store instance data
        self.instance_map[ref] = { 'lib': lib_name, 'x': x, 'y': y }

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

    def add_label(self, text, x, y, orientation=0, justify="left bottom"):
        # Orientation: 0 is right, 2 is left
        u = self.generate_uuid()
        self.labels.append(f"""
  (label "{text}" (at {x} {y} {orientation}) (fields_autoplaced)
    (effects (font (size 1.27 1.27)) (justify {justify}))
    (uuid {u})
  )""")

    def add_wire(self, x1, y1, x2, y2):
        self.wires.append(f"""
  (wire (pts (xy {x1} {y1}) (xy {x2} {y2}))
    (stroke (width 0) (type default) (color 0 0 0 0))
    (uuid {self.generate_uuid()})
  )""")

    def connect_pin(self, ref, pin_name, label_text):
        if not label_text:
            return

        if ref not in self.instance_map:
            print(f"Error: Instance {ref} not found.")
            return

        inst_data = self.instance_map[ref]
        lib_name = inst_data['lib']
        inst_x = inst_data['x']
        inst_y = inst_data['y']

        if lib_name not in self.symbol_pin_map:
            print(f"Error: Symbol {lib_name} not found.")
            return

        pins = self.symbol_pin_map[lib_name]
        if pin_name not in pins:
            print(f"Error: Pin {pin_name} not found in symbol {lib_name}.")
            return

        pin_x, pin_y, rot = pins[pin_name]

        # Absolute Coords
        # NOTE: KiCad Y increases downwards.
        # Symbols defined with positive Y at bottom? No, typically standard is Y-down.
        # So abs_y = inst_y + pin_y.
        abs_x = inst_x + pin_x
        abs_y = inst_y + pin_y

        stub_len = 5.08 # 5mm stub

        # Determine direction based on pin X relative to symbol center (0)
        # If pin_x < 0 (Left side), wire goes Left.
        if pin_x < 0:
            end_x = abs_x - stub_len
            self.add_wire(abs_x, abs_y, end_x, abs_y)
            # Label justified Right, placed slightly left of wire end
            self.add_label(label_text, end_x - 1.27, abs_y, 0, justify="right bottom")
        else:
            # Right side, wire goes Right
            end_x = abs_x + stub_len
            self.add_wire(abs_x, abs_y, end_x, abs_y)
            # Label justified Left, placed slightly right of wire end
            self.add_label(label_text, end_x + 1.27, abs_y, 0, justify="left bottom")

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
    # Start from Top (-Y) and go Down (+Y)
    y = -5.08
    esp_pins.append(("1", "GND", "power_in", -10.16, y, 0)); y += 2.54
    esp_pins.append(("2", "3V3", "power_in", -10.16, y, 0)); y += 2.54
    esp_pins.append(("3", "EN", "input", -10.16, y, 0)); y += 2.54
    esp_pins.append(("0", "IO0_BOOT", "input", -10.16, y, 0)); y += 2.54
    esp_pins.append(("4", "IO4_SENS", "input", -10.16, y, 0)); y += 2.54

    # Right side (SPI, DACs)
    # Start from Top (-Y)
    y = -7.62
    esp_pins.append(("10", "IO10_CSXY", "output", 10.16, y, 180)); y += 2.54
    esp_pins.append(("9",  "IO9_CSRG",  "output", 10.16, y, 180)); y += 2.54
    esp_pins.append(("14", "IO14_CSBI", "output", 10.16, y, 180)); y += 2.54
    esp_pins.append(("13", "IO13_LDAC", "output", 10.16, y, 180)); y += 2.54
    esp_pins.append(("11", "IO11_MOSI", "output", 10.16, y, 180)); y += 2.54
    esp_pins.append(("12", "IO12_CLK",  "output", 10.16, y, 180)); y += 2.54

    sch.add_lib_symbol("ESP32-S3-Custom", esp_pins)

    # 2. Define MCP4922 Symbol
    # 14 pins.
    dac_pins = []
    # Left (Control)
    y = -5.08
    dac_pins.append(("1", "VDD", "power_in", -10.16, y, 0)); y += 2.54
    dac_pins.append(("2", "CS", "input", -10.16, y, 0)); y += 2.54
    dac_pins.append(("3", "SCK", "input", -10.16, y, 0)); y += 2.54
    dac_pins.append(("4", "SDI", "input", -10.16, y, 0)); y += 2.54
    dac_pins.append(("5", "LDAC", "input", -10.16, y, 0)); y += 2.54
    # Right (Analog Out)
    y = -5.08
    dac_pins.append(("14", "VOUTA", "output", 10.16, y, 180)); y += 2.54
    dac_pins.append(("13", "VREF", "input", 10.16, y, 180)); y += 2.54
    dac_pins.append(("11", "VOUTB", "output", 10.16, y, 180)); y += 2.54
    dac_pins.append(("12", "VSS", "power_in", 10.16, y, 180)); y += 2.54

    sch.add_lib_symbol("MCP4922-Custom", dac_pins)

    # 3. Place Components
    sch.add_instance("ESP32-S3-Custom", "U4", "ESP32-S3", 100, 100)

    # Place DACs
    sch.add_instance("MCP4922-Custom", "U1", "DAC_XY", 180, 60)
    sch.add_instance("MCP4922-Custom", "U2", "DAC_RG", 180, 100)
    sch.add_instance("MCP4922-Custom", "U3", "DAC_BI", 180, 140)

    # 4. Wiring / Labels

    # ESP32 Connections
    sch.connect_pin("U4", "IO4_SENS", "SENSOR_IN")

    # Right Side ESP
    sch.connect_pin("U4", "IO10_CSXY", "CS_XY")
    sch.connect_pin("U4", "IO9_CSRG",  "CS_RG")
    sch.connect_pin("U4", "IO14_CSBI", "CS_BI")
    sch.connect_pin("U4", "IO13_LDAC", "LDAC")
    sch.connect_pin("U4", "IO11_MOSI", "SPI_MOSI")
    sch.connect_pin("U4", "IO12_CLK",  "SPI_CLK")

    # DACs
    # U1 (DAC_XY) at 180, 60
    sch.connect_pin("U1", "CS",   "CS_XY")
    sch.connect_pin("U1", "SCK",  "SPI_CLK")
    sch.connect_pin("U1", "SDI",  "SPI_MOSI")
    sch.connect_pin("U1", "LDAC", "LDAC")

    sch.connect_pin("U1", "VOUTA", "OUT_X")
    sch.connect_pin("U1", "VOUTB", "OUT_Y")

    # U2 (DAC_RG) at 180, 100
    sch.connect_pin("U2", "CS",   "CS_RG")
    sch.connect_pin("U2", "SCK",  "SPI_CLK")
    sch.connect_pin("U2", "SDI",  "SPI_MOSI")
    sch.connect_pin("U2", "LDAC", "LDAC")

    sch.connect_pin("U2", "VOUTA", "OUT_R")
    sch.connect_pin("U2", "VOUTB", "OUT_G")

    # U3 (DAC_BI) at 180, 140
    sch.connect_pin("U3", "CS",   "CS_BI")
    sch.connect_pin("U3", "SCK",  "SPI_CLK")
    sch.connect_pin("U3", "SDI",  "SPI_MOSI")
    sch.connect_pin("U3", "LDAC", "LDAC")

    sch.connect_pin("U3", "VOUTA", "OUT_B")
    sch.connect_pin("U3", "VOUTB", "OUT_I")

    sch.write()
    print("Schematic generated.")

if __name__ == "__main__":
    generate_schematic()
