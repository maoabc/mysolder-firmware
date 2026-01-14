
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



/* usbc.rst port data object start */
struct port0_data_t {
	/** Sink Capabilities */
	uint32_t snk_caps[DT_PROP_LEN(USBC_PORT0_NODE, sink_pdos)];
	/** Number of Sink Capabilities */
	int snk_cap_cnt;
	/** Source Capabilities */
	uint32_t src_caps[PDO_MAX_DATA_OBJECTS];
	/** Number of Source Capabilities */
	int src_cap_cnt;
    uint8_t req_idx;
	/* Power Supply Ready flag */
	atomic_t ps_ready;
};



#ifdef CONFIG_IS_T12
#define MAX_REQUEST_VOLTAGE 20000

// 用于限制固定档位PDO索引，防止pdo索引读取错误导致请求电压过大
// ucpd的bug,有时pdo读取不全，可能导致请求电压档位过高
#define MAX_FIXED_PDO_IDX   4
#else
#define MAX_REQUEST_VOLTAGE 15000
#define MAX_FIXED_PDO_IDX   3
#endif



struct app;

void pd_start(struct app *app);

bool check_pd_ready(struct port0_data_t *data);

void pd_send_hard_reset();

#endif //__USB_PD_H_
