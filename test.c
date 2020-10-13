#include "json.h"
#include "strings/strings.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    wide_string_t src = __W(L"\"hello, \\\\\"");
    json_element_t *root = parse_json(&src);

    destroy_json_element(&root->base);
    return 0;
}
