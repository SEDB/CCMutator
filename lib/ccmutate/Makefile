##===- lib/Transforms/Makefile -----------------------------*- Makefile -*-===##
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
##===----------------------------------------------------------------------===##

LEVEL = ../..
PARALLEL_DIRS = Tools CompareExchange Load AtomicRMW Store Fence FindLockUnlockPairs PosixCondSignal PosixJoin PosixSemaphore PosixYield RmVolatileKeyword PosixCondWait PosixLock ThreadJoin Mutex CondWait

include $(LEVEL)/Makefile.common
include $(LEVEL)/Makefile.llvm.config

# No support for plugins on windows targets
ifeq ($(HOST_OS), $(filter $(HOST_OS), Cygwin MingW Minix))
  PARALLEL_DIRS := $(filter-out Hello, $(PARALLEL_DIRS))
endif

