/*
    Copyright (c) 2020 Ivan Kniazkov <ivan.kniazkov.com>

    The implementation of the JSON parser
*/

#include <assert.h>
#include "json.h"
#include "allocator.h"
#include "tree_map.h"

typedef struct
{
    tree_map_t base;
} json_object_data_impt_t;

typedef struct 
{
    json_element_type_t type;
    const json_element_t *parent;
    union 
    {
        json_object_data_impt_t *object;
        wide_string_t *string;
    } data;
} json_element_impl_t;

typedef void (*destructor_t)(json_element_impl_t *elem);

static void json_object_destructor(json_element_impl_t *elem)
{
    assert(elem->type = json_object);
    destroy_tree_map_and_content(&elem->data.object->base, free, (void*)destroy_json_element);
    free(elem);
}

static void json_string_destructor(json_element_impl_t *elem)
{
    assert(elem->type = json_string);
    free(elem->data.string);
    free(elem);
}

static destructor_t destructors[] =
{
    NULL,
    json_object_destructor,
    NULL,
    json_string_destructor,
    NULL,
    NULL
};

void destroy_json_element(json_element_t *iface)
{
    json_element_impl_t *this = (json_element_impl_t*)iface;    
    destructors[this->type](this);
}

json_object_t * create_json_object()
{
    json_element_impl_t *elem = nnalloc(sizeof(json_element_impl_t));
    elem->parent = NULL;
    elem->type = json_object;
    elem->data.object = (json_object_data_impt_t*)create_tree_map((void*)compare_wide_strings);
    return (json_object_t*)elem;
}

json_string_t * create_json_string(const wchar_t *value)
{
    json_element_impl_t *elem = nnalloc(sizeof(json_element_impl_t));
    wide_string_t *string = (wide_string_t*)(elem + 1);
    elem->parent = NULL;
    elem->type = json_string;
    elem->data.string = duplicate_wide_string(_W(value));
    return (json_string_t*)elem;
}

json_string_t * create_json_string_owned_by_object(json_object_t *iface, const wchar_t *key, const wchar_t *value)
{
    json_element_impl_t *this = (json_element_impl_t*)iface;
    wide_string_t *key_copy = duplicate_wide_string(_W(key));
    json_element_impl_t *new_elem = (json_element_impl_t*)create_json_string(value);
    new_elem->parent = (json_element_t*)iface;
    json_element_t *old_elem = (json_element_t*)add_pair_to_tree_map(&this->data.object->base, key_copy, new_elem);
    if (old_elem)
    {
        free(key_copy);
        destroy_json_element(old_elem);
    }
    return (json_string_t*)new_elem;
}

typedef wide_string_builder_t * (*simple_string_builder_t)(json_element_impl_t *elem, wide_string_builder_t *builder);

static wide_string_builder_t * json_object_to_simple_string(json_element_impl_t *elem, wide_string_builder_t *builder);
static wide_string_builder_t * json_string_to_simple_string(json_element_impl_t *elem, wide_string_builder_t *builder);

static simple_string_builder_t simple_string_builders[] =
{
    NULL,
    json_object_to_simple_string,
    NULL,
    json_string_to_simple_string,
    NULL,
    NULL
};

static wide_string_builder_t * json_object_to_simple_string(json_element_impl_t *elem, wide_string_builder_t *builder)
{
    assert(elem->type = json_object);
    builder = append_wide_string(builder, _W(L"{"));
    map_iterator_t *iter = create_iterator_from_tree_map(&elem->data.object->base);
    bool flag = false;
    while(has_next_pair(iter))
    {
        if (flag)
            builder = append_wide_string(builder, _W(L", "));
        flag = true;
        json_pair_t *pair = (json_pair_t*)next_pair(iter);
        builder = append_formatted_wide_string(builder, L"\"%W\":", *pair->key);
        json_element_impl_t *value = (json_element_impl_t*)pair->value;
        builder = simple_string_builders[value->type](value, builder);
    }
    destroy_map_iterator(iter);
    builder = append_wide_string(builder, _W(L"}"));
    return builder;
}

static wide_string_builder_t * json_string_to_simple_string(json_element_impl_t *elem, wide_string_builder_t *builder)
{
    assert(elem->type = json_string);
    return append_formatted_wide_string(builder, L"\"%W\"", *elem->data.string);
}

wide_string_t * json_element_to_simple_string(json_element_t *iface)
{
    json_element_impl_t *this = (json_element_impl_t*)iface;    
    return (wide_string_t*)simple_string_builders[this->type](this, NULL);
}
