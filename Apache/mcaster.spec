Summary:     MediaCaster server
Name:        MCaster-Server
Version:     1.60Beta
Release:     1
BuildArch:   i386
Group:       MediaCaster
License:     Commerical
Vendor:      Smada Nohj Industries
URL:         http://mcaster.kicks-ass.net:9000/mcaster
AutoReqProv: no
Requires:    perl-MP3-Info perl-Apache-MP3 mod_perl apache-devel perl lame-devel

%description
MediaCaster server allow you to enjoy your HP Digitial Entertainment
Center MP3 collection anywhere with an Internet connection.


%prep
tar -xvf ../SOURCES/sources.tar


%install
cp -r usr /
mkdir -p /var/MCaster/cache


%files 
%attr(555,root,root)     %dir    /usr/MCaster
%attr(444,root,root)             /usr/MCaster/README
%attr(555,root,root)     %dir    /usr/MCaster/bin
%attr(555,root,root)             /usr/MCaster/bin/mcuseradd
%attr(555,root,root)             /usr/MCaster/bin/mcuserdel
%attr(555,root,root)             /usr/MCaster/bin/mcusermod
%attr(555,root,root)             /usr/MCaster/bin/buildLibrary
%attr(555,root,root)             /usr/MCaster/bin/getMlData.pl
%attr(555,root,root)             /usr/MCaster/bin/playlistFixer
%attr(555,root,root)     %dir    /usr/MCaster/htdocs
%attr(444,root,root)             /usr/MCaster/htdocs/index.html
%attr(444,root,root)             /usr/MCaster/htdocs/ml_mcaster.gif
%attr(444,root,root)             /usr/MCaster/htdocs/ml_mcaster_config.gif
%attr(555,root,root)     %dir    /usr/MCaster/perl
%attr(555,root,root)     %dir    /usr/MCaster/perl/MCaster
%attr(555,root,root)             /usr/MCaster/perl/MCaster/Server.pm
%attr(555,root,root)     %dir    /usr/MCaster/conf
%attr(444,root,root)     %config /usr/MCaster/conf/httpd.conf
%attr(600,nobody,nobody) %config /usr/MCaster/conf/passwords
%attr(600,nobody,nobody) %config /usr/MCaster/conf/groups
%attr(777,nobody,nobody) %dir    /var/MCaster/cache
# %attr(555,root,root)             /etc/profile.d/MCaster.sh


%post
# set -x
APACHE_DIR="/usr/local/apache"
APACHE_CNF="${APACHE_DIR}/conf"
APACHE_BIN="${APACHE_DIR}/bin"

if ! fgrep -q "audio/mpeg" $APACHE_CNF/httpd.conf; then
   echo "AddType audio/mpeg     .mp3" >> $APACHE_CNF/httpd.conf
fi

if ! fgrep -q "audio/playlist" $APACHE_CNF/httpd.conf; then
   echo "AddType audio/playlist .m3u" >> $APACHE_CNF/httpd.conf
fi

if ! fgrep -q "<Location /mcaster/web>" $APACHE_CNF/httpd.conf; then
   cat /usr/MCaster/conf/httpd.conf >> $APACHE_CNF/httpd.conf
fi

${APACHE_BIN}/apachectl restart >/dev/null
