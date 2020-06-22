CXX = gcc 
CXXFLAGS = -I../../ -I./

COMPILE.c = $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c

all:kcp_client kcp_server

kcp_client:kcpClient.o 
	gcc -o $@ kcpClient.c ../../ikcp.c -I../../

kcp_server:kcpServer.o 
	gcc -o $@ kcpServer.c ../../ikcp.c -I../../

%.o:%.c
	$(COMPILE.c) $< -o $@

clean:
	rm kcpServer.o kcpClient.o kcp_client kcp_server

.PHONY : all clean rebuild
