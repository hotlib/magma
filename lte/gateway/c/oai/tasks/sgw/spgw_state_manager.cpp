/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the Apache License, Version 2.0  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include "spgw_state_manager.h"

extern "C" {
#include <dynamic_memory_check.h>
}

// TODO: Move this to redis wrapper
namespace {
const char* LOCALHOST = "127.0.0.1";
}

namespace magma {
namespace lte {

SpgwStateManager::SpgwStateManager(): config_(nullptr) {}

SpgwStateManager::~SpgwStateManager()
{
  free_state();
}

SpgwStateManager& SpgwStateManager::getInstance()
{
  static SpgwStateManager instance;
  return instance;
}

void SpgwStateManager::init(bool persist_state, const spgw_config_t* config)
{
  log_task = LOG_SPGW_APP;
  table_key = SPGW_STATE_TABLE_NAME;
  persist_state_enabled = persist_state;
  config_ = config;
  create_state();
  init_db_connection(LOCALHOST);
  is_initialized = true;
}

void SpgwStateManager::create_state()
{
  // Allocating spgw_state_p
  state_cache_p = (spgw_state_t*) calloc(1, sizeof(spgw_state_t));

  bstring b = bfromcstr(SGW_S11_TEID_MME_HT_NAME);
  state_cache_p->sgw_state.s11teid2mme =
    hashtable_ts_create(SGW_STATE_CONTEXT_HT_MAX_SIZE, nullptr, nullptr, b);
  btrunc(b, 0);

  bassigncstr(b, S11_BEARER_CONTEXT_INFO_HT_NAME);
  state_cache_p->sgw_state.s11_bearer_context_information = hashtable_ts_create(
    SGW_STATE_CONTEXT_HT_MAX_SIZE,
    nullptr,
    (void (*)(void**)) sgw_free_s11_bearer_context_information,
    b);
  bdestroy_wrapper(&b);

  state_cache_p->sgw_state.sgw_ip_address_S1u_S12_S4_up.s_addr =
    config_->sgw_config.ipv4.S1u_S12_S4_up.s_addr;

  // TODO: Refactor GTPv1u_data state
  state_cache_p->sgw_state.gtpv1u_data.sgw_ip_address_for_S1u_S12_S4_up =
    state_cache_p->sgw_state.sgw_ip_address_S1u_S12_S4_up;

  // Creating PGW related state structs
  state_cache_p->pgw_state.deactivated_predefined_pcc_rules = hashtable_ts_create(
    MAX_PREDEFINED_PCC_RULES_HT_SIZE, nullptr, pgw_free_pcc_rule, nullptr);

  state_cache_p->pgw_state.predefined_pcc_rules = hashtable_ts_create(
    MAX_PREDEFINED_PCC_RULES_HT_SIZE, nullptr, pgw_free_pcc_rule, nullptr);

  // TO DO: RANDOM
  state_cache_p->sgw_state.tunnel_id = 0;

  state_cache_p->sgw_state.gtpv1u_teid = 0;
}

void SpgwStateManager::free_state()
{
  AssertFatal(
    is_initialized,
    "SpgwStateManager init() function should be called to initialize state.");

  if (state_cache_p == nullptr) {
    return;
  }

  if (
    hashtable_ts_destroy(state_cache_p->sgw_state.s11teid2mme) !=
    HASH_TABLE_OK) {
    OAI_FPRINTF_ERR(
      "An error occurred while destroying SGW s11teid2mme hashtable");
  }

  if (
    hashtable_ts_destroy(
      state_cache_p->sgw_state.s11_bearer_context_information) !=
    HASH_TABLE_OK) {
    OAI_FPRINTF_ERR(
      "An error occurred while destroying SGW s11_bearer_context_information "
      "hashtable");
  }

  if (state_cache_p->pgw_state.deactivated_predefined_pcc_rules) {
    hashtable_ts_destroy(
      state_cache_p->pgw_state.deactivated_predefined_pcc_rules);
  }

  if (state_cache_p->pgw_state.predefined_pcc_rules) {
    hashtable_ts_destroy(state_cache_p->pgw_state.predefined_pcc_rules);
  }
  free(state_cache_p);
}

} // namespace lte
} // namespace magma
