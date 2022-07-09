EXEC = c2scl

#CFLAGS = -fopenmp -Wall -g3 -g -ggdb  -O0 -std=c++11 -I/home/ss/test/try/opencl/include
CFLAGS = -fopenmp -Wall -g3 -g -ggdb  -O0 -std=c++11 -I/home/jianbin/work/code/c2scl/include
LDFLAGS =
LDFLAGS += -lltdl -lm -lgomp

LLCLANG = clang++ -stdlib=libc++
LDCONFIG = `llvm-config --ldflags --system-libs --libs`
LCCONFIG =  `llvm-config --cxxflags`
CLANGLIBS := \
        -Wl,--start-group \
        -lclangAST \
        -lclangASTMatchers \
        -lclangAnalysis \
        -lclangBasic \
        -lclangDriver \
        -lclangEdit \
        -lclangFrontend \
        -lclangFrontendTool \
        -lclangLex \
        -lclangParse \
        -lclangSema \
        -lclangEdit \
        -lclangRewrite \
        -lclangRewriteFrontend \
        -lclangStaticAnalyzerFrontend \
        -lclangStaticAnalyzerCheckers \
        -lclangStaticAnalyzerCore \
        -lclangSerialization \
        -lclangToolingCore \
        -lclangTooling \
        -lclangFormat \
        -Wl,--end-group


CC = g++
RM = rm -f

.PHONY: clean 

all: $(EXEC)

OBJS = main.o HCodeGen.o HFunctionInfo.o HKernelExtractor.o HLoopInfo.o HPassManager.o HScopeIR.o HScopeIRGen.o Util.o

$(EXEC): ${OBJS}
	${CC} ${OBJS} ${CLANGLIBS} ${LDCONFIG}  -o ../bin/$(EXEC)

main.o: main.cpp
	${CC} ${CFLAGS} ${LCCONFIG} -c -o $@ $^ -g

HScopeIRGen.o: HScopeIRGen.cpp
	${CC} ${CFLAGS} ${LCCONFIG} -c -o $@ $^ -g
HFunctionInfo.o: HFunctionInfo.cpp
	${CC} ${CFLAGS} ${LCCONFIG} -c -o $@ $^ -g
HKernelExtractor.o: HKernelExtractor.cpp
	${CC} ${CFLAGS} ${LCCONFIG} -c -o $@ $^ -g
HLoopInfo.o: HLoopInfo.cpp
	${CC} ${CFLAGS} ${LCCONFIG} -c -o $@ $^ -g
HPassManager.o: HPassManager.cpp
	${CC} ${CFLAGS} ${LCCONFIG} -c -o $@ $^ -g
HScopeIR.o: HScopeIR.cpp
	${CC} ${CFLAGS} ${LCCONFIG} -c -o $@ $^ -g
Util.o: Util.cpp
	${CC} ${CFLAGS} ${LCCONFIG} -c -o $@ $^ -g

HCodeGen.o: HCodeGen.cpp
	$(CC) $(CFLAGS) -c -o $@ $^ $(LDFLAGS) -g

clean:
	rm ../bin/$(EXEC) *.o 
