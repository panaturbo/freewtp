#include "wtp.h"
#include "capwap_network.h"
#include "capwap_protocol.h"
#include "capwap_dfa.h"
#include "capwap_array.h"
#include "capwap_element.h"
#include "capwap_dtls.h"
#include "wtp_dfa.h"

#include <arpa/inet.h>
#include <libconfig.h>

struct wtp_t g_wtp;

/* Local param */
#define WTP_STANDARD_NAME				"Unknown WTP"
#define WTP_STANDARD_LOCATION			"Unknown Location"

static char g_configurationfile[260] = WTP_STANDARD_CONFIGURATION_FILE;

/* Alloc WTP */
static int wtp_init(void) {
	/* Init WTP with default value */
	memset(&g_wtp, 0, sizeof(struct wtp_t));

	/* Standard name */
	strcpy(g_wtp.name.name, WTP_STANDARD_NAME);
	strcpy(g_wtp.location.value, WTP_STANDARD_LOCATION);
	
	/* State machine */
	g_wtp.dfa.state = CAPWAP_START_STATE;
	g_wtp.dfa.rfcMaxDiscoveryInterval = WTP_DEFAULT_DISCOVERY_INTERVAL;
	g_wtp.dfa.rfcMaxDiscoveries = WTP_DEFAULT_DISCOVERY_COUNT;
	g_wtp.dfa.rfcSilentInterval = WTP_DEFAULT_SILENT_INTERVAL;
	g_wtp.dfa.rfcRetransmitInterval = WTP_DEFAULT_RETRANSMIT_INTERVAL;
	g_wtp.dfa.rfcMaxRetransmit = WTP_MAX_RETRANSMIT;
	g_wtp.dfa.rfcWaitDTLS = WTP_DEFAULT_WAITDTLS_INTERVAL;
	g_wtp.dfa.rfcDataChannelKeepAlive = WTP_DEFAULT_DATACHANNEL_KEEPALIVE;
	g_wtp.dfa.rfcDataChannelDeadInterval = WTP_DEFAULT_DATACHANNEL_KEEPALIVEDEAD;
	g_wtp.dfa.rfcEchoInterval = WTP_DEFAULT_ECHO_INTERVAL;
	g_wtp.dfa.rfcDTLSSessionDelete = WTP_DEFAULT_DTLS_SESSION_DELETE;
	g_wtp.dfa.rfcMaxFailedDTLSSessionRetry = WTP_DEFAULT_FAILED_DTLS_SESSION_RETRY;

	/* Socket */
	capwap_network_init(&g_wtp.net);
		
	/* Standard configuration */
	g_wtp.boarddata.boardsubelement = capwap_array_create(sizeof(struct capwap_wtpboarddata_board_subelement), 0);
	g_wtp.descriptor.encryptsubelement = capwap_array_create(sizeof(struct capwap_wtpdescriptor_encrypt_subelement), 0);
	g_wtp.descriptor.descsubelement = capwap_array_create(sizeof(struct capwap_wtpdescriptor_desc_subelement), 0);
	
	g_wtp.binding = CAPWAP_WIRELESS_BINDING_NONE;

	g_wtp.ecn.flag = CAPWAP_LIMITED_ECN_SUPPORT;
	g_wtp.transport.type = CAPWAP_UDP_TRANSPORT;
	g_wtp.statisticstimer.timer = WTP_DEFAULT_STATISTICSTIMER_INTERVAL;

	g_wtp.mactype.type = CAPWAP_LOCALMAC;
	g_wtp.mactunnel.mode = CAPWAP_WTP_LOCAL_BRIDGING;
	
	/* DTLS */
	g_wtp.validdtlsdatapolicy = CAPWAP_ACDESC_CLEAR_DATA_CHANNEL_ENABLED;
	
	/* Tx fragment packets */
	g_wtp.mtu = CAPWAP_MTU_DEFAULT;
	g_wtp.requestfragmentpacket = capwap_array_create(sizeof(struct capwap_packet), 0);
	g_wtp.responsefragmentpacket = capwap_array_create(sizeof(struct capwap_packet), 0);

	/* AC information */
	g_wtp.discoverytype.type = CAPWAP_ELEMENT_DISCOVERYTYPE_TYPE_UNKNOWN;
	g_wtp.acdiscoveryrequest = 1;
	g_wtp.acdiscoveryarray = capwap_array_create(sizeof(struct sockaddr_storage), 0);
	g_wtp.acpreferedarray = capwap_array_create(sizeof(struct sockaddr_storage), 0);
	g_wtp.acdiscoveryresponse = capwap_array_create(sizeof(struct wtp_discovery_response), 0);
	
	/* Radios */
	g_wtp.radios = capwap_array_create(sizeof(struct wtp_radio), 0);
	
	return 1;
}

