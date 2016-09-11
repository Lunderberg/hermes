#ifndef _BOOSTBINARYUNPACKER_H_
#define _BOOSTBINARYUNPACKER_H_

#include "MessageUnpacker.hh"

#ifdef HERMES_ENABLE_BOOST_SERIALIZE

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

namespace hermes {
  template<typename T>
  class BoostBinaryUnpacker :  public MessageUnpacker {
  public:
    BoostBinaryUnpacker(id_type id)
      : MessageUnpacker(id) { }

    std::unique_ptr<UnpackedMessage> unpack(const std::string& packed) const {
      auto obj = make_unique<T>();
      std::stringstream ss(packed);
      {
        boost::archive::binary_iarchive iarchive(ss);
        iarchive >> *obj;
      }
      return make_unique<UnpackedMessageHolder<T> >(std::move(obj));
    }

    std::string pack(const void* voidp) const {
      auto obj = static_cast<const T*>(voidp);

      std::stringstream ss;
      {
        boost::archive::binary_oarchive oarchive(ss);
        oarchive << *obj;
      }
      return ss.str();
    }
  };
}

#else

#include "TemplatedBool.hh"

namespace hermes {
  template<typename T>
  class BoostBinaryUnpacker : public MessageUnpacker {
    static_assert(TemplatedBool<T>::False,
                  "To enable use of boost::serialize, -DHERMES_ENABLE_BOOST_SERIALIZE");
  };
}

#endif

#endif /* _BOOSTBINARYUNPACKER_H_ */
