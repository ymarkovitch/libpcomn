#! /bin/bash

for a in "ENABLE_PCOMN_AVX=NO" "ENABLE_PCOMN_AVX=YES" "ENABLE_PCOMN_AVX2=YES"
do
    for b in debug release relwithdebinfo dbgsanitize relsanitize
    do
        for s in 14 17
        do
            echo ''
            echo '===================================== C++' $s $b $a '====================================='
            echo ''
            ./switch-builddir $b
            pushd build
            chmod -R u+w `pwd`/../build/ && { find `pwd`/../build/ -type f -print0 | xargs -0 rm ; }
            cbmake -DCMAKE_CXX_STANDARD=$s -D$a ..
            popd
            bunit
        done
    done
done
