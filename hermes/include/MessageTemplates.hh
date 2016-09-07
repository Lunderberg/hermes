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

namespace hermes {
  class MessageTemplates {
    template<typename T>
    struct VoidPTypeChecker {
      static char x;

      static void* get() {
        return &x;
      }
    };

  public:
    MessageTemplates()
      : highest_id(0) { }

    template<typename T>
    void define() {
      id_type next_id = highest_id;
      while(m_templates_by_id.count(next_id)) {
        next_id++;
        // Make sure that we don't infinite-loop,
        //   if all 16-bit ids have been used.
        assert(next_id != highest_id);
      }

      define<T>(next_id);
    }

    template<typename T>
    void define(id_type id) {
      m_templates_by_id[id] = make_unique<PlainOldDataUnpacker<T> >(id);
      auto voidp = VoidPTypeChecker<T>::get();
      m_templates_by_class[voidp] = make_unique<PlainOldDataUnpacker<T> >(id);

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
    std::map<id_type, std::unique_ptr<MessageUnpacker> > m_templates_by_id;
    std::map<void*, std::unique_ptr<MessageUnpacker> > m_templates_by_class;
    id_type highest_id;
  };

  template<typename T>
  char MessageTemplates::VoidPTypeChecker<T>::x;
}

#endif /* _MESSAGETEMPLATES_H_ */
