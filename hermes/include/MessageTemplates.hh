#ifndef _MESSAGETEMPLATES_H_
#define _MESSAGETEMPLATES_H_

#include <algorithm>
#include <cassert>
#include <map>

#include "BoostBinaryUnpacker.hh"
#include "BoostTextUnpacker.hh"
#include "Message.hh"
#include "MessageUnpacker.hh"
#include "PlainOldDataUnpacker.hh"
#include "PackingMethod.hh"

namespace hermes {
  class MessageTemplates {
    template<typename T>
    struct VoidPTypeChecker {
      static char x;

      static void* get() {
        return &x;
      }
    };

    template<typename T, PackingMethod Method>
    struct unpacker_gen;

  public:
    MessageTemplates()
      : highest_id(0) { }

    template<typename T, PackingMethod Method>
    void define() {
      id_type next_id = highest_id;
      while(m_templates_by_id.count(next_id)) {
        next_id++;
        // Make sure that we don't infinite-loop,
        //   if all 16-bit ids have been used.
        assert(next_id != highest_id);
      }

      define<T,Method>(next_id);
    }

    template<typename T,PackingMethod Method>
    void define(id_type id) {
      auto voidp = VoidPTypeChecker<T>::get();
      m_templates_by_class[voidp] = unpacker_gen<T,Method>::construct(id);
      m_templates_by_id[id] = unpacker_gen<T,Method>::construct(id);

      highest_id = std::max(highest_id, id);
    }

    const MessageUnpacker& get_by_id(id_type id) const {
      // TODO: Throw custom exception, rather than std::out_of_range if not defined
      return *m_templates_by_id.at(id);
    }

    template<typename T>
    const MessageUnpacker& get_by_class() const {
      auto voidp = VoidPTypeChecker<T>::get();
      return *m_templates_by_class.at(voidp);
    }

  private:

    template<typename T>
    struct unpacker_gen<T, PackingMethod::PlainOldData> {
      static std::unique_ptr<MessageUnpacker> construct(id_type id) {
        return make_unique<PlainOldDataUnpacker<T> >(id);
      }
    };

    template<typename T>
    struct unpacker_gen<T, PackingMethod::BoostBinaryArchive> {
      static std::unique_ptr<MessageUnpacker> construct(id_type id) {
        return make_unique<BoostBinaryUnpacker<T> >(id);
      }
    };

    template<typename T>
    struct unpacker_gen<T, PackingMethod::BoostTextArchive> {
      static std::unique_ptr<MessageUnpacker> construct(id_type id) {
        return make_unique<BoostTextUnpacker<T> >(id);
      }
    };




    std::map<id_type, std::unique_ptr<MessageUnpacker> > m_templates_by_id;
    std::map<void*, std::unique_ptr<MessageUnpacker> > m_templates_by_class;
    id_type highest_id;
  };

  template<typename T>
  char MessageTemplates::VoidPTypeChecker<T>::x;
}

#endif /* _MESSAGETEMPLATES_H_ */
