package MCaster::Server;

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

my $GroupsFileName = "GroupsFile";
my $AdminGroupName = "AdminGroup";
my $DwnldGroupName = "DownloadGroup";

my $LibraryTxt     = "/usr/MCaster/htdocs/library.txt";
my $PlaylistsTxt   = "/usr/MCaster/htdocs/playlists.txt";
my $GroupsFile     = "/usr/MCaster/conf/groups";
my %groups         = (admin    =>qw(admin),
                      download =>qw(admin));


sub get_mp3 {
   my $file    = $_[0];
   my $hashref = $mp3s{$file};

   return $hashref;
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
   my $conn = $r->connection;
   my $auth = $conn->auth_type;
   my $user = $conn->user;

   if ($self->Apache::MP3::download_ok()) {
      read_users($r);
      my $tmp     = $groups{$DwnldGroupName};
      my @dwnldrs = @$tmp;
      for my $dwnldr (@dwnldrs) {
         if ($dwnldr eq $user) {
            $r->log_error("Allowing download: '$user'");
            return 1;
         }
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


sub read_library {
   open(LIBR,$LibraryTxt);
   while (<LIBR>) {
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

   close LIBR;
   return \%mp3s;
}


sub read_playlists {
   open(PLAY,$PlaylistsTxt);
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


sub read_users {
   my $r = shift;
   my $adminGrp;
   my $dwnldGrp;

   $GroupsFile = $r->dir_config($GroupsFileName);
   $adminGrp   = $r->dir_config($AdminGroupName);
   $dwnldGrp   = $r->dir_config($DwnldGroupName);

#  $r->log_error("Groups File:    $GroupsFile");
#  $r->log_error("Admin Group:    $adminGrp");
#  $r->log_error("Download Group: $dwnldGrp");

   open(GRPS,$GroupsFile);
   while (<GRPS>) {
      chop;
      my @cols  = split(":",$_);
      my $group = $cols[0];
      my @list  = split(" ",$cols[1]);

      $groups{$AdminGroupName} = \@list if ($group eq $adminGrp);
      $groups{$DwnldGroupName} = \@list if ($group eq $dwnldGrp);
   }
   close GRPS;
}


sub BEGIN {
#  read_users();
   read_library();
   read_playlists();
}
