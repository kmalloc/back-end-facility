CPUPROFILE=./profile_ouput ./log $1 8
pprof --pdf ./log ./profile_ouput > profile_result.pdf
gpdf ./profile_result.pdf

