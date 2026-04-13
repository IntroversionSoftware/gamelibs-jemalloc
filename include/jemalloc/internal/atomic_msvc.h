#ifndef JEMALLOC_INTERNAL_ATOMIC_MSVC_H
#define JEMALLOC_INTERNAL_ATOMIC_MSVC_H

#include "jemalloc/internal/jemalloc_preamble.h"

#define ATOMIC_INLINE JEMALLOC_ALWAYS_INLINE

#define ATOMIC_INIT(...) {__VA_ARGS__}

typedef enum
{
	atomic_memory_order_relaxed,
	atomic_memory_order_acquire,
	atomic_memory_order_release,
	atomic_memory_order_acq_rel,
	atomic_memory_order_seq_cst
} atomic_memory_order_t;

typedef char atomic_repr_0_t;
typedef short atomic_repr_1_t;
typedef long atomic_repr_2_t;
typedef __int64 atomic_repr_3_t;

ATOMIC_INLINE void atomic_fence(atomic_memory_order_t mo)
{
	_ReadWriteBarrier();
#if defined(_M_ARM) || defined(_M_ARM64) || defined(_M_ARM64EC)
	/* ARM needs a barrier for everything but relaxed. */
	if (mo != atomic_memory_order_relaxed) {
		MemoryBarrier();
	}
#elif defined(_M_IX86) || defined(_M_X64)
	/* x86 needs a barrier only for seq_cst. */
	if (mo == atomic_memory_order_seq_cst) {
		MemoryBarrier();
	}
#else
#error "Don't know how to create atomics for this platform for MSVC."
#endif
	_ReadWriteBarrier();
}

#define ATOMIC_INTERLOCKED_REPR(lg_size) atomic_repr_##lg_size##_t

#define ATOMIC_CONCAT(a, b) ATOMIC_RAW_CONCAT(a, b)
#define ATOMIC_RAW_CONCAT(a, b) a##b

/*
 * ATOMIC_INTERLOCKED_NAME(base_name, lg_size) constructs the size-suffixed
 * Interlocked function name, e.g.:
 *   ATOMIC_INTERLOCKED_NAME(_InterlockedAnd, 3) => _InterlockedAnd64
 */
#define ATOMIC_INTERLOCKED_NAME(base_name, lg_size) ATOMIC_CONCAT(base_name, ATOMIC_INTERLOCKED_SUFFIX(lg_size))

#define ATOMIC_INTERLOCKED_SUFFIX(lg_size) ATOMIC_CONCAT(ATOMIC_INTERLOCKED_SUFFIX_, lg_size)

#define ATOMIC_INTERLOCKED_SUFFIX_0 8
#define ATOMIC_INTERLOCKED_SUFFIX_1 16
#define ATOMIC_INTERLOCKED_SUFFIX_2
#define ATOMIC_INTERLOCKED_SUFFIX_3 64

#if defined(_M_ARM64) || defined(_M_ARM64EC)

/*
 * On ARM64, the Interlocked APIs have _acq/_rel/_nf variants that map to
 * one-sided barrier instructions (LDAXR/STLXR etc.), avoiding the full DMB
 * that the unsuffixed variants emit.
 *
 * ATOMIC_INTERLOCKED_NAME_MO constructs the fully size- and MO-suffixed name:
 *   ATOMIC_INTERLOCKED_NAME_MO(_InterlockedExchangeAdd, 3, relaxed)
 *   => _InterlockedExchangeAdd64_nf
 *
 * The MO suffix tokens are defined per memory order name. acq_rel and seq_cst
 * use the unsuffixed (full barrier) form since there is no single intrinsic
 * providing both acquire and release simultaneously.
 *
 * The two-level ATOMIC_CONCAT indirection forces full expansion of both the
 * size suffix and the MO suffix before token-pasting, preventing the
 * unexpanded closing ')' of nested macros from being pasted instead.
 *
 * Since mo is always a compile-time constant at real callsites (always_inline
 * functions called with atomic_memory_order_* enum values), the if/else chain
 * in ATOMIC_MO_CALL folds to a single direct intrinsic call with no runtime
 * overhead.
 *
 * clang-cl requires builtin intrinsics to be called directly rather than
 * through a function pointer, so each branch must be a complete call
 * expression -- hence ATOMIC_MO_CALL includes __VA_ARGS__.
 */
