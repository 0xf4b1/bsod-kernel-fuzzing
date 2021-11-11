#include <stdlib.h>

#include "private.h"

bool setup_vmi(vmi_instance_t *vmi, char *socket, char *json) {
    vmi_init_data_t *init_data = malloc(sizeof(vmi_init_data_t) + sizeof(vmi_init_data_entry_t));
    init_data->count = 1;
    init_data->entry[0].type = VMI_INIT_DATA_KVMI_SOCKET;
    init_data->entry[0].data = strdup(socket);

    if (VMI_FAILURE ==
        vmi_init(vmi, VMI_KVM, NULL, VMI_INIT_EVENTS | VMI_INIT_DOMAINNAME, init_data, NULL))
        return false;

    if (VMI_OS_UNKNOWN == vmi_init_os(*vmi, VMI_CONFIG_JSON_PATH, json, NULL)) {
        vmi_destroy(*vmi);
        return false;
    }

    if (mode == DYNAMIC && cs_handle == 0) {
        if (cs_open(CS_ARCH_X86, vmi_get_page_mode(*vmi, 0) == VMI_PM_IA32E ? CS_MODE_64 : CS_MODE_32, &cs_handle)) {
            fprintf(stderr, "Capstone init failed\n");
            exit(-1);
        }
    }

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
