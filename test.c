#include "json.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    json_object_t *obj = create_json_object();
    create_json_string_owned_by_object(obj, L"zero", L"0");
    create_json_string_owned_by_object(obj, L"one", L"1");
    create_json_string_owned_by_object(obj, L"two", L"2");
    wide_string_t *wstr = json_element_to_simple_string((json_element_t*)obj);
    wprintf(L"%s", wstr->data);
    free(wstr);
    destroy_json_element((json_element_t*)obj);
    return 0;
}
