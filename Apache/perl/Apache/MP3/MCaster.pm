package Apache::MP3::MCaster;

use strict;
use Apache::MP3::Resample;
use CGI qw/:standard *TR param/;
use File::Basename 'dirname','basename';
use File::Path;
use vars qw(@ISA $VERSION @EXPORT);

@ISA     = qw(Apache::MP3::Resample);
@EXPORT  = qw(read_library read_playlists get_mp3 read_directory);
$VERSION = 1.60;


my %mp3s;
my %plsts;
my @admins;
my @dwnldrs;


sub users {
   my $r = shift;
   @admins  = split /\W+/,$r->dir_config('AdminUsers');
   @dwnldrs = split /\W+/,$r->dir_config('DownloadUsers');
}


sub get_mp3 {
   my $file    = $_[0];
   my $hashref = $mp3s{$file};

   return $hashref;
}


sub read_library {
   my $file = "/usr/local/apache/htdocs/mcaster/library.txt";

my $top = 0;
   open(LIBR,$file);
   while (<LIBR>) {
if ($top++ < 100000) {
      chop;
      my $line = $_;
      my @cols = split(/\|/,$line);
      my %flds;
   
      # Coerce it back into a relative file path
      $_ = $cols[7];
      s|/mp3/||g;
      s|%20| |g;
      s|%23|#|g;
      my $file = $_;

      $flds{title}    = $cols[0];
      $flds{artist}   = $cols[1];
      $flds{album}    = $cols[2];
      $flds{genre}    = $cols[3];
      $flds{year}     = $cols[4];
      $flds{track}    = $cols[5];
      $flds{seconds}  = $cols[6];
      $flds{filename} = $file;
      $flds{comment}  = $cols[8];

      $flds{sec}      = int($flds{seconds}%60);
      $flds{min}      = int($flds{seconds}/60);
      $flds{duration} ="$flds{min}:$flds{sec}";

      $mp3s{$file} = \%flds;
   }
}
   close LIBR;
   return \%mp3s;
}


sub read_playlists {
   my $file = "/usr/local/apache/htdocs/mcaster/playlists.txt";

   open(PLAY,$file);
   while (<PLAY>) {
      chop;
      my $line = $_;
      my ($name,$type,$file) = split(/\|/,$line);
      if($type && $type eq "m3u") {
         my $url  = "/mcaster/$file";
         my %plst = (name => $name, url => $url);
         $plsts{$url} = \%plst;
      }
   }
   close(PLAY);
}


# read a single directory, returning lists of subdirectories and MP3 files
sub read_directory {
   my $self      = shift;
   my $dir       = shift;
   my @playlists = keys(%plsts);
   my @directories;
   my @txtfiles;

   for my $d (keys(%mp3s)) {
      my $data  = get_mp3($d);
      $mp3s{$d} = $data;

      unless ($self->read_cache("$dir/$d")) {
         $self->write_cache("$dir/$d" => $data);
      }
   }
   return \(@directories,%mp3s,@playlists,@txtfiles);
}


sub download_ok {
   my $self = shift;
   my $r    = $self->r;
   my $user = $r->connection->user;

   if ($self->Apache::MP3::download_ok()) {
      users($r);
      for my $dwnldr (@dwnldrs) {
         return 1 if ($dwnldr eq $user);
      }
   }
   return 0;
}


# format a playlist entry and return its HTML
sub format_playlist {
  my $self     = $_[0];
  my $playlist = $plsts{$_[1]};
  my $title    = $playlist->{name};
  my $url      = $playlist->{url} . "?play=1";
  return p(
			a({-href => $url},
             	img({-src => $self->playlist_icon,
                     -align  => 'ABSMIDDLE',
                     -class  => 'subdir',
                     -alt    => $self->x('Playlist'),
                     -border => 0}))
           		. "&nbsp;" .
           	a({-href  => $url},
           		font({-class => 'subdirectory'},
                	$title)));
}


sub BEGIN {
   read_library();
   read_playlists();
   push(@admins,  "admin");
   push(@dwnldrs, "admin");
}
