#ifndef _MESSAGEUNPACKER_H_
#define _MESSAGEUNPACKER_H_

#include <memory>
#include <string>

#include "UnpackedMessage.hh"
#include "Message.hh"

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
}

#endif /* _MESSAGEUNPACKER_H_ */
