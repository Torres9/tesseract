// Copyright 2008 Google Inc. All Rights Reserved.
// Author: scharron@google.com (Samuel Charron)

#include "ccutil.h"

namespace tesseract {
CCUtil::CCUtil() :
  params_(),
  STRING_INIT_MEMBER(m_data_sub_dir,
                     "tessdata/", "Directory for data files", &params_),
  INT_INIT_MEMBER(ambigs_debug_level, 0, "Debug level for unichar ambiguities",
                  &params_),
  BOOL_MEMBER(use_definite_ambigs_for_classifier, 0, "Use definite"
              " ambiguities when running character classifier", &params_),
  BOOL_MEMBER(use_ambigs_for_adaption, 0, "Use ambigs for deciding"
              " whether to adapt to a character", &params_) {
}

CCUtil::~CCUtil() {
}


CCUtilMutex::CCUtilMutex() {
  pthread_mutex_init(&mutex_, NULL);
}

void CCUtilMutex::Lock() {
  pthread_mutex_lock(&mutex_);
}

void CCUtilMutex::Unlock() {
  pthread_mutex_unlock(&mutex_);
}

CCUtilMutex tprintfMutex;  // should remain global
} // namespace tesseract
