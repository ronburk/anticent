# go.sh - try to compile and run anticent

# can't figure out damn ninja exit status, let's just fail on everything
set -e

Release=debug
SymPath=~/src/embedv8/v8/third_party/llvm-build/Release+Asserts/bin/llvm-symbolizer
pushd .
cd ~/src/embedv8/v8
ninja -v -C out/$Release anticent 

cd ~/src/embedv8/v8/out/$Release
echo ASAN_OPTIONS="detect_leaks=1 symbolize=1 external_symbolizer_path=$SymPath" ./anticent
ASAN_OPTIONS="detect_leaks=1 symbolize=1 external_symbolizer_path=$SymPath" ./anticent

popd

