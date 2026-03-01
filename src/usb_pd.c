

#include <zephyr/drivers/usb_c/usbc_tcpc.h>

#include "app_ui.h"
#include "usb_pd.h"
#include "zephyr/usb_c/usbc.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbc, LOG_LEVEL_DBG);

/**
 * @brief A structure that encapsulates Port data.
 */
static struct port0_data_t port0_data = {
	.snk_caps = {DT_FOREACH_PROP_ELEM(USBC_PORT0_NODE, sink_pdos, SINK_PDO)},
	.snk_cap_cnt = DT_PROP_LEN(USBC_PORT0_NODE, sink_pdos),
	.src_caps = {0},
	.src_cap_cnt = 0,
	.req_idx = 0,
	.ps_ready = 0};

/* usbc.rst port data object end */

/**
 * @brief Builds a Request Data Object (RDO) with the following properties:
 *		- Maximum operating current 100mA
 *		- Operating current is 100mA
 *		- Unchunked Extended Messages Not Supported
 *		- No USB Suspend
 *		- Not USB Communications Capable
 *		- No capability mismatch
 *		- Does not Giveback
 *		- Select object position 1 (5V Power Data Object (PDO))
 *
 * @note Generally a sink application would build an RDO from the
 *	 Source Capabilities stored in the dpm_data object
 */
static uint32_t build_rdo(struct port0_data_t *dpm_data)
{
	union pd_rdo rdo;

	union pd_fixed_supply_pdo_source src_pdo;

	uint8_t req_idx = dpm_data->req_idx;
	src_pdo.raw_value = dpm_data->src_caps[req_idx];

	if (dpm_data->src_cap_cnt > 0 && src_pdo.type == PDO_FIXED) {
		rdo.fixed.min_or_max_operating_current = src_pdo.max_current;
		rdo.fixed.operating_current = src_pdo.max_current;
	} else {
		/* Maximum operating current 100mA (GIVEBACK = 0) */
		rdo.fixed.min_or_max_operating_current = PD_CONVERT_MA_TO_FIXED_PDO_CURRENT(100);
		/* Operating current 100mA */
		rdo.fixed.operating_current = PD_CONVERT_MA_TO_FIXED_PDO_CURRENT(100);
		req_idx = 0;
	}

	/* Unchunked Extended Messages Not Supported */
	rdo.fixed.unchunked_ext_msg_supported = 0;
	/* No USB Suspend */
	rdo.fixed.no_usb_suspend = 1;
	/* Not USB Communications Capable */
	rdo.fixed.usb_comm_capable = 0;
	/* No capability mismatch */
	rdo.fixed.cap_mismatch = 0;
	/* Don't giveback */
	rdo.fixed.giveback = 0;

	rdo.fixed.object_pos = req_idx + 1;

	return rdo.raw_value;
}

/* usbc.rst callbacks start */
static int port0_policy_cb_get_snk_cap(const struct device *dev, uint32_t **pdos, int *num_pdos)
{
	struct port0_data_t *dpm_data = usbc_get_dpm_data(dev);

	*pdos = dpm_data->snk_caps;
	*num_pdos = dpm_data->snk_cap_cnt;

	return 0;
}

static void port0_policy_cb_set_src_cap(const struct device *dev, const uint32_t *pdos,
					const int num_pdos)
{
	struct port0_data_t *dpm_data;
	int num;

	dpm_data = usbc_get_dpm_data(dev);
	dpm_data->req_idx = 0;

	num = num_pdos;
	if (num > PDO_MAX_DATA_OBJECTS) {
		num = PDO_MAX_DATA_OBJECTS;
	}

	union pd_fixed_supply_pdo_source src_pdo;
	for (int i = 0; i < num; i++) {
		dpm_data->src_caps[i] = *(pdos + i);
		src_pdo.raw_value = dpm_data->src_caps[i];

		uint16_t vol = PD_CONVERT_FIXED_PDO_VOLTAGE_TO_MV(src_pdo.voltage);
		if (src_pdo.type == PDO_FIXED && (vol > 0 && vol <= CONFIG_PD_MAX_REQUESTED_VOLTAGE)
                ) { // 从低到高尽可能匹配目标电压
			dpm_data->req_idx = i;
		}
	}

	dpm_data->src_cap_cnt = num;
}

static uint32_t port0_policy_cb_get_rdo(const struct device *dev)
{
	struct port0_data_t *dpm_data = usbc_get_dpm_data(dev);

	return build_rdo(dpm_data);
}
/* usbc.rst callbacks end */

