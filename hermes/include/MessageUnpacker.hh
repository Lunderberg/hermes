#ifndef _MESSAGEUNPACKER_H_
#define _MESSAGEUNPACKER_H_

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>

#include "MakeUnique.hh"
#include "UnpackedMessage.hh"

namespace hermes {
  template<typename T>
  class MessageUnpackerType;

  class MessageUnpacker {
  public:
    MessageUnpacker(id_type id)
      : m_id(id) { }
    virtual ~MessageUnpacker() { }

    virtual std::unique_ptr<UnpackedMessage> unpack(const std::string& packed) const = 0;
    virtual std::string pack(const void*) const = 0;

    id_type id() const { return m_id; }

  private:
    id_type m_id;
  };

  template<typename T>
  class PlainOldDataUnpacker : public MessageUnpacker {
    static_assert(std::is_pod<T>::value,
                  "PlainOldDataUnpacker requires type to be plain-old-data");
  public:
    PlainOldDataUnpacker(id_type id)
      : MessageUnpacker(id) { }

    std::unique_ptr<UnpackedMessage> unpack(const std::string& packed) const {
      assert(packed.size() == sizeof(T));

      auto obj = make_unique<T>();
      memcpy(&*obj, packed.data(), std::min(packed.size(),sizeof(T)));
      return make_unique<UnpackedMessageHolder<T> >(std::move(obj));
    }

    std::string pack(const void* voidp) const {
      auto obj = static_cast<const T*>(voidp);

      std::string output;
      output.resize(sizeof(T));
      memcpy(&output[0], obj, sizeof(T));
      return output;
    }
  };
}

#endif /* _MESSAGEUNPACKER_H_ */
