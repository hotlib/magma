/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
/*! \file mme_app_messages_types.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#ifndef FILE_MME_APP_MESSAGES_TYPES_SEEN
#define FILE_MME_APP_MESSAGES_TYPES_SEEN

#include <stdint.h>

#include "bstrlib.h"

#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "common_types.h"
#include "nas/securityDef.h"

#define MME_APP_CONNECTION_ESTABLISHMENT_CNF(mSGpTR)                           \
  (mSGpTR)->ittiMsg.mme_app_connection_establishment_cnf
#define MME_APP_INITIAL_CONTEXT_SETUP_RSP(mSGpTR)                              \
  (mSGpTR)->ittiMsg.mme_app_initial_context_setup_rsp
#define MME_APP_INITIAL_CONTEXT_SETUP_FAILURE(mSGpTR)                          \
  (mSGpTR)->ittiMsg.mme_app_initial_context_setup_failure
#define MME_APP_S1AP_MME_UE_ID_NOTIFICATION(mSGpTR)                            \
  (mSGpTR)->ittiMsg.mme_app_s1ap_mme_ue_id_notification
#define MME_APP_UL_DATA_IND(mSGpTR) (mSGpTR)->ittiMsg.mme_app_ul_data_ind
#define MME_APP_DL_DATA_CNF(mSGpTR) (mSGpTR)->ittiMsg.mme_app_dl_data_cnf
#define MME_APP_DL_DATA_REJ(mSGpTR) (mSGpTR)->ittiMsg.mme_app_dl_data_rej
#define MME_APP_CREATE_DEDICATED_BEARER_REQ(mSGpTR)                            \
  (mSGpTR)->ittiMsg.mme_app_create_dedicated_bearer_req
#define MME_APP_CREATE_DEDICATED_BEARER_RSP(mSGpTR)                            \
  (mSGpTR)->ittiMsg.mme_app_create_dedicated_bearer_rsp
#define MME_APP_CREATE_DEDICATED_BEARER_REJ(mSGpTR)                            \
  (mSGpTR)->ittiMsg.mme_app_create_dedicated_bearer_rej
#define MME_APP_DELETE_DEDICATED_BEARER_REQ(mSGpTR)                            \
  (mSGpTR)->ittiMsg.mme_app_delete_dedicated_bearer_req
#define MME_APP_DELETE_DEDICATED_BEARER_RSP(mSGpTR)                            \
  (mSGpTR)->ittiMsg.mme_app_delete_dedicated_bearer_rsp
#define MME_APP_DELETE_DEDICATED_BEARER_REJ(mSGpTR)                            \
  (mSGpTR)->ittiMsg.mme_app_delete_dedicated_bearer_rej

typedef struct itti_mme_app_connection_establishment_cnf_s {
  mme_ue_s1ap_id_t ue_id;
  ambr_t ue_ambr;
  // E-RAB to Be Setup List
  uint8_t no_of_e_rabs; // spec says max 256, actually stay with BEARERS_PER_UE
  //     >>E-RAB ID
  ebi_t e_rab_id[BEARERS_PER_UE];
  //     >>E-RAB Level QoS Parameters
  qci_t e_rab_level_qos_qci[BEARERS_PER_UE];
  //       >>>Allocation and Retention Priority
  priority_level_t e_rab_level_qos_priority_level[BEARERS_PER_UE];
  //       >>>Pre-emption Capability
  pre_emption_capability_t
    e_rab_level_qos_preemption_capability[BEARERS_PER_UE];
  //       >>>Pre-emption Vulnerability
  pre_emption_vulnerability_t
    e_rab_level_qos_preemption_vulnerability[BEARERS_PER_UE];
  //     >>Transport Layer Address
  bstring transport_layer_address[BEARERS_PER_UE];
  //     >>GTP-TEID
  teid_t gtp_teid[BEARERS_PER_UE];
  //     >>NAS-PDU (optional)
  bstring nas_pdu[BEARERS_PER_UE];
  //     >>Correlation ID TODO? later...

  // UE Security Capabilities
  uint16_t ue_security_capabilities_encryption_algorithms;
  uint16_t ue_security_capabilities_integrity_algorithms;

  // Security key
  uint8_t kenb[AUTH_KENB_SIZE];

  bstring ue_radio_capability;

  uint8_t presencemask;
#define S1AP_CSFB_INDICATOR_PRESENT (1 << 0)
  s1ap_csfb_indicator_t cs_fallback_indicator;
  // Trace Activation (optional)
  // Handover Restriction List (optional)
  // UE Radio Capability (optional)
  // Subscriber Profile ID for RAT/Frequency priority (optional)
  // CS Fallback Indicator (optional)
  // SRVCC Operation Possible (optional)
  // CSG Membership Status (optional)
  // Registered LAI (optional)
  // GUMMEI ID (optional)
  // MME UE S1AP ID 2  (optional)
  // Management Based MDT Allowed (optional)

  //itti_nas_conn_est_cnf_t nas_conn_est_cnf;
} itti_mme_app_connection_establishment_cnf_t;

typedef struct itti_mme_app_initial_context_setup_rsp_s {
  mme_ue_s1ap_id_t ue_id;
  uint8_t no_of_e_rabs;
  ebi_t e_rab_id[BEARERS_PER_UE];
  bstring transport_layer_address[BEARERS_PER_UE];
  s1u_teid_t gtp_teid[BEARERS_PER_UE];
} itti_mme_app_initial_context_setup_rsp_t;

typedef struct itti_mme_app_initial_context_setup_failure_s {
  uint32_t mme_ue_s1ap_id;
} itti_mme_app_initial_context_setup_failure_t;

typedef struct itti_mme_app_delete_session_rsp_s {
  /* UE identifier */
  mme_ue_s1ap_id_t ue_id;
} itti_mme_app_delete_session_rsp_t;

