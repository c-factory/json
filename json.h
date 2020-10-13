/*
    Copyright (c) 2020 Ivan Kniazkov <ivan.kniazkov.com>

    The structs and function definitions related to the JSON format and parser
*/

#pragma once

#include "strings/strings.h"
#include <stdbool.h>

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

typedef struct
{
    const wide_string_t * const key;
    const json_element_t * const value;
} json_pair_t;

typedef struct
{
    const size_t count;
} json_object_data_t;

typedef struct
{
    const size_t count;
} json_array_data_t;

typedef struct
{
    const json_element_type_t type;
    const json_element_t * const parent;
} json_element_base_t;

struct json_element_t
{
    const json_element_base_t base;
    union 
    {
        const json_object_data_t * const object;
        const json_array_data_t * const array;
        const wide_string_t * const string_value;
        double num_value;
        bool bool_value;
    } data;    
};

typedef struct 
{
    const json_element_base_t base;
} json_null_t;

typedef struct 
{
    const json_element_base_t base;
    const json_object_data_t * const object;
} json_object_t;

typedef struct 
{
    const json_element_base_t base;
    const json_array_data_t * const array;
} json_array_t;

typedef struct 
{
    const json_element_base_t base;
    const wide_string_t * const value;
} json_string_t;

typedef struct 
{
    const json_element_base_t base;
    const double value;
} json_number_t;

typedef struct 
{
    const json_element_base_t base;
    const bool value;
} json_boolean_t;

json_element_t * parse_json(wide_string_t *text);

void destroy_json_element(const json_element_base_t *iface);

json_null_t * create_json_null();
json_null_t * create_json_null_at_end_of_array(json_array_t *iface);

json_object_t * create_json_object();

json_array_t * create_json_array();

json_string_t * create_json_string(const wchar_t *value);
json_string_t * create_json_string_owned_by_object(json_object_t *iface, const wchar_t *key, const wchar_t *value);
json_string_t * create_json_string_at_end_of_array(json_array_t *iface, const wchar_t *value);

json_number_t * create_json_number(double value);
json_number_t * create_json_number_at_end_of_array(json_array_t *iface, double value);

json_boolean_t * create_json_boolean(bool value);
json_boolean_t * create_json_boolean_at_end_of_array(json_array_t *iface, bool value);

wide_string_t * json_element_to_simple_string(const json_element_base_t *iface);