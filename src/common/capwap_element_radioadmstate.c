#include "capwap.h"
#include "capwap_element.h"

/********************************************************************

 0                   1
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   Radio ID    |  Admin State  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Type:   31 for Radio Administrative State

Length:  2

********************************************************************/

/* */
static void capwap_radioadmstate_element_create(void* data, capwap_message_elements_handle handle, struct capwap_write_message_elements_ops* func) {
	struct capwap_radioadmstate_element* element = (struct capwap_radioadmstate_element*)data;

	ASSERT(data != NULL);

	/* */
	func->write_u8(handle, element->radioid);
	func->write_u8(handle, element->state);
}

/* */
static void* capwap_radioadmstate_element_parsing(capwap_message_elements_handle handle, struct capwap_read_message_elements_ops* func) {
	struct capwap_radioadmstate_element* data;

	ASSERT(handle != NULL);
	ASSERT(func != NULL);

	if (func->read_ready(handle) != 2) {
		capwap_logging_debug("Invalid Radio Administrative State element");
		return NULL;
	}

	/* */
	data = (struct capwap_radioadmstate_element*)capwap_alloc(sizeof(struct capwap_radioadmstate_element));
	if (!data) {
		capwap_outofmemory();
	}

	/* Retrieve data */
	memset(data, 0, sizeof(struct capwap_radioadmstate_element));
	func->read_u8(handle, &data->radioid);
	func->read_u8(handle, &data->state);

	return data;
}

/* */
static void capwap_radioadmstate_element_free(void* data) {
	ASSERT(data != NULL);
	
	capwap_free(data);
}

/* */
struct capwap_message_elements_ops capwap_element_radioadmstate_ops = {
	.create_message_element = capwap_radioadmstate_element_create,
	.parsing_message_element = capwap_radioadmstate_element_parsing,
	.free_parsed_message_element = capwap_radioadmstate_element_free
};
