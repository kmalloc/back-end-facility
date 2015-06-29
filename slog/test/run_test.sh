pmake clean; pmake DEBUG_FLAGS=-g

echo "run normal test"
for i in {1..10}
do
./test_aslog
if [ $? -ne 0 ]; then
    break
fi
done

if [ $? == 0 ]
then

echo "run test to crash"
env crash_me=1 ./test_aslog

echo "now time to check crash handling for asyn logging"

./check_crash.sh

fi


pmake clean
