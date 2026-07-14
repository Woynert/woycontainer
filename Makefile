.PHONY: *
default:

# ===== basic commands =====

clean:
	rm -rf build

mesonSetupDebug:
	meson setup --reconfigure --prefix=$(CURDIR)/build build \
		--debug -Db_ndebug=false --buildtype=debug  -Doptimization=1

compile:
	meson compile -C build

test:
	meson test --interactive -C build
