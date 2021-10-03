#ifndef VMI_H
#define VMI_H

// clang-format off
#include <libvmi/libvmi.h>
#include <libvmi/events.h>
// clang-format on

bool setup_vmi(vmi_instance_t *vmi, char *socket, char *json);
void loop(vmi_instance_t vmi);

#endif
