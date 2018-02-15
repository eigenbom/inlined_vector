CLANG =  /Users/ben/bin/clang-5.0.1

CXX := $(CLANG)/bin/clang++
LLVMCONFIG := $(CLANG)/bin/llvm-config
CXXFLAGS2 := -std=c++11
DEFAULTFLAGS := -I$(CLANG)/include -Wall
CXXFLAGS := -I$(shell $(LLVMCONFIG) --src-root)/tools/clang/include -I$(shell $(LLVMCONFIG) --obj-root)/tools/clang/include $(DEFAULTFLAGS) $(CXXFLAGS2)
LLVMLDFLAGS := $(shell $(LLVMCONFIG) --ldflags --libs $(LLVMCOMPONENTS))
INC = -I ../include

tests: tests.cpp Makefile ../include/inlined_vector.h
	@$(CXX) -o ../bin/$@ tests.cpp $(INC) $(CXXFLAGS) $(LLVMLDFLAGS)
