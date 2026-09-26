.PHONY: *
default:

# ===== basic commands =====

clean:
	rm -rf build

# https://mesonbuild.com/howtox.html#use-address-sanitizer
# https://mesonbuild.com/Builtin-options.html#details-for-buildtype
# '--buildtype=debug' implicitely adds: '--debug','-Db_ndebug=false','-Doptimization=g'
mesonSetupDebug:
	meson setup --reconfigure --prefix=$(CURDIR)/build build \
		--buildtype=debug -Doptimization=g -Db_sanitize=address,undefined

mesonSetupDebugClang:
	CC=clang \
	meson setup --reconfigure --prefix=$(CURDIR)/build build \
		--buildtype=debug -Doptimization=g -Db_sanitize=address,undefined \
		-Db_coverage=true -Db_lundef=false

compile:
	meson compile -C build

# To run a single test: meson test --interactive -C build umapstr_test
test:
	meson test --interactive -C build

fuzz:
	make compile && \
	./build/wmap_test_fuzzy    -runs=5000 && \
	./build/wmapstr_test_fuzzy -runs=5000 && \
	./build/strpool_test_fuzzy -runs=5000 && \
	echo "done."

coverage:
	ninja coverage-html -C build

coverageClean:
	find build -name "*.profraw" -delete
	find build -name "*.gcda" -delete
	find build -name "*.gcno" -delete


