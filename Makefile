CXX      = g++
CXXFLAGS = -std=c++98 -Wall -O2
COBC     = cobc
COBFLAGS = -x -I src/cobol/copy

.PHONY: all batch teller serve serve-bg stop test clean

all: bin/postpend bin/postdiv bin/stmt bin/teller bin/webd

bin:
	mkdir -p bin

bin/postpend: src/cobol/postpend.cob src/cobol/copy/account.cpy src/cobol/copy/pending.cpy src/cobol/copy/txn.cpy src/cobol/copy/reject.cpy src/cobol/copy/control.cpy | bin
	$(COBC) $(COBFLAGS) -o $@ src/cobol/postpend.cob

bin/postdiv: src/cobol/postdiv.cob src/cobol/copy/account.cpy src/cobol/copy/txn.cpy src/cobol/copy/control.cpy | bin
	$(COBC) $(COBFLAGS) -o $@ src/cobol/postdiv.cob

bin/stmt: src/cobol/stmt.cob src/cobol/copy/member.cpy src/cobol/copy/account.cpy src/cobol/copy/txn.cpy | bin
	$(COBC) $(COBFLAGS) -o $@ src/cobol/stmt.cob

bin/teller: src/cpp/teller.cpp src/cpp/layout.h | bin
	$(CXX) $(CXXFLAGS) -o $@ src/cpp/teller.cpp -lncurses

bin/webd: src/cpp/webd.cpp src/cpp/layout.h | bin
	$(CXX) $(CXXFLAGS) -o $@ src/cpp/webd.cpp

batch: bin/postpend bin/postdiv
	./bin/postpend
	./bin/postdiv

teller: bin/teller
	./bin/teller

serve: bin/webd
	./bin/webd

serve-bg: bin/webd
	@if [ -f webd.pid ] && kill -0 `cat webd.pid` 2>/dev/null; then \
		echo "webd already running pid `cat webd.pid`"; \
	else \
		nohup ./bin/webd > webd.log 2>&1 & echo $$! > webd.pid; \
		sleep 0.3; \
		echo "webd listening on 127.0.0.1:8080 pid `cat webd.pid`"; \
	fi

stop:
	@if [ -f webd.pid ]; then \
		kill `cat webd.pid` 2>/dev/null || true; \
		rm -f webd.pid; \
		echo "webd stopped"; \
	else \
		echo "webd is not running"; \
	fi

test: all
	sh tests/run.sh

clean:
	rm -rf bin webd.pid webd.log
