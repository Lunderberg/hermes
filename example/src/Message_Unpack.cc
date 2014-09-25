#include "Message.hh"

#include "RawTextMessage.hh"
#include "IntegerMessage.hh"

NETWORK_UNPACK( (RawTextMessage)(IntegerMessage) )
