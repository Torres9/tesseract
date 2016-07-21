/**********************************************************************
 * File:        errcode.c  (Formerly error.c)
 * Description: Generic error handler function
 * Author:      Ray Smith
 * Created:     Tue May  1 16:28:39 BST 1990
 *
 * (C) Copyright 1989, Hewlett-Packard Ltd.
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 ** http://www.apache.org/licenses/LICENSE-2.0
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 *
 **********************************************************************/

#include          <signal.h>
#ifdef __linux__
#include          <sys/syscall.h>   // For SYS_gettid.
#include          <unistd.h>        // For syscall itself.
#endif
#include          "allheaders.h"
#include          "errcode.h"
#include          "tprintf.h"

// Size of thread-id array of pixes to keep in case of crash.
const int kMaxNumThreadPixes = 32768;

Pix* global_crash_pixes[kMaxNumThreadPixes];

void SavePixForCrash(int resolution, Pix* pix) {
}

// CALL ONLY from a signal handler! Writes a crash image to stderr.
void signal_exit(int signal_code) {
  tprintf("Received signal %d!\n", signal_code);
  abort();
}

void err_exit() {
  ASSERT_HOST("Fatal error encountered!" == NULL);
}


void set_global_loc_code(int loc_code) {
  // global_loc_code = loc_code;

}


void set_global_subloc_code(int loc_code) {
  // global_subloc_code = loc_code;

}


void set_global_subsubloc_code(int loc_code) {
  // global_subsubloc_code = loc_code;

}
