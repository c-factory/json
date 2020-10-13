/*
    Copyright (c) 2020 Ivan Kniazkov <ivan.kniazkov.com>

    The implementation of the JSON parser
*/

#include <assert.h>
#include "json.h"
#include "allocator.h"
#include "tree_map.h"
#include "vector.h"

typedef struct
{
    tree_map_t base;
} private_object_data_t;

typedef struct
{
    vector_t base;
} private_array_data_t;

typedef struct 
{
    json_element_type_t type;
    const json_element_t *parent;
    union 
    {
        private_object_data_t *object;
        private_array_data_t *array;
        wide_string_t *string_value;
        double num_value;
        bool bool_value;;
    } data;
} element_t;

// --- destructor -------------------------------------------------------------

typedef void (*destructor_t)(element_t *elem);

static void json_null_destructor(element_t *elem);
static void json_object_destructor(element_t *elem);
static void json_array_destructor(element_t *elem);
static void json_string_destructor(element_t *elem);
static void json_number_destructor(element_t *elem);
static void json_boolean_destructor(element_t *elem);

static destructor_t destructors[] =
{
    json_null_destructor,
    json_object_destructor,
    json_array_destructor,
    json_string_destructor,
    json_number_destructor,
    json_boolean_destructor
};

static void json_null_destructor(element_t *elem)
{
    assert(elem->type == json_null);
    free(elem);
}

static void json_object_destructor(element_t *elem)
{
    assert(elem->type == json_object);
    destroy_tree_map_and_content(&elem->data.object->base, free, (void*)destroy_json_element);
    free(elem);
}

static void json_array_destructor(element_t *elem)
{
    assert(elem->type == json_array);
    destroy_vector_and_content(&elem->data.array->base, (void*)destroy_json_element);
    free(elem);
}

static void json_string_destructor(element_t *elem)
{
    assert(elem->type == json_string);
    free(elem->data.string_value);
    free(elem);
}

void destroy_json_element(const json_element_base_t *iface)
{
    element_t *this = (element_t*)iface;    
    destructors[this->type](this);
}

static void json_number_destructor(element_t *elem)
{
    assert(elem->type == json_number);
    free(elem);
}

static void json_boolean_destructor(element_t *elem)
{
    assert(elem->type == json_boolean);
    free(elem);
}

// --- null constructors ------------------------------------------------------

static __inline element_t * instantiate_json_null()
{
    element_t *elem = nnalloc(sizeof(element_t));
    elem->type = json_null;
    elem->data.object = NULL;
    return elem;
}

json_null_t * create_json_null()
{
    element_t *elem = instantiate_json_null();
    elem->parent = NULL;
    return (json_null_t*)elem;
}

json_null_t * create_json_null_at_end_of_array(json_array_t *iface)
{
    element_t *this = (element_t*)iface;
    element_t *elem = instantiate_json_null();
    elem->parent = (json_element_t*)iface;
    add_item_to_vector(&this->data.array->base, elem);
    return (json_null_t*)elem;
}

// --- object constructors ----------------------------------------------------

static __inline element_t * instantiate_json_object()
{
    element_t *elem = nnalloc(sizeof(element_t));
    elem->type = json_object;
    elem->data.object = (private_object_data_t*)create_tree_map((void*)compare_wide_strings);
    return elem;
}

json_object_t * create_json_object()
{
    element_t *elem = instantiate_json_object();
    elem->parent = NULL;
    return (json_object_t*)elem;
}

// --- array constructors -----------------------------------------------------

static __inline element_t * instantiate_json_array()
{
    element_t *elem = nnalloc(sizeof(element_t));
    elem->type = json_array;
    elem->data.array = (private_array_data_t*)create_vector();
    return elem;
}

json_array_t * create_json_array()
{
    element_t *elem = instantiate_json_array();
    elem->parent = NULL;
    return (json_array_t*)elem;
}

// --- string constructors ----------------------------------------------------

static __inline element_t * instantiate_json_string(const wchar_t *value)
{
    element_t *elem = nnalloc(sizeof(element_t));
    wide_string_t *string = (wide_string_t*)(elem + 1);
    elem->type = json_string;
    elem->data.string_value = duplicate_wide_string(_W(value));
    return elem;
}

json_string_t * create_json_string(const wchar_t *value)
{
    element_t *elem = instantiate_json_string(value);
    elem->parent = NULL;
    return (json_string_t*)elem;
}

json_string_t * create_json_string_owned_by_object(json_object_t *iface, const wchar_t *key, const wchar_t *value)
{
    element_t *this = (element_t*)iface;
    wide_string_t *key_copy = duplicate_wide_string(_W(key));
    element_t *new_elem = instantiate_json_string(value);
    new_elem->parent = (json_element_t*)this;
    json_element_t *old_elem = (json_element_t*)add_pair_to_tree_map(&this->data.object->base, key_copy, new_elem);
    if (old_elem)
    {
        free(key_copy);
        destroy_json_element(&old_elem->base);
    }
    return (json_string_t*)new_elem;
}

json_string_t * create_json_string_at_end_of_array(json_array_t *iface, const wchar_t *value)
{
    element_t *this = (element_t*)iface;
    element_t *elem = instantiate_json_string(value);
    elem->parent = (json_element_t*)this;
    add_item_to_vector(&this->data.array->base, elem);
    return (json_string_t*)elem;
}

