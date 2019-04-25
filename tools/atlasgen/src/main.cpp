/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <utils/Path.h>

#include <getopt/getopt.h>

#include <xatlas.h>

#include <fstream>
#include <iostream>
#include <string>

#define CGLTF_IMPLEMENTATION
#define CGLTF_WRITE_IMPLEMENTATION
#include "cgltf_write.h"

static bool g_discardTextures = false;

static const char* USAGE = R"TXT(
XATLAS consumes a glTF 2.0 file and produces a new glTF file that adds a new UV set to each mesh
suitable for baking lightmaps. The mesh topology in the output will not necessarily match with the
input, since new vertices might be inserted into the geometry.

Usage:
    XATLAS [options] <input path> <output filename> ...

Options:
   --help, -h
       Print this message
   --license, -L
       Print copyright and license information
   --discard, -d
       Discard all textures from the original model

Example:
    XATLAS -d bistro_in.gltf bistro_out.gltf
)TXT";

static void printUsage(const char* name) {
    std::string execName(utils::Path(name).getName());
    const std::string from("XATLAS");
    std::string usage(USAGE);
    for (size_t pos = usage.find(from); pos != std::string::npos; pos = usage.find(from, pos)) {
        usage.replace(pos, from.length(), execName);
    }
    puts(usage.c_str());
}

static void license() {
    std::cout <<
    #include "licenses/licenses.inc"
    ;
}

static int handleArguments(int argc, char* argv[]) {
    static constexpr const char* OPTSTR = "hLd";
    static const struct option OPTIONS[] = {
        { "help",     no_argument, 0, 'h' },
        { "license",  no_argument, 0, 'L' },
        { "discard",  no_argument, 0, 'd' },
        { }
    };

    int opt;
    int optionIndex = 0;

    while ((opt = getopt_long(argc, argv, OPTSTR, OPTIONS, &optionIndex)) >= 0) {
        std::string arg(optarg ? optarg : "");
        switch (opt) {
            default:
            case 'h':
                printUsage(argv[0]);
                exit(0);
            case 'L':
                license();
                exit(0);
            case 'd':
                g_discardTextures = true;
        }
    }

    return optind;
}

static std::ifstream::pos_type getFileSize(const char* filename) {
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

int main(int argc, char* argv[]) {
    const int optionIndex = handleArguments(argc, argv);
    const int numArgs = argc - optionIndex;
    if (numArgs < 2) {
        printUsage(argv[0]);
        return 1;
    }

    utils::Path inputPath = argv[optionIndex];
    if (!inputPath.exists()) {
        std::cerr << inputPath << " not found!" << std::endl;
        return 1;
    }
    if (inputPath.isDirectory()) {
        auto files = inputPath.listContents();
        for (auto file : files) {
            if (file.getExtension() == "gltf") {
                inputPath = file;
                std::cout << "Found " << inputPath.getName() << std::endl;
                break;
            }
        }
        if (inputPath.isDirectory()) {
            std::cerr << "no glTF file found in " << inputPath << std::endl;
            return 1;
        }
    }

    utils::Path outputPath = argv[optionIndex + 1];
    if (inputPath.getExtension() != "gltf" || outputPath.getExtension() != "gltf") {
        std::cerr << "File extension must be gltf." << std::endl;
        return 1;
    }

    // Peek at the file size to allow pre-allocation.
    long contentSize = static_cast<long>(getFileSize(inputPath.c_str()));
    if (contentSize <= 0) {
        std::cerr << "Unable to open " << inputPath << std::endl;
        exit(1);
    }

    // Consume the glTF file.
    std::ifstream in(inputPath.c_str(), std::ifstream::in);
    std::vector<uint8_t> buffer(static_cast<unsigned long>(contentSize));
    if (!in.read((char*) buffer.data(), contentSize)) {
        std::cerr << "Unable to read " << inputPath << std::endl;
        exit(1);
    }

    // Parse the glTF file.
    cgltf_options options { cgltf_file_type_gltf };
    cgltf_data* inputAsset;
    cgltf_result result = cgltf_parse(&options, buffer.data(), buffer.size(), &inputAsset);
    if (result != cgltf_result_success) {
        std::cerr << "Error parsing glTF file." << std::endl;
        return 1;
    }

    // Load external resources.
    utils::Path inputFolder = inputPath.getParent();
    result = cgltf_load_buffers(&options, inputAsset, inputFolder.c_str());
    if (result != cgltf_result_success) {
        std::cerr << "Error loading glTF resources." << std::endl;
        return 1;
    }

    cgltf_write_file(&options, outputPath.c_str(), inputAsset);

    cgltf_free(inputAsset);

    std::cout << "Generated " << outputPath << std::endl;
}
