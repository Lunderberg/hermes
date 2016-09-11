#ifndef _UNPACKEDMESSAGE_H_
#define _UNPACKEDMESSAGE_H_

namespace hermes {
  template<typename T>
  class UnpackedMessageHolder;

  class UnpackedMessage {
  public:
    virtual ~UnpackedMessage() { }

    template<typename T>
    T* view() {
      if(auto m = dynamic_cast<UnpackedMessageHolder<T>*>(this)) {
        return m->t.get();
      } else {
        return nullptr;
      }
    }

    template<typename T>
    std::unique_ptr<T> claim() {
      if(auto m = dynamic_cast<UnpackedMessageHolder<T>*>(this)) {
        return std::move(m->t);
      } else {
        return nullptr;
      }
    }
  };

  template<typename T>
  class UnpackedMessageHolder : public UnpackedMessage {
  public:
    UnpackedMessageHolder(std::unique_ptr<T> t)
      : t(std::move(t)) { }

    std::unique_ptr<T> t;
  };
}

#endif /* _UNPACKEDMESSAGE_H_ */