// --- number constructors ----------------------------------------------------

static __inline element_t * instantiate_json_number(double value)
{
    element_t *elem = nnalloc(sizeof(element_t));
    elem->type = json_number;
    elem->data.num_value = value;
    return elem;
}

json_number_t * create_json_number(double value)
{
    element_t *elem = instantiate_json_number(value);
    elem->parent = NULL;
    return (json_number_t*)elem;
}

json_number_t * create_json_number_at_end_of_array(json_array_t *iface, double value)
{
    element_t *this = (element_t*)iface;
    element_t *elem = instantiate_json_number(value);
    elem->parent = (json_element_t*)iface;
    add_item_to_vector(&this->data.array->base, elem);
    return (json_number_t*)elem;
}

// --- boolean constructors ---------------------------------------------------

static __inline element_t * instantiate_json_boolean(bool value)
{
    element_t *elem = nnalloc(sizeof(element_t));
    elem->type = json_boolean;
    elem->data.bool_value = value;
    return elem;
}

json_boolean_t * create_json_boolean(bool value)
{
    element_t *elem = instantiate_json_boolean(value);
    elem->parent = NULL;
    return (json_boolean_t*)elem;
}

json_boolean_t * create_json_boolean_at_end_of_array(json_array_t *iface, bool value)
{
    element_t *this = (element_t*)iface;
    element_t *elem = instantiate_json_boolean(value);
    elem->parent = (json_element_t*)iface;
    add_item_to_vector(&this->data.array->base, elem);
    return (json_boolean_t*)elem;
}

// --- stringify (to simple string) -------------------------------------------

typedef wide_string_builder_t * (*simple_string_builder_t)(element_t *elem, wide_string_builder_t *builder);

static wide_string_builder_t * json_null_to_simple_string(element_t *elem, wide_string_builder_t *builder);
static wide_string_builder_t * json_object_to_simple_string(element_t *elem, wide_string_builder_t *builder);
static wide_string_builder_t * json_array_to_simple_string(element_t *elem, wide_string_builder_t *builder);
static wide_string_builder_t * json_string_to_simple_string(element_t *elem, wide_string_builder_t *builder);
static wide_string_builder_t * json_number_to_simple_string(element_t *elem, wide_string_builder_t *builder);
static wide_string_builder_t * json_boolean_to_simple_string(element_t *elem, wide_string_builder_t *builder);

static simple_string_builder_t simple_string_builders[] =
{
    json_null_to_simple_string,
    json_object_to_simple_string,
    json_array_to_simple_string,
    json_string_to_simple_string,
    json_number_to_simple_string,
    json_boolean_to_simple_string
};

static wide_string_builder_t * json_null_to_simple_string(element_t *elem, wide_string_builder_t *builder)
{
    assert(elem->type == json_null);
    return append_wide_string(builder, __W(L"null"));
}

static wide_string_builder_t * json_object_to_simple_string(element_t *elem, wide_string_builder_t *builder)
{
    assert(elem->type == json_object);
    builder = append_wide_string(builder, __W(L"{"));
    map_iterator_t *iter = create_iterator_from_tree_map(&elem->data.object->base);
    bool flag = false;
    while(has_next_pair(iter))
    {
        if (flag)
            builder = append_wide_string(builder, __W(L", "));
        flag = true;
        json_pair_t *pair = (json_pair_t*)next_pair(iter);
        builder = append_formatted_wide_string(builder, L"\"%W\": ", *pair->key);
        element_t *value = (element_t*)pair->value;
        builder = simple_string_builders[value->type](value, builder);
    }
    destroy_map_iterator(iter);
    builder = append_wide_string(builder, __W(L"}"));
    return builder;
}

static wide_string_builder_t * json_array_to_simple_string(element_t *elem, wide_string_builder_t *builder)
{
    assert(elem->type == json_array);
    builder = append_wide_string(builder, __W(L"["));
    vector_index_t i;
    vector_t *vector = &elem->data.array->base;
    for (i = 0; i < vector->size; i++)
    {
        if (i)
            builder = append_wide_string(builder, __W(L", "));
        element_t *child = (element_t*)vector->data[i];
        builder = simple_string_builders[child->type](child, builder);
    }
    builder = append_wide_string(builder, __W(L"]"));
    return builder;
}

static wide_string_builder_t * json_string_to_simple_string(element_t *elem, wide_string_builder_t *builder)
{
    assert(elem->type == json_string);
    return append_formatted_wide_string(builder, L"\"%W\"", *elem->data.string_value);
}

static wide_string_builder_t * json_number_to_simple_string(element_t *elem, wide_string_builder_t *builder)
{
    assert(elem->type == json_number);
    return append_formatted_wide_string(builder, L"\"%f\"", elem->data.num_value);
}

static wide_string_builder_t * json_boolean_to_simple_string(element_t *elem, wide_string_builder_t *builder)
{
    assert(elem->type == json_boolean);
    return append_wide_string(builder, elem->data.bool_value ? __W(L"true") : __W(L"false"));
}

wide_string_t * json_element_to_simple_string(const json_element_base_t *iface)
{
    element_t *this = (element_t*)iface;    
    return (wide_string_t*)simple_string_builders[this->type](this, NULL);
}
