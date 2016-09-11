#ifndef _INTEGERMESSAGE_H_
#define _INTEGERMESSAGE_H_

struct IntegerMessage{
  template<typename Archiver>
  void serialize(Archiver& ar, unsigned int /*version*/) {
    ar & value;
  }

  int value;
};

#endif /* _INTEGERMESSAGE_H_ */
