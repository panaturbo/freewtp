#include "capwap.h"
#include "capwap_element.h"

/********************************************************************

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          IP Address                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            Netmask                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            Gateway                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    Static     |
+-+-+-+-+-+-+-+-+

Type:   49 for WTP Static IP Address Information

Length:  13

********************************************************************/

/* */
static void capwap_wtpstaticipaddress_element_create(void* data, capwap_message_elements_handle handle, struct capwap_write_message_elements_ops* func) {
	struct capwap_wtpstaticipaddress_element* element = (struct capwap_wtpstaticipaddress_element*)data;

	ASSERT(data != NULL);

	/* */
	func->write_block(handle, (uint8_t*)&element->address, sizeof(struct in_addr));
	func->write_block(handle, (uint8_t*)&element->netmask, sizeof(struct in_addr));
	func->write_block(handle, (uint8_t*)&element->gateway, sizeof(struct in_addr));
	func->write_u8(handle, element->staticip);
}

/* */
static void* capwap_wtpstaticipaddress_element_parsing(capwap_message_elements_handle handle, struct capwap_read_message_elements_ops* func) {
	struct capwap_wtpstaticipaddress_element* data;

	ASSERT(handle != NULL);
	ASSERT(func != NULL);

	if (func->read_ready(handle) != 13) {
		capwap_logging_debug("Invalid WTP Static IP Address Information element");
		return NULL;
	}

	/* */
	data = (struct capwap_wtpstaticipaddress_element*)capwap_alloc(sizeof(struct capwap_wtpstaticipaddress_element));
	if (!data) {
		capwap_outofmemory();
	}

	/* Retrieve data */
	memset(data, 0, sizeof(struct capwap_wtpstaticipaddress_element));
	func->read_block(handle, (uint8_t*)&data->address, sizeof(struct in_addr));
	func->read_block(handle, (uint8_t*)&data->netmask, sizeof(struct in_addr));
	func->read_block(handle, (uint8_t*)&data->gateway, sizeof(struct in_addr));
	func->read_u8(handle, &data->staticip);

	return data;
}

/* */
static void capwap_wtpstaticipaddress_element_free(void* data) {
	ASSERT(data != NULL);
	
	capwap_free(data);
}

/* */
struct capwap_message_elements_ops capwap_element_wtpstaticipaddress_ops = {
	.create_message_element = capwap_wtpstaticipaddress_element_create,
	.parsing_message_element = capwap_wtpstaticipaddress_element_parsing,
	.free_parsed_message_element = capwap_wtpstaticipaddress_element_free
};
