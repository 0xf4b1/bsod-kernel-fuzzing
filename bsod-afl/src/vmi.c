#include <stdlib.h>

#include "vmi.h"

extern os_t os;
extern int interrupted;
extern bool failure;
extern page_mode_t pm;

bool setup_vmi(vmi_instance_t *vmi, char *socket, char *json) {
    vmi_init_data_t *init_data = malloc(sizeof(vmi_init_data_t) + sizeof(vmi_init_data_entry_t));
    init_data->count = 1;
    init_data->entry[0].type = VMI_INIT_DATA_KVMI_SOCKET;
    init_data->entry[0].data = strdup(socket);

    if (VMI_FAILURE ==
        vmi_init(vmi, VMI_KVM, NULL, VMI_INIT_EVENTS | VMI_INIT_DOMAINNAME, init_data, NULL))
        return false;

    if (VMI_OS_UNKNOWN == (os = vmi_init_os(*vmi, VMI_CONFIG_JSON_PATH, json, NULL))) {
        vmi_destroy(*vmi);
        return false;
    }

    pm = vmi_get_page_mode(*vmi, 0);

    return true;
}

void loop(vmi_instance_t vmi) {
    if (!vmi)
        return;

    vmi_resume_vm(vmi);

    while (!interrupted && !failure) {
        if (vmi_events_listen(vmi, 500) == VMI_FAILURE) {
            fprintf(stderr, "Error in vmi_events_listen!\n");
            break;
        }
    }
}
