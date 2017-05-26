#! /bin/sh 
#
#   Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
#   2008, The Regents of the University of California, through Lawrence
#   Berkeley National Laboratory (subject to receipt of any required
#   approvals from the U.S. Dept. of Energy).  All rights reserved.
#
#   Portions may be copyrighted by others, as may be noted in specific
#   copyright notices within specific files.
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#  $Id: autogen.sh,v 1.7.38.4 2014/10/06 23:12:43 phargrov Exp $

set -e

mkdir -p config
for dir in . ./config; do
  ( cd $dir && rm -f config.guess config.sub depcomp install-sh libtool ltmain.sh missing mkinstalldirs test-driver )
done
rm -rf autom4te.cache
aclocal
autoheader
touch blcr_config.h.in
autoconf
libtoolize --automake --copy
if [ -f ./ltmain.sh ]; then
	# older libtool didn't find AC_CONFIG_AUX_DIR in configure.ac
	mv ltmain.sh config/
	rm -f config.sub config.guess
fi
automake --include-deps --add-missing --copy
if test -e config/test-driver; then # Disable parallel tests:
  rm config/test-driver
  perl -pi -e 's/^#AUTOMAKE_OPTIONS/AUTOMAKE_OPTIONS/;' -- tests/Makefile.am
  automake --include-deps --add-missing --copy
fi

if [ -x config.status ]; then
    echo "################################################################"
    echo "# You appear to have already configured the build environment. #"
    echo "# Trying to update your configuration.                         #"
    echo "################################################################"
    ./config.status
    make clean
fi

