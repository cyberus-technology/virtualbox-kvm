/*
 * Copyright © 2021 Collabora, Ltd.
 * Author: Antonio Caggiano <antonio.caggiano@collabora.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

#include <intel/dev/intel_device_info.h>
#include <intel/perf/intel_perf.h>
#include <intel/perf/intel_perf_query.h>

namespace pps
{
int perf_ioctl(int fd, unsigned long request, void *arg);

class IntelPerf
{
   public:
   IntelPerf(int drm_fd);

   IntelPerf(const IntelPerf &) = delete;
   IntelPerf &operator=(const IntelPerf &) = delete;

   IntelPerf(IntelPerf &&);
   IntelPerf &operator=(IntelPerf &&) noexcept;

   ~IntelPerf();

   std::optional<struct intel_perf_query_info> find_query_by_name(const std::string &name) const;

   std::vector<struct intel_perf_query_info*> get_queries() const;

   bool open(uint64_t sampling_period_ns);
   void close();

   bool oa_stream_ready() const;
   ssize_t read_oa_stream(void *buf, size_t bytes) const;

   int drm_fd = -1;

   void *ralloc_ctx = nullptr;
   void *ralloc_cfg = nullptr;

   struct intel_perf_context *ctx = nullptr;
   struct intel_perf_config *cfg = nullptr;

   struct intel_device_info devinfo = {};

   std::optional<struct intel_perf_query_info> query = std::nullopt;
};

} // namespace pps
