$EXTRACTRC `find . -name \*.ui` >>  rc.cpp
$XGETTEXT *.cpp -o $podir/skanlite.pot

