///////////////////////////////////////////////////////////////////////
// File:        svutil.cpp
// Description: ScrollView Utilities
// Author:      Joern Wanke
// Created:     Thu Nov 29 2007
//
// (C) Copyright 2007, Google Inc.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
///////////////////////////////////////////////////////////////////////
//
// SVUtil contains the SVSync and SVNetwork classes, which are used for
// thread/process creation & synchronization and network connection.

#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

// Include automatically generated configuration file if running autoconf.
#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif

#ifndef GRAPHICS_DISABLED
#include "svutil.h"

const int kMaxMsgSize = 4096;

// Signals a thread to exit.
void SVSync::ExitThread() {
  pthread_exit(0);
}

// Starts a new process.
void SVSync::StartProcess(const char* executable, const char* args) {
  std::string proc;
  proc.append(executable);
  proc.append(" ");
  proc.append(args);
  std::cout << "Starting " << proc << std::endl;

  int pid = fork();
  if (pid != 0) {   // The father process returns
  } else {
    char* mutable_args = strdup(args);
    int argc = 1;
    for (int i = 0; mutable_args[i]; ++i) {
      if (mutable_args[i] == ' ') {
        ++argc;
      }
    }
    char** argv = new char*[argc + 2];
    argv[0] = strdup(executable);
    argv[1] = mutable_args;
    argc = 2;
    bool inquote = false;
    for (int i = 0; mutable_args[i]; ++i) {
      if (!inquote && mutable_args[i] == ' ') {
        mutable_args[i] = '\0';
        argv[argc++] = mutable_args + i + 1;
      } else if (mutable_args[i] == '"') {
        inquote = !inquote;
        mutable_args[i] = ' ';
      }
    }
    argv[argc] = NULL;
    execvp(executable, argv);
  }
}

SVSemaphore::SVSemaphore() {
  char name[50];
  snprintf(name, sizeof(name), "%ld", random());
  sem_unlink(name);
  semaphore_ = sem_open(name, O_CREAT , S_IWUSR, 0);
  if (semaphore_ == SEM_FAILED) {
    perror("sem_open");
  }
}

void SVSemaphore::Signal() {
  sem_post(semaphore_);
}

void SVSemaphore::Wait() {
  sem_wait(semaphore_);
}

SVMutex::SVMutex() {
  pthread_mutex_init(&mutex_, NULL);
}

void SVMutex::Lock() {
  pthread_mutex_lock(&mutex_);
}

void SVMutex::Unlock() {
  pthread_mutex_unlock(&mutex_);
}

// Create new thread.

void SVSync::StartThread(void *(*func)(void*), void* arg) {
  pthread_t helper;
  pthread_create(&helper, NULL, func, arg);
}

// Place a message in the message buffer (and flush it).
void SVNetwork::Send(const char* msg) {
  mutex_send_->Lock();
  msg_buffer_out_.append(msg);
  mutex_send_->Unlock();
}

// Send the whole buffer.
void SVNetwork::Flush() {
  mutex_send_->Lock();
  while (msg_buffer_out_.size() > 0) {
    int i = send(stream_, msg_buffer_out_.c_str(), msg_buffer_out_.length(), 0);
    msg_buffer_out_.erase(0, i);
  }
  mutex_send_->Unlock();
}

// Receive a message from the server.
// This will always return one line of char* (denoted by \n).
char* SVNetwork::Receive() {
  char* result = NULL;
  if (buffer_ptr_ != NULL) { result = strtok_r(NULL, "\n", &buffer_ptr_); }

  // This means there is something left in the buffer and we return it.
  if (result != NULL) { return result;
  // Otherwise, we read from the stream_.
  } else {
    buffer_ptr_ = NULL;
    has_content = false;

    // The timeout length is not really important since we are looping anyway
    // until a new message is delivered.
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;

    // Set the flags to return when the stream_ is ready to be read.
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(stream_, &readfds);

    int i = select(stream_+1, &readfds, NULL, NULL, &tv);

    // The stream_ died.
    if (i == 0) { return NULL; }

    // Read the message buffer.
    i = recv(stream_, msg_buffer_in_, kMaxMsgSize, 0);

    // Server quit (0) or error (-1).
    if (i <= 0) { return NULL; }
    msg_buffer_in_[i] = '\0';
    has_content = true;
    // Setup a new string tokenizer.
    return strtok_r(msg_buffer_in_, "\n", &buffer_ptr_);
  }
}

// Close the connection to the server.
void SVNetwork::Close() {
  close(stream_);
}


