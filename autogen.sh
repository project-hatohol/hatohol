touch AUTHORS NEWS touch ChangeLog
if [ ! -d m4 ]; then
  mkdir m4
fi
aclocal
autoreconf -i
