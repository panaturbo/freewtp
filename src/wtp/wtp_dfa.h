#ifndef __WTP_DFA_HEADER__
#define __WTP_DFA_HEADER__

#include "wtp.h"
#include "capwap_network.h"
#include "capwap_protocol.h"
#include "capwap_element.h"

/* Execute WTP DFA */
#define WTP_DFA_NO_PACKET			1
#define WTP_DFA_ACCEPT_PACKET		2
#define WTP_DFA_DROP_PACKET			3

/* */
struct wtp_discovery_response {
	struct sockaddr_storage acaddr;
	struct capwap_build_packet* packet;
	struct capwap_element_discovery_response discoveryresponse;
};

void wtp_free_discovery_response_array(void);

/* */
int wtp_bio_send(struct capwap_dtls* dtls, char* buffer, int length, void* param);

/* */
int wtp_teardown_connection(struct timeout_control* timeout);

/* */
void wtp_free_reference_last_request(void);
void wtp_free_reference_last_response(void);

/* State machine */
int wtp_dfa_execute(void);
void wtp_dfa_change_state(int state);

int wtp_dfa_state_idle(struct capwap_packet* packet, struct timeout_control* timeout);
int wtp_dfa_state_idle_to_discovery(struct capwap_packet* packet, struct timeout_control* timeout);
int wtp_dfa_state_idle_to_dtlssetup(struct capwap_packet* packet, struct timeout_control* timeout);

int wtp_dfa_state_discovery(struct capwap_packet* packet, struct timeout_control* timeout);
int wtp_dfa_state_discovery_to_sulking(struct capwap_packet* packet, struct timeout_control* timeout);
int wtp_dfa_state_discovery_to_dtlssetup(struct capwap_packet* packet, struct timeout_control* timeout);

int wtp_dfa_state_sulking(struct capwap_packet* packet, struct timeout_control* timeout);
int wtp_dfa_state_sulking_to_idle(struct capwap_packet* packet, struct timeout_control* timeout);

int wtp_dfa_state_dtlssetup(struct capwap_packet* packet, struct timeout_control* timeout);
int wtp_dfa_state_dtlsconnect(struct capwap_packet* packet, struct timeout_control* timeout);
int wtp_dfa_state_dtlsconnect_to_dtlsteardown(struct capwap_packet* packet, struct timeout_control* timeout);

int wtp_dfa_state_dtlsteardown(struct capwap_packet* packet, struct timeout_control* timeout);
int wtp_dfa_state_dtlsteardown_to_sulking(struct capwap_packet* packet, struct timeout_control* timeout);
int wtp_dfa_state_dtlsteardown_to_idle(struct capwap_packet* packet, struct timeout_control* timeout);

int wtp_dfa_state_dtlsconnect_to_join(struct capwap_packet* packet, struct timeout_control* timeout);
int wtp_dfa_state_join(struct capwap_packet* packet, struct timeout_control* timeout);
int wtp_dfa_state_join_to_configure(struct capwap_packet* packet, struct timeout_control* timeout);
int wtp_dfa_state_join_to_dtlsteardown(struct capwap_packet* packet, struct timeout_control* timeout);

int wtp_dfa_state_imagedata_to_dtlsteardown(struct capwap_packet* packet, struct timeout_control* timeout);

int wtp_dfa_state_configure(struct capwap_packet* packet, struct timeout_control* timeout);
int wtp_dfa_state_configure_to_datacheck(struct capwap_packet* packet, struct timeout_control* timeout);
int wtp_dfa_state_configure_to_dtlsteardown(struct capwap_packet* packet, struct timeout_control* timeout);

int wtp_dfa_state_datacheck(struct capwap_packet* packet, struct timeout_control* timeout);
int wtp_dfa_state_datacheck_to_run(struct capwap_packet* packet, struct timeout_control* timeout);
int wtp_dfa_state_datacheck_to_dtlsteardown(struct capwap_packet* packet, struct timeout_control* timeout);

int wtp_dfa_state_run(struct capwap_packet* packet, struct timeout_control* timeout);
int wtp_dfa_state_run_to_dtlsteardown(struct capwap_packet* packet, struct timeout_control* timeout);

int wtp_dfa_state_reset(struct capwap_packet* packet, struct timeout_control* timeout);

#endif /* __WTP_DFA_HEADER__ */