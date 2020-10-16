#include "json.h"
#include "strings/strings.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    wide_string_t src = __W(L"{ a : 1, \"b\" : true, zzz : [\"hello\", null, {}] }");
    json_error_t err;
    json_element_t *root = parse_json_ext(&src, &err);
    
    wide_string_t *result;
    if (root)
    {
        result = json_element_to_simple_string(&root->base);
        destroy_json_element(&root->base);
    }
    else
    {
        result = json_error_to_string(&err);
    }    
    wprintf(L"%s", result->data);
    free(result);
    return 0;
}
