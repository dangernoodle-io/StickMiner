"""Pre-build script: convert partitions.bin to C source with const uint8_t array.

Writes partition_fixup_data.c into the env's own .pio/build/<env>/ directory
so parallel multi-env builds (`pio run -e a -e b`) can't race on a shared
src/ output (TA-332). src/CMakeLists.txt picks the file up via
${CMAKE_BINARY_DIR}/partition_fixup_data.c.

If partitions.bin doesn't exist yet (first clean build), generate it
synchronously from the env's partitions CSV via ESP-IDF's gen_esp32part.py.
Without this, the script wrote a stub (len=1) and the compile baked the
stub into firmware — the partition fixup logic then SKIPped at runtime,
silently disabling the WROOM-32 layout-correction path on factory installs.
"""
import os
import subprocess
import sys

Import("env")

env_name = env.subst("$PIOENV")

# native envs don't compile partition_fixup_data.c — nothing to generate.
if env_name.startswith("native"):
    Return()

build_dir = os.path.join(".pio", "build", env_name)
bin_path = os.path.join(build_dir, "partitions.bin")
c_path = os.path.join(build_dir, "partition_fixup_data.c")

os.makedirs(build_dir, exist_ok=True)


def write_c_file(data):
    with open(c_path, "w") as out:
        out.write("// Auto-generated from partitions.bin — do not edit\n")
        out.write("#include <stdint.h>\n")
        out.write("const uint8_t g_partitions_bin[] = {\n")
        for i, b in enumerate(data):
            out.write(f"0x{b:02x}")
            if i < len(data) - 1:
                out.write(",")
            if (i + 1) % 16 == 0:
                out.write("\n")
        out.write("};\n")
        out.write(f"const unsigned int g_partitions_bin_len = {len(data)};\n")


def bootstrap_partitions_bin():
    """Generate partitions.bin from the env's CSV via gen_esp32part.py."""
    try:
        board_cfg = env.BoardConfig()
    except Exception:
        return False  # native env or no board defined
    csv_name = board_cfg.get("build.partitions", "partitions.csv")
    csv_path = os.path.join(env.subst("$PROJECT_DIR"), csv_name)
    if not os.path.exists(csv_path):
        return False

    try:
        idf_dir = env.PioPlatform().get_package_dir("framework-espidf")
    except Exception:
        return False
    if not idf_dir:
        return False

    gen_script = os.path.join(idf_dir, "components", "partition_table",
                              "gen_esp32part.py")
    if not os.path.exists(gen_script):
        return False

    flash_size = board_cfg.get("upload.flash_size",
                                board_cfg.get("build.flash_size", "4MB"))
    cmd = [sys.executable, gen_script, "--flash-size", flash_size,
           csv_path, bin_path]
    try:
        subprocess.check_call(cmd)
    except subprocess.CalledProcessError as e:
        print(f"embed_partitions: gen_esp32part.py failed: {e}")
        return False
    return os.path.exists(bin_path)


if not os.path.exists(bin_path):
    if bootstrap_partitions_bin():
        print(f"embed_partitions: bootstrapped {bin_path} from CSV")
    elif not os.path.exists(c_path):
        # CSV/gen tool not available. Stub keeps compilation honest;
        # len=1 makes the fixup logic SKIP at runtime.
        write_c_file(b"\x00")
        print(f"embed_partitions: {bin_path} unavailable, wrote stub at {c_path}")

if os.path.exists(bin_path):
    with open(bin_path, "rb") as f:
        data = f.read()
    write_c_file(data)
    print(f"embed_partitions: partitions.bin -> {c_path} ({len(data)} bytes)")
