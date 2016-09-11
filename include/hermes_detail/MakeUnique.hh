#ifndef _MAKEUNIQUE_H_
#define _MAKEUNIQUE_H_

#include <memory>

namespace hermes {
  template<typename T, typename... Args>
  std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
  }
}

#endif /* _MAKEUNIQUE_H_ */
