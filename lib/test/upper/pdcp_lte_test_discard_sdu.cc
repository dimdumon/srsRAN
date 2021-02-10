/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2021 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */
#include "pdcp_lte_test.h"
#include <numeric>

/*
 * Test of imediate notification from RLC
 */
int test_tx_sdu_notify(const srslte::pdcp_lte_state_t& init_state,
                       srslte::pdcp_discard_timer_t    discard_timeout,
                       srslog::basic_logger&           logger)
{
  srslte::pdcp_config_t cfg = {1,
                               srslte::PDCP_RB_IS_DRB,
                               srslte::SECURITY_DIRECTION_UPLINK,
                               srslte::SECURITY_DIRECTION_DOWNLINK,
                               srslte::PDCP_SN_LEN_12,
                               srslte::pdcp_t_reordering_t::ms500,
                               discard_timeout};

  pdcp_lte_test_helper     pdcp_hlp(cfg, sec_cfg, logger);
  srslte::pdcp_entity_lte* pdcp  = &pdcp_hlp.pdcp;
  rlc_dummy*               rlc   = &pdcp_hlp.rlc;
  srsue::stack_test_dummy* stack = &pdcp_hlp.stack;

  pdcp_hlp.set_pdcp_initial_state(init_state);

  // Write test SDU
  srslte::unique_byte_buffer_t sdu = srslte::make_byte_buffer();
  sdu->append_bytes(sdu1, sizeof(sdu1));
  pdcp->write_sdu(std::move(sdu));

  srslte::unique_byte_buffer_t out_pdu = srslte::make_byte_buffer();
  rlc->get_last_sdu(out_pdu);
  TESTASSERT(out_pdu->N_bytes == 4);

  TESTASSERT(pdcp->nof_discard_timers() == 1); // One timer should be running
  std::vector<uint32_t> sns_notified = {0};
  pdcp->notify_delivery(sns_notified);
  TESTASSERT(pdcp->nof_discard_timers() == 0); // Timer should have been difused after

  // RLC should not be notified of SDU discard
  TESTASSERT(rlc->discard_count == 0);

  // Make sure there are no timers still left on the map
  TESTASSERT(pdcp->nof_discard_timers() == 0);

  return 0;
}

/*
 * Test discard timer expiry
 */
int test_tx_sdu_discard(const srslte::pdcp_lte_state_t& init_state,
                        srslte::pdcp_discard_timer_t    discard_timeout,
                        srslog::basic_logger&           logger)
{
  srslte::pdcp_config_t cfg = {1,
                               srslte::PDCP_RB_IS_DRB,
                               srslte::SECURITY_DIRECTION_UPLINK,
                               srslte::SECURITY_DIRECTION_DOWNLINK,
                               srslte::PDCP_SN_LEN_12,
                               srslte::pdcp_t_reordering_t::ms500,
                               discard_timeout};

  pdcp_lte_test_helper     pdcp_hlp(cfg, sec_cfg, logger);
  srslte::pdcp_entity_lte* pdcp  = &pdcp_hlp.pdcp;
  rlc_dummy*               rlc   = &pdcp_hlp.rlc;
  srsue::stack_test_dummy* stack = &pdcp_hlp.stack;

  pdcp_hlp.set_pdcp_initial_state(init_state);

  // Write test SDU
  srslte::unique_byte_buffer_t sdu = srslte::make_byte_buffer();
  sdu->append_bytes(sdu1, sizeof(sdu1));
  pdcp->write_sdu(std::move(sdu));

  srslte::unique_byte_buffer_t out_pdu = srslte::make_byte_buffer();
  rlc->get_last_sdu(out_pdu);
  TESTASSERT(out_pdu->N_bytes == 4);

  // Run TTIs for Discard timers
  for (uint32_t i = 0; i < static_cast<uint32_t>(cfg.discard_timer) - 1; ++i) {
    stack->run_tti();
  }
  TESTASSERT(pdcp->nof_discard_timers() == 1); // One timer should be running
  TESTASSERT(rlc->discard_count == 0);         // No discard yet

  // Last timer step
  stack->run_tti();

  TESTASSERT(pdcp->nof_discard_timers() == 0); // Timer should have been difused after expiry
  TESTASSERT(rlc->discard_count == 1);         // RLC should be notified of discard

  std::vector<uint32_t> sns_notified = {0};
  pdcp->notify_delivery(sns_notified); // PDCP should not find PDU to notify.
  return 0;
}
/*
 * TX Test: PDCP Entity with SN LEN = 12 and 18.
 * PDCP entity configured with EIA2 and EEA2
 */
int test_tx_discard_all(srslog::basic_logger& logger)
{
  /*
   * TX Test 1: PDCP Entity with SN LEN = 12
   * Test TX PDU discard.
   */
  TESTASSERT(test_tx_sdu_notify(normal_init_state, srslte::pdcp_discard_timer_t::ms50, logger) == 0);

  /*
   * TX Test 2: PDCP Entity with SN LEN = 12
   * Test TX PDU discard.
   */
  TESTASSERT(test_tx_sdu_discard(normal_init_state, srslte::pdcp_discard_timer_t::ms50, logger) == 0);
  return 0;
}

// Setup all tests
int run_all_tests()
{
  // Setup log
  auto& logger = srslog::fetch_basic_logger("PDCP LTE Test", false);
  logger.set_level(srslog::basic_levels::debug);
  logger.set_hex_dump_max_size(128);

  TESTASSERT(test_tx_discard_all(logger) == 0);
  return 0;
}

int main()
{
  if (run_all_tests() != SRSLTE_SUCCESS) {
    fprintf(stderr, "pdcp_lte_tests() failed\n");
    return SRSLTE_ERROR;
  }

  return SRSLTE_SUCCESS;
}
