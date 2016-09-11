#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <cstdint>
#include <string>
#include <memory>
#include <sstream>

#include "MakeUnique.hh"

namespace hermes {
  typedef std::uint16_t id_type;
  typedef std::uint32_t size_type;
  constexpr size_type max_message_size = UINT32_MAX;

  union network_header {
    struct packed_t {
      size_type size;
      id_type id;
      char acknowledge;
    };

    packed_t packed;
    char arr[sizeof(packed)];
  };
  constexpr size_t header_size = sizeof(network_header);

  struct Message {
    network_header header;
    std::string body;
  };

  // template<typename T>
  // class MessageType;

  // class Message {
  // public:
  //   virtual ~Message() { }

  //   virtual std::size_t size() const = 0;
  //   virtual std::uint16_t id() const = 0;
  //   virtual char* raw() = 0;
  //   virtual const char* raw() const = 0;
  //   virtual std::unique_ptr<Message> create() const = 0;

  //   template<typename T>
  //   operator T() {
  //     return static_cast<MessageType<T>*>(this)->unpacked();
  //   }
  // };

  // template<typename T>
  // union converter {
  //   T t;
  //   char packed[sizeof(T)];
  // };

  // template<typename T>
  // class MessageType : public Message {
  //   static_assert(std::is_pod<T>::value, "Message types must be plain old data (pod) types");

  // public:
  //   MessageType(std::uint16_t id)
  //     : id_(id) { }

  //   virtual std::size_t size() const {
  //     return sizeof(T);
  //   }

  //   virtual std::uint16_t id() const {
  //     return id_;
  //   }

  //   virtual char* raw() {
  //     return convert.packed;
  //   }

  //   virtual const char* raw() const {
  //     return convert.packed;
  //   }

  //   virtual std::unique_ptr<Message> create() const {
  //     return make_unique<MessageType>(id_);
  //   }

  //   T& unpacked() {
  //     return convert.t;
  //   }

  // private:
  //   converter<T> convert;
  //   id_type id_;
  // };

}

#endif /* _MESSAGE_H_ */
