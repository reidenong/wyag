BUILD_DIR := build

FORMAT_FILES := $(shell find src include/wyag -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' -o -name '*.h' -o -name '*.hh' -o -name '*.hpp' \))

.PHONY: all run clean format

all: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR)

$(BUILD_DIR)/Makefile: CMakeLists.txt
	cmake -S . -B $(BUILD_DIR) -G "Unix Makefiles"

run: all
	./$(BUILD_DIR)/wyag --help

format:
	clang-format -i $(FORMAT_FILES)

clean:
	rm -rf $(BUILD_DIR)
