#!/bin/bash

echo "=========================================="
echo "Comparing RTE vs CTE Assembly Output..."
echo "=========================================="

# Create test file
mkdir -p test_output
echo "return 2 + 3 * 4;" > test_output/compare_test.c

# Compile with RTE
build/compiler-rte test_output/compare_test.c > /dev/null 2>&1
cp out/output.s test_output/rte_output.s

# Compile with CTE
build/compiler-cte -O test_output/compare_test.c > /dev/null 2>&1
cp out/output.s test_output/cte_output.s

# Count instructions
rte_lines=$(grep -c "^\s*[a-z]" test_output/rte_output.s)
cte_lines=$(grep -c "^\s*[a-z]" test_output/cte_output.s)

echo ""
echo "Expression: 2 + 3 * 4"
echo ""
echo "RTE Assembly ($rte_lines instructions):"
echo "----------------------------------------"
cat test_output/rte_output.s
echo ""
echo "CTE Assembly ($cte_lines instructions):"
echo "----------------------------------------"
cat test_output/cte_output.s
echo ""
echo "Optimization: CTE reduced code by $((rte_lines - cte_lines)) instructions"
echo ""

# Test both produce same result
as --32 test_output/rte_output.s -o test_output/rte.o 2>/dev/null
ld -m elf_i386 test_output/rte.o -o test_output/rte_prog 2>/dev/null
./test_output/rte_prog
rte_result=$?

as --32 test_output/cte_output.s -o test_output/cte.o 2>/dev/null
ld -m elf_i386 test_output/cte.o -o test_output/cte_prog 2>/dev/null
./test_output/cte_prog
cte_result=$?

if [ $rte_result -eq $cte_result ]; then
    echo "✓ Both modes produce identical results: exit code $rte_result"
else
    echo "✗ MISMATCH: RTE=$rte_result, CTE=$cte_result"
    exit 1
fi

# Cleanup
rm -f test_output/*.o test_output/*_prog test_output/compare_test.c

echo "=========================================="
