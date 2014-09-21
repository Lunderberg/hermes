#ifndef _CLIENTNETWORKSOCKET_H_
#define _CLIENTNETWORKSOCKET_H_

#include "NetworkSocket.hh"

class ClientNetworkSocket : public NetworkSocket{
public:
	ClientNetworkSocket(std::shared_ptr<boost::asio::io_service> io_service,
											boost::asio::ip::tcp::resolver::iterator endpoint);
};

#endif /* _CLIENTNETWORKSOCKET_H_ */
