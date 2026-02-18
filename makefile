.PHONY: clean-deps clean clean-all

clean-deps:
	rm -rf build-deps
	find third_party -mindepth 2 -maxdepth 2 -type d -name install -exec rm -rf {} +

clean-toolchain-tests:
	rm -rf toolchain-tests/build
	rm -rf toolchain-tests/.cache

clean:
	rm -rf build
	rm -rf .cache

clean-all: clean clean-deps clean-toolchain-tests