#define ATOMIC_MO_SUFFIX_relaxed _nf
#define ATOMIC_MO_SUFFIX_acquire _acq
#define ATOMIC_MO_SUFFIX_release _rel
#define ATOMIC_MO_SUFFIX_acq_rel /* full barrier, no suffix */
#define ATOMIC_MO_SUFFIX_seq_cst /* full barrier, no suffix */

#define ATOMIC_INTERLOCKED_MO_SUFFIX(mo_name) ATOMIC_CONCAT(ATOMIC_MO_SUFFIX_, mo_name)

#define ATOMIC_INTERLOCKED_NAME_MO(base_name, lg_size, mo_name)                                                        \
	ATOMIC_CONCAT(ATOMIC_INTERLOCKED_NAME(base_name, lg_size), ATOMIC_INTERLOCKED_MO_SUFFIX(mo_name))

#define ATOMIC_MO_CALL(base_name, lg_size, mo, ...)                                                                    \
	((mo) == atomic_memory_order_relaxed   ? ATOMIC_INTERLOCKED_NAME_MO(base_name, lg_size, relaxed)(__VA_ARGS__)      \
	 : (mo) == atomic_memory_order_acquire ? ATOMIC_INTERLOCKED_NAME_MO(base_name, lg_size, acquire)(__VA_ARGS__)      \
	 : (mo) == atomic_memory_order_release ? ATOMIC_INTERLOCKED_NAME_MO(base_name, lg_size, release)(__VA_ARGS__)      \
	                                       : ATOMIC_INTERLOCKED_NAME_MO(base_name, lg_size, acq_rel)(__VA_ARGS__))

/*
 * ARM64 load-acquire / store-release using LDAR/STLR instructions.
 * These are cheaper than a plain load/store followed by a separate DMB.
 *
 * clang-cl doesn't expose __ldar* / __stlr* in its intrin.h, so we provide
 * them via inline asm. MSVC exposes them directly.
 *
 * ATOMIC_ARM64_LDAR(lg_size, p) / ATOMIC_ARM64_STLR(lg_size, p, v) select
 * the right width variant by lg_size via token-paste.
 */
#if defined(__clang__)
JEMALLOC_ALWAYS_INLINE unsigned __int8 atomic_arm64_ldar8(unsigned __int8 volatile *p)
{
	unsigned __int8 x;
	__asm__ volatile("ldarb %w0, [%1]" : "=r"(x) : "r"(p) : "memory");
	return x;
}
JEMALLOC_ALWAYS_INLINE unsigned __int16 atomic_arm64_ldar16(unsigned __int16 volatile *p)
{
	unsigned __int16 x;
	__asm__ volatile("ldarh %w0, [%1]" : "=r"(x) : "r"(p) : "memory");
	return x;
}
JEMALLOC_ALWAYS_INLINE unsigned __int32 atomic_arm64_ldar32(unsigned __int32 volatile *p)
{
	unsigned __int32 x;
	__asm__ volatile("ldar %w0, [%1]" : "=r"(x) : "r"(p) : "memory");
	return x;
}
JEMALLOC_ALWAYS_INLINE unsigned __int64 atomic_arm64_ldar64(unsigned __int64 volatile *p)
{
	unsigned __int64 x;
	__asm__ volatile("ldar %0, [%1]" : "=r"(x) : "r"(p) : "memory");
	return x;
}
JEMALLOC_ALWAYS_INLINE void atomic_arm64_stlr8(unsigned __int8 volatile *p, unsigned __int8 v)
{
	__asm__ volatile("stlrb %w1, [%0]" ::"r"(p), "r"((unsigned __int32)v) : "memory");
}
JEMALLOC_ALWAYS_INLINE void atomic_arm64_stlr16(unsigned __int16 volatile *p, unsigned __int16 v)
{
	__asm__ volatile("stlrh %w1, [%0]" ::"r"(p), "r"((unsigned __int32)v) : "memory");
}
JEMALLOC_ALWAYS_INLINE void atomic_arm64_stlr32(unsigned __int32 volatile *p, unsigned __int32 v)
{
	__asm__ volatile("stlr %w1, [%0]" ::"r"(p), "r"(v) : "memory");
}
JEMALLOC_ALWAYS_INLINE void atomic_arm64_stlr64(unsigned __int64 volatile *p, unsigned __int64 v)
{
	__asm__ volatile("stlr %1, [%0]" ::"r"(p), "r"(v) : "memory");
}
#else /* MSVC */
#define atomic_arm64_ldar8 __ldar8
#define atomic_arm64_ldar16 __ldar16
#define atomic_arm64_ldar32 __ldar32
#define atomic_arm64_ldar64 __ldar64
#define atomic_arm64_stlr8 __stlr8
#define atomic_arm64_stlr16 __stlr16
#define atomic_arm64_stlr32 __stlr32
#define atomic_arm64_stlr64 __stlr64
#endif /* __clang__ */

