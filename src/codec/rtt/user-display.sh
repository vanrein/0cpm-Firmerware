#!/bin/sh
#
# user-display.sh
#
# This is the chatty portion of the Realtime Text test environment.
# It creates a pipe named "rtp-setuppipe" and waits until RTP hints
# are received.  It the opens an RTT session for that data.
#
# This file is part of 0cpm Firmerware.
#  
# 0cpm Firmerware is Copyright (c)2011 Rick van Rein, OpenFortress.
#  
# 0cpm Firmerware is free software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation, version 3.
#  
# 0cpm Firmerware is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#  
# You should have received a copy of the GNU General Public License
# along with 0cpm Firmerware.  If not, see <http://www.gnu.org/licenses/>.


if [ ! -p ./rtp-setup-pipe ]
then
	echo >&2 'Creating ./rtp-setup-pipe'
	rm -f ./rtp-setup-pipe
	mknod ./rtp-setup-pipe p
fi

echo >&2 Waiting for RTP suggestion from SIPP scripts

X=''
while [ -z "$X" ]
do
	read < ./rtp-setup-pipe X
done

# X=`echo $X | sed 's/\[\([^ ]*\)\]/\1/g'`
echo >&2 RTP local/remote coordinates: $X
./rtt_desktop_test $X

