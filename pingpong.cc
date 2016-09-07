#include "NetworkIO.hh"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

template<int N>
struct SizedMessage {
  char buf[N];

  void fill_rand() {
    for(int i=0; i<N; i++) {
      buf[i] = rand()%256;
    }
  }
};

using std::chrono::steady_clock;

steady_clock::time_point now() {
  return steady_clock::now();
}


template<int N>
void write_msg(hermes::NetworkSocket& socket, const SizedMessage<N>& msg) {
  socket.write(msg);
}

void read_msg(hermes::NetworkSocket& socket) {
  auto message = socket.WaitForMessage();
}

template<int N>
void test_with(hermes::NetworkSocket& socket, bool send_first) {
  auto msg = make_unique<SizedMessage<N> >();
  msg->fill_rand();

  auto before = now();

  if(send_first) {
    write_msg(socket, *msg);
    read_msg(socket);
  } else {
    read_msg(socket);
    write_msg(socket, *msg);
  }

  auto after = now();

  double seconds = std::chrono::duration<double>(after-before).count();

  std::cout << "Size: " << std::setw(8) << N << " bytes"
            << "\ttime: " << std::setw(8) << std::chrono::duration_cast<std::chrono::microseconds>(after-before).count() << " us"
            << "\trate (MB/s): " << 2*N/seconds / (1024*1024)
            << std::endl;
}

void start_client(std::string exe_name) {
  std::string command = exe_name + " client > /dev/null";
  std::cout << "Running " << command << std::endl;
  popen(command.c_str(),"r");
}

int main(int argc, char** argv) {
  bool send_first = (argc == 1);
  bool is_server = (argc == 1);


  hermes::NetworkIO network;
  network.message_type<SizedMessage<1> >();
  network.message_type<SizedMessage<2> >();
  network.message_type<SizedMessage<4> >();
  network.message_type<SizedMessage<8> >();
  network.message_type<SizedMessage<16> >();
  network.message_type<SizedMessage<32> >();
  network.message_type<SizedMessage<64> >();
  network.message_type<SizedMessage<128> >();
  network.message_type<SizedMessage<256> >();
  network.message_type<SizedMessage<512> >();
  network.message_type<SizedMessage<1024> >();
  network.message_type<SizedMessage<2048> >();
  network.message_type<SizedMessage<4096> >();
  network.message_type<SizedMessage<8192> >();
  network.message_type<SizedMessage<16384> >();
  network.message_type<SizedMessage<32768> >();
  network.message_type<SizedMessage<65536> >();
  network.message_type<SizedMessage<131072> >();
  network.message_type<SizedMessage<262144> >();
  network.message_type<SizedMessage<524288> >();
  network.message_type<SizedMessage<1048576> >();
  network.message_type<SizedMessage<2097152> >();
  network.message_type<SizedMessage<4194304> >();
  network.message_type<SizedMessage<8388608> >();
  network.message_type<SizedMessage<16777216> >();

  std::cout << "Stuff defined" << std::endl;

  std::unique_ptr<hermes::NetworkSocket> conn = nullptr;
  if(is_server) {
    std::cout << "opening acceptor" << std::endl;
    auto listener = network.listen(12346);
    std::cout << "acceptor opened" << std::endl;
    start_client(argv[0]);

    conn = listener->WaitForConnection();
    std::cout << "connection accepted" << std::endl;
  } else {
    std::cout << "connecting to 12345" << std::endl;
    conn = network.connect("localhost",12346);
    std::cout << "connection made" << std::endl;
  }


  test_with<1>(*conn, send_first);
  test_with<2>(*conn, send_first);
  test_with<4>(*conn, send_first);
  test_with<8>(*conn, send_first);
  test_with<16>(*conn, send_first);
  test_with<32>(*conn, send_first);
  test_with<64>(*conn, send_first);
  test_with<128>(*conn, send_first);
  test_with<256>(*conn, send_first);
  test_with<512>(*conn, send_first);
  test_with<1024>(*conn, send_first);
  test_with<2048>(*conn, send_first);
  test_with<4096>(*conn, send_first);
  test_with<8192>(*conn, send_first);
  test_with<16384>(*conn, send_first);
  test_with<32768>(*conn, send_first);
  test_with<65536>(*conn, send_first);
  test_with<131072>(*conn, send_first);
  test_with<262144>(*conn, send_first);
  test_with<524288>(*conn, send_first);
  test_with<1048576>(*conn, send_first);
  test_with<2097152>(*conn, send_first);
  test_with<4194304>(*conn, send_first);
  test_with<8388608>(*conn, send_first);
  test_with<16777216>(*conn, send_first);
}
