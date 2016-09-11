#ifndef _TEMPLATEDBOOL_H_
#define _TEMPLATEDBOOL_H_

namespace hermes {
  template<typename T>
  struct TemplatedBool {
    enum Value { False = 0, True = 1 };
  };
}

#endif /* _TEMPLATEDBOOL_H_ */
