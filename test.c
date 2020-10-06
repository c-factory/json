#include "json.h"

int main(void)
{
    json_element_t *elem = create_json_string(NULL);
    destroy_json_element(elem);
    return 0;
}
