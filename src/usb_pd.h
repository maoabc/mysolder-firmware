
#ifndef __USB_PD_H_

#define __USB_PD_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/usb_c/usbc.h>


#define USBC_PORT0_NODE       DT_ALIAS(usbc_port0)
#define USBC_PORT0_POWER_ROLE DT_ENUM_IDX(USBC_PORT0_NODE, power_role)

#if (USBC_PORT0_POWER_ROLE != TC_ROLE_SINK)
#error "Unsupported board: Only Sink device supported"
#endif

#define SINK_PDO(node_id, prop, idx) (DT_PROP_BY_IDX(node_id, prop, idx)),




struct app;

void pd_start(struct app *app);

bool check_pd_ready(const struct device *port);

uint16_t pd_get_requested_voltage(const struct device *port);

void pd_send_hard_reset(const struct device *port);

#endif //__USB_PD_H_
