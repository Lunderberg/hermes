#ifndef _BOOSTTEXTUNPACKER_H_
#define _BOOSTTEXTUNPACKER_H_

#include "MessageUnpacker.hh"

#ifdef HERMES_ENABLE_BOOST_SERIALIZE

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

namespace hermes {
  template<typename T>
  class BoostTextUnpacker :  public MessageUnpacker {
  public:
    BoostTextUnpacker(id_type id)
      : MessageUnpacker(id) { }

    std::unique_ptr<UnpackedMessage> unpack(const std::string& packed) const {
      auto obj = make_unique<T>();
      std::stringstream ss(packed);
      {
        boost::archive::text_iarchive iarchive(ss);
        iarchive >> *obj;
      }
      return make_unique<UnpackedMessageHolder<T> >(std::move(obj));
    }

    std::string pack(const void* voidp) const {
      auto obj = static_cast<const T*>(voidp);

      std::stringstream ss;
      {
        boost::archive::text_oarchive oarchive(ss);
        oarchive << *obj;
      }
      return ss.str();
    }
  };
}

#else

namespace hermes {
  template<typename T>
  class BoostTextUnpacker : public MessageUnpacker {
    static_assert(false, "To enable use of boost::serialize, -DHERMES_ENABLE_BOOST_SERIALIZE");
  };
}

#endif

#endif /* _BOOSTTEXTUNPACKER_H_ */
