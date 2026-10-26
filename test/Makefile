.PHONY: *
default:

CC ?= gcc

clean:
	rm -rf build

test:
	premake5 ninja --cc=$(CC) && ninja -C build && premake5 test

fuzztest:
	premake5 ninja --cc=clang && ninja -C build && premake5 fuzztest
	# Clang only.

coverage:
	mkdir -p ./build/coverage/
	gcovr -r .. --html-details ./build/coverage/coverage.html

coverageClang:
	mkdir -p ./build/coverage/
	gcovr -r .. --gcov-executable "llvm-cov gcov" --html-details ./build/coverage/coverage.html

coverageClean:
	find build -name "*.profraw" -delete
	find build -name "*.gcda" -delete
	find build -name "*.gcno" -delete