/* Destroy WTP */
static void wtp_destroy(void) {
	/* Dtls */
	capwap_crypt_freecontext(&g_wtp.dtlscontext);
	
	/* Free standard configuration */
	capwap_array_free(g_wtp.descriptor.encryptsubelement);
	capwap_array_free(g_wtp.descriptor.descsubelement);
	capwap_array_free(g_wtp.boarddata.boardsubelement);
	
	/* Free fragments packet */
	capwap_fragment_free(g_wtp.requestfragmentpacket);
	capwap_fragment_free(g_wtp.responsefragmentpacket);
	capwap_array_free(g_wtp.requestfragmentpacket);
	capwap_array_free(g_wtp.responsefragmentpacket);
	
	/* Free list AC */
	capwap_array_free(g_wtp.acdiscoveryarray);
	capwap_array_free(g_wtp.acpreferedarray);
	
	wtp_free_discovery_response_array();
	capwap_array_free(g_wtp.acdiscoveryresponse);
	
	/* Free radios */
	capwap_array_free(g_wtp.radios);
}

/* Save AC address */
static int wtp_add_acaddress(struct sockaddr_storage* source, struct capwap_array* array) {
	ASSERT(source != NULL);
	ASSERT(array != NULL);
	
	if ((g_wtp.net.sock_family == AF_UNSPEC) || (g_wtp.net.sock_family == source->ss_family)) {
		struct sockaddr_storage* destaddr = (struct sockaddr_storage*)capwap_array_get_item_pointer(array, array->count);

		/* Save address, if request, mapping IPv4 to IPv6 */
		if ((g_wtp.net.sock_family == AF_UNSPEC) && (source->ss_family == AF_INET) && !(g_wtp.net.bind_ctrl_flags & CAPWAP_IPV6ONLY_FLAG)) {
			if (!capwap_ipv4_mapped_ipv6(source, destaddr)) {
				memcpy(destaddr, source, sizeof(struct sockaddr_storage));
			}
		} else {
			memcpy(destaddr, source, sizeof(struct sockaddr_storage));
		}

		return 1;
	}

	return 0;
}

/* */
static int wtp_add_default_acaddress() {
	struct sockaddr_storage address;
	struct sockaddr_in* addressv4 = (struct sockaddr_in*)&address;
	/*struct sockaddr_in6* addressv6 = (struct sockaddr_in6*)&address;*/
	
	/* Broadcast IPv4 */
	addressv4->sin_family = AF_INET;
	addressv4->sin_addr.s_addr = INADDR_BROADCAST;
	addressv4->sin_port = htons(CAPWAP_CONTROL_PORT);
	wtp_add_acaddress(&address, g_wtp.acdiscoveryarray);
	
	/* Multicast IPv4 */
	/* TODO */
	
	/* Multicast IPv6 */
	/* TODO */
	
	return ((g_wtp.acdiscoveryarray->count > 0) ? 1 : 0);
}

/* Help */
static void wtp_print_usage(void) {
	/* TODO */
}