typedef struct itti_mme_app_create_dedicated_bearer_req_s {
  /* UE identifier */
  mme_ue_s1ap_id_t ue_id;
  pdn_cid_t cid;
  ebi_t ebi;
  ebi_t linked_ebi;
  bearer_qos_t bearer_qos;
  traffic_flow_template_t *tft;
  protocol_configuration_options_t *pco;
  fteid_t sgw_fteid;
} itti_mme_app_create_dedicated_bearer_req_t;

typedef struct itti_mme_app_create_dedicated_bearer_rsp_s {
  /* UE identifier */
  mme_ue_s1ap_id_t ue_id;
  ebi_t ebi;
} itti_mme_app_create_dedicated_bearer_rsp_t;

typedef struct itti_mme_app_create_dedicated_bearer_rej_s {
  /* UE identifier */
  mme_ue_s1ap_id_t ue_id;
  ebi_t ebi;
} itti_mme_app_create_dedicated_bearer_rej_t;

typedef struct itti_mme_app_s1ap_mme_ue_id_notification_s {
  enb_ue_s1ap_id_t enb_ue_s1ap_id;
  mme_ue_s1ap_id_t mme_ue_s1ap_id;
  sctp_assoc_id_t sctp_assoc_id;
} itti_mme_app_s1ap_mme_ue_id_notification_t;

typedef struct itti_mme_app_dl_data_cnf_s {
  mme_ue_s1ap_id_t ue_id;    /* UE lower layer identifier        */
  nas_error_code_t err_code; /* Transaction status               */
} itti_mme_app_dl_data_cnf_t;

typedef struct itti_mme_app_dl_data_rej_s {
  mme_ue_s1ap_id_t ue_id; /* UE lower layer identifier   */
  bstring nas_msg;        /* Uplink NAS message           */
  int err_code;
} itti_mme_app_dl_data_rej_t;

typedef struct itti_mme_app_ul_data_ind_s {
  mme_ue_s1ap_id_t ue_id; /* UE lower layer identifier    */
  bstring nas_msg;        /* Uplink NAS message           */
  /* Indicating the Tracking Area from which the UE has sent the NAS message */
  tai_t tai;
  /* Indicating the cell from which the UE has sent the NAS message  */
  ecgi_t cgi;
} itti_mme_app_ul_data_ind_t;

typedef struct itti_mme_app_delete_dedicated_bearer_req_s {
  /* UE identifier */
  uint32_t no_of_bearers;
  ebi_t ebi[BEARERS_PER_UE]; //EPS Bearer ID
  mme_ue_s1ap_id_t ue_id;
} itti_mme_app_delete_dedicated_bearer_req_t;

typedef struct itti_mme_app_delete_dedicated_bearer_rsp_s {
  /* UE identifier */
  uint32_t no_of_bearers;
  ebi_t ebi[BEARERS_PER_UE]; //EPS Bearer ID
  mme_ue_s1ap_id_t ue_id;
  bool delete_default_bearer;
  teid_t s_gw_teid_s11_s4;
} itti_mme_app_delete_dedicated_bearer_rsp_t;

typedef struct itti_mme_app_delete_dedicated_bearer_rej_s {
  /* UE identifier */
  mme_ue_s1ap_id_t ue_id;
  uint32_t no_of_bearers;
  ebi_t ebi[BEARERS_PER_UE]; //EPS Bearer ID
  teid_t s_gw_teid_s11_s4;
  bool delete_default_bearer;
} itti_mme_app_delete_dedicated_bearer_rej_t;

#endif /* FILE_MME_APP_MESSAGES_TYPES_SEEN */
