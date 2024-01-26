/*
 * Copyright © 2021 Collabora, Ltd.
 * Author: Antonio Caggiano <antonio.caggiano@collabora.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "intel_pps_perf.h"

#include <i915_drm.h>
#include <math.h>
#include <sys/ioctl.h>
#include <util/ralloc.h>
#include <utility>

#include <pps/pps.h>
#include <pps/pps_device.h>

namespace pps
{
int perf_ioctl(int fd, unsigned long request, void *arg)
{
   int ret;

   do {
      ret = ioctl(fd, request, arg);
   } while (ret == -1 && (errno == EINTR || errno == EAGAIN));

   return ret;
}

IntelPerf::IntelPerf(const int drm_fd)
   : drm_fd {drm_fd}
   , ralloc_ctx {ralloc_context(nullptr)}
   , ralloc_cfg {ralloc_context(nullptr)}
   , cfg {intel_perf_new(ralloc_cfg)}
{
   assert(drm_fd >= 0 && "DRM fd is not valid");

   if (!intel_get_device_info_from_fd(drm_fd, &devinfo)) {
      PPS_LOG_FATAL("Failed to get devinfo");
   }

   intel_perf_init_metrics(cfg,
      &devinfo,
      drm_fd,
      false, // no pipeline statistics
      false  // no register snapshots
   );

   // Enable RenderBasic counters
   auto query_name = "RenderBasic";
   query = find_query_by_name(query_name);
   if (!query) {
      PPS_LOG_FATAL("Failed to find %s query", query_name);
   }
}

IntelPerf::IntelPerf(IntelPerf &&o)
   : drm_fd {o.drm_fd}
   , ralloc_ctx {o.ralloc_ctx}
   , ralloc_cfg {o.ralloc_cfg}
   , ctx {o.ctx}
   , cfg {o.cfg}
   , devinfo {std::move(o.devinfo)}
   , query {std::move(o.query)}
{
   o.drm_fd = -1;
   o.ralloc_ctx = nullptr;
   o.ralloc_cfg = nullptr;
   o.ctx = nullptr;
   o.cfg = nullptr;
}

IntelPerf &IntelPerf::operator=(IntelPerf &&o) noexcept
{
   std::swap(drm_fd, o.drm_fd);
   std::swap(ralloc_ctx, o.ralloc_ctx);
   std::swap(ralloc_cfg, o.ralloc_cfg);
   std::swap(ctx, o.ctx);
   std::swap(cfg, o.cfg);
   std::swap(devinfo, o.devinfo);
   std::swap(query, o.query);
   return *this;
}

IntelPerf::~IntelPerf()
{
   close();

   if (ralloc_ctx) {
      ralloc_free(ralloc_ctx);
   }

   if (ralloc_cfg) {
      ralloc_free(ralloc_cfg);
   }
}

/// @return A query info, which is something like a group of counters
std::optional<struct intel_perf_query_info> IntelPerf::find_query_by_name(
   const std::string &name) const
{
   for (int i = 0; i < cfg->n_queries; ++i) {
      struct intel_perf_query_info query = cfg->queries[i];
      if (name == query.symbol_name) {
         return query;
      }
   }

   return std::nullopt;
}

std::vector<struct intel_perf_query_info *> IntelPerf::get_queries() const
{
   assert(cfg && "Intel perf config should be valid");
   assert(cfg->n_queries && "Intel perf queries not initialized");

   std::vector<struct intel_perf_query_info *> queries = {};

   for (int i = 0; i < cfg->n_queries; ++i) {
      struct intel_perf_query_info *query = &cfg->queries[i];
      // Skip invalid queries
      if (query && query->symbol_name) {
         queries.push_back(query);
      }
   }

   return queries;
}

static uint64_t query_timestamp_frequency(const int drm_fd)
{
   int timestamp_frequency;

   drm_i915_getparam_t gp = {};
   gp.param = I915_PARAM_CS_TIMESTAMP_FREQUENCY;
   gp.value = &timestamp_frequency;
   if (perf_ioctl(drm_fd, DRM_IOCTL_I915_GETPARAM, &gp) == 0) {
      return timestamp_frequency;
   }

   PPS_LOG_ERROR("Unable to query timestamp frequency from i915, guessing values...");
   return 12000000;
}

// The period_exponent gives a sampling period as follows:
// sample_period = timestamp_period * 2^(period_exponent + 1)
// where timestamp_period is 80ns for Haswell+
static uint32_t get_oa_exponent(const int drm_fd, const uint64_t sampling_period_ns)
{
   uint64_t timestamp_frequency = query_timestamp_frequency(drm_fd);
   return static_cast<uint32_t>(log2(sampling_period_ns * timestamp_frequency / 1000000000ull)) - 1;
}

bool IntelPerf::open(const uint64_t sampling_period_ns)
{
   assert(!ctx && "Perf context should not be initialized at this point");

   ctx = intel_perf_new_context(ralloc_ctx);
   intel_perf_init_context(ctx, cfg, nullptr, nullptr, nullptr, &devinfo, 0, drm_fd);

   auto oa_exponent = get_oa_exponent(drm_fd, sampling_period_ns);

   return intel_perf_open(ctx,
      query->oa_metrics_set_id,
      query->oa_format,
      oa_exponent,
      drm_fd,
      INTEL_PERF_INVALID_CTX_ID,
      true /* enable stream immediately */);
}

void IntelPerf::close()
{
   if (ctx) {
      intel_perf_close(ctx, nullptr);
      ctx = nullptr;
   }
}

bool IntelPerf::oa_stream_ready() const
{
   assert(ctx && "Perf context was not open");
   return intel_perf_oa_stream_ready(ctx);
}

ssize_t IntelPerf::read_oa_stream(void *buf, size_t bytes) const
{
   assert(ctx && "Perf context was not open");
   return intel_perf_read_oa_stream(ctx, buf, bytes);
}

} // namespace pps
