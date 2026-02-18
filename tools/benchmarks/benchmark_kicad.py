import sys
import os
import time

# Add the parent directory (tools/) to sys.path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

# We need to import the class, but generate_kicad is not a package.
try:
    from generate_kicad import KiCadSchematicGenerator
except ImportError:
    # If run from root, sys.path might need explicit adjustment
    sys.path.append(os.path.abspath("tools"))
    from generate_kicad import KiCadSchematicGenerator

def benchmark_add_lib_symbol(n_pins=100000):
    # Mocking filename to avoid disk writes in the loop (though the loop is inside add_lib_symbol)
    generator = KiCadSchematicGenerator("benchmark_output.kicad_sch")

    # Generate pins list
    # Using a simple list comp for speed
    pins = [(str(i), f"Pin{i}", "input", 0, 0, 0) for i in range(n_pins)]

    print(f"Running benchmark with {n_pins} pins...")
    start_time = time.perf_counter()
    generator.add_lib_symbol("BenchmarkComponent", pins)
    end_time = time.perf_counter()

    duration = end_time - start_time
    print(f"Benchmark: add_lib_symbol with {n_pins} pins took {duration:.6f} seconds")
    return duration

if __name__ == "__main__":
    benchmark_add_lib_symbol()
