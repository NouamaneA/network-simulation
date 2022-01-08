CC = g++

CFLAGS  = -l ns3.34-core-debug -l ns3.34-network-debug -l ns3.34-internet-debug -l ns3.34-point-to-point-debug -l ns3.34-applications-debug -l ns3.34-traffic-control-debug -l ns3.34-flow-monitor-debug

TARGET = simulation

all: $(TARGET)

$(TARGET): $(TARGET).cc
	$(CC) $(TARGET).cc $(CFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET) res*
