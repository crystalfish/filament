/**
 * cgltf_write - a single-file glTF 2.0 writer written in C99.
 *
 * Version: 1.0
 *
 * Website: https://github.com/jkuhlmann/cgltf
 *
 * Distributed under the MIT License, see notice at the end of this file.
 *
 * Building:
 * Include this file where you need the struct and function
 * declarations. Have exactly one source file where you define
 * `CGLTF_WRITE_IMPLEMENTATION` before including this file to get the
 * function definitions.
 *
 */
#ifndef CGLTF_WRITE_H_INCLUDED__
#define CGLTF_WRITE_H_INCLUDED__

#include "cgltf.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

cgltf_result cgltf_write_file(const cgltf_options* options, const char* path, const cgltf_data* out_data);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef CGLTF_WRITE_H_INCLUDED__ */

/*
 *
 * Stop now, if you are only interested in the API.
 * Below, you find the implementation.
 *
 */

#ifdef __INTELLISENSE__
/* This makes MSVC intellisense work. */
#define CGLTF_WRITE_IMPLEMENTATION
#endif

#ifdef CGLTF_WRITE_IMPLEMENTATION

#include <stdio.h>

typedef struct {
    FILE* file;
    const cgltf_data* data;
    int depth;
    const char* indent;
    int needs_comma;
} cgltf_write_context;

static void cgltf_write_cont(cgltf_write_context* context, const char* line)
{
    fputs(line, context->file);
    int last = strlen(line) - 1;
    if (line[0] == ']' || line[0] == '}')
    {
        context->needs_comma = 1;
    }
    if (line[last] == '[' || line[last] == '{')
    {
        ++context->depth;
        context->needs_comma = 0;
    }
}

static void cgltf_write_indent(cgltf_write_context* context)
{
    if (context->needs_comma)
    {
        fputs(",\n", context->file);
        context->needs_comma = 0;
    }
    else {
        fputc('\n', context->file);
    }
    for (int i = 0; i < context->depth; ++i)
    {
        fputs(context->indent, context->file);
    }
}

static void cgltf_write_line(cgltf_write_context* context, const char* line)
{
    if (line[0] == ']' || line[0] == '}')
    {
        --context->depth;
        context->needs_comma = 0;
    }
    cgltf_write_indent(context);
    cgltf_write_cont(context, line);
}

static void cgltf_write_strprop(cgltf_write_context* context, const char* label, const char* val)
{
    if (val)
    {
        cgltf_write_indent(context);
        fprintf(context->file, "'%s': '%s'", label, val);
        context->needs_comma = 1;
    }
}

static void cgltf_write_intprop(cgltf_write_context* context, const char* label, int val, int def)
{
    if (val != def)
    {
        cgltf_write_indent(context);
        fprintf(context->file, "'%s': %d", label, val);
        context->needs_comma = 1;
    }
}

#define cgltf_write_idxprop(context, label, val, start) if (val) { \
        cgltf_write_indent(context); \
        fprintf(context->file, "'%s': %d", label, (int) (val - start)); \
        context->needs_comma = 1; }

static void cgltf_write_asset(cgltf_write_context* context, const cgltf_asset* asset)
{
    cgltf_write_line(context, "'asset': {");
    cgltf_write_strprop(context, "copyright", asset->copyright);
    cgltf_write_strprop(context, "generator", asset->generator);
    cgltf_write_strprop(context, "version", asset->version);
    cgltf_write_strprop(context, "min_version", asset->min_version);
    cgltf_write_line(context, "}");
}

static void cgltf_write_primitive(cgltf_write_context* context, const cgltf_primitive* prim)
{
    cgltf_write_intprop(context, "mode", (int) prim->type, 4);
    cgltf_write_idxprop(context, "indices", prim->indices, context->data->accessors);
    cgltf_write_idxprop(context, "material", prim->material, context->data->materials);

    cgltf_write_line(context, "'attributes': [");
    for (cgltf_size i = 0; i < prim->attributes_count; ++i)
    {
        const cgltf_attribute* attr = prim->attributes + i;
        cgltf_write_idxprop(context, attr->name, attr->data, context->data->accessors);
    }
    cgltf_write_line(context, "]");

    // TODO: prim->targets
}

static void cgltf_write_mesh(cgltf_write_context* context, const cgltf_mesh* mesh)
{
    cgltf_write_line(context, "{");
    cgltf_write_strprop(context, "name", mesh->name);
    
    cgltf_write_line(context, "'primitives': [");
    for (cgltf_size i = 0; i < mesh->primitives_count; ++i)
    {
        cgltf_write_line(context, "{");
        cgltf_write_primitive(context, mesh->primitives + i);
        cgltf_write_line(context, "}");
    }
    cgltf_write_line(context, "]");

    // TODO: mesh->weights

    cgltf_write_line(context, "}");
}

cgltf_result cgltf_write_file(const cgltf_options* options, const char* path, const cgltf_data* data)
{
    cgltf_write_context context;
	context.file = fopen(path, "wt");	
    context.data = data;
    context.depth = 1;
    context.needs_comma = 0;
    context.indent = "  ";

	fputs("{", context.file);

    if (data->asset.copyright || data->asset.generator || data->asset.version || data->asset.min_version)
    {
        cgltf_write_asset(&context, &data->asset);
    }

	cgltf_write_line(&context, "'meshes': [");
    for (cgltf_size i = 0; i < data->meshes_count; ++i)
    {
        cgltf_write_mesh(&context, data->meshes + i);
    }
	cgltf_write_line(&context, "]");

	cgltf_write_line(&context, "'accessors': {");
	cgltf_write_line(&context, "}");
	cgltf_write_line(&context, "'bufferViews': {");
	cgltf_write_line(&context, "}");
	cgltf_write_line(&context, "'buffers': {");
	cgltf_write_line(&context, "}");
	cgltf_write_line(&context, "'materials': {");
	cgltf_write_line(&context, "}");
	cgltf_write_line(&context, "'images': {");
	cgltf_write_line(&context, "}");
	cgltf_write_line(&context, "'textures': {");
	cgltf_write_line(&context, "}");
	cgltf_write_line(&context, "'samplers': {");
	cgltf_write_line(&context, "}");
	cgltf_write_line(&context, "'skins': {");
	cgltf_write_line(&context, "}");
	cgltf_write_line(&context, "'cameras': {");
	cgltf_write_line(&context, "}");
	cgltf_write_line(&context, "'nodes': {");
	cgltf_write_line(&context, "}");
	cgltf_write_line(&context, "'scenes': {");
	cgltf_write_line(&context, "}");
	cgltf_write_line(&context, "'scene': {");
	cgltf_write_line(&context, "}");
	cgltf_write_line(&context, "'animations': {");
	cgltf_write_line(&context, "}");

	// if (data->extensions) {
	// 	fprintf(out, "  'extensions': {\n");
	// 	fprintf(out, "  }\n");
	// }

	fputs("\n}\n", context.file);
	fclose(context.file);

    // TODO: replace trailing commas with spaces

	return cgltf_result_success;
}

#endif /* #ifdef CGLTF_WRITE_IMPLEMENTATION */

/* cgltf is distributed under MIT license:
 *
 * Copyright (c) 2018 Johannes Kuhlmann

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
