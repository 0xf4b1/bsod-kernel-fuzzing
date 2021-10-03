#ifndef TRACER_DYNAMIC_H
#define TRACER_DYNAMIC_H

event_response_t handle_event_dynamic(vmi_instance_t vmi, vmi_event_t *event);
bool start_trace(vmi_instance_t vmi, addr_t address);

#endif
