#pragma once

// TODO(yzhao): According to https://github.com/cilium/cilium/blob/master/Documentation/bpf.rst
// and https://lwn.net/Articles/741773/, kernel 4.16 & llvm 6.0 or newer are required to support BPF
// to BPF calls for C code. Figure out how to detect kernel and llvm versions.
#ifndef __inline
#ifdef SUPPORT_BPF2BPF_CALL
#define __inline
#else
// TODO(yzhao): Clarify the effect on GCC and Clang, and see if we can remove the inline keyword.
#define __inline inline __attribute__((__always_inline__))
#endif  // SUPPORT_BPF2BPF_CALL
#endif  // __inline

// This macro is essentially a min() function that caps a number.
// It performs the min in a way that keeps the the BPF verifier happy.
// It is essentially a traditional min(), plus a mask that helps old versions of the BPF verifier
// reason about the maximum value of a number.
//
// NOTE: cap must be a power-of-2.
// This is not checked for the caller, and behavior is undefined when this is not true.
//
// Note that we still apply a min() function before masking, otherwise, the mask may create a number
// lower than the min if the original number is greater than the cap_mask.
//
// Example:
//   cap = 16
//   cap-1 = 16-1 = 0xf
//   x = 36 = 0x24
//   BPF_LEN_CAP(x, cap) = 16
//
// However, if we remove the min() before applying the mask, we would get a smaller number.
//   x & (cap-1) = 4
#define BPF_LEN_CAP(x, cap) (x < cap ? (x & (cap - 1)) : cap)

/***********************************************************
 * General helper functions
 ***********************************************************/

// This is how Linux converts nanoseconds to clock ticks.
// Used to report PID start times in clock ticks, just like /proc/<pid>/stat does.
static __inline uint64_t pl_nsec_to_clock_t(uint64_t x) {
  return div_u64(x, NSEC_PER_SEC / USER_HZ);
}

static __inline uint64_t get_tgid_start_time() {
  struct task_struct* task = (struct task_struct*)bpf_get_current_task();
  // Linux 5.5 renames the variable to start_boottime.
#if LINUX_VERSION_CODE >= 327680
  uint64_t start_boottime = task->group_leader->start_boottime;
#else
  uint64_t start_boottime = task->group_leader->real_start_time;
#endif

  return pl_nsec_to_clock_t(start_boottime);
}

static __inline int32_t read_big_endian_int32(const char* buf) {
  int32_t length;
  bpf_probe_read(&length, 4, buf);
  return bpf_ntohl(length);
}

// Returns 0 if lhs and rhs compares equal up to n bytes. Otherwise a non-zero value is returned.
// NOTE #1: Cannot use C standard library's strncmp() because that cannot be compiled by BCC.
// NOTE #2: Different from the C standard library's strncmp(), this does not distinguish order.
// NOTE #3: n must be a literal so that the BCC runtime can unroll the inner loop.
// NOTE #4: Loop unrolling increases instruction code, be aware when BPF verifier complains about
// breaching instruction count limit.
static __inline int bpf_strncmp(const char* lhs, const char* rhs, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    if (lhs[i] != rhs[i]) {
      return 1;
    }
  }
  return 0;
}