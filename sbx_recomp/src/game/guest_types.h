// Typed views of guest (Xbox 360) memory for the native game code.
//
// The game's objects stay in guest memory while any recompiled function
// still reads them: big-endian fields, 32-bit guest pointers. The types here
// let our C++ use them as ordinary structs:
//
//   be<T>    a big-endian field (reads and writes swap the bytes)
//   ptr<T>   a 4-byte guest pointer field; -> and * give the host object
//   At<T>    the host object at a guest address (a register, a global)
//   AddressOf  a host object's guest address, to hand back to guest code
//   Guest    the calling thread's registers, to call recompiled functions
//
// Game structs (src/game/*.h) are laid out byte for byte as the game's, each
// with a static_assert on its size and on the offsets worked out so far.

#pragma once

#include "guest.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace guest {

// Where guest address 0 is in host memory. One block for the whole run; set
// by the first Guest made (every override makes one).
inline uint8_t* memory_base = nullptr;

inline uint8_t* Host(uint32_t address) {
  uint8_t* base = memory_base;
  return REX_RAW_ADDR(address);
}

// A big-endian field of guest memory. Reads as a T; a copy from another
// be<T> keeps the bits as they are.
template <typename T>
struct be {
  using Raw = std::conditional_t<sizeof(T) == 8, uint64_t,
                                 std::conditional_t<sizeof(T) == 4, uint32_t, uint16_t>>;
  static_assert(sizeof(T) == sizeof(Raw) && std::is_trivially_copyable_v<T>);

  be() = default;
  be(T value) : raw(std::byteswap(std::bit_cast<Raw>(value))) {}
  operator T() const { return std::bit_cast<T>(std::byteswap(raw)); }

  be& operator+=(T value) { return *this = T(*this) + value; }
  be& operator-=(T value) { return *this = T(*this) - value; }
  be& operator|=(T value) { return *this = T(*this) | value; }
  be& operator&=(T value) { return *this = T(*this) & value; }
  be& operator++() { return *this += 1; }
  be& operator--() { return *this -= 1; }

  Raw raw;
};

using u16_be = be<uint16_t>;
using i16_be = be<int16_t>;
using u32_be = be<uint32_t>;
using i32_be = be<int32_t>;
using f32_be = be<float>;

// The host object at a guest address; nullptr for address 0.
template <typename T>
T* At(uint32_t address) {
  return address != 0 ? reinterpret_cast<T*>(Host(address)) : nullptr;
}

// Element `index` of a table of T at a guest address.
template <typename T>
T* Element(uint32_t table, uint32_t index) {
  return At<T>(table + uint32_t(sizeof(T)) * index);
}

// The guest address of a host object in guest memory; 0 for nullptr.
inline uint32_t AddressOf(const void* object) {
  if (object == nullptr) {
    return 0;
  }
  const uint64_t offset = uint64_t(static_cast<const uint8_t*>(object) - memory_base);
  // Undo Host's step (guest memory from 0xE0000000 may sit 4 KB further on).
  const uint64_t step = uint64_t(Host(0xE0000000u) - memory_base) - 0xE0000000u;
  return uint32_t(offset >= 0xE0000000u + step ? offset - step : offset);
}

// A guest pointer field: 4 bytes, big-endian, used like a T*.
template <typename T>
struct ptr {
  ptr() = default;
  ptr(T* object) : address(AddressOf(object)) {}

  T* get() const { return At<T>(address); }
  T* operator->() const { return get(); }
  T& operator*() const { return *get(); }
  operator T*() const { return get(); }

  u32_be address;
};

// Fields the game's data files keep little-endian (the host's order).
using u32_le = uint32_t;

// The calling thread's guest registers, for calls into recompiled code.
// Arguments go in r3 onward as the game passes them: pointers as guest
// addresses, signed integers sign-extended. No other register is touched, so
// a callee's extra inputs (a float in f1, say) can be set on `ctx` first.
struct Guest {
  using Function = void(PPCContext&, uint8_t*);

  Guest(PPCContext& ctx, uint8_t* base) : ctx(ctx), base(base) {
    if (memory_base == nullptr) {
      memory_base = base;
    }
  }

  // Calls a recompiled function (or an override of one). Returns r3.
  template <typename... Args>
  uint32_t Call(Function* function, Args... args) {
    SetArguments(args...);
    function(ctx, base);
    return ctx.r3.u32;
  }

  // Calls the function at a guest address (a callback, a vtable entry).
  template <typename... Args>
  uint32_t CallAddress(uint32_t address, Args... args) {
    SetArguments(args...);
    REX_CALL_INDIRECT_FUNC(address);
    return ctx.r3.u32;
  }

  // Calls a virtual function of a guest object: r3 the object, then args.
  template <typename... Args>
  uint32_t CallVirtual(void* object, uint32_t slot, Args... args) {
    const u32_be* vtable = At<u32_be>(*static_cast<u32_be*>(object));
    return CallAddress(vtable[slot], object, args...);
  }

  template <typename... Args>
  void SetArguments(Args... args) {
    PPCRegister* registers[] = {&ctx.r3, &ctx.r4, &ctx.r5, &ctx.r6,
                                &ctx.r7, &ctx.r8, &ctx.r9, &ctx.r10};
    static_assert(sizeof...(Args) <= 8);
    [[maybe_unused]] size_t n = 0;
    ((registers[n++]->u64 = Word(args)), ...);
  }

  template <typename T>
  static uint64_t Word(T value) {
    if constexpr (std::is_pointer_v<T> || std::is_null_pointer_v<T>) {
      return AddressOf(value);
    } else if constexpr (std::is_signed_v<T>) {
      return uint64_t(int64_t(value));
    } else {
      return uint64_t(value);
    }
  }
  template <typename T>
  static uint64_t Word(const ptr<T>& value) {
    return value.address;
  }
  template <typename T>
  static uint64_t Word(const be<T>& value) {
    return Word(T(value));
  }

  PPCContext& ctx;
  uint8_t* base;
};

// While one lives, callees see `address` as where they return to. A few
// hooks tell their callers apart by it, so calls the original made from a
// known place keep that place.
class ReturnAddress {
 public:
  ReturnAddress(Guest& g, uint32_t address) : ctx_(g.ctx), saved_(g.ctx.lr) {
    ctx_.lr = address;
  }
  ~ReturnAddress() { ctx_.lr = saved_; }
  ReturnAddress(const ReturnAddress&) = delete;
  ReturnAddress& operator=(const ReturnAddress&) = delete;

 private:
  PPCContext& ctx_;
  uint64_t saved_;
};

}  // namespace guest

// Checks a guest struct's layout against the game's.
#define GUEST_SIZE(Type, bytes) static_assert(sizeof(Type) == (bytes), #Type " size")
#define GUEST_OFFSET(Type, field, offset) \
  static_assert(offsetof(Type, field) == (offset), #Type "::" #field " offset")
