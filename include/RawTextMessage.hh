#ifndef _RAWTEXTMESSAGE_H_
#define _RAWTEXTMESSAGE_H_

struct RawTextMessage {
  template<typename Archiver>
  void serialize(Archiver& ar, unsigned int /*version*/) {
    ar & buf;
  }

  char buf[80];
};

#endif /* _RAWTEXTMESSAGE_H_ */
