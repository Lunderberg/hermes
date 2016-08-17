#ifndef _MESSAGETEMPLATES_H_
#define _MESSAGETEMPLATES_H_

#include "Message.hh"

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
    MessageTemplates() { }

    template<typename T>
    void define(id_type id) {
      m_templates_by_id[id] = make_unique<MessageType<T> >(id);
      auto voidp = VoidPTypeChecker<T>::get();
      m_templates_by_class[voidp] = make_unique<MessageType<T> >(id);
    }

    std::unique_ptr<Message> create_by_id(id_type id) const {
      // TODO: Throw custom exception, rather than std::out_of_range if not defined
      auto& message_template = m_templates_by_id.at(id);
      return message_template->create();
    }

    template<typename T>
    std::unique_ptr<MessageType<T> > create_by_class() const {
      auto voidp = VoidPTypeChecker<T>::get();
      auto& message_template = m_templates_by_class.at(voidp);
      auto msg = message_template->create();
      return std::unique_ptr<MessageType<T> >(
        static_cast<MessageType<T>*>(msg.release())
      );
    }

  private:
    std::map<id_type, std::unique_ptr<Message> > m_templates_by_id;
    std::map<void*, std::unique_ptr<Message> > m_templates_by_class;
  };

  template<typename T>
  char MessageTemplates::VoidPTypeChecker<T>::x;
}

#endif /* _MESSAGETEMPLATES_H_ */