// The program to invoke to start ScrollView
static const char* ScrollViewProg() {
  const char* prog = "sh";
  return prog;
}


// The arguments to the program to invoke to start ScrollView
static std::string ScrollViewCommand(std::string scrollview_path) {
  // The following ugly ifdef is to enable the output of the java runtime
  // to be sent down a black hole on non-windows to ignore all the
  // exceptions in piccolo. Ideally piccolo would be debugged to make
  // this unnecessary.
  // Also the path has to be separated by ; on windows and : otherwise.
  const char* cmd_template = "-c \"trap 'kill %%1' 0 1 2 ; java "
      "-Xms1024m -Xmx2048m -jar %s/ScrollView.jar"
      " & wait\"";
  int cmdlen = strlen(cmd_template) + 4*strlen(scrollview_path.c_str()) + 1;
  char* cmd = new char[cmdlen];
  const char* sv_path = scrollview_path.c_str();
  snprintf(cmd, cmdlen, cmd_template, sv_path, sv_path, sv_path, sv_path);
  std::string command(cmd);
  delete [] cmd;
  return command;
}


// Platform-independent freeaddrinfo()
static void FreeAddrInfo(struct addrinfo* addr_info) {
  delete addr_info->ai_addr;
  delete addr_info;
}


// Non-linux version of getaddrinfo()
static int GetAddrInfoNonLinux(const char* hostname, int port,
                               struct addrinfo** addr_info) {
// Get the host data depending on the OS.
  struct sockaddr_in* address;
  *addr_info = new struct addrinfo;
  memset(*addr_info, 0, sizeof(struct addrinfo));
  address = new struct sockaddr_in;
  memset(address, 0, sizeof(struct sockaddr_in));

  (*addr_info)->ai_addr = (struct sockaddr*) address;
  (*addr_info)->ai_addrlen = sizeof(struct sockaddr);
  (*addr_info)->ai_family = AF_INET;
  (*addr_info)->ai_socktype = SOCK_STREAM;

  struct hostent *name;
  name = gethostbyname(hostname);

  if (name == NULL) {
    FreeAddrInfo(*addr_info);
    *addr_info = NULL;
    return -1;
  }

  // Fill in the appropriate variables to be able to connect to the server.
  address->sin_family = name->h_addrtype;
  memcpy((char *) &address->sin_addr.s_addr,
         name->h_addr_list[0], name->h_length);
  address->sin_port = htons(port);
  return 0;
}


// Platform independent version of getaddrinfo()
//   Given a hostname:port, produce an addrinfo struct
static int GetAddrInfo(const char* hostname, int port,
                       struct addrinfo** address) {
  return GetAddrInfoNonLinux(hostname, port, address);
}


// Set up a connection to a ScrollView on hostname:port.
SVNetwork::SVNetwork(const char* hostname, int port) {
  mutex_send_ = new SVMutex();
  msg_buffer_in_ = new char[kMaxMsgSize + 1];
  msg_buffer_in_[0] = '\0';

  has_content = false;
  buffer_ptr_ = NULL;

  struct addrinfo *addr_info = NULL;

  if (GetAddrInfo(hostname, port, &addr_info) != 0) {
    std::cerr << "Error resolving name for ScrollView host "
              << std::string(hostname) << ":" << port << std::endl;
  }

  stream_ = socket(addr_info->ai_family, addr_info->ai_socktype,
                   addr_info->ai_protocol);

  // If server is not there, we will start a new server as local child process.
  if (connect(stream_, addr_info->ai_addr, addr_info->ai_addrlen) < 0) {
    const char* scrollview_path = getenv("SCROLLVIEW_PATH");
    if (scrollview_path == NULL) {
      scrollview_path = ".";
    }
    const char *prog = ScrollViewProg();
    std::string command = ScrollViewCommand(scrollview_path);
    SVSync::StartProcess(prog, command.c_str());

    // Wait for server to show up.
    // Note: There is no exception handling in case the server never turns up.

    stream_ = socket(addr_info->ai_family, addr_info->ai_socktype,
                   addr_info->ai_protocol);

    while (connect(stream_, addr_info->ai_addr,
                   addr_info->ai_addrlen) < 0) {
      std::cout << "ScrollView: Waiting for server...\n";
      sleep(1);

      stream_ = socket(addr_info->ai_family, addr_info->ai_socktype,
                   addr_info->ai_protocol);
    }
  }
  FreeAddrInfo(addr_info);
}

SVNetwork::~SVNetwork() {
  delete[] msg_buffer_in_;
  delete mutex_send_;
}

#endif  // GRAPHICS_DISABLED