/* Width-indexed wrappers selected by lg_size via token-paste */
#define ATOMIC_ARM64_LDAR_0(p) atomic_arm64_ldar8((unsigned __int8 volatile *)(p))
#define ATOMIC_ARM64_LDAR_1(p) atomic_arm64_ldar16((unsigned __int16 volatile *)(p))
#define ATOMIC_ARM64_LDAR_2(p) atomic_arm64_ldar32((unsigned __int32 volatile *)(p))
#define ATOMIC_ARM64_LDAR_3(p) atomic_arm64_ldar64((unsigned __int64 volatile *)(p))
#define ATOMIC_ARM64_LDAR(lg_size, p) ATOMIC_CONCAT(ATOMIC_ARM64_LDAR_, lg_size)(p)

#define ATOMIC_ARM64_STLR_0(p, v) atomic_arm64_stlr8((unsigned __int8 volatile *)(p), (unsigned __int8)(v))
#define ATOMIC_ARM64_STLR_1(p, v) atomic_arm64_stlr16((unsigned __int16 volatile *)(p), (unsigned __int16)(v))
#define ATOMIC_ARM64_STLR_2(p, v) atomic_arm64_stlr32((unsigned __int32 volatile *)(p), (unsigned __int32)(v))
#define ATOMIC_ARM64_STLR_3(p, v) atomic_arm64_stlr64((unsigned __int64 volatile *)(p), (unsigned __int64)(v))
#define ATOMIC_ARM64_STLR(lg_size, p, v) ATOMIC_CONCAT(ATOMIC_ARM64_STLR_, lg_size)(p, v)

#else /* not ARM64 -- x86/x64 or ARM32, use plain Interlocked forms */

/*
 * On x86/x64 the hardware memory model makes all loads acquire and all stores
 * release, so the unsuffixed (full barrier) Interlocked forms are correct and
 * there is nothing to gain from MO-aware selection.  mo is consumed to
 * suppress unused-variable warnings.
 */
#define ATOMIC_MO_CALL(base_name, lg_size, mo, ...)                                                                    \
	((void)(mo), ATOMIC_INTERLOCKED_NAME(base_name, lg_size)(__VA_ARGS__))

#endif /* _M_ARM64 || _M_ARM64EC */

/*
 * On non-ARM64 targets the load/store paths use the original fence-based
 * approach since LDAR/STLR are ARM64-specific.
 */
#if !defined(_M_ARM64) && !defined(_M_ARM64EC)
#define ATOMIC_LOAD_ACQUIRE(lg_size, repr_ptr, ret_var)                                                                \
	do {                                                                                                               \
		(ret_var) = *(repr_ptr);                                                                                       \
		atomic_fence(atomic_memory_order_acquire);                                                                     \
	} while (0)
