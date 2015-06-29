important_num=3

for i in {1..1001}
do

env crash_me=$i ./test_aslog

num=`grep "important" -nr "./asyn_logging.log" | wc -l`
doom=`grep "doom to crash, doom to crash" -nr "./asyn_logging.log" | wc -l`

if [ `expr $i % 2` == 0 ]; then
    echo "found $doom ASLOG_CHECK msg"
    if [ $doom == 0 ]; then
        echo "test $i: ASLOG_CHECK message is missing, please check"
        break
    fi
fi

echo "$num important msg"
if [ $num -ge $important_num ]; then
    echo "all important log found"
else
    echo "test $i:missing important log, found:$num of $important_num"
    break
fi
done

