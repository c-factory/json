/*
    Copyright (c) 2020 Ivan Kniazkov <ivan.kniazkov.com>

    The structs and function definitions related to the JSON format and parser
*/

#pragma once

#include <wchar.h>

typedef enum
{
    json_null,
    json_object,
    json_array,
    json_string,
    json_number,
    json_boolean
} json_element_type_t;

typedef struct json_element_t json_element_t;

typedef struct json
{
    const wchar_t * const key;
    const json_element_t * const value;
} json_pair_t;

typedef struct
{
    const size_t count;
} json_object_t;

struct json_element_t
{
    const json_element_type_t type;
    const json_element_t * const parent;
    union data
    {
        const json_object_t * const object;
        const wchar_t * const string;
    };    
};

void destroy_json_element(json_element_t *iface);
json_element_t * create_json_string(const wchar_t *c_str);