#define ATOMIC_STORE_RELEASE(lg_size, repr_ptr, val)                                                                   \
	do {                                                                                                               \
		atomic_fence(atomic_memory_order_release);                                                                     \
		*(repr_ptr) = (val);                                                                                           \
	} while (0)
#define ATOMIC_STORE_SEQ_CST(lg_size, repr_ptr, val)                                                                   \
	do {                                                                                                               \
		atomic_fence(atomic_memory_order_release);                                                                     \
		*(repr_ptr) = (val);                                                                                           \
		atomic_fence(atomic_memory_order_seq_cst);                                                                     \
	} while (0)
#else
#define ATOMIC_LOAD_ACQUIRE(lg_size, repr_ptr, ret_var)                                                                \
	do {                                                                                                               \
		_ReadWriteBarrier();                                                                                           \
		(ret_var) = (ATOMIC_INTERLOCKED_REPR(lg_size))ATOMIC_ARM64_LDAR(lg_size, (repr_ptr));                          \
	} while (0)
/* release and acq_rel: STLR provides store-release */
#define ATOMIC_STORE_RELEASE(lg_size, repr_ptr, val) ATOMIC_ARM64_STLR(lg_size, (repr_ptr), (val))
/* seq_cst: STLR + full barrier */
#define ATOMIC_STORE_SEQ_CST(lg_size, repr_ptr, val)                                                                   \
	do {                                                                                                               \
		ATOMIC_ARM64_STLR(lg_size, (repr_ptr), (val));                                                                 \
		MemoryBarrier();                                                                                               \
	} while (0)
#endif

#define JEMALLOC_GENERATE_ATOMICS(type, short_type, lg_size)                                                           \
	typedef struct                                                                                                     \
	{                                                                                                                  \
		ATOMIC_INTERLOCKED_REPR(lg_size) repr;                                                                         \
	} atomic_##short_type##_t;                                                                                         \
                                                                                                                       \
	ATOMIC_INLINE type atomic_load_##short_type(const atomic_##short_type##_t *a, atomic_memory_order_t mo)            \
	{                                                                                                                  \
		ATOMIC_INTERLOCKED_REPR(lg_size) ret;                                                                          \
		if (mo == atomic_memory_order_relaxed) {                                                                       \
			ret = a->repr;                                                                                             \
		} else {                                                                                                       \
			ATOMIC_LOAD_ACQUIRE(lg_size, &a->repr, ret);                                                               \
		}                                                                                                              \
		return (type)ret;                                                                                              \
	}                                                                                                                  \
                                                                                                                       \
	ATOMIC_INLINE void atomic_store_##short_type(atomic_##short_type##_t *a, type val, atomic_memory_order_t mo)       \
	{                                                                                                                  \
		ATOMIC_INTERLOCKED_REPR(lg_size) v = (ATOMIC_INTERLOCKED_REPR(lg_size))val;                                    \
		if (mo == atomic_memory_order_relaxed) {                                                                       \
			a->repr = v;                                                                                               \
		} else if (mo == atomic_memory_order_seq_cst) {                                                                \
			ATOMIC_STORE_SEQ_CST(lg_size, &a->repr, v);                                                                \
		} else {                                                                                                       \
			ATOMIC_STORE_RELEASE(lg_size, &a->repr, v);                                                                \
		}                                                                                                              \
	}                                                                                                                  \
                                                                                                                       \
	ATOMIC_INLINE type atomic_exchange_##short_type(atomic_##short_type##_t *a, type val, atomic_memory_order_t mo)    \
	{                                                                                                                  \
		return (type)ATOMIC_MO_CALL(                                                                                   \
			_InterlockedExchange, lg_size, mo, &a->repr, (ATOMIC_INTERLOCKED_REPR(lg_size))val);                       \
	}                                                                                                                  \
                                                                                                                       \
	ATOMIC_INLINE bool atomic_compare_exchange_weak_##short_type(atomic_##short_type##_t *a,                           \
	                                                             type *expected,                                       \
	                                                             type desired,                                         \
	                                                             atomic_memory_order_t success_mo,                     \
	                                                             atomic_memory_order_t failure_mo)                     \
	{                                                                                                                  \
		ATOMIC_INTERLOCKED_REPR(lg_size)                                                                               \
		e = (ATOMIC_INTERLOCKED_REPR(lg_size)) * expected;                                                             \
		ATOMIC_INTERLOCKED_REPR(lg_size)                                                                               \
		d = (ATOMIC_INTERLOCKED_REPR(lg_size))desired;                                                                 \
		ATOMIC_INTERLOCKED_REPR(lg_size)                                                                               \
		old = ATOMIC_MO_CALL(_InterlockedCompareExchange, lg_size, success_mo, &a->repr, d, e);                        \
		if (old == e) {                                                                                                \
			return true;                                                                                               \
		} else {                                                                                                       \
			*expected = (type)old;                                                                                     \
			return false;                                                                                              \
		}                                                                                                              \
	}                                                                                                                  \
                                                                                                                       \
	ATOMIC_INLINE bool atomic_compare_exchange_strong_##short_type(atomic_##short_type##_t *a,                         \
	                                                               type *expected,                                     \
	                                                               type desired,                                       \
	                                                               atomic_memory_order_t success_mo,                   \
	                                                               atomic_memory_order_t failure_mo)                   \
	{                                                                                                                  \
		/* We implement the weak version with strong semantics. */                                                     \
		return atomic_compare_exchange_weak_##short_type(a, expected, desired, success_mo, failure_mo);                \
	}

