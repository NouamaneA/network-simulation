# Network Simulation

This project uses ns-3 to simulate a network in order to compare the performance of it with CBR (Constant Bit Rate) or ON/OFF traffic.

Prerequisites:

- g++
- [ns-3](https://www.nsnam.org/wiki/Installation) (the version I use is 3.34)

To compile the code, run: (the commands are given for Linux)

```sh
make
```

or

```sh
g++ simulation.cc -l ns3.34-core-debug -l ns3.34-network-debug -l ns3.34-internet-debug -l ns3.34-point-to-point-debug -l ns3.34-applications-debug -l ns3.34-traffic-control-debug -l ns3.34-flow-monitor-debug -o simulation
```

Finally, to run a simulation plan, you can run the [simplan](/simplan) script as follows:

```sh
chmod +x ./simplan
./simplan
```

To compare CBR traffic and ON/OFF traffic, change the values of simDuration and percentCBR. Then, check the text files for the results.

You can also find some of the results I had in the [plot](/plot) directory.