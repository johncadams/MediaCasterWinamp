#!/usr/bin/perl -w
BEGIN {
  if ($ENV{'ODIN_HOME'}) {
    @INC = (@INC,
            "$ENV{'ODIN_HOME'}/perl5/lib/site_perl/i386-linux",
            "$ENV{'ODIN_HOME'}/perl5/lib/site_perl");
  }
}


use strict;
use MP3::Info;
use File::Find;
use File::stat;


my $dir   = $ARGV[0]?$ARGV[0]:"/content/music/slurped";
my $newer = $ARGV[1]?$ARGV[1]:0;


# Load all of the WINAMP known genres
use_winamp_genres();

sub getInfo {
   if ( -f && ( /\.mp3$/ || /\.Mp3$/ || /\.MP3$/ )) {
      my $file   = $_;
      my $date   = stat($file)->mtime;

      if ($date > $newer) {
         my $tag    = get_mp3tag($file);
         my $info   = get_mp3info($file);

         my $title  = $tag->{TITLE}   ?$tag->{TITLE}   :"";
         my $artist = $tag->{ARTIST}  ?$tag->{ARTIST}  :"";
         my $album  = $tag->{ALBUM}   ?$tag->{ALBUM}   :"";
         my $genre  = $tag->{GENRE}   ?$tag->{GENRE}   :"";
         my $year   = $tag->{YEAR}    ?$tag->{YEAR}    :"";
         my $track  = $tag->{TRACKNUM}?$tag->{TRACKNUM}:"";
         my $comment= $tag->{COMMENT} ?$tag->{COMMENT} :"";
         my $length = $info->{SECS};

#        $_ =~ s/\ /%20/g;
         my $url = "$File::Find::dir/$_";
         

         # Poor attempt to encode the 'funny' characters in the URL
         # This is to normalize the URL for MediaCaster Client for Winamp
         # since it currently does not.
         #
         # The paths (verbatim) are then referenced in any m3u-style playlists
         # so whatever we do here has to agree with the playlistFixer.
         #
         # And to confuse matter the web interface (MediaCaster Server) needs
         # to undo these paths otherwise the browser gets confused so again any changes
         # here have to be reflected there as well.
         $url =~ s/\.\//\/mcaster\/web\//g;
         $url =~ s/\ /%20/g;
         $url =~ s/\#/%23/g;

         $comment =~ s/\n/ /g;
         $comment =~ s/\|/&#124;/g;  # <<<< This really doesn't work well

         printf("%s|%s|%s|%s|%s|%s|%d|%s|%s\n", 
                $title, $artist, $album, $genre, $year, $track, $length, $url, $comment);
      }
   }
}

chdir $dir;
find(\&getInfo, ".");
