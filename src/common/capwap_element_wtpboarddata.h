#ifndef __CAPWAP_ELEMENT_WTPBOARDDATA_HEADER__
#define __CAPWAP_ELEMENT_WTPBOARDDATA_HEADER__

#define CAPWAP_ELEMENT_WTPBOARDDATA			38

struct capwap_wtpboarddata_element {
	uint32_t vendor;
	struct capwap_array* boardsubelement;
};

#define CAPWAP_BOARD_SUBELEMENT_MODELNUMBER			0
#define CAPWAP_BOARD_SUBELEMENT_SERIALNUMBER		1
#define CAPWAP_BOARD_SUBELEMENT_ID					2
#define CAPWAP_BOARD_SUBELEMENT_REVISION			3
#define CAPWAP_BOARD_SUBELEMENT_MACADDRESS			4
#define CAPWAP_BOARD_SUBELEMENT_MAXDATA				1024

struct capwap_wtpboarddata_board_subelement {
	uint16_t type;
	uint16_t length;
	uint8_t data[CAPWAP_BOARD_SUBELEMENT_MAXDATA];
};

extern struct capwap_message_elements_ops capwap_element_wtpboarddata_ops;

#endif /* __CAPWAP_ELEMENT_WTPBOARDDATA_HEADER__ */
