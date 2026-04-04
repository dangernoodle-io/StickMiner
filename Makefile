PIO ?= pio

.PHONY: help check test coverage build clean monitor

help: ## Show available targets
	@grep -E '^[a-zA-Z_%-]+:.*##' $(MAKEFILE_LIST) | sort | \
		awk 'BEGIN {FS = ":.*## "}; {printf "  \033[36m%-20s\033[0m %s\n", $$1, $$2}'

check: ## Static analysis (cppcheck)
	$(PIO) check --skip-packages

test: ## Run host unit tests
	$(PIO) test -e native

coverage: test ## Coverage report (gcovr)
	gcovr --root . --filter 'components/' --print-summary --coveralls gcovr-coveralls.json

build: ## Build default envs (tdongle-s3 + bitaxe-601)
	$(PIO) run

build-%: ## Build specific env (e.g. make build-tdongle-s3)
	$(PIO) run -e $*

flash-%: ## Flash specific env (e.g. make flash-bitaxe-601)
	$(PIO) run -e $* -t upload

monitor: ## Serial monitor
	$(PIO) device monitor

clean: ## Clean build artifacts
	$(PIO) run -t clean
