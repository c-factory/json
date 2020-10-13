#include "json.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    json_array_t *arr = create_json_array();
    create_json_string_at_end_of_array(arr, L"one");
    create_json_string_at_end_of_array(arr, L"two");
    create_json_string_at_end_of_array(arr, L"three");
    create_json_null_at_end_of_array(arr);
    create_json_number_at_end_of_array(arr, 13);
    create_json_boolean_at_end_of_array(arr, true);
    wide_string_t *wstr = json_element_to_simple_string(&arr->base);
    wprintf(L"%s", wstr->data);
    free(wstr);
    destroy_json_element(&arr->base);
    return 0;
}
