#ifndef YASR_RAII_HPP
#define YASR_RAII_HPP

#include "yasr.hpp"

namespace yasr {

template <typename Resource, void (Device::*deleter)(Resource)>
struct UniqueResource {
  Device* device_ = nullptr;
  Resource resource_ = {};

  UniqueResource(Device& device, Resource resource)
      : device_{&device}, resource_{resource}
  {
  }

  ~UniqueResource() noexcept
  {
    if (device_) {
      (device_->*deleter)(resource_);
    }
  }

  UniqueResource(const UniqueResource&) = delete;
  auto operator=(const UniqueResource&) & -> UniqueResource& = delete;
  UniqueResource(UniqueResource&& other) noexcept
      : device_{std::exchange(other.device_, nullptr)}, resource_{std::exchange(
                                                            other.resource_,
                                                            {})}
  {
  }
  auto operator=(UniqueResource&& other) & noexcept -> UniqueResource&
  {
    device_ = std::exchange(other.device_, nullptr);
    resource_ = std::exchange(other.resource_, {});
    return *this;
  }

  explicit(false) operator Resource()
  {
    return resource_;
  }
};

struct UniqueBuffer : UniqueResource<Buffer, &Device::destroy_buffer> {
  using UniqueResource::UniqueResource;
};

[[nodiscard]] inline auto create_unique_buffer(Device& device,
                                               const BufferDesc& desc)
    -> UniqueBuffer
{
  return UniqueBuffer{device, device.create_buffer(desc)};
}

} // namespace yasr

#endif // YASR_RAII_HPP
