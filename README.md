# RF24Pi nanomsg Network
A NRF24L01+ sensor network relay built with [nanomsg](http://nanomsg.org/) and [RF24Network](https://tmrh20.github.io/RF24Network/).

- For relaying messages to and from a NRF24L01+ sensor network using [RF24Network](https://tmrh20.github.io/RF24Network/)
- Utilizes JSON for message serialization

## Build
`make`

## Install
`make install`

## Tips
- Use JSON arrays to save space sending messages to microcontrollers
- Limited to [24bytes](https://tmrh20.github.io/RF24Network/structRF24NetworkFrame.html) for message payloads
    - Have not tested fragmented payloads

## Links / References
- [RF24](https://tmrh20.github.io/RF24/)
- [RF24Network](https://tmrh20.github.io/RF24/)
- [jsoncpp](https://github.com/open-source-parsers/jsoncpp)
- [nanomsg](http://nanomsg.org/)
- [Elixir to C++ Messaging](http://wisol.ch/w/articles/2015-06-19-elixir-to-cpp-messaging/)