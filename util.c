#include<stdio.h>
#include"util.h"

void setupTcpConnection(ProtocolDesc* pd,
                        const char* remote_host,
                        const char* port)
{
  if(remote_host==NULL)
  { if(protocolAcceptTcp2P(pd,port)!=0)
    { fprintf(stderr,"TCP accept failed\n");
      exit(-1);
    }
  }
  else 
    if(protocolConnectTcp2P(pd,remote_host,port)!=0) 
    { fprintf(stderr,"TCP connect failed\n");
      exit(-1);
    }
}

