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
        number_t *num_value;
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

void destroy_json_element(const json_element_base_t *iface)
{
    element_t *this = (element_t*)iface;
    if (this)
        destructors[this->type](this);
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

// --- object methods ---------------------------------------------------------

json_pair_t * get_pair_from_json_object(const json_object_data_t *iface, const wchar_t *key)
{
    private_object_data_t *object = (private_object_data_t*)iface;
    wide_string_t key_w = _W(key);
    return (json_pair_t*)get_pair_from_tree_map(&object->base, &key_w);
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

// --- array methods ----------------------------------------------------------

json_element_t * get_element_from_json_array(const json_array_data_t *iface, size_t index)
{
    private_array_data_t *array = (private_array_data_t*)iface;
    return (json_element_t*)get_vector_item(&array->base, index);
}

// --- string constructors ----------------------------------------------------

static __inline element_t * instantiate_json_string(const wchar_t *value)
{
    element_t *elem = nnalloc(sizeof(element_t));
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

static __inline element_t * instantiate_json_number(number_t *value)
{
    element_t *elem = nnalloc(sizeof(element_t) + sizeof(number_t));
    elem->type = json_number;
    elem->data.num_value = (number_t*)(elem + 1);
    memcpy(elem->data.num_value, value, sizeof(number_t));
    return elem;
}

json_number_t * create_json_number(real_t value)
{
    number_t num;
    init_number_by_real(&num, value);
    element_t *elem = instantiate_json_number(&num);
    elem->parent = NULL;
    return (json_number_t*)elem;
}

json_number_t * create_json_number_at_end_of_array(json_array_t *iface, real_t value)
{
    element_t *this = (element_t*)iface;
    number_t num;
    init_number_by_real(&num, value);
    element_t *elem = instantiate_json_number(&num);
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
    char buff[64];
    print_number(buff, elem->data.num_value);
    return append_formatted_wide_string(builder, L"%s", buff);
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

// --- error ------------------------------------------------------------------

const wchar_t *str_error[] =
{
    L"ok",
    L"unknown symbol",
    L"incorrect number format",
    L"incorrect escape character",
    L"missing closing quotation mark in string",
    L"missing closing bracket",
    L"unrecognized entity",
    L"expected comma as a separator",
    L"expected colon as a separator",
    L"expected a name",
    L"expected an element"
};

wide_string_t * json_error_to_string(const json_error_t *err)
{
    wide_string_builder_t *buff = append_formatted_wide_string(NULL, L"%d.%d, %w",
        err->where.row, err->where.column, str_error[err->type]);
    if (err->text.length)
        buff = append_formatted_wide_string(buff, L": '%W'", err->text);
    return (wide_string_t*)buff;
}

// --- parser -----------------------------------------------------------------

static __inline bool is_space(wchar_t c)
{
    return c == L' ' || c == L'\t' || c == L'\n' || c == L'\r';
}

static __inline bool is_letter(wchar_t c)
{
    return (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') || c == '_';
}

static __inline bool is_digit(wchar_t c)
{
    return c >= L'0' && c <= L'9';
}

static __inline bool is_hex_digit(wchar_t c)
{
    return (c >= L'0' && c <= L'9') || (c >= L'A' && c <= L'F') || (c >= L'a' && c <= L'f');
}

static __inline int convert_hex_digit(wchar_t c)
{
    if (c >= L'0' && c <= L'9') return c - L'0';
    if (c >= L'A' && c <= L'F') return c - L'A' + 10;
    if (c >= L'a' && c <= L'f') return c - L'a' + 10;
    return -1;
}

typedef struct
{
    wide_string_t *text;
    size_t index;
    json_position_t pos;
} source_t;

static __inline void init_source(source_t *src, wide_string_t *text)
{
    src->text = text;
    src->index = 0;
    src->pos.row = 1;
    src->pos.column = 1;
}

static __inline wchar_t get_char(source_t *src)
{
    return src->index < src->text->length ? src->text->data[src->index] : L'\0';
}

static __inline wchar_t next_char(source_t *src)
{
    if (src->index < src->text->length)
    {
        wchar_t c = src->text->data[src->index];
        if (c == L'\n')
        {
            src->pos.row++;
            src->pos.column = 1;            
        }
        else if (c == L'\r')
        {
            src->pos.column = 1;
        }
        else
        {
            src->pos.column++;
        }
        src->index++;
        return get_char(src);
    }
    return '\0';
}

static __inline wchar_t get_char_but_not_space(source_t *src)
{
    wchar_t c = get_char(src);
    while(is_space(c))
    {
        c = next_char(src);
    }
    return c;
}

static __inline wchar_t next_char_but_not_space(source_t *src)
{
    wchar_t c = next_char(src);
    while(is_space(c))
    {
        c = next_char(src);
    }
    return c;
}

static wide_string_t * parse_string(source_t *src, json_error_t *err);
static element_t * parse_element(source_t *src, json_error_t *err);

static element_t * parse_object(source_t *src, json_error_t *err)
{
    element_t *obj = instantiate_json_object();
    size_t count = 0;
    
    while(true)
    {
        wchar_t c = get_char_but_not_space(src);
                    
        if (c == L'\0')
        {
            if (err)
            {
                err->type = json_missing_closing_bracket;
                err->text.data[0] = L'}';
                err->text.length = 1;
            }
            json_object_destructor(obj);
            return NULL;
        }
        if (c == L'}')
        {
            next_char(src);
            return obj;
        }
        if (count > 0)
        {
            if (c != L',')
            {
                if (err)
                    err->type = json_expected_comma_separator;
                json_object_destructor(obj);
                return NULL;
            }
            c = next_char_but_not_space(src);
            if (c == 0)
            {
                if (err)
                {
                    err->type = json_missing_closing_bracket;
                    err->text.data[0] = L'}';
                    err->text.length = 1;
                }
                json_object_destructor(obj);
                return NULL;
            }
            if (c == L'}')
            {
                next_char(src);
                return obj;
            }
        }
        wide_string_t *name = NULL;
        if (c == L'\"')
        {
            next_char(src);
            name = parse_string(src, err);
            if (!name)
                return NULL;
        }
        else if (is_letter(c))
        {
            wide_string_builder_t *b = NULL;
            do
            {
                b = append_wide_char(b, c);;
                c = next_char(src);
            } while(is_letter(c) || is_digit(c));
            name = (wide_string_t*)b;
        }
        else
        {
            if (err)
                err->type = json_expected_name;
            json_object_destructor(obj);
            return NULL;
        }
        c = get_char_but_not_space(src);
        if (c != L':')
        {
            if (err)
                err->type = json_expected_colon_separator;
            json_object_destructor(obj);
            return NULL;
        }
        c = next_char_but_not_space(src);
        if (c == 0)
        {
            if (err)
                err->type = json_expected_element;
            json_object_destructor(obj);
            return NULL;
        }
        element_t *value = parse_element(src, err);
        if (!value)
            return NULL;
        add_pair_to_tree_map(&obj->data.object->base, name, value);
        value->parent = (json_element_t*)obj;
        count++;
    }
}

static element_t * parse_array(source_t *src, json_error_t *err)
{
    element_t *elem = instantiate_json_array();
    size_t count = 0;
    
    while(true)
    {
        wchar_t c = get_char_but_not_space(src);
                    
        if (c == L'\0')
        {
            if (err)
            {
                err->type = json_missing_closing_bracket;
                err->text.data[0] = L'}';
                err->text.length = 1;
            }
            json_array_destructor(elem);
            return NULL;
        }
        if (c == L']')
        {
            next_char(src);
            return elem;
        }
        if (count > 0)
        {
            if (c != L',')
            {
                if (err)
                    err->type = json_expected_comma_separator;
                json_array_destructor(elem);;
                return NULL;
            }
            c = next_char_but_not_space(src);
            if (c == 0)
            {
                if (err)
                {
                    err->type = json_missing_closing_bracket;
                    err->text.data[0] = L']';
                    err->text.length = 1;
                }
                json_array_destructor(elem);;
                return NULL;
            }
            if (c == L']')
            {
                next_char(src);
                return elem;
            }
        }
        element_t *child = parse_element(src, err);
        if (!child)
            return NULL;
        add_item_to_vector(&elem->data.array->base, child);
        child->parent = (json_element_t*)elem;
        count++;
    }
}

static wide_string_t * parse_string(source_t *src, json_error_t *err)
{
    wide_string_builder_t *b = NULL;
    wchar_t c = get_char(src);
    while (c != L'\"' && c != L'\0')
    {
        if (c == L'\\')
        {
            c = next_char(src);
            switch(c)
            {
                case L'"':
                    b = append_wide_char(b, L'"');
                    break;
                case '\\':
                    b = append_wide_char(b, L'\\');
                    break;
                case '/':
                    b = append_wide_char(b, L'/');
                    break;
                case 'b':
                    b = append_wide_char(b, L'\b');
                    break;
                case 'f':
                    b = append_wide_char(b, L'\f');
                    break;
                case 'n':
                    b = append_wide_char(b, L'\n');
                    break;
                case 'r':
                    b = append_wide_char(b, L'\r');
                    break;
                case 't':
                    b = append_wide_char(b, L'\t');
                    break;
                case 'u':
                {
                    int k = 0;
                    wchar_t h[4];
                    for (size_t i = 0; i < 4; i++)
                    {
                        c = next_char(src);
                        h[i] = c;
                        if (!is_hex_digit(c))
                        {
                            if (err)
                            {
                                err->type = json_incorrect_number_format;
                                memcpy(err->text.data, h, (i + 1) * sizeof(wchar_t));
                                err->text.length = i + 1;
                            }
                            free(b);
                            return NULL;
                        }
                        k = (k << 4) | convert_hex_digit(c);
                    }
                    c = (wchar_t)k;
                    b = append_wide_char(b, c);
                    break;				
                }
                default:
                    if (err)
                    {
                        err->type = json_incorrect_escape_character;
                        err->text.data[0] = c;
                        err->text.length = 1;
                    }
                    free(b);
                    return NULL;
            }
        }
        else
        {
            b = append_wide_char(b, c);
        }
        c = next_char(src);
    }
    if (c == 0)
    {
        if (err)
            err->type = json_missing_closing_quotation_mark_in_string;
        free(b);
        return NULL;
    }
    next_char(src);
    return (wide_string_t*)b;
}

static element_t * parse_number_element(source_t *src, bool neg, json_error_t *err)
{
    string_builder_t *b = NULL;
    wchar_t c = get_char(src);
    do
    {
        b = append_char(b, (char)c);
        c = next_char(src);
    } while(is_digit(c));
    if (c == '.')
    {
        b = append_char(b, (char)c);
        c = next_char(src);
        if (is_digit(c))
        {
            do
            {
                b = append_char(b, (char)c);
                c = next_char(src);
            } while(is_digit(c));
        }
        else
            goto error;        
    }
    if (c == L'e' || c == L'E')
    {
        b = append_char(b, (char)c);
        c = next_char(src);
        if (c == '+' || c == '-')
        {
            b = append_char(b, (char)c);
            c = next_char(src);
        }
        if (is_digit(c))
        {
            do
            {
                b = append_char(b, (char)c);
                c = next_char(src);
            } while(is_digit(c));
        }
        else
            goto error;        
    }
    number_t result;
    parse_number(&result, b->data);
    if (!result.is_number)
        goto error;
    free(b);
    if (neg)
        negate_number(&result);
    return instantiate_json_number(&result);

error:
    if (err)
    {
        b = append_char(b, (char)c);
        err->type = json_incorrect_number_format;
        err->text.length = b->length < json_error_text_max_length ? b->length : json_error_text_max_length;
        for (size_t k = 0; k < err->text.length; k++)
            err->text.data[k] = b->data[k];
    }
    free(b);
    return NULL;
}

static element_t * parse_element(source_t *src, json_error_t *err)
{
    wchar_t c = get_char_but_not_space(src);
    
    if (c == L'{')
    {
        next_char(src);
        return parse_object(src, err);
    }
    else if (c == L'[')
    {
        next_char(src);
        return parse_array(src, err);
    }
    else if (c == L'\"')
    {
        next_char(src);
        wide_string_t *text = parse_string(src, err);
        if (text)
        {
            element_t *elem = nnalloc(sizeof(element_t));
            elem->type = json_string;
            elem->data.string_value = text;
            return elem;
        }
        return NULL;
    }
    else if (is_letter(c))
    {
        wide_string_builder_t *b = create_wide_string_builder(8);
        do
        {
            b = append_wide_char(b, c);
            c = next_char(src);
        } while (is_letter(c));
        wide_string_t *ws = (wide_string_t*)b;
        
        if (are_wide_strings_equal(*ws, __W(L"null")))
            return instantiate_json_null();
        if (are_wide_strings_equal(*ws, __W(L"true")))
            return instantiate_json_boolean(true);
        if (are_wide_strings_equal(*ws, __W(L"false")))
            return instantiate_json_boolean(false);

        if (err)
        {
            err->type = json_unrecognized_entity;
            err->text.length = ws->length < json_error_text_max_length ? ws->length : json_error_text_max_length;
            memcpy(err->text.data, ws->data, err->text.length * sizeof(wchar_t));
        }
        return NULL;
    }
    else if (is_digit(c))
    {
        return parse_number_element(src, false, err);
    }
    else if (c == '-')
    {
        next_char(src);
        return parse_number_element(src, true, err);
    }

    if (err)
    {
        err->type = json_unknown_symbol;
        err->text.data[0] = c;
        err->text.length = 1;
    }
    return NULL;
}

json_element_t * parse_json_ext(wide_string_t *text, json_error_t *err)
{
    source_t src;
    init_source(&src, text);
    if (err)
    {
        memset(err, 0, sizeof(json_error_t));
        err->text.data = (wchar_t*)err->_buff;
    }
    element_t *root = parse_element(&src, err);
    if (err && err->type != json_ok)
        err->where = src.pos;
    if (root)
        root->parent = NULL;
    return (json_element_t*)root;
}

json_element_t * parse_json(wide_string_t *text)
{
    return parse_json_ext(text, NULL);
}
