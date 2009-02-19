test:
	@./compare_generated.sh
	@./compare_confirmed.sh
	@./compare_siemens.sh

clean:
	rm -f compare_generated/barcode_*_?.pnm
	rm -f compare_confirmed/barcode_*_?.pnm
	rm -f compare_siemens/siemens_*_?.pnm

.PHONY: test clean
