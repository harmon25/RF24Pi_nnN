# RF24Pi nanomsg Network
A NRF24L01+ sensor network relay built with [nanomsgxx](https://github.com/achille-roussel/nanomsgxx/) and [RF24Network](https://tmrh20.github.io/RF24Network/) in C++.

- For relaying messages to and from a NRF24L01+ sensor network using [RF24Network](https://tmrh20.github.io/RF24Network/)
- Utilizes JSON for message serialization

## Build
`make`

## Install
`make install`

## Tips
- Limited to [24bytes](https://tmrh20.github.io/RF24Network/structRF24NetworkFrame.html) for message payloads
    - Have not tested fragmented payloads


## Links / References
- [RF24](https://tmrh20.github.io/RF24/)
- [RF24Network](https://tmrh20.github.io/RF24/)
- [RapidJSON](https://github.com/miloyip/rapidjson)
- [nanomsgxx](https://achille-roussel.github.io/nanomsgxx/doc/nanomsgxx.7.html)
- [nanomsg](http://nanomsg.org/)
- [rf24sensornet](https://bitbucket.org/pjhardy/rf24sensornet/)
- [Elixir to C++ Messaging](http://wisol.ch/w/articles/2015-06-19-elixir-to-cpp-messaging/)