/* Parsing configuration */
static int wtp_parsing_configuration_1_0(config_t* config) {
	int i;
	int configInt;
	int configIPv4;
	int configIPv6;
	long int configLongInt;
	const char* configString;
	config_setting_t* configSetting;

	/* Logging configuration */
	if (config_lookup_bool(config, "logging.enable", &configInt) == CONFIG_TRUE) {
		if (!configInt) {
			capwap_logging_verboselevel(CAPWAP_LOGGING_NONE);
			capwap_logging_disable_allinterface();
		} else {
			if (config_lookup_string(config, "logging.level", &configString) == CONFIG_TRUE) {
				if (!strcmp(configString, "fatal")) {
					capwap_logging_verboselevel(CAPWAP_LOGGING_FATAL);
				} else if (!strcmp(configString, "error")) {
					capwap_logging_verboselevel(CAPWAP_LOGGING_ERROR);
				} else if (!strcmp(configString, "warning")) {
					capwap_logging_verboselevel(CAPWAP_LOGGING_WARNING);
				} else if (!strcmp(configString, "info")) {
					capwap_logging_verboselevel(CAPWAP_LOGGING_INFO);
				} else if (!strcmp(configString, "debug")) {
					capwap_logging_verboselevel(CAPWAP_LOGGING_DEBUG);
				} else {
					capwap_logging_error("Invalid configuration file, unknown logging.level value");
					return 0;
				}
			}

			/* Logging output interface */
			configSetting = config_lookup(config, "logging.output");
			if (configSetting != NULL) {
				int count = config_setting_length(configSetting);

				/* Disable output interface */		
				capwap_logging_disable_allinterface();

				/* Enable selected interface */
				for (i = 0; i < count; i++) {
					config_setting_t* configElement = config_setting_get_elem(configSetting, i);
					if ((configElement != NULL) && (config_setting_lookup_string(configElement, "mode", &configString) == CONFIG_TRUE)) {
						if (!strcmp(configString, "stdout")) {
							capwap_logging_enable_console(0);
						} else if (!strcmp(configString, "stderr")) {
							capwap_logging_enable_console(1);
						} else {
							capwap_logging_error("Invalid configuration file, unknown logging.output value");
							return 0;
						}
					}
				}
			}
		}
	}

	/* Set name of WTP */
	if (config_lookup_string(config, "application.name", &configString) == CONFIG_TRUE) {
		if (strlen(configString) > CAPWAP_WTPNAME_MAXLENGTH) {
			capwap_logging_error("Invalid configuration file, application.name string length exceeded");
			return 0;
		}

		strcpy(g_wtp.name.name, configString);
	}

	/* Set location of WTP */
	if (config_lookup_string(config, "application.location", &configString) == CONFIG_TRUE) {
		if (strlen(configString) > CAPWAP_LOCATION_MAXLENGTH) {
			capwap_logging_error("Invalid configuration file, application.location string length exceeded");
			return 0;
		}

		strcpy(g_wtp.location.value, configString);
	}

	/* Set binding of WTP */
	if (config_lookup_string(config, "application.binding", &configString) == CONFIG_TRUE) {
		if (!strcmp(configString, "802.11")) {
			g_wtp.binding = CAPWAP_WIRELESS_BINDING_IEEE80211;
		} else if (!strcmp(configString, "EPCGlobal")) {
			g_wtp.binding = CAPWAP_WIRELESS_BINDING_EPCGLOBAL;
		} else {
			capwap_logging_error("Invalid configuration file, unknown application.binding value");
			return 0;
		}
	}

	/* Set tunnelmode of WTP */
	if (config_lookup(config, "application.tunnelmode") != NULL) {
		g_wtp.mactunnel.mode = 0;
		if (config_lookup_bool(config, "application.tunnelmode.nativeframe", &configInt) == CONFIG_TRUE) {
			if (configInt != 0) {
				g_wtp.mactunnel.mode |= CAPWAP_WTP_NATIVE_FRAME_TUNNEL;
			}
		}

		if (config_lookup_bool(config, "application.tunnelmode.ethframe", &configInt) == CONFIG_TRUE) {
			if (configInt != 0) {
				g_wtp.mactunnel.mode |=  CAPWAP_WTP_8023_FRAME_TUNNEL;
			}
		}

		if (config_lookup_bool(config, "application.tunnelmode.localbridging", &configInt) == CONFIG_TRUE) {
			if (configInt != 0) {
				g_wtp.mactunnel.mode |=  CAPWAP_WTP_LOCAL_BRIDGING;
			}
		}
	}

	/* Set mactype of WTP */
	if (config_lookup_string(config, "application.mactype", &configString) == CONFIG_TRUE) {
		if (!strcmp(configString, "localmac")) {
			g_wtp.mactype.type = CAPWAP_LOCALMAC;
		} else if (!strcmp(configString, "splitmac")) {
			g_wtp.mactype.type = CAPWAP_SPLITMAC;
		} else {
			capwap_logging_error("Invalid configuration file, unknown application.mactype value");
			return 0;
		}
	}

	/* Set VendorID Boardinfo of WTP */
	if (config_lookup_int(config, "application.boardinfo.idvendor", &configLongInt) == CONFIG_TRUE) {
		g_wtp.boarddata.vendor = (unsigned long)configLongInt;
	}

	/* Set Element Boardinfo of WTP */
	configSetting = config_lookup(config, "application.boardinfo.element");
	if (configSetting != NULL) {
		int count = config_setting_length(configSetting);

		for (i = 0; i < count; i++) {
			config_setting_t* configElement = config_setting_get_elem(configSetting, i);
			if (configElement != NULL) {
				const char* configName;
				if (config_setting_lookup_string(configElement, "name", &configName) == CONFIG_TRUE) {
					const char* configValue;
					if (config_setting_lookup_string(configElement, "value", &configValue) == CONFIG_TRUE) {
						int lengthValue = strlen(configValue);
						if (lengthValue < CAPWAP_BOARD_SUBELEMENT_MAXDATA) {
							struct capwap_wtpboarddata_board_subelement* element = (struct capwap_wtpboarddata_board_subelement*)capwap_array_get_item_pointer(g_wtp.boarddata.boardsubelement, g_wtp.boarddata.boardsubelement->count);

							if (!strcmp(configName, "model")) {
								element->type = CAPWAP_BOARD_SUBELEMENT_MODELNUMBER;
								element->length = lengthValue;
								strcpy(element->data, configValue);
							} else if (!strcmp(configName, "serial")) {
								element->type = CAPWAP_BOARD_SUBELEMENT_SERIALNUMBER;
								element->length = lengthValue;
								strcpy(element->data, configValue);
							} else if (!strcmp(configName, "id")) {
								element->type = CAPWAP_BOARD_SUBELEMENT_ID;
								element->length = lengthValue;
								strcpy(element->data, configValue);
							} else if (!strcmp(configName, "revision")) {
								element->type = CAPWAP_BOARD_SUBELEMENT_REVISION;
								element->length = lengthValue;
								strcpy(element->data, configValue);
							} else if (!strcmp(configName, "macaddress")) {
								const char* configType;
								if (config_setting_lookup_string(configElement, "type", &configType) == CONFIG_TRUE) {
									if (!strcmp(configType, "interface")) {
										element->type = CAPWAP_BOARD_SUBELEMENT_MACADDRESS;
										element->length = capwap_get_macaddress_from_interface(configValue, element->data);
										if (!element->length) {
											capwap_logging_error("Invalid configuration file, unable found macaddress of interface: '%s'", configValue);
											return 0;
										}
									} else {
										capwap_logging_error("Invalid configuration file, unknown application.boardinfo.element.type value");
										return 0;
									}
								} else {
									capwap_logging_error("Invalid configuration file, element application.boardinfo.element.type not found");
									return 0;
								}
							} else {
								capwap_logging_error("Invalid configuration file, unknown application.boardinfo.element.name value");
								return 0;
							}
						} else {
							capwap_logging_error("Invalid configuration file, application.boardinfo.element.value string length exceeded");
							return 0;
						}
					} else {
						capwap_logging_error("Invalid configuration file, element application.boardinfo.element.value not found");
						return 0;
					}
				} else {
					capwap_logging_error("Invalid configuration file, element application.boardinfo.element.name not found");
					return 0;
				}
			}
		}
	}

	/* Set Radio descriptor of WTP */
	configSetting = config_lookup(config, "application.descriptor.radio");
	if (configSetting != NULL) {
		int count = config_setting_length(configSetting);

		if (g_wtp.binding == CAPWAP_WIRELESS_BINDING_IEEE80211) {
			for (i = 0; i < count; i++) {
				struct wtp_radio* radio;
				char radioname[IFNAMSIZ];
				unsigned char radiotype = 0;
				int radiostatus = WTP_RADIO_ENABLED;
	
				config_setting_t* configElement = config_setting_get_elem(configSetting, i);
				if (configElement != NULL) {
					if (config_setting_lookup_string(configElement, "device", &configString) == CONFIG_TRUE) {
						if (*configString && (strlen(configString) < IFNAMSIZ)) {
							strcpy(radioname, configString);
							if (config_setting_lookup_string(configElement, "type", &configString) == CONFIG_TRUE) {
								int length = strlen(configString);
								
								for (i = 0; i < length; i++) {
									switch (configString[i]) {
										case 'a': {
											radiotype |= CAPWAP_RADIO_TYPE_80211A;
											break;
										}	
										
										case 'b': {
											radiotype |= CAPWAP_RADIO_TYPE_80211B;
											break;
										}	

										case 'g': {
											radiotype |= CAPWAP_RADIO_TYPE_80211G;
											break;
										}	

										case 'n': {
											radiotype |= CAPWAP_RADIO_TYPE_80211N;
											break;
										}
										
										default: {
											capwap_logging_error("Invalid configuration file, unknown application.descriptor.radio.type value");
											return 0;
										}
									}
								}

								if (radiotype != 0) {
									if (config_setting_lookup_string(configElement, "status", &configString) == CONFIG_TRUE) {
										if (!strcmp(configString, "enabled")) {
											radiostatus = WTP_RADIO_ENABLED;
										} else if (!strcmp(configString, "disabled")) {
											radiostatus = WTP_RADIO_DISABLED;
										} else if (!strcmp(configString, "hwfailure")) {
											radiostatus = WTP_RADIO_HWFAILURE;
										} else if (!strcmp(configString, "swfailure")) {
											radiostatus = WTP_RADIO_SWFAILURE;
										} else {
											capwap_logging_error("Invalid configuration file, unknown application.descriptor.radio.type value");
											return 0;
										}
									}
	
									/* Create new radio device */
									radio = (struct wtp_radio*)capwap_array_get_item_pointer(g_wtp.radios, g_wtp.radios->count);
									strcpy(radio->device, radioname);
									radio->radioinformation.radioid = g_wtp.radios->count;
									radio->radioinformation.radiotype = radiotype;
									radio->status = radiostatus;
								} else {
									capwap_logging_error("Invalid configuration file, unknown application.descriptor.radio.type value");
									return 0;
								}
							} else {
								capwap_logging_error("Invalid configuration file, element application.descriptor.radio.type not found");
								return 0;
							}
						} else {
							capwap_logging_error("Invalid configuration file, application.descriptor.radio.device string length exceeded");
							return 0;
						}
					} else {
						capwap_logging_error("Invalid configuration file, element application.descriptor.radio.device not found");
						return 0;
					}
				}
			}

			/* Update radio status */
			g_wtp.descriptor.maxradios = g_wtp.radios->count;
			g_wtp.descriptor.radiosinuse = wtp_update_radio_in_use();
		}
	}

	/* Set encryption of WTP */
	configSetting = config_lookup(config, "application.descriptor.encryption");
	if (configSetting != NULL) {
		unsigned short capability = 0;
		int count = config_setting_length(configSetting);
		struct capwap_wtpdescriptor_encrypt_subelement* encrypt;
		
		if (g_wtp.binding == CAPWAP_WIRELESS_BINDING_IEEE80211) {
			for (i = 0; i < count; i++) {
				const char* encryption = config_setting_get_string_elem(configSetting, i);
				if (encryption != NULL) {
					if (!strcmp(encryption, "802.11_AES")) {
						capability |= 0; /* TODO */
					} else if (!strcmp(encryption, "802.11_TKIP")) {
						capability |= 0; /* TODO */
					} else {
						capwap_logging_error("Invalid configuration file, invalid application.descriptor.encryption value");
						return 0;
					}
				}
			}
		}

		/* */
		encrypt = (struct capwap_wtpdescriptor_encrypt_subelement*)capwap_array_get_item_pointer(g_wtp.descriptor.encryptsubelement, g_wtp.descriptor.encryptsubelement->count);
		encrypt->wbid = g_wtp.binding;
		encrypt->capabilities = capability;
	} else {
		capwap_logging_error("Invalid configuration file, application.descriptor.encryption not found");
		return 0;
	}

	/* Set info descriptor of WTP */
	configSetting = config_lookup(config, "application.descriptor.info");
	if (configSetting != NULL) {
		int count = config_setting_length(configSetting);

		for (i = 0; i < count; i++) {
			config_setting_t* configElement = config_setting_get_elem(configSetting, i);
			if (configElement != NULL) {
				long int configVendor;
				if (config_setting_lookup_int(configElement, "idvendor", &configVendor) == CONFIG_TRUE) {
					const char* configType;
					if (config_setting_lookup_string(configElement, "type", &configType) == CONFIG_TRUE) {
						const char* configValue;
						if (config_setting_lookup_string(configElement, "value", &configValue) == CONFIG_TRUE) {
							int lengthValue = strlen(configValue);
							if (lengthValue < CAPWAP_WTPDESC_SUBELEMENT_MAXDATA) {
								unsigned short type;
								struct capwap_wtpdescriptor_desc_subelement* desc;

								if (!strcmp(configType, "hardware")) {
									type = CAPWAP_WTPDESC_SUBELEMENT_HARDWAREVERSION;
								} else if (!strcmp(configType, "software")) {
									type = CAPWAP_WTPDESC_SUBELEMENT_SOFTWAREVERSION;
								} else if (!strcmp(configType, "boot")) {
									type = CAPWAP_WTPDESC_SUBELEMENT_BOOTVERSION;
								} else if (!strcmp(configType, "other")) {
									type = CAPWAP_WTPDESC_SUBELEMENT_OTHERVERSION;
								} else {
									capwap_logging_error("Invalid configuration file, unknown application.descriptor.info.type value");
									return 0;
								}

								desc = (struct capwap_wtpdescriptor_desc_subelement*)capwap_array_get_item_pointer(g_wtp.descriptor.descsubelement, g_wtp.descriptor.descsubelement->count);
								desc->vendor = (unsigned long)configVendor;
								desc->type = type;
								desc->length = lengthValue;
								strcpy(desc->data, configValue);
							} else {
								capwap_logging_error("Invalid configuration file, application.descriptor.info.value string length exceeded");
								return 0;
							}
						} else {
							capwap_logging_error("Invalid configuration file, element application.descriptor.info.value not found");
							return 0;
						}
					} else {
						capwap_logging_error("Invalid configuration file, element application.descriptor.info.type not found");
						return 0;
					}
				} else {
					capwap_logging_error("Invalid configuration file, element application.descriptor.info.idvendor not found");
					return 0;
				}
			}
		}
	}

	/* Set ECN of WTP */
	if (config_lookup_string(config, "application.ecn", &configString) == CONFIG_TRUE) {
		if (!strcmp(configString, "full")) {
			g_wtp.ecn.flag = CAPWAP_FULL_ECN_SUPPORT;
		} else if (!strcmp(configString, "limited")) {
			g_wtp.ecn.flag = CAPWAP_LIMITED_ECN_SUPPORT;
		} else {
			capwap_logging_error("Invalid configuration file, unknown application.ecn value");
			return 0;
		}
	}

	/* Set Timer of WTP */
	if (config_lookup_int(config, "application.timer.statistics", &configLongInt) == CONFIG_TRUE) {
		if ((configLongInt > 0) && (configLongInt < 65536)) {
			g_wtp.statisticstimer.timer = (unsigned short)configLongInt;
		} else {
			capwap_logging_error("Invalid configuration file, invalid application.timer.statistics value");
			return 0;
		}
	}

	/* Set DTLS of WTP */
	if (config_lookup_bool(config, "application.dtls.enable", &configInt) == CONFIG_TRUE) {
		if (configInt != 0) {
			struct capwap_dtls_param dtlsparam;

			/* Init dtls param */
			memset(&dtlsparam, 0, sizeof(struct capwap_dtls_param));
			dtlsparam.type = CAPWAP_DTLS_CLIENT;

			/* Set DTLS Policy of WTP */
			if (config_lookup(config, "application.dtls.dtlspolicy") != NULL) {
				g_wtp.validdtlsdatapolicy = 0;
				if (config_lookup_bool(config, "application.dtls.dtlspolicy.cleardatachannel", &configInt) == CONFIG_TRUE) {
					if (configInt != 0) {
						g_wtp.validdtlsdatapolicy |= CAPWAP_ACDESC_CLEAR_DATA_CHANNEL_ENABLED;
					}
				}
		
				if (config_lookup_bool(config, "application.dtls.dtlspolicy.dtlsdatachannel", &configInt) == CONFIG_TRUE) {
					if (configInt != 0) {
						g_wtp.validdtlsdatapolicy |= CAPWAP_ACDESC_DTLS_DATA_CHANNEL_ENABLED;
					}
				}
			}

			/* Set DTLS type of WTP */
			if (config_lookup_string(config, "application.dtls.type", &configString) == CONFIG_TRUE) {
				if (!strcmp(configString, "x509")) {
					dtlsparam.mode = CAPWAP_DTLS_MODE_CERTIFICATE;
				} else if (!strcmp(configString, "presharedkey")) {
					dtlsparam.mode = CAPWAP_DTLS_MODE_PRESHAREDKEY;
				} else {
					capwap_logging_error("Invalid configuration file, unknown application.dtls.type value");
					return 0;
				}
			}

			/* Set DTLS configuration of WTP */
			if (dtlsparam.mode == CAPWAP_DTLS_MODE_CERTIFICATE) {
				if (config_lookup_string(config, "application.dtls.x509.calist", &configString) == CONFIG_TRUE) {
					if (strlen(configString) > 0) {
						dtlsparam.cert.fileca = capwap_duplicate_string(configString);
					}
				}

				if (config_lookup_string(config, "application.dtls.x509.certificate", &configString) == CONFIG_TRUE) {
					if (strlen(configString) > 0) {
						dtlsparam.cert.filecert = capwap_duplicate_string(configString);
					}
				}

				if (config_lookup_string(config, "application.dtls.x509.privatekey", &configString) == CONFIG_TRUE) {
					if (strlen(configString) > 0) {
						dtlsparam.cert.filekey = capwap_duplicate_string(configString);
					}
				}

				if (config_lookup_string(config, "application.dtls.x509.privatekeypassword", &configString) == CONFIG_TRUE) {
					if (strlen(configString) > 0) {
						dtlsparam.cert.pwdprivatekey = capwap_duplicate_string(configString);
					}
				}

				if (dtlsparam.cert.fileca && dtlsparam.cert.filecert && dtlsparam.cert.filekey) {
					if (capwap_crypt_createcontext(&g_wtp.dtlscontext, &dtlsparam)) {
						g_wtp.enabledtls = 1;
					}
				}

				/* Free dtls param */
				if (dtlsparam.cert.fileca) {
					capwap_free(dtlsparam.cert.fileca);
				}
				
				if (dtlsparam.cert.filecert) {
					capwap_free(dtlsparam.cert.filecert);
				}
				
				if (dtlsparam.cert.filekey) {
					capwap_free(dtlsparam.cert.filekey);
				}
				
				if (dtlsparam.cert.pwdprivatekey) {
					capwap_free(dtlsparam.cert.pwdprivatekey);
				}

				if (!g_wtp.enabledtls) {
					return 0;
				}
			} else if (dtlsparam.mode == CAPWAP_DTLS_MODE_PRESHAREDKEY) {
				/* TODO */
			}
		}
	}

	/* Set interface binding of WTP */
	if (config_lookup_string(config, "application.network.binding", &configString) == CONFIG_TRUE) {
		if (strlen(configString) > (IFNAMSIZ - 1)) {
			capwap_logging_error("Invalid configuration file, application.network.binding string length exceeded");
			return 0;
		}			
			
		strcpy(g_wtp.net.bind_interface, configString);
	}

	/* Set mtu of WTP */
	if (config_lookup_int(config, "application.network.mtu", &configLongInt) == CONFIG_TRUE) {
		if ((configLongInt > 0) && (configLongInt < 65536)) {
			g_wtp.mtu = (unsigned short)configLongInt;
		} else {
			capwap_logging_error("Invalid configuration file, invalid application.network.mtu value");
			return 0;
		}
	}

	/* Set network port of WTP */
	if (config_lookup_int(config, "application.network.port", &configLongInt) == CONFIG_TRUE) {
		if ((configLongInt > 0) && (configLongInt < 65536)) {
			g_wtp.net.bind_sock_ctrl_port = (unsigned short)configLongInt;
		} else {
			capwap_logging_error("Invalid configuration file, invalid application.network.port value");
			return 0;
		}
	}

	/* Set transport of WTP */
	if (config_lookup_string(config, "application.network.transport", &configString) == CONFIG_TRUE) {
		if (!strcmp(configString, "udp")) {
			g_wtp.transport.type = CAPWAP_UDP_TRANSPORT;
		} else if (!strcmp(configString, "udplite")) {
			g_wtp.transport.type = CAPWAP_UDPLITE_TRANSPORT;
		} else {
			capwap_logging_error("Invalid configuration file, unknown application.network.transport value");
			return 0;
		}
	}

	/* Set ipv4 & ipv6 of WTP */
	if (config_lookup_bool(config, "application.network.ipv4", &configIPv4) != CONFIG_TRUE) {
		configIPv4 = 1;
	}

	if (config_lookup_bool(config, "application.network.ipv6", &configIPv6) != CONFIG_TRUE) {
		configIPv6 = 1;
	}
	
	if (configIPv4 && configIPv6) {
		g_wtp.net.sock_family = AF_UNSPEC;
	} else if (!configIPv4 && !configIPv6) {
		capwap_logging_error("Invalid configuration file, request enable application.network.ipv4 or application.network.ipv6");
		return 0;
	} else {
		g_wtp.net.sock_family = (configIPv4 ? AF_INET : AF_INET6);
	}

	/* Set ip dual stack of WTP */
	if (config_lookup_bool(config, "application.network.ipdualstack", &configInt) == CONFIG_TRUE) {
		if (!configInt) {
			g_wtp.net.bind_ctrl_flags |= CAPWAP_IPV6ONLY_FLAG;
			g_wtp.net.bind_data_flags |= CAPWAP_IPV6ONLY_FLAG;
		} else {
			g_wtp.net.bind_ctrl_flags &= ~CAPWAP_IPV6ONLY_FLAG;
			g_wtp.net.bind_data_flags &= ~CAPWAP_IPV6ONLY_FLAG;
		}
	}

	/* Set search discovery of WTP */
	if (config_lookup_bool(config, "application.acdiscovery.search", &configInt) == CONFIG_TRUE) {
		g_wtp.acdiscoveryrequest = (configInt ? 1 : 0);
	}

	/* Set discovery host of WTP */
	configSetting = config_lookup(config, "application.acdiscovery.host");
	if (configSetting != NULL) {
		int count = config_setting_length(configSetting);
		
		for (i = 0; i < count; i++) {
			const char* address = config_setting_get_string_elem(configSetting, i);
			if (address != NULL) {
				struct sockaddr_storage acaddr;
				
				/* Parsing address */
				if (capwap_address_from_string(address, &acaddr)) {
					if (!CAPWAP_GET_NETWORK_PORT(&acaddr)) {
						CAPWAP_SET_NETWORK_PORT(&acaddr, CAPWAP_CONTROL_PORT);
					}

					wtp_add_acaddress(&acaddr, g_wtp.acdiscoveryarray);
					g_wtp.discoverytype.type = CAPWAP_ELEMENT_DISCOVERYTYPE_TYPE_STATIC;
				} else {
					capwap_logging_error("Invalid configuration file, invalid application.acdiscovery.host value");
					return 0;
				}
			}
		}
	}

	/* Set preferred ac of WTP */
	configSetting = config_lookup(config, "application.acprefered.host");
	if (configSetting != NULL) {
		int count = config_setting_length(configSetting);
		
		for (i = 0; i < count; i++) {
			const char* address = config_setting_get_string_elem(configSetting, i);
			if (address != NULL) {
				struct sockaddr_storage acaddr;
				
				/* Parsing address */
				if (capwap_address_from_string(address, &acaddr)) {
					if (!CAPWAP_GET_NETWORK_PORT(&acaddr)) {
						CAPWAP_SET_NETWORK_PORT(&acaddr, CAPWAP_CONTROL_PORT);
					}

					wtp_add_acaddress(&acaddr, g_wtp.acpreferedarray);
				} else {
					capwap_logging_error("Invalid configuration file, invalid application.acprefered.host value");
					return 0;
				}
			}
		}
	}

	return 1;
}

