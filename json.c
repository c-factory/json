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
} json_object_impt_t;

typedef struct 
{
    json_element_type_t type;
    const json_element_t *parent;
    union 
    {
        json_object_t *object;
        wchar_t * string;
    } data;
} json_element_impl_t;

static void object_destructor(json_element_impl_t *elem)
{
    assert(elem->type = json_object);
    destroy_tree_map((tree_map_t*)elem->data.object);
    free(elem);
}

static void string_destructor(json_element_impl_t *elem)
{
    assert(elem->type = json_string);
    free(elem);
}

typedef void (*destructor_t)(json_element_impl_t *elem);
static destructor_t destructors[] =
{
    NULL,
    object_destructor,
    NULL,
    string_destructor,
    NULL,
    NULL
};

void destroy_json_element(json_element_t *iface)
{
    json_element_impl_t *this = (json_element_impl_t*)iface;    
    destructors[this->type](this);
}

json_element_t * create_json_object()
{
    json_element_impl_t *elem = nnalloc(sizeof(json_element_impl_t));
    elem->parent = NULL;
    elem->type = json_object;
    elem->data.object = (json_object_t*)create_tree_map((void*)wcscmp);
    return (json_element_t*)elem;
}

json_element_t * create_json_string(const wchar_t *c_str)
{
    size_t str_length = c_str ? wcslen(c_str) : 0;
    json_element_impl_t *elem = nnalloc(sizeof(json_element_impl_t) + (str_length + 1 * sizeof(wchar_t)));
    elem->parent = NULL;
    elem->type = json_string;
    elem->data.string = (wchar_t*)(elem + 1);
    memcpy(elem->data.string, c_str, str_length * sizeof(wchar_t));
    elem->data.string[str_length] = L'\0';
    return (json_element_t*)elem;
}
