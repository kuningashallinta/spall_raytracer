# SPDX-FileCopyrightText: 2026 King Hallinta
# SPDX-License-Identifier: Apache-2.0

import argparse
import io
import os
import subprocess
import sys


def compile_shader(dxc, arguments, source, output):
    if not os.path.isfile(dxc):
        sys.stderr.write("dxc not found: %s\n" % dxc)
        raise SystemExit(1)

    command = [dxc, "-nologo", "-T", "lib_6_3"] + arguments + ["-Fo", output, source]
    result = subprocess.run(command, capture_output=True, text=True)

    if result.returncode != 0:
        sys.stderr.write(" ".join(command) + "\n")
        sys.stderr.write(result.stdout + result.stderr)
        raise SystemExit(result.returncode)

    with open(output, "rb") as blob:
        data = blob.read()

    os.remove(output)

    return data


def byte_array(name, data):
    lines = ["\tinline constexpr std::uint8_t %s[] = {" % name]

    for offset in range(0, len(data), 12):
        values = ", ".join("0x%02x" % byte for byte in data[offset : offset + 12])
        lines.append("\t\t" + values + ("," if (offset + 12) < len(data) else "};"))

    return "\n".join(lines) + "\n"


def word_array(name, data):
    words = [
        int.from_bytes(data[offset : offset + 4], "little")
        for offset in range(0, len(data), 4)
    ]
    lines = ["\tinline constexpr std::uint32_t %s[] = {" % name]

    for offset in range(0, len(words), 6):
        values = ", ".join("0x%08x" % word for word in words[offset : offset + 6])
        lines.append("\t\t" + values + ("," if (offset + 6) < len(words) else "};"))

    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--dxil-dxc", required=True)
    parser.add_argument("--spirv-dxc", required=True)
    arguments = parser.parse_args()

    name = os.path.splitext(os.path.basename(arguments.source))[0]
    os.makedirs(arguments.output, exist_ok=True)

    dxil = compile_shader(
        arguments.dxil_dxc,
        [],
        arguments.source,
        os.path.join(arguments.output, name + ".cso"),
    )

    spirv = compile_shader(
        arguments.spirv_dxc,
        ["-spirv", "-fspv-target-env=vulkan1.2"],
        arguments.source,
        os.path.join(arguments.output, name + ".spv"),
    )

    if (len(spirv) % 4) != 0:
        raise SystemExit("SPIR-V size is not a multiple of four")

    header = io.StringIO()
    header.write(
        "// This file has been automatically generated. Do not edit by hand!\n"
    )
    header.write("\n")
    header.write("#pragma once\n")
    header.write("\n")
    header.write("#include <cstdint>\n")
    header.write("\n")
    header.write("namespace shaders\n")
    header.write("{\n")
    header.write(byte_array(name + "Library", dxil))
    header.write("\n")
    header.write(word_array(name + "LibrarySpirv", spirv))
    header.write("} // namespace shaders\n")

    path = os.path.join(arguments.output, name + "Shaders.h")

    with io.open(path, "w", encoding="utf-8", newline="\n") as file:
        file.write(header.getvalue())

    print(
        "%s -> %s (%u DXIL bytes, %u SPIR-V bytes)"
        % (
            os.path.basename(arguments.source),
            os.path.basename(path),
            len(dxil),
            len(spirv),
        )
    )


if __name__ == "__main__":
    main()
