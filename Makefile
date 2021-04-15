
ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))


.PHONY: clean_tests
clean_tests:
	rm -rf build && mkdir -p build



.PHONY: configure_vs
configure_vs:
	mkdir -p build && \
	cd build && \
	cmake -G "Visual Studio 16 2019" -T host=x64 -A x64 ..



.PHONY: configure_ninja
configure_ninja:
	mkdir -p build && \
	cd build && \
	cmake -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -G Ninja ..



.PHONY: configure_mac
configure_mac:
	mkdir -p buildXC && cmake -G Xcode ..




.PHONY: app_build
app_build:
	cd build && \
	cmake --build . --config RelWithDebInfo --target dispatchcpp -- -j 15



.PHONY: app_run_linux
app_run_linux:
	cd build/ && ./dispatchcpp
