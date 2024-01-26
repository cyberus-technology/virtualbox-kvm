/*
 * Copyright © 2020-2021 Collabora, Ltd.
 * Author: Antonio Caggiano <antonio.caggiano@collabora.com>
 * Author: Corentin Noël <corentin.noel@collabora.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "intel_pps_driver.h"

#include <dirent.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <i915_drm.h>
#include <intel/perf/intel_perf_query.h>

#include <pps/pps.h>
#include <pps/pps_algorithm.h>

#include "intel_pps_perf.h"

namespace pps
{
uint64_t IntelDriver::get_min_sampling_period_ns()
{
   return 500000;
}

void IntelDriver::enable_counter(uint32_t counter_id)
{
   auto &counter = counters[counter_id];
   auto &group = groups[counter.group];
   if (perf->query) {
      if (perf->query->symbol_name != group.name) {
         PPS_LOG_ERROR(
            "Unable to enable metrics from different sets: %u "
            "belongs to %s but %s is currently in use.",
            counter_id,
            perf->query->symbol_name,
            group.name.c_str());
         return;
      }
   }

   enabled_counters.emplace_back(counter);
   if (!perf->query) {
      perf->query = perf->find_query_by_name(group.name);
   }
}

void IntelDriver::enable_all_counters()
{
   // We can only enable one metric set at a time so at least enable one.
   for (auto &group : groups) {
      if (group.name == "RenderBasic") {
         for (uint32_t counter_id : group.counters) {
            auto &counter = counters[counter_id];
            enabled_counters.emplace_back(counter);
         }

         perf->query = perf->find_query_by_name(group.name);
         break;
      }
   }
}

static uint64_t timespec_diff(timespec *begin, timespec *end)
{
   return 1000000000ull * (end->tv_sec - begin->tv_sec) + end->tv_nsec - begin->tv_nsec;
}

/// @brief This function tries to correlate CPU time with GPU time
std::optional<TimestampCorrelation> IntelDriver::query_correlation_timestamps() const
{
   TimestampCorrelation corr = {};

   clock_t correlation_clock_id = CLOCK_BOOTTIME;

   drm_i915_reg_read reg_read = {};
   const uint64_t render_ring_timestamp = 0x2358;
   reg_read.offset = render_ring_timestamp | I915_REG_READ_8B_WA;

   constexpr size_t attempt_count = 3;
   struct {
      timespec cpu_ts_begin;
      timespec cpu_ts_end;
      uint64_t gpu_ts;
   } attempts[attempt_count] = {};

   uint32_t best = 0;

   // Gather 3 correlations
   for (uint32_t i = 0; i < attempt_count; i++) {
      clock_gettime(correlation_clock_id, &attempts[i].cpu_ts_begin);
      if (perf_ioctl(drm_device.fd, DRM_IOCTL_I915_REG_READ, &reg_read) < 0) {
         return std::nullopt;
      }
      clock_gettime(correlation_clock_id, &attempts[i].cpu_ts_end);

      attempts[i].gpu_ts = reg_read.val;
   }

   // Now select the best
   for (uint32_t i = 1; i < attempt_count; i++) {
      if (timespec_diff(&attempts[i].cpu_ts_begin, &attempts[i].cpu_ts_end) <
         timespec_diff(&attempts[best].cpu_ts_begin, &attempts[best].cpu_ts_end)) {
         best = i;
      }
   }

   corr.cpu_timestamp =
      (attempts[best].cpu_ts_begin.tv_sec * 1000000000ull + attempts[best].cpu_ts_begin.tv_nsec) +
      timespec_diff(&attempts[best].cpu_ts_begin, &attempts[best].cpu_ts_end) / 2;
   corr.gpu_timestamp = attempts[best].gpu_ts;

   return corr;
}

void IntelDriver::get_new_correlation()
{
   // Rotate left correlations by one position so to make space at the end
   std::rotate(correlations.begin(), correlations.begin() + 1, correlations.end());

   // Then we overwrite the last correlation with a new one
   if (auto corr = query_correlation_timestamps()) {
      correlations.back() = *corr;
   } else {
      PPS_LOG_FATAL("Failed to get correlation timestamps");
   }
}

bool IntelDriver::init_perfcnt()
{
   assert(!perf && "Intel perf should not be initialized at this point");

   perf = std::make_unique<IntelPerf>(drm_device.fd);

   for (auto &query : perf->get_queries()) {
      // Create group
      CounterGroup group = {};
      group.id = groups.size();
      group.name = query->symbol_name;

      for (int i = 0; i < query->n_counters; ++i) {
         intel_perf_query_counter &counter = query->counters[i];

         // Create counter
         Counter counter_desc = {};
         counter_desc.id = counters.size();
         counter_desc.name = counter.symbol_name;
         counter_desc.group = group.id;
         counter_desc.getter = [counter, query, this](
                                  const Counter &c, const Driver &dri) -> Counter::Value {
            switch (counter.data_type) {
            case INTEL_PERF_COUNTER_DATA_TYPE_UINT64:
            case INTEL_PERF_COUNTER_DATA_TYPE_UINT32:
            case INTEL_PERF_COUNTER_DATA_TYPE_BOOL32:
               return (int64_t)counter.oa_counter_read_uint64(perf->cfg, query, &result);
               break;
            case INTEL_PERF_COUNTER_DATA_TYPE_DOUBLE:
            case INTEL_PERF_COUNTER_DATA_TYPE_FLOAT:
               return counter.oa_counter_read_float(perf->cfg, query, &result);
               break;
            }

            return {};
         };

         // Add counter id to the group
         group.counters.emplace_back(counter_desc.id);

         // Store counter
         counters.emplace_back(std::move(counter_desc));
      }

      // Store group
      groups.emplace_back(std::move(group));
   }

   assert(groups.size() && "Failed to query groups");
   assert(counters.size() && "Failed to query counters");

   // Clear accumulations
   intel_perf_query_result_clear(&result);

   return true;
}

void IntelDriver::enable_perfcnt(uint64_t sampling_period_ns)
{
   this->sampling_period_ns = sampling_period_ns;

   // Fill correlations with an initial one
   if (auto corr = query_correlation_timestamps()) {
      correlations.fill(*corr);
   } else {
      PPS_LOG_FATAL("Failed to get correlation timestamps");
   }

   if (!perf->open(sampling_period_ns)) {
      PPS_LOG_FATAL("Failed to open intel perf");
   }
}

/// @brief Transforms the GPU timestop into a CPU timestamp equivalent
uint64_t IntelDriver::correlate_gpu_timestamp(const uint32_t gpu_ts)
{
   auto &corr_a = correlations[0];
   auto &corr_b = correlations[correlations.size() - 1];

   // A correlation timestamp has 36 bits, so get the first 32 to make it work with gpu_ts
   uint64_t mask = 0xffffffff;
   uint32_t corr_a_gpu_ts = corr_a.gpu_timestamp & mask;
   uint32_t corr_b_gpu_ts = corr_b.gpu_timestamp & mask;

   // Make sure it is within the interval [a,b)
   assert(gpu_ts >= corr_a_gpu_ts && "GPU TS < Corr a");
   assert(gpu_ts < corr_b_gpu_ts && "GPU TS >= Corr b");

   uint32_t gpu_delta = gpu_ts - corr_a_gpu_ts;
   // Factor to convert gpu time to cpu time
   double gpu_to_cpu = (corr_b.cpu_timestamp - corr_a.cpu_timestamp) /
      double(corr_b.gpu_timestamp - corr_a.gpu_timestamp);
   uint64_t cpu_delta = gpu_delta * gpu_to_cpu;
   return corr_a.cpu_timestamp + cpu_delta;
}

void IntelDriver::disable_perfcnt()
{
   perf = nullptr;
   groups.clear();
   counters.clear();
   enabled_counters.clear();
}

struct Report {
   uint32_t version;
   uint32_t timestamp;
   uint32_t id;
};

/// @brief Some perf record durations can be really short
/// @return True if the duration is at least close to the sampling period
static bool close_enough(uint64_t duration, uint64_t sampling_period)
{
   return duration > sampling_period - 100000;
}

/// @brief Transforms the raw data received in from the driver into records
std::vector<PerfRecord> IntelDriver::parse_perf_records(const std::vector<uint8_t> &data,
   const size_t byte_count)
{
   std::vector<PerfRecord> records;
   records.reserve(128);

   PerfRecord record;
   record.reserve(512);

   const uint8_t *iter = data.data();
   const uint8_t *end = iter + byte_count;

   uint64_t prev_cpu_timestamp = last_cpu_timestamp;

   while (iter < end) {
      // Iterate a record at a time
      auto header = reinterpret_cast<const drm_i915_perf_record_header *>(iter);

      if (header->type == DRM_I915_PERF_RECORD_SAMPLE) {
         // Report is next to the header
         auto report = reinterpret_cast<const Report *>(header + 1);
         auto cpu_timestamp = correlate_gpu_timestamp(report->timestamp);
         auto duration = cpu_timestamp - prev_cpu_timestamp;

         // Skip perf-records that are too short by checking
         // the distance between last report and this one
         if (close_enough(duration, sampling_period_ns)) {
            prev_cpu_timestamp = cpu_timestamp;

            // Add the new record to the list
            record.resize(header->size); // Possibly 264?
            memcpy(record.data(), iter, header->size);
            records.emplace_back(record);
         }
      }

      // Go to the next record
      iter += header->size;
   }

   return records;
}

/// @brief Read all the available data from the metric set currently in use
void IntelDriver::read_data_from_metric_set()
{
   assert(metric_buffer.size() >= 1024 && "Metric buffer should have space for reading");

   ssize_t bytes_read = 0;
   while ((bytes_read = perf->read_oa_stream(metric_buffer.data() + total_bytes_read,
              metric_buffer.size() - total_bytes_read)) > 0 ||
      errno == EINTR) {
      total_bytes_read += std::max(ssize_t(0), bytes_read);

      // Increase size of the buffer for the next read
      if (metric_buffer.size() / 2 < total_bytes_read) {
         metric_buffer.resize(metric_buffer.size() * 2);
      }
   }

   assert(total_bytes_read < metric_buffer.size() && "Buffer not big enough");
}

bool IntelDriver::dump_perfcnt()
{
   if (!perf->oa_stream_ready()) {
      return false;
   }

   read_data_from_metric_set();

   get_new_correlation();

   auto new_records = parse_perf_records(metric_buffer, total_bytes_read);
   if (new_records.empty()) {
      PPS_LOG("No new records");
      // No new records from the GPU yet
      return false;
   } else {
      PPS_LOG("Records parsed bytes: %lu", total_bytes_read);
      // Records are parsed correctly, so we can reset the
      // number of bytes read so far from the metric set
      total_bytes_read = 0;
   }

   APPEND(records, new_records);

   if (records.size() < 2) {
      // Not enough records to accumulate
      return false;
   }

   return true;
}

uint32_t IntelDriver::gpu_next()
{
   if (records.size() < 2) {
      // Not enough records to accumulate
      return 0;
   }

   // Get first and second
   auto record_a = reinterpret_cast<const drm_i915_perf_record_header *>(records[0].data());
   auto record_b = reinterpret_cast<const drm_i915_perf_record_header *>(records[1].data());

   intel_perf_query_result_accumulate_fields(&result,
      &perf->query.value(),
      &perf->devinfo,
      record_a + 1,
      record_b + 1,
      false /* no_oa_accumulate */);

   // Get last timestamp
   auto report_b = reinterpret_cast<const Report *>(record_b + 1);
   auto gpu_timestamp = report_b->timestamp;

   // Consume first record
   records.erase(std::begin(records), std::begin(records) + 1);

   return gpu_timestamp;
}

uint64_t IntelDriver::cpu_next()
{
   if (auto gpu_timestamp = gpu_next()) {
      auto cpu_timestamp = correlate_gpu_timestamp(gpu_timestamp);

      last_cpu_timestamp = cpu_timestamp;
      return cpu_timestamp;
   }

   return 0;
}

uint64_t IntelDriver::next()
{
   // Reset accumulation
   intel_perf_query_result_clear(&result);
   return cpu_next();
}

} // namespace pps