/* Parsing configuration */
static int wtp_parsing_configuration(config_t* config) {
	const char* configString;
	
	if (config_lookup_string(config, "version", &configString) == CONFIG_TRUE) {
		if (strcmp(configString, "1.0") == 0) {
			return wtp_parsing_configuration_1_0(config);
		}
		
		capwap_logging_error("Invalid configuration file, '%s' is not supported", configString);
	} else {
		capwap_logging_error("Invalid configuration file, unable to found version tag");
	}

	return 0;
}

/* Load configuration */
static int wtp_load_configuration(int argc, char **argv) {
	int c;
	int result = 0;
	config_t config;
	
	ASSERT(argc >= 0);
	ASSERT(argv != NULL);
	
	/* Parsing command line */
	opterr = 0;
	while ((c = getopt(argc, argv, "hc:")) != -1) {
		switch (c) {
			case 'h': {
				wtp_print_usage();
				return 0;
			}
						
			case 'c': {
				if (strlen(optarg) < sizeof(g_configurationfile)) {
					strcpy(g_configurationfile, optarg);
				} else {
					capwap_logging_error("Invalid -%c argument", optopt);
					return -1;
				}
				
				break;
			}
			
			case '?': {
				if (optopt == 'c') {
					capwap_logging_error("Option -%c requires an argument", optopt);
				} else {
					capwap_logging_error("Unknown option character `\\x%x'", optopt);
				}
				
				wtp_print_usage();
				return -1;
			}
		}
	}

	/* Init libconfig */
	config_init(&config);

	/* Load configuration */
	if (config_read_file(&config, g_configurationfile) == CONFIG_TRUE) {
		result = wtp_parsing_configuration(&config);
	} else {
		result = -1;
		capwap_logging_error("Unable load the configuration file '%s': %s (%d)", g_configurationfile, config_error_text(&config), config_error_line(&config));
	}

	/* Free libconfig */
	config_destroy(&config);
	return result;
}

