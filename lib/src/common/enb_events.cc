/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2020 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "srslte/common/enb_events.h"
#include "srslte/srslog/context.h"
#include "srslte/srslog/log_channel.h"

using namespace srsenb;

namespace {

/// Null object implementation that is used when no log channel is configured.
class null_event_logger : public event_logger_interface
{
public:
  void log_rrc_connected(unsigned cause) override {}
  void log_rrc_disconnect(unsigned reason) override {}
  void log_s1_ctx_create(uint32_t mme_id, uint32_t enb_id, uint16_t rnti) override {}
  void log_s1_ctx_delete(uint32_t mme_id, uint32_t enb_id, uint16_t rnti) override {}
  void log_sector_start(uint32_t cc_idx, uint32_t pci, uint32_t cell_id) override {}
  void log_sector_stop(uint32_t cc_idx, uint32_t pci, uint32_t cell_id) override {}
  void log_rlf(uint32_t cc_idx, const std::string& asn1, uint16_t rnti) override {}
};

} // namespace

namespace {

/// Common metrics to all events.
DECLARE_METRIC("type", metric_type_tag, std::string, "");
DECLARE_METRIC("event_name", metric_event_name, std::string, "");

DECLARE_METRIC("rnti", metric_rnti, uint16_t, "");
DECLARE_METRIC("sector_id", metric_sector_id, uint32_t, "");

/// ASN1 message metrics.
DECLARE_METRIC("asn1_length", metric_asn1_length, uint32_t, "");
DECLARE_METRIC("asn1_message", metric_asn1_message, std::string, "");

/// Context for sector start/stop.
DECLARE_METRIC("pci", metric_pci, uint32_t, "");
DECLARE_METRIC("cell_identity", metric_cell_identity, uint32_t, "");
DECLARE_METRIC_SET("event_data", mset_sector_event, metric_pci, metric_cell_identity, metric_sector_id);
using sector_event_t = srslog::build_context_type<metric_type_tag, metric_event_name, mset_sector_event>;

/// Context for RRC connect/disconnect.
DECLARE_METRIC("cause", metric_cause, uint32_t, "");
DECLARE_METRIC_SET("event_data", mset_rrc_event, metric_cause);
using rrc_event_t = srslog::build_context_type<metric_type_tag, metric_event_name, mset_rrc_event>;

/// Context for S1 context create/delete.
DECLARE_METRIC("mme_ue_s1ap_id", metric_ue_mme_id, uint32_t, "");
DECLARE_METRIC("enb_ue_s1ap_id", metric_ue_enb_id, uint32_t, "");
DECLARE_METRIC_SET("event_data", mset_s1apctx_event, metric_ue_mme_id, metric_ue_enb_id, metric_rnti);
using s1apctx_event_t = srslog::build_context_type<metric_type_tag, metric_event_name, mset_s1apctx_event>;

/// Context for the RLC event.
DECLARE_METRIC_SET("event_data", mset_rlfctx_event, metric_asn1_length, metric_asn1_message, metric_rnti);
using rlfctx_event_t = srslog::build_context_type<metric_type_tag, metric_event_name, mset_rlfctx_event>;

/// Logs events into the configured log channel.
class logging_event_logger : public event_logger_interface
{
public:
  explicit logging_event_logger(srslog::log_channel& c) : event_channel(c) {}

  void log_rrc_connected(unsigned cause) override
  {
    rrc_event_t ctx("");

    ctx.write<metric_type_tag>("event");
    ctx.write<metric_event_name>("rrc_connect");
    ctx.get<mset_rrc_event>().write<metric_cause>(cause);
    event_channel(ctx);
  }

  void log_rrc_disconnect(unsigned reason) override
  {
    rrc_event_t ctx("");

    ctx.write<metric_type_tag>("event");
    ctx.write<metric_event_name>("rrc_disconnect");
    ctx.get<mset_rrc_event>().write<metric_cause>(reason);
    event_channel(ctx);
  }

  void log_s1_ctx_create(uint32_t mme_id, uint32_t enb_id, uint16_t rnti) override
  {
    s1apctx_event_t ctx("");

    ctx.write<metric_type_tag>("event");
    ctx.write<metric_event_name>("s1_context_create");
    ctx.get<mset_s1apctx_event>().write<metric_ue_mme_id>(mme_id);
    ctx.get<mset_s1apctx_event>().write<metric_ue_enb_id>(enb_id);
    ctx.get<mset_s1apctx_event>().write<metric_rnti>(rnti);
    event_channel(ctx);
  }

  void log_s1_ctx_delete(uint32_t mme_id, uint32_t enb_id, uint16_t rnti) override
  {
    s1apctx_event_t ctx("");

    ctx.write<metric_type_tag>("event");
    ctx.write<metric_event_name>("s1_context_delete");
    ctx.get<mset_s1apctx_event>().write<metric_ue_mme_id>(mme_id);
    ctx.get<mset_s1apctx_event>().write<metric_ue_enb_id>(enb_id);
    ctx.get<mset_s1apctx_event>().write<metric_rnti>(rnti);
    event_channel(ctx);
  }

  void log_sector_start(uint32_t cc_idx, uint32_t pci, uint32_t cell_id) override
  {
    sector_event_t ctx("");

    ctx.write<metric_type_tag>("event");
    ctx.write<metric_event_name>("sector_start");
    ctx.get<mset_sector_event>().write<metric_pci>(pci);
    ctx.get<mset_sector_event>().write<metric_cell_identity>(cell_id);
    ctx.get<mset_sector_event>().write<metric_sector_id>(cc_idx);
    event_channel(ctx);
  }

  void log_sector_stop(uint32_t cc_idx, uint32_t pci, uint32_t cell_id) override
  {
    sector_event_t ctx("");

    ctx.write<metric_type_tag>("event");
    ctx.write<metric_event_name>("sector_stop");
    ctx.get<mset_sector_event>().write<metric_pci>(pci);
    ctx.get<mset_sector_event>().write<metric_cell_identity>(cell_id);
    ctx.get<mset_sector_event>().write<metric_sector_id>(cc_idx);
    event_channel(ctx);
  }

  void log_rlf(uint32_t cc_idx, const std::string& asn1, uint16_t rnti) override
  {
    rlfctx_event_t ctx("");

    ctx.write<metric_type_tag>("event");
    ctx.write<metric_event_name>("radio_link_failure");
    ctx.get<mset_rlfctx_event>().write<metric_asn1_length>(asn1.size());
    ctx.get<mset_rlfctx_event>().write<metric_asn1_message>(asn1);
    ctx.get<mset_rlfctx_event>().write<metric_rnti>(rnti);
    event_channel(ctx);
  }

private:
  srslog::log_channel& event_channel;
};

} // namespace

std::unique_ptr<event_logger_interface> event_logger::pimpl = std::unique_ptr<null_event_logger>(new null_event_logger);

event_logger_interface& event_logger::get()
{
  return *pimpl;
}

void event_logger::configure(srslog::log_channel& c)
{
  pimpl = std::unique_ptr<logging_event_logger>(new logging_event_logger(c));
}
