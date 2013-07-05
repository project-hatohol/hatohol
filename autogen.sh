touch AUTHORS NEWS touch
if [ ! -d m4 ]; then
  mkdir m4
fi

# autoreconf needs README
if [ ! -f README ]; then
  ln -s README.md README
fi

aclocal
autoreconf -i
