#ifndef __CAPWAP_NLSMARTCAPWAP_HEADER__
#define __CAPWAP_NLSMARTCAPWAP_HEADER__

/* */
#define SMARTCAPWAP_GENL_NAME			"smartcapwap"

/* */
#define SMARTCAPWAP_FLAGS_SEND_USERSPACE		0x00000001
#define SMARTCAPWAP_FLAGS_BLOCK_DATA_FRAME		0x00000002
#define SMARTCAPWAP_FLAGS_TUNNEL_8023			0x00000004

/* */
enum nlsmartcapwap_attrs {
	NLSMARTCAPWAP_ATTR_UNSPEC,

	NLSMARTCAPWAP_ATTR_IFINDEX,

	NLSMARTCAPWAP_ATTR_FLAGS,

	NLSMARTCAPWAP_ATTR_MGMT_SUBTYPE_MASK,
	NLSMARTCAPWAP_ATTR_CTRL_SUBTYPE_MASK,
	NLSMARTCAPWAP_ATTR_DATA_SUBTYPE_MASK,

	NLSMARTCAPWAP_ATTR_FRAME,
	NLSMARTCAPWAP_ATTR_8023_FRAME,
	NLSMARTCAPWAP_ATTR_RX_SIGNAL_DBM,
	NLSMARTCAPWAP_ATTR_RX_RATE,

	/* Last attribute */
	__NLSMARTCAPWAP_ATTR_AFTER_LAST,
	NLSMARTCAPWAP_ATTR_MAX = __NLSMARTCAPWAP_ATTR_AFTER_LAST - 1
};

/* */
enum nlsmartcapwap_commands {
	NLSMARTCAPWAP_CMD_UNSPEC,

	NLSMARTCAPWAP_CMD_LINK,

	NLSMARTCAPWAP_CMD_SET_AC_ADDRESS,
	NLSMARTCAPWAP_CMD_CONNECT,
	NLSMARTCAPWAP_CMD_TEARDOWN,

	NLSMARTCAPWAP_CMD_SEND_KEEPALIVE,
	NLSMARTCAPWAP_CMD_RECV_KEEPALIVE,

	NLSMARTCAPWAP_CMD_FRAME,

	NLSMARTCAPWAP_CMD_JOIN_MAC80211_DEVICE,
	NLSMARTCAPWAP_CMD_LEAVE_MAC80211_DEVICE,

	/* Last command */
	__NLSMARTCAPWAP_CMD_AFTER_LAST,
	NLSMARTCAPWAP_CMD_MAX = __NLSMARTCAPWAP_CMD_AFTER_LAST - 1
};

#endif /* __CAPWAP_NLSMARTCAPWAP_HEADER__ */