#define JEMALLOC_GENERATE_INT_ATOMICS(type, short_type, lg_size)                                                       \
	JEMALLOC_GENERATE_ATOMICS(type, short_type, lg_size)                                                               \
                                                                                                                       \
	ATOMIC_INLINE type atomic_fetch_add_##short_type(atomic_##short_type##_t *a, type val, atomic_memory_order_t mo)   \
	{                                                                                                                  \
		return (type)ATOMIC_MO_CALL(                                                                                   \
			_InterlockedExchangeAdd, lg_size, mo, &a->repr, (ATOMIC_INTERLOCKED_REPR(lg_size))val);                    \
	}                                                                                                                  \
                                                                                                                       \
	ATOMIC_INLINE type atomic_fetch_sub_##short_type(atomic_##short_type##_t *a, type val, atomic_memory_order_t mo)   \
	{                                                                                                                  \
		/*                                                                                                             \
		 * MSVC warns on negation of unsigned operands, but for us it                                                  \
		 * gives exactly the right semantics (MAX_TYPE + 1 - operand).                                                 \
		 */                                                                                                            \
		__pragma(warning(push)) __pragma(warning(disable : 4146)) return atomic_fetch_add_##short_type(a, -val, mo);   \
		__pragma(warning(pop))                                                                                         \
	}                                                                                                                  \
	ATOMIC_INLINE type atomic_fetch_and_##short_type(atomic_##short_type##_t *a, type val, atomic_memory_order_t mo)   \
	{                                                                                                                  \
		return (type)ATOMIC_MO_CALL(_InterlockedAnd, lg_size, mo, &a->repr, (ATOMIC_INTERLOCKED_REPR(lg_size))val);    \
	}                                                                                                                  \
	ATOMIC_INLINE type atomic_fetch_or_##short_type(atomic_##short_type##_t *a, type val, atomic_memory_order_t mo)    \
	{                                                                                                                  \
		return (type)ATOMIC_MO_CALL(_InterlockedOr, lg_size, mo, &a->repr, (ATOMIC_INTERLOCKED_REPR(lg_size))val);     \
	}                                                                                                                  \
	ATOMIC_INLINE type atomic_fetch_xor_##short_type(atomic_##short_type##_t *a, type val, atomic_memory_order_t mo)   \
	{                                                                                                                  \
		return (type)ATOMIC_MO_CALL(_InterlockedXor, lg_size, mo, &a->repr, (ATOMIC_INTERLOCKED_REPR(lg_size))val);    \
	}

#undef ATOMIC_INLINE

#endif /* JEMALLOC_INTERNAL_ATOMIC_MSVC_H */
