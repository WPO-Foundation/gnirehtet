// Copyright 2013 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance  with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// Author: szym@google.com (Szymon Jakubczak)

// Host-side runner forwards traffic between adb-forwarded TCP socket and the
// local tun interface.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>

#include <linux/if.h>
#include <linux/if_tun.h>

#include "forwarder.h"

int get_interface(char* name) {
  int interface = open("/dev/net/tun", O_RDWR | O_NONBLOCK);

  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));

  if (ioctl(interface, TUNSETIFF, &ifr)) {
    perror("Cannot get TUN interface");
    return -1;
  }

  return interface;
}

int get_tunnel(int port) {
  int tunnel = -1;
  int listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  if(bind(listen_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("Binding for tunnel\n");
    return -1;
  }
  if(listen(listen_socket, 5) < 0) {
    perror("Listen for tunnel\n");
    return -1;
  }
  puts("Listening for tunnel");

  tunnel = accept(listen_socket, NULL, NULL);
  puts("Tunnel connected\n");

  return tunnel;
}

//-----------------------------------------------------------------------------

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("Usage: %s <tunN> <port>\n", argv[0]);
    return 2;
  }

  int interface = get_interface(argv[1]);
  if (interface < 0)
    return 1;

  int tunnel = -1;
  int port = atoi(argv[2]);
  if (port) {
    tunnel = get_tunnel(port);
  }
  if (tunnel < 0)
    return 1;

  puts("Connected.");
  puts("Forwarding. Press Enter to stop.");

  // Keep forwarding packets until something goes wrong.
  forward(STDIN_FILENO, tunnel, interface);

  perror("Finished");
  return 1;
}
