#ifndef _MESSAGECALLBACK_H_
#define _MESSAGECALLBACK_H_

#include <functional>

#include "UnpackedMessage.hh"

namespace hermes {
  class MessageCallback {
  public:
    virtual ~MessageCallback() { }
    virtual bool apply_on(UnpackedMessage& message) = 0;
  };

  template<typename T>
  class MessageCallbackType : public MessageCallback {
  public:
    MessageCallbackType(std::function<void(std::unique_ptr<T>)> func)
      : func(func) { }

    bool apply_on(UnpackedMessage& message) {
      auto obj = message.claim<T>();
      if(obj) {
        // Message is of correct type, use it
        func(std::move(obj));
        return true;
      } else {
        // Message is of some other type, don't bother.
        return false;
      }
    }

  private:
    std::function<void(std::unique_ptr<T>)> func;
  };
}

#endif /* _MESSAGECALLBACK_H_ */
