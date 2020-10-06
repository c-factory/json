/*
    Copyright (c) 2020 Ivan Kniazkov <ivan.kniazkov.com>

    The implementation of the JSON parser
*/

#include "json.h"
#include "allocator.h"

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

json_element_t * create_json_string(const wchar_t *c_str)
{
    size_t str_length = c_str ? wcslen(c_str) : 0;
    json_element_impl_t *elem = get_system_allocator()->allocate(sizeof(json_element_impl_t) + (str_length + 1 * sizeof(wchar_t)));
    elem->parent = NULL;
    elem->type = json_string;
    elem->data.string = (wchar_t*)(elem + 1);
    if (str_length)
        memcpy(elem->data.string, c_str, str_length * sizeof(wchar_t));
    elem->data.string[str_length] = L'\0';
    return (json_element_t*)elem;
}