/* usbc.rst notify start */
static void port0_notify(const struct device *dev, const enum usbc_policy_notify_t policy_notify)
{
	struct port0_data_t *dpm_data = usbc_get_dpm_data(dev);

	switch (policy_notify) {
	case PROTOCOL_ERROR:
		break;
	case MSG_DISCARDED:
		break;
	case MSG_ACCEPT_RECEIVED:
		break;
	case MSG_REJECTED_RECEIVED:
		break;
	case MSG_NOT_SUPPORTED_RECEIVED:
		break;
	case TRANSITION_PS:
		atomic_set_bit(&dpm_data->ps_ready, 0);
		break;
	case PD_CONNECTED:
		break;
	case NOT_PD_CONNECTED:
		atomic_clear_bit(&dpm_data->ps_ready, 0);
		break;
	case POWER_CHANGE_0A0:
		LOG_INF("PWR 0A");
		break;
	case POWER_CHANGE_DEF:
		LOG_INF("PWR DEF");
		break;
	case POWER_CHANGE_1A5:
		LOG_INF("PWR 1A5");
		break;
	case POWER_CHANGE_3A0:
		LOG_INF("PWR 3A0");
		break;
	case DATA_ROLE_IS_UFP:
		break;
	case DATA_ROLE_IS_DFP:
		break;
	case PORT_PARTNER_NOT_RESPONSIVE:
		LOG_INF("Port Partner not PD Capable");
		break;
	case SNK_TRANSITION_TO_DEFAULT:
		break;
	case HARD_RESET_RECEIVED:
		break;
	case SENDER_RESPONSE_TIMEOUT:
		break;
	case SOURCE_CAPABILITIES_RECEIVED:
		break;
	}
}
/* usbc.rst notify end */

/* usbc.rst check start */
bool port0_policy_check(const struct device *dev, const enum usbc_policy_check_t policy_check)
{
	switch (policy_check) {
	case CHECK_POWER_ROLE_SWAP:
		/* Reject power role swaps */
		return false;
	case CHECK_DATA_ROLE_SWAP_TO_DFP:
		/* Reject data role swap to DFP */
		return false;
	case CHECK_DATA_ROLE_SWAP_TO_UFP:
		/* Accept data role swap to UFP */
		return true;
	case CHECK_SNK_AT_DEFAULT_LEVEL:
		/* This device is always at the default power level */
		return true;
	default:
		/* Reject all other policy checks */
		return false;
	}
}

const struct device *usbc_port0 = DEVICE_DT_GET(USBC_PORT0_NODE);

void pd_start(struct app *app)
{

	/* Get the device for this port */
	if (!device_is_ready(usbc_port0)) {
		LOG_ERR("PORT0 device not ready");
		return;
	}
	/* usbc.rst register start */
	/* Register USB-C Callbacks */

	/* Register Policy Check callback */
	usbc_set_policy_cb_check(usbc_port0, port0_policy_check);
	/* Register Policy Notify callback */
	usbc_set_policy_cb_notify(usbc_port0, port0_notify);
	/* Register Policy Get Sink Capabilities callback */
	usbc_set_policy_cb_get_snk_cap(usbc_port0, port0_policy_cb_get_snk_cap);
	/* Register Policy Set Source Capabilities callback */
	usbc_set_policy_cb_set_src_cap(usbc_port0, port0_policy_cb_set_src_cap);
	/* Register Policy Get Request Data Object callback */
	usbc_set_policy_cb_get_rdo(usbc_port0, port0_policy_cb_get_rdo);
	/* usbc.rst register end */

	/* usbc.rst user data start */
	/* Set Application port data object. This object is passed to the policy
	 * callbacks */
	port0_data.ps_ready = ATOMIC_INIT(0);
	usbc_set_dpm_data(usbc_port0, &port0_data);
	/* usbc.rst user data end */

	/* usbc.rst usbc start */
	/* Start the USB-C Subsystem */
	usbc_start(usbc_port0);
	/* usbc.rst usbc end */

	app->pd_data = &port0_data;
}

bool check_pd_ready(struct port0_data_t *data)
{
	return atomic_test_bit(&data->ps_ready, 0);
}

void pd_send_hard_reset()
{
	usbc_request(usbc_port0, REQUEST_PE_HARD_RESET_SEND);
}