/* Init WTP */
static int wtp_configure(void) {
	/* If request add default acdiscovery */
	if (!g_wtp.acdiscoveryarray->count) {
		if (!wtp_add_default_acaddress()) {
			capwap_logging_debug("Unable add default AC discovery");
			return WTP_ERROR_NETWORK;
		}
	}
		
	/* Bind to any address */
	if (!capwap_bind_sockets(&g_wtp.net)) {
		capwap_logging_fatal("Cannot bind address");
		return WTP_ERROR_NETWORK;
	}

	return CAPWAP_SUCCESSFUL;
}

/* */
int wtp_update_radio_in_use() {
	/* TODO */
	return g_wtp.radios->count;
}

/* Main*/
int main(int argc, char** argv) {
	int value;
	int result = CAPWAP_SUCCESSFUL;

	/* Init logging */
	capwap_logging_init();
	capwap_logging_verboselevel(CAPWAP_LOGGING_ERROR);
	capwap_logging_enable_console(1);

	/* Init capwap */
	if (geteuid() != 0) {
		capwap_logging_fatal("Request root privileges");
		result = CAPWAP_REQUEST_ROOT;
	} else {
		/* Init random generator */
		capwap_init_rand();

		/* Init crypt */
		if (!capwap_crypt_init()) {
			result = CAPWAP_CRYPT_ERROR;
			capwap_logging_fatal("Error to init crypt engine");
		} else {
			/* Alloc a WTP */
			if (!wtp_init()) {
				result = WTP_ERROR_SYSTEM_FAILER;
				capwap_logging_fatal("Error to init WTP engine");
			} else {
				/* Read configuration file */
				value = wtp_load_configuration(argc, argv);
				if (value < 0) {
					result = WTP_ERROR_LOAD_CONFIGURATION;
					capwap_logging_fatal("Error to load configuration");
				} else if (value > 0) {
					/* Initialize binding */
					switch (g_wtp.binding) {
						case CAPWAP_WIRELESS_BINDING_NONE: {
							value = 0;
							break;
						}

						case CAPWAP_WIRELESS_BINDING_IEEE80211: {
							/* Initialize wifi binding driver */
							capwap_logging_info("Initializing wifi binding engine");
							value = wifi_init_driver();
							break;
						}
					}

					/* */
					if (value) {
						result = WTP_ERROR_INIT_BINDING;
						capwap_logging_fatal("Unable initialize binding engine");
					} else {
						capwap_logging_info("Startup WTP");

						/* Start WTP */
						wtp_dfa_change_state(CAPWAP_START_TO_IDLE_STATE);

						/* Complete configuration WTP */
						result = wtp_configure();
						if (result == CAPWAP_SUCCESSFUL) {
							/* Init complete */
							wtp_dfa_change_state(CAPWAP_IDLE_STATE);

							/* Running WTP */
							result = wtp_dfa_execute();

							/* Close socket */
							capwap_close_sockets(&g_wtp.net);
						}

						capwap_logging_info("Terminate WTP");

						/* Free binding */
						switch (g_wtp.binding) {
							case CAPWAP_WIRELESS_BINDING_IEEE80211: {
								/* Free wifi binding driver */
								wifi_free_driver();
								capwap_logging_info("Free wifi binding engine");
								break;
							}
						}
					}
				}

				/* Free memory */
				wtp_destroy();
			}

			/* Free crypt */
			capwap_crypt_free();
		}

		/* Check memory leak */
		if (capwap_check_memory_leak(1)) {
			if (result == CAPWAP_SUCCESSFUL) {
				result = WTP_ERROR_MEMORY_LEAK;
			}
		}
	}

	/* Close logging */
	capwap_logging_close();

	return result